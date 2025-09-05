#include "exportthread.h"

ExportThread::ExportThread(Progressable* receiver, ModelInputPictures* mip,
                           const ExportConfig& config, volatile bool* stopped,
                           LogFile* logFile) {
    m_receiver = receiver;
    m_config = config;
    auto rp = std::make_shared<ReaderParams>();
    rp->initialize(m_config.original_resolution);
    rp->setWorkingResolution(m_config.working_resolution);
    if (m_config.roi.has_value()) {
        rp->setUseRoi(true);
        rp->setRoi(*m_config.roi);
    }
    m_reader = mip->getReader()->copy(rp);
    m_keyframes = mip->getAllKeyframes(true);
    if (m_config.destination.endsWith("/images"))
        m_config.destination = m_config.destination.left(
            m_config.destination.length() - QString("/images").length());
    m_stopped = stopped;
    m_logFile = logFile;
    m_progress = 0;

    // log general info
    m_logFile->setInputInfo(m_keyframes);

    m_exportExif = new ExportExif();
}

ExportThread::~ExportThread() {
    for (int i = 0; i < (int)m_config.transformations.size(); i++) {
        ITransform* iTr = m_config.transformations[i];
        delete iTr;
    }

    delete m_exportExif;
    delete m_reader;
}

ExportResult ExportThread::getResult() const { return m_result; }

void ExportThread::run() {
    m_receiver->slot_makeProgress(0, tr("Exporting images"));

    // We use image index 0 to get the size of our input images
    cv::Mat originalMat = m_reader->getPic(0);
    QPoint imageSize = QPoint(originalMat.cols, originalMat.rows);

    // ROI is used if it's enabled and it's not the entire image (which would be
    // default)
    bool usesRoi = m_config.roi.has_value() &&
                   !m_config.roi->isDefault();

    // Resize if the working resolution differs from the input resolution
    bool usesResize = !(m_config.original_resolution ==
                        m_config.export_resolution);

    // if input is an image and we do not modify it in any way (i.e. resizing)
    // we do not need to load the image and write it back, instead we copy the
    // input file including all metadata
    bool useCopy = m_reader->isDir() && !usesResize && !usesRoi;

    // We need to export gps meta data if available
    bool useExif = false;
    MetaDataReader* gpsReader = nullptr;

    QList<MetaDataReader*> mdList =
        MetaDataManager::instance().loadAllMetaData();
    for (MetaDataReader* md : mdList) {
        if (md->getName().contains("gps", Qt::CaseInsensitive)) {
            gpsReader = md;
            useExif = true;
            break;
        }
    }

    // Setup the ImageProcessor
    ImageProcessor processor;
    if (usesResize)
        processor.addCommand(std::make_unique<ResizeCommand>(
            m_config.export_resolution.toQPoint()));
    if (usesRoi)
        processor.addCommand(
            std::make_unique<CropCommand>(m_config.roi->cropAsQRect(m_config.export_resolution)));

    // setup writing to disk or copying input image

    // exporting from cv::Mat or copying input image
    QString imagePath = m_config.destination + QString("/images");
    if (useCopy)
        processor.addCommand(std::make_unique<CopyFileCommand>(
            m_reader->getFileVector(), imagePath));
    else
        processor.addCommand(std::make_unique<WriteToDiskCommand>(
            imagePath, "", m_config.format));
    // adding an exif tag only if we have gps data available, we don't need to
    // do this if we copied the image before
    if (!useCopy && useExif)
        processor.addCommand(std::make_unique<ExifTagCommand>(gpsReader));
    // export itransform plugin output
    for (auto plugin : m_config.transformations)
        processor.addCommand(std::make_unique<TransformCommand>(
            plugin, m_config.working_resolution, m_config.export_resolution,
            m_config.roi.has_value() ? *m_config.roi : ROI(),
            m_config.destination));

    // run the processor to export images
    SequentialReader* seq_reader =
        m_reader->createSequentialReader(m_keyframes, Reader::APPLY_NONE);

    // Shared variables for error handling
    std::atomic<bool> errorOccurred(false);
    std::mutex errorMutex;
    std::optional<QString> firstError;

    std::function<void(int*)> writeToDrive = [seq_reader, &processor, this,
                                              &errorOccurred, &errorMutex,
                                              &firstError](int* num_imgs) {
        ImageContext ctx;
        *num_imgs = 0;
        while (seq_reader->getNext(ctx.image, ctx.index)) {
            if (*m_stopped || errorOccurred.load()) {
                return;  // user stopped the computation or error occurred ->
                         // return
            }
            if (ctx.image.empty())
                continue;  // broken input image (happens with some codecs)
            ctx.originalImage = ctx.image.clone();
            auto res = processor.process(ctx);
            if (res) {
                // something went wrong during export!
                // abort here!
                // Store the first error message
                std::lock_guard<std::mutex> lock(errorMutex);
                if (!errorOccurred.exchange(true)) {
                    firstError = res;
                    m_receiver->slot_makeProgress(
                        0, tr("Encountered an error! Aborting..."));
                }
                return;
            }
            // successfully exported the image
            *num_imgs += 1;
            reportProgress();
        }
    };

    // start the computation in multiple worker threads
    QFutureSynchronizer<void> synchronizer;
    // use all available threads for now
    int n_threads = QThread::idealThreadCount();
    std::vector<int> n_imgs_exported(n_threads, 0);

    for (int i = 0; i < n_threads; i++) {
        synchronizer.addFuture(
            QtConcurrent::run(writeToDrive, &n_imgs_exported[i]));
    }
    synchronizer.waitForFinished();
    delete seq_reader;

    // Check for error after threads finish
    if (firstError) {
        m_receiver->slot_displayMessage(*firstError);
        m_result = ExportResult::failed(*firstError);
        return;
    }

    if (*m_stopped) {
        m_result = ExportResult::aborted();
        return;
    }

    int total_images_exported = 0;
    for (int i = 0; i < n_threads; i++) {
        total_images_exported += n_imgs_exported[i];
    }

    // report broken frames
    if (total_images_exported < int(m_keyframes.size())) {
        m_result = ExportResult::partialSuccess(int(m_keyframes.size()) -
                                                total_images_exported);
        m_receiver->slot_displayMessage(QString::number(m_result.brokenImages) +
                                        tr(" images where skipped."));
    } else {
        m_result = ExportResult::success();
        m_receiver->slot_displayMessage(
            tr("All images exported successfully."));
    }
}

void ExportThread::reportProgress() {
    m_progress++;
    if (m_receiver) {
        int percentProgress = m_progress * 100 / (int)m_keyframes.size();
        m_receiver->slot_makeProgress(percentProgress, tr("Exporting images"));
    }
}
