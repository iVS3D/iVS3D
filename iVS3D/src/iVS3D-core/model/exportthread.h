#ifndef EXPORTTHREAD_H
#define EXPORTTHREAD_H

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <QThread>
#include <QRect>
#include <QFileDialog>
#include <iostream>
#include <QtConcurrent/QtConcurrentMap>
#include <QMutex>
#include <QMutexLocker>

#include "logfile.h"
#include "stringcontainer.h"
#include "modelinputpictures.h"
#include "itransform.h"
#include "progressable.h"

//debug
#include <QDebug>

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
    explicit ExportThread(Progressable* receiver, ModelInputPictures* mip, QPoint resolution, const QString &path, const QString &name, volatile bool* stopped, QRect roi, const std::vector<ITransform*> &iTransformCopies, LogFile *logFile);
    ~ExportThread();

    /**
     * @brief getResult returns @a 0 if export was sucesfull, greater @a 0 otherwise.
     * @return The result
     */
    int getResult();


private:
    Progressable* m_receiver;
    Reader* m_reader;
    std::vector<uint> m_keyframes;
    volatile bool* m_stopped;
    QPoint m_resolution;
    QString m_path;
    QString m_name;
    int m_result = 1;
    QRect m_roi;
    std::vector<ITransform*> m_iTransformCopies;
    LogFile *m_logFile;
    int m_progress = 0;

    void reportProgress();
    cv::Mat resizeCrop(cv::Mat image, cv::Size resize, bool useResize, cv::Rect crop, bool useCrop);
    bool exportImages(cv::Mat image, int iTransformCopiesSize, const QString &fileName, bool isDirImages, int currentKeyframe, std::vector<std::string> imageFiles);

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
