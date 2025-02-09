#include "exportthread.h"

ExportThread::ExportThread(Progressable* receiver, ModelInputPictures* mip, QPoint resolution, const QString &path, const QString &name, volatile bool* stopped, QRect roi, const std::vector<ITransform*> &iTransformCopies, LogFile *logFile)
{
    m_receiver = receiver;
    m_reader = mip->getReader()->copy();
    m_keyframes = mip->getAllKeyframes(true);
    m_resolution = resolution;
    m_path = path;
    if(m_path.endsWith("/images"))
        m_path = m_path.left(m_path.length() - QString("/images").length());
    m_name = name;
    m_stopped = stopped;
    m_roi = roi;
    m_iTransformCopies = iTransformCopies;
    m_logFile = logFile;
    m_progress = 0;

    // log general info
    m_logFile->setInputInfo(m_keyframes);

    m_exportExif = new ExportExif();
}

ExportThread::~ExportThread()
{
    for(int i = 0; i<(int)m_iTransformCopies.size(); i++){
        ITransform *iTr = m_iTransformCopies[i];
        delete iTr;
    }

    delete m_exportExif;
    delete m_reader;
}


int ExportThread::getResult(){
    return m_result;
}

void ExportThread::run(){
    m_receiver->slot_makeProgress(0, tr("Exporting images"));

    // We use image index 0 to get the size of our input images
    cv::Mat originalMat = m_reader->getPic(0);
    QPoint imageSize = QPoint(originalMat.cols, originalMat.rows);

    // width or height of zero indicates an invalid roi -> don't use it
    bool useRoi = (m_roi.width() > 0 && m_roi.height() > 0);
    if (imageSize.x() < m_roi.width() || imageSize.y() < m_roi.height()) {
        useRoi = false; // roi is larger than our input image -> don't use it
    }

    // Only use resize if the output resolution differs from the input resolution
    bool useResize = (imageSize.x() != m_resolution.x()) || (imageSize.y() != m_resolution.y());

    // If the resolution changed the roi Rect has to be scaled
    if (useResize) {
       //Get scale factor in x & y direction
       float ratioX = (float) m_resolution.x() / (float) imageSize.x();
       float ratioY = (float) m_resolution.y() / (float) imageSize.y();
       //Scale the topLeft point
       QPoint topLeft = m_roi.topLeft();
       int topLeftXWithRatio = topLeft.x() * ratioX;
       int topLeftYWithRatio = topLeft.y() * ratioY;
       QPoint topLeftWitRatio(topLeftXWithRatio, topLeftYWithRatio);
       //Scale the bottomRight point
       QPoint bottomRight = m_roi.bottomRight();
       int bottomRightXWithRatio = bottomRight.x() * ratioX;
       int bottomRightYWithRatio = bottomRight.y() * ratioY;
       QPoint bottomRightWitRatio(bottomRightXWithRatio, bottomRightYWithRatio);
       //create the scaled roi
       m_roi = QRect(topLeftWitRatio, bottomRightWitRatio);
    }

    // if input is an image and we do not modify it in any way (i.e. resizing) we do not need to
    // load the image and write it back, instead we copy the input file including all metadata
    bool useCopy = m_reader->isDir() && !useResize && !useRoi;

    // We need to export gps meta data if available
    bool useExif = false;
    MetaDataReader* gpsReader = nullptr;

    QList<MetaDataReader*> mdList =  MetaDataManager::instance().loadAllMetaData();
    for (MetaDataReader* md : mdList) {
        if (md->getName().contains("gps", Qt::CaseInsensitive)) {
            gpsReader = md;
            useExif = true;
            break;
        }
    }

    // Setup the ImageProcessor
    ImageProcessor processor;
    // resize and roi are optional
    if(useResize)           processor.addCommand(std::make_unique<ResizeCommand>(m_resolution));
    if(useRoi)              processor.addCommand(std::make_unique<CropCommand>(m_roi));
    // exporting from cv::Mat or copying input image
    QString imagePath = m_path + QString("/images");
    if(useCopy)             processor.addCommand(std::make_unique<CopyFileCommand>(m_reader->getFileVector(), imagePath));
    else                    processor.addCommand(std::make_unique<WriteToDiskCommand>(imagePath));
    // adding an exif tag only if we have gps data available, we don't need to do this if we copied the image before
    if(!useCopy && useExif) processor.addCommand(std::make_unique<ExifTagCommand>(gpsReader));
    // export itransform plugin output
    for(auto plugin : m_iTransformCopies)
        processor.addCommand(std::make_unique<TransformCommand>(plugin, m_path));

    // run the processor to export images
    SequentialReader *seq_reader = m_reader->createSequentialReader(m_keyframes);

    std::function<void(int*)> writeToDrive = [seq_reader, &processor, this](int *num_imgs) {
        ImageContext ctx;
        *num_imgs = 0;
        while (seq_reader->getNext(ctx.image, ctx.index)) {
            if (*m_stopped) {
                return; // user stopped the computation -> return
            }
            if (ctx.image.empty()) continue; // broken input image (happens with some codecs)
            if (!processor.process(ctx)) {
                // something went wrong during export!
                // handle gracefully, but maybe display a message?
                continue;
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

    for(int i=0; i<n_threads; i++){
        synchronizer.addFuture(QtConcurrent::run(writeToDrive, &n_imgs_exported[i]));
    }
    synchronizer.waitForFinished();

    int total_images_exported = 0;
    for(int i=0; i<n_threads; i++){
        total_images_exported += n_imgs_exported[i];
    }

    // report broken frames
    if (total_images_exported < int(m_keyframes.size())){
        m_result = int(m_keyframes.size())-total_images_exported;
        m_receiver->slot_displayMessage(QString::number(m_result) + tr(" images where skipped."));
    } else {
        m_result = 0;
        m_receiver->slot_displayMessage(tr("All images exported successfully."));
    }
    if (*m_stopped) m_result = 1;
    delete seq_reader;
}

void ExportThread::reportProgress() {
    m_progress++;
    if (m_receiver) {
        int percentProgress = m_progress * 100 / (int)m_keyframes.size();
        m_receiver->slot_makeProgress(percentProgress, tr("Exporting images"));
    }

}
