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

#include "logfile.h"
#include "stringcontainer.h"
#include "modelinputpictures.h"
#include "itransform.h"
#include "progressable.h"
#include "exportexif.h"
#include "imageprocessor.h"
#include "copyfilecommand.h"
#include "cropcommand.h"
#include "resizecommand.h"
#include "transformcommand.h"
#include "writetodiskcommand.h"
#include "exiftagcommand.h"

#define JPEG_COMPRESSION_PARAMS {cv::IMWRITE_JPEG_QUALITY, 100}

struct ExportConfig
{
    QString name;
    QString destination;
    QString format;
    ReaderParams readerParams;
    std::vector<ITransform *> transformations;
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
 * @brief The ExportThread class provides functionality to export images with given resolution and region of interest to a specific location. It extends QThread
 * in order to be executed in a separate thread.
 *
 * @author Dominik Wüst
 *
 * @date 2021/04/14
 */
class ExportThread : public QThread
{
    Q_OBJECT

public:
    /**
     * @brief ExportThread creates an instance which reports progress and exports images from given ModelInputPictures. Start using QThread::start() to execute in
     * separate thread!
     * @param receiver The Progressable to report progress to
     * @param mip The Model InputPictures instance to get keyframes from
     * @param resolution The resolution for the exported images
     * @param path The path to export to
     * @param name The name of the exported files
     * @param stopped Thread continues export if @a false, aborts export if @a false
     * @param roi The region of interest
     * @param iTransformCopies The ITransform instances to run fro creating additional images
     */
    explicit ExportThread(Progressable *receiver, ModelInputPictures *mip, const ExportConfig &config, volatile bool *stopped, LogFile *logFile);
    ~ExportThread();

    /**
     * @brief getResult returns the result of the export operation.
     * @return The ExportResult struct describing the outcome.
     */
    ExportResult getResult() const;

private:
    Progressable *m_receiver;
    Reader *m_reader;
    std::vector<uint> m_keyframes;
    volatile bool *m_stopped;
    ExportConfig m_config;
    ExportResult m_result = ExportResult::success();
    LogFile *m_logFile;
    int m_progress = 0;
    ExportExif *m_exportExif;

    void reportProgress();

    volatile int m_exportProgress = 0;
    QMutex m_mutex;
    bool currentOperation(uint n);

protected:
    /**
     * @brief run implements the export logic and is executed in a worker thread.
     */
    void run() override;
};

#endif // EXPORTTHREAD_H
