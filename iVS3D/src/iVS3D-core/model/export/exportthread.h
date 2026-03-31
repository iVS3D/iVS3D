#ifndef EXPORTTHREAD_H
#define EXPORTTHREAD_H

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <QThread>
#include <QFileDialog>
#include <iostream>
#include <QtConcurrent>
#include <QMutex>
#include <QMutexLocker>
#include <fstream>
#include <QFutureSynchronizer>
#include <QFile>
#include <QStringList>

#include <memory>
#include <atomic>

#include <tl/expected.hpp>
#include <chrono>
#include <thread>

#include "logfile.h"
#include "stringcontainer.h"
#include "modelinputpictures.h"
#include "progressable.h"
#include "exportexif.h"
#include "imageprocessor.h"
#include "copyfilecommand.h"
#include "cropcommand.h"
#include "resizecommand.h"
#include "writetodiskcommand.h"
#include "exiftagcommand.h"
#include "maskmergecommand.h"
#include "maskstack.h"
#include "pluginthread.h"

#define JPEG_COMPRESSION_PARAMS {cv::IMWRITE_JPEG_QUALITY, 100}

struct ExportError {
    QString message;
};

struct ExportValidationResult {
    QStringList warnings;
};

struct ExportConfig {
    QString name;
    QString destination;
    QString format;
    std::optional<ROI> roi;
    Resolution original_resolution;
    Resolution working_resolution;
    Resolution export_resolution;
    const MaskStack* maskStack = nullptr;  // Masks to generate during export
    bool copy_images = false;
};

/**
 * @brief ExportResultType enumerates possible export outcomes.
 */
enum class ExportResultType
{
    Success,        // all images were exported
    PartialSuccess, // some images were broken and needed to be skipped
    Aborted,        // export was aborted by the user
    Failed          // export failed due to a runtime error
};

/**
 * @brief ExportResult holds the result of an export operation.
 */
struct ExportResult
{
    ExportResultType type;
    int brokenImages = 0; // Only relevant for PartialSuccess
    QString errorMessage; // Only relevant for Failed

    static ExportResult success()
    {
        return {ExportResultType::Success, 0, ""};
    }
    static ExportResult partialSuccess(int broken)
    {
        return {ExportResultType::PartialSuccess, broken, ""};
    }
    static ExportResult aborted()
    {
        return {ExportResultType::Aborted, 0, ""};
    }
    static ExportResult failed(const QString &msg)
    {
        return {ExportResultType::Failed, 0, msg};
    }
};

Q_DECLARE_METATYPE(ExportResult)

/**
 * @class ExportThread
 *
 * @ingroup Model
 *
 * @brief The ExportThread class provides functionality to export images with given resolution, ROI, and optional mask generation.
 * It extends QThread to be executed in a separate thread.
 *
 * Supports generating masks during export using the MaskStack from the application state.
 * Validation ensures working resolution and ROI match the MaskRecord configuration.
 *
 * @author Dominik Wüst
 * @date 2021/04/14
 */
class ExportThread : public QThread {
    Q_OBJECT

public:
    explicit ExportThread(Progressable* receiver, ModelInputPictures* mip,
                         const ExportConfig& config, volatile bool* stopped,
                         std::shared_ptr<LogFile> logFile, PluginThread* pluginThread = nullptr);
    ~ExportThread();

    ExportResult getResult() const;

    static tl::expected<ExportValidationResult, ExportError>
    validateMaskStack(const ExportConfig& config, PluginThread* pluginThread);

private:
    Progressable* m_receiver;
    Reader* m_reader;
    std::vector<uint> m_keyframes;
    volatile bool* m_stopped;
    ExportConfig m_config;
    ExportResult m_result = ExportResult::success();
    std::shared_ptr<LogFile> m_logFile;
    std::atomic<int> m_completedWorkUnits{0};
    int m_totalWorkUnits = 0;
    ExportExif* m_exportExif;
    PluginThread* m_pluginThread = nullptr;

    void reportProgress(const QString& operation, int stepIndex, int totalSteps);

protected:
    void run() override;
};

#endif // EXPORTTHREAD_H
