#include "exportthread.h"

#include "maskcommand.h"

#include <algorithm>

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

tl::expected<ExportValidationResult, ExportError> ExportThread::validateMaskStack(
    const ExportConfig& config, PluginThread* pluginThread) {
    ExportValidationResult validationResult;

    if (!config.maskStack) {
        return validationResult;  // No mask stack, validation passes
    }

    const auto& maskStack = *config.maskStack;

    if (maskStack.size() == 0) {
        return validationResult;  // Explicitly empty stack is valid
    }

    if (!pluginThread) {
        return tl::unexpected(ExportError{
            "Mask export requested, but PluginThread is not available"});
    }

    const ROI exportMaskRoi = config.roi.value_or(ROI());
    const bool exportHasRoi = !exportMaskRoi.isDefault();

    int roiMismatchCount = 0;
    QStringList roiMismatchPlugins;

    for (int i = 0; i < maskStack.size(); ++i) {
        const auto* record = maskStack.getRecord(i);
        if (!record) {
            return tl::unexpected(
                ExportError{QString("Failed to retrieve mask record at index %1")
                                .arg(i)});
        }

        if (record->pluginName.isEmpty()) {
            return tl::unexpected(
                ExportError{QString("Mask record at index %1 has no plugin name")
                                .arg(i)});
        }

        // Working resolution and ROI mismatches are allowed:
        // - mask computation uses each record's stored working resolution
        // - ROI for export is always taken from ExportConfig

        const bool recordHasRoi = !record->roi.isDefault();
        const bool roiMismatch =
            (recordHasRoi != exportHasRoi) ||
            (exportHasRoi && record->roi.toQRectF() != exportMaskRoi.toQRectF());
        if (roiMismatch) {
            ++roiMismatchCount;
            roiMismatchPlugins.push_back(record->pluginName);
        }
    }

    if (roiMismatchCount > 0) {
        validationResult.warnings.push_back(
            QObject::tr("Warning: %1 mask stack %2 use a different ROI than the current export ROI. "
                        "Export will use the current ROI, so resulting masks may differ from preview. "
                        "Affected plugins: %3")
                .arg(roiMismatchCount)
                .arg(roiMismatchCount == 1 ? QObject::tr("entry")
                                           : QObject::tr("entries"))
                .arg(roiMismatchPlugins.join(", ")));
    }

    return validationResult;
}

void ExportThread::run() {
    m_receiver->slot_makeProgress(0, tr("Preparing export"));

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

    // Validate mask stack if present (safety check; main validation is done
    // before starting this thread)
    if (m_config.maskStack) {
        const auto validation = validateMaskStack(m_config, m_pluginThread);
        if (!validation) {
            m_result = ExportResult::failed(validation.error().message);
            m_receiver->slot_displayMessage(validation.error().message);
            if (m_logFile) {
                m_logFile->addCustomEntry("MaskStackValidation",
                                          validation.error().message,
                                          "Error");
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

    // Setup the image export processor (always executed in step 1)
    ImageProcessor imageProcessor;
    if (usesResize) {
        imageProcessor.addCommand(std::make_unique<ResizeCommand>(
            m_config.export_resolution.toQPoint()));
    }
    if (usesRoi) {
        imageProcessor.addCommand(std::make_unique<CropCommand>(
            m_config.roi->cropAsQRect(m_config.export_resolution)));
    }

    // Setup writing to disk or copying input image
    QString imagePath = m_config.destination + QString("/images");
    if (m_config.copy_images) {
        imageProcessor.addCommand(std::make_unique<CopyFileCommand>(
            m_reader->getFileVector(), imagePath));
    } else {
        qDebug() << m_config.format;
        imageProcessor.addCommand(std::make_unique<WriteToDiskCommand>(
            imagePath, "", m_config.format, m_reader->getFileVector()));
    }

    // Add exif tag only if we have gps data available and didn't copy images
    if (!m_config.copy_images && useExif) {
        imageProcessor.addCommand(std::make_unique<ExifTagCommand>(gpsReader));
    }

    QVector<MaskRecord> maskRecords;
    if (m_config.maskStack) {
        maskRecords = m_config.maskStack->getAllRecords();
    }

    const ROI exportMaskRoi = m_config.roi.value_or(ROI());
    const bool exportHasRoi = !exportMaskRoi.isDefault();

    const int totalSteps = std::max(1, maskRecords.size());
    m_totalWorkUnits = int(m_keyframes.size()) * totalSteps;
    m_completedWorkUnits.store(0, std::memory_order_relaxed);

    m_receiver->slot_displayMessage(
        tr("Starting export with %1 step(s) and %2 image(s) per step.")
            .arg(totalSteps)
            .arg(m_keyframes.size()));

    const QString maskPath = m_config.destination + QString("/masks");
    if (!maskRecords.isEmpty()) {
        if (!QDir().exists(maskPath) && !QDir().mkpath(maskPath)) {
            QString errorMsg =
                tr("Failed to create mask output directory: %1").arg(maskPath);
            m_receiver->slot_displayMessage(errorMsg);
            if (m_logFile) {
                m_logFile->addCustomEntry("MaskOutputDirectory", errorMsg,
                                          "Error");
            }
            m_result = ExportResult::failed(errorMsg);
            return;
        }
        if (m_logFile) {
            m_logFile->addCustomEntry(
                "MaskOutputDirectory",
                QString("Mask output directory prepared: %1").arg(maskPath));
        }
    }

    auto createMaskProcessor =
        [&](const MaskRecord& record, bool mergeWithExisting,
            QMetaObject::Connection& connection)
        -> std::unique_ptr<ImageProcessor> {
        auto processor = std::make_unique<ImageProcessor>();

        if (record.workingResolution != m_config.original_resolution) {
            processor->addCommand(std::make_unique<ResizeCommand>(
                record.workingResolution.toQPoint()));
        }

        if (exportHasRoi) {
            processor->addCommand(std::make_unique<CropCommand>(
                exportMaskRoi.cropAsQRect(record.workingResolution)));
        }

        auto maskCmd = std::make_unique<MaskCommand>(
            &record, m_config.export_resolution, exportMaskRoi,
            m_config.destination,
            m_pluginThread, m_stopped);

        MaskCommand* maskCmdPtr = maskCmd.get();
        connection = connect(
            m_pluginThread, &PluginThread::maskFinished, this,
            [maskCmdPtr](const MaskGenerationResult& result) {
                if (maskCmdPtr) {
                    maskCmdPtr->onMaskFinished(result);
                }
            },
            Qt::QueuedConnection);

        processor->addCommand(std::move(maskCmd));

        if (mergeWithExisting) {
            processor->addCommand(std::make_unique<MaskMergeCommand>(
                maskPath, "png", m_reader->getFileVector()));
        }

        processor->addCommand(std::make_unique<WriteToDiskCommand>(
            maskPath, "", "png", m_reader->getFileVector()));

        return processor;
    };

    auto executeSweep =
        [&](const QString& operationLabel,
            const std::vector<ImageProcessor*>& processors, int stepIndex,
            int stepCount, int* processedImages) -> std::optional<QString> {
        SequentialReader* seqReader =
            m_reader->createSequentialReader(m_keyframes, Reader::APPLY_NONE);

        std::atomic<bool> errorOccurred(false);
        std::mutex errorMutex;
        std::optional<QString> firstError;

        auto writeToDrive = [seqReader, &processors, this, &errorOccurred,
                             &errorMutex, &firstError, &operationLabel,
                             stepIndex, stepCount](int* numImgs) {
            *numImgs = 0;
            cv::Mat image;
            uint index;
            while (seqReader->getNext(image, index)) {
                if (*m_stopped || errorOccurred.load()) {
                    return;
                }
                if (image.empty()) {
                    continue;
                }

                for (ImageProcessor* processor : processors) {
                    ImageContext ctx;
                    ctx.originalImage = image.clone();
                    ctx.image = image.clone();
                    ctx.index = index;
                    auto res = processor->process(ctx);
                    if (res) {
                        if (*m_stopped || *res == "ABORTED") {
                            return;
                        }
                        std::lock_guard<std::mutex> lock(errorMutex);
                        if (!errorOccurred.exchange(true)) {
                            firstError = res;
                            m_receiver->slot_makeProgress(
                                0, tr("Encountered an error! Aborting..."));
                        }
                        return;
                    }
                }

                *numImgs += 1;
                reportProgress(operationLabel, stepIndex, stepCount);
            }
        };

        QFutureSynchronizer<void> synchronizer;
        const int nThreads = std::max(1, QThread::idealThreadCount());
        std::vector<int> processedPerThread(nThreads, 0);
        for (int i = 0; i < nThreads; ++i) {
            synchronizer.addFuture(
                QtConcurrent::run(writeToDrive, &processedPerThread[i]));
        }
        synchronizer.waitForFinished();
        delete seqReader;

        int totalProcessed = 0;
        for (int v : processedPerThread) {
            totalProcessed += v;
        }
        if (processedImages) {
            *processedImages = totalProcessed;
        }

        if (firstError) {
            return *firstError;
        }

        return std::nullopt;
    };

    int total_images_exported = 0;

    // Step 1: always export images, optionally also generate first mask
    {
        std::vector<ImageProcessor*> processors;
        processors.push_back(&imageProcessor);

        std::unique_ptr<ImageProcessor> maskProcessor;
        QMetaObject::Connection maskConnection;

        if (!maskRecords.isEmpty()) {
            maskProcessor = createMaskProcessor(maskRecords[0], false, maskConnection);
            processors.push_back(maskProcessor.get());
        }

        m_receiver->slot_displayMessage(
            tr("Starting step 1/%1: exporting %2 image(s) at resolution %3.")
                .arg(totalSteps)
                .arg(m_keyframes.size())
                .arg(m_config.export_resolution.toString()));

        if (!maskRecords.isEmpty()) {
            m_receiver->slot_displayMessage(
                tr("Starting step 1/%1 mask plugin: %2 (working resolution: %3).")
                    .arg(totalSteps)
                    .arg(maskRecords[0].pluginName)
                    .arg(maskRecords[0].workingResolution.toString()));
        }

        const QString operationLabel =
            maskRecords.isEmpty()
                ? tr("Exporting images")
                : tr("Exporting images + generating mask (%1)")
                      .arg(maskRecords[0].pluginName);

        int processedImages = 0;
        auto sweepError = executeSweep(operationLabel, processors, 1, totalSteps,
                                       &processedImages);

        if (maskConnection) {
            disconnect(maskConnection);
        }

        if (sweepError) {
            m_receiver->slot_displayMessage(*sweepError);
            m_result = ExportResult::failed(*sweepError);
            return;
        }

        if (*m_stopped) {
            m_result = ExportResult::aborted();
            m_receiver->slot_displayMessage(tr("Export aborted by user."));
            return;
        }

        total_images_exported = processedImages;
    }

    // Step 2..N: generate additional masks and merge with existing mask on disk
    for (int i = 1; i < maskRecords.size(); ++i) {
        if (*m_stopped) {
            m_result = ExportResult::aborted();
            return;
        }

        QMetaObject::Connection maskConnection;
        auto maskProcessor =
            createMaskProcessor(maskRecords[i], true, maskConnection);

        std::vector<ImageProcessor*> processors;
        processors.push_back(maskProcessor.get());

        m_receiver->slot_displayMessage(
            tr("Starting step %1/%2: mask plugin %3 (working resolution: %4).")
                .arg(i + 1)
                .arg(totalSteps)
                .arg(maskRecords[i].pluginName)
                .arg(maskRecords[i].workingResolution.toString()));

        const QString operationLabel =
            tr("Generating + merging mask (%1)")
                .arg(maskRecords[i].pluginName);

        auto sweepError =
            executeSweep(operationLabel, processors, i + 1, totalSteps, nullptr);

        if (maskConnection) {
            disconnect(maskConnection);
        }

        if (sweepError) {
            m_receiver->slot_displayMessage(*sweepError);
            m_result = ExportResult::failed(*sweepError);
            return;
        }

        if (*m_stopped) {
            m_result = ExportResult::aborted();
            m_receiver->slot_displayMessage(tr("Export aborted by user."));
            return;
        }
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

void ExportThread::reportProgress(const QString& operation, int stepIndex,
                                  int totalSteps) {
    const int completed =
        m_completedWorkUnits.fetch_add(1, std::memory_order_relaxed) + 1;

    int percentProgress = 100;
    if (m_totalWorkUnits > 0) {
        percentProgress = std::clamp((completed * 100) / m_totalWorkUnits, 0, 100);
    }

    if (m_receiver) {
        m_receiver->slot_makeProgress(percentProgress,
                                      tr("Step %1/%2 - %3")
                                          .arg(stepIndex)
                                          .arg(totalSteps)
                                          .arg(operation));
    }
}
