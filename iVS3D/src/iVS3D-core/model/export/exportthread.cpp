#include "exportthread.h"

#include "maskcommand.h"

ExportThread::ExportThread(Progressable* receiver, ModelInputPictures* mip,
                           const ExportConfig& config, volatile bool* stopped,
                           std::shared_ptr<LogFile> logFile, PluginThread* pluginThread)
    : m_receiver(receiver),
      m_config(config),
      m_stopped(stopped),
      m_logFile(logFile),
      m_pluginThread(pluginThread) {
    auto rp = std::make_shared<ReaderParams>();
    rp->initialize(m_config.original_resolution);
    rp->setWorkingResolution(m_config.working_resolution);
    if (m_config.roi.has_value()) {
        rp->setUseRoi(true);
        rp->setRoi(*m_config.roi);
    } else {
        rp->setUseRoi(false);
    }
    m_reader = mip->getReader()->copy(rp);
    m_keyframes = mip->getAllKeyframes(true);

    if (m_config.destination.endsWith("/images")) {
        m_config.destination = m_config.destination.left(
            m_config.destination.length() - QString("/images").length());
    }

    if (m_logFile) {
        m_logFile->setInputInfo(m_keyframes);
    }
    m_exportExif = new ExportExif();
}

ExportThread::~ExportThread() {
    delete m_exportExif;
    delete m_reader;
}

ExportResult ExportThread::getResult() const { return m_result; }

tl::expected<void, ExportError> ExportThread::validateMaskStack() {
    if (!m_config.maskStack) {
        return {};  // No mask stack, validation passes
    }

    const auto& maskStack = *m_config.maskStack;

    // Currently only support single mask record
    if (maskStack.size() != 1) {
        return tl::unexpected(ExportError{
            QString("MaskStack must contain exactly 1 entry, but contains %1")
                .arg(maskStack.size())});
    }

    const auto* record = maskStack.getRecord(0);
    if (!record) {
        return tl::unexpected(
            ExportError{"Failed to retrieve mask record from stack"});
    }

    // Validate working resolution matches
    if (!(record->workingResolution == m_config.working_resolution)) {
        return tl::unexpected(
            ExportError{QString("Mask working resolution (%1) does not match "
                                "export resolution (%2)")
                            .arg(record->workingResolution.toString())
                            .arg(m_config.working_resolution.toString())});
    }

    // Validate ROI matches
    const auto& recordRoi = record->roi;

    // Check if both have ROI or both don't have ROI
    bool recordHasRoi = !recordRoi.isDefault();
    bool configHasRoi = m_config.roi.has_value();

    if (recordHasRoi != configHasRoi) {
        return tl::unexpected(
            ExportError{"Mask ROI configuration does not match export ROI"});
    }

    // If both have ROI, compare them by converting to QRectF
    if (configHasRoi) {
        if (!(recordRoi.toQRectF() == m_config.roi->toQRectF())) {
            return tl::unexpected(
                ExportError{"Mask ROI does not match export ROI"});
        }
    }

    return {};
}

void ExportThread::run() {
    m_receiver->slot_makeProgress(0, tr("Exporting images"));

    // Log export configuration to file
    QString configLog = QString("Export Configuration:\n"
                                "  Original Resolution: %1\n"
                                "  Working Resolution: %2\n"
                                "  Export Resolution: %3\n"
                                "  ROI: %4\n"
                                "  Mask Stack: %5")
        .arg(m_config.original_resolution.toString())
        .arg(m_config.working_resolution.toString())
        .arg(m_config.export_resolution.toString())
        .arg(m_config.roi.has_value() ? "Yes" : "No")
        .arg(m_config.maskStack ? QString::number(m_config.maskStack->size()) : "No Stack");
    if (m_logFile) {
        m_logFile->addCustomEntry("ExportConfiguration", configLog);
    }

    // Validate mask stack if present
    if (m_config.maskStack) {
        const auto validation = validateMaskStack();
        if (!validation) {
            m_result = ExportResult::failed(validation.error().message);
            m_receiver->slot_displayMessage(validation.error().message);
            if (m_logFile) {
                m_logFile->addCustomEntry("MaskStackValidation", validation.error().message, "Error");
            }
            return;
        }
    }

    // We use image index 0 to get the size of our input images
    cv::Mat originalMat = m_reader->getPic(0);
    if (originalMat.empty()) {
        m_result = ExportResult::failed(tr("Failed to read first image"));
        return;
    }

    // ROI is used if it's enabled and it's not the entire image (which would be
    // default)
    bool usesRoi = m_config.roi.has_value() && !m_config.roi->isDefault();

    // Resize if the working resolution differs from the input resolution
    bool usesResize =
        m_config.original_resolution != m_config.export_resolution;

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
    std::vector<ImageProcessor> processors;
    ImageProcessor& processor = processors.emplace_back();
    if (usesResize) {
        processor.addCommand(std::make_unique<ResizeCommand>(
            m_config.export_resolution.toQPoint()));
    }
    if (usesRoi) {
        processor.addCommand(std::make_unique<CropCommand>(
            m_config.roi->cropAsQRect(m_config.export_resolution)));
    }

    // Setup writing to disk or copying input image
    QString imagePath = m_config.destination + QString("/images");
    if (m_config.copy_images) {
        processor.addCommand(std::make_unique<CopyFileCommand>(
            m_reader->getFileVector(), imagePath));
    } else {
        processor.addCommand(std::make_unique<WriteToDiskCommand>(
            imagePath, "", m_config.format, m_reader->getFileVector()));
    }

    // Add exif tag only if we have gps data available and didn't copy images
    if (!m_config.copy_images && useExif) {
        processor.addCommand(std::make_unique<ExifTagCommand>(gpsReader));
    }

    // Create a separate processor for mask generation if a mask stack is present.
    // Masks are generated asynchronously by the plugin thread and have their own
    // processing pipeline (resize/crop to match mask generation resolution before generation).
    ImageProcessor &maskProcessor = processors.emplace_back();
    if (m_config.maskStack && m_config.maskStack->size() > 0 && m_pluginThread) {
        const auto* record = m_config.maskStack->getRecord(0);
        if (record) {
            if (m_logFile) {
                m_logFile->addCustomEntry("MaskGeneration", "Adding mask generation to export pipeline");
            }

            // Resize/crop to the working resolution that the mask generator expects
            if (record->workingResolution != m_config.original_resolution) {
                maskProcessor.addCommand(std::make_unique<ResizeCommand>(
                    record->workingResolution.toQPoint()));
            }
            if (usesRoi) {
                maskProcessor.addCommand(std::make_unique<CropCommand>(
                    m_config.roi->cropAsQRect(m_config.working_resolution)));
            }

            // Create the mask command (expensive plugin setup happens in constructor)
            auto maskCmd = std::make_unique<MaskCommand>(
                record, m_config.export_resolution,
                m_config.roi.has_value() ? *m_config.roi : ROI(),
                m_config.destination, m_pluginThread);

            // Connect PluginThread mask result signal to MaskCommand's callback handler
            // This allows MaskCommand to receive async results from the plugin thread
            connect(m_pluginThread, &PluginThread::maskFinished, this,
                    [maskCmdPtr = maskCmd.get()](const MaskGenerationResult& result) {
                        if (maskCmdPtr) {
                            maskCmdPtr->onMaskFinished(result);
                        }
                    },
                    Qt::QueuedConnection);

            maskProcessor.addCommand(std::move(maskCmd));

            // Ensure output directory exists
            QString maskPath = m_config.destination + QString("/masks");
            if (!QDir().exists(maskPath) && !QDir().mkpath(maskPath)) {
                QString errorMsg = tr("Failed to create mask output directory: %1").arg(maskPath);
                m_receiver->slot_displayMessage(errorMsg);
                if (m_logFile) {
                    m_logFile->addCustomEntry("MaskOutputDirectory", errorMsg, "Error");
                }
                m_result = ExportResult::failed(errorMsg);
                return;
            }
            if (m_logFile) {
                m_logFile->addCustomEntry("MaskOutputDirectory", QString("Mask output directory created: %1").arg(maskPath));
            }
            maskProcessor.addCommand(std::make_unique<WriteToDiskCommand>(
                maskPath, "", "png", m_reader->getFileVector()));
        }
    }

    // run the processor to export images
    SequentialReader* seq_reader =
        m_reader->createSequentialReader(m_keyframes, Reader::APPLY_NONE);

    // Shared variables for error handling
    std::atomic<bool> errorOccurred(false);
    std::mutex errorMutex;
    std::optional<QString> firstError;

    std::function<void(int*)> writeToDrive = [seq_reader, &processors, this,
                                              &errorOccurred, &errorMutex,
                                              &firstError](int* num_imgs) {
        *num_imgs = 0;
        cv::Mat image;
        uint index;
        while (seq_reader->getNext(image, index)) {
            if (*m_stopped || errorOccurred.load()) {
                return;  // user stopped the computation or error occurred ->
                         // return
            }
            if (image.empty())
                continue;  // broken input image (happens with some codecs)
            
            for (ImageProcessor& processor : processors) {
                ImageContext ctx;
                ctx.originalImage = image.clone();  // clone to ensure thread safety for plugins
                ctx.image = image.clone();
                ctx.index = index;
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
            }
            
            // successfully exported the image
            *num_imgs += 1;
            reportProgress();
        }
    };

    m_receiver->slot_displayMessage(tr("Starting export..."));
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
