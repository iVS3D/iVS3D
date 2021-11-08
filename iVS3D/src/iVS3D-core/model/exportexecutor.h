#ifndef EXPORTEXECUTOR_H
#define EXPORTEXECUTOR_H

#include <QRect>
#include <QCoreApplication>


#include "progressable.h"
#include "DataManager.h"
#include "exportthread.h"
#include "itransform.h"
#include "logfile.h"
#include "stringcontainer.h"

/**
 * @class ExportExecutor
 *
 * @ingroup Model
 *
 * @brief The ExportExecutor class manages the export and its progress. It encapsules the export process and provides signals and slots
 * to interact with it while running the export in a separate worker thread. Following interactions are possible:
 *
 *  - control
 *      -# start an export process (ExportExecutor::startExport)
 *      -# abort an export process (ExportExecutor::slot_abort)
 *  - progress
 *      -# report progress of export (ExportExecutor::sig_progress)
 *      -# report on start of export (ExportExecutor::sig_exportStarted)
 *      -# report on abort of export (ExportExecutor::sig_exportAborted)
 *      -# report on finish of export (ExportExecutor::sig_exportFinished)
 *
 * @date 2021/04/14
 *
 * @author Dominik Wüst
 *
 */
class ExportExecutor : public Progressable
{
    Q_OBJECT

public:
    /**
     * @brief ExportExecutor creates an executor operating on the data provided by given DataManager.
     * @param parent The parent for the QObject
     * @param dataManager The DataManager to access images for export
     */
    explicit ExportExecutor(QObject* parent, DataManager* dataManager);

    /**
     * @brief startExport starts a new Export with the currently selected keyframes in ModelInputPictures provided by the DataManager.
     * @param resolution The output resolution
     * @param path The output folder path
     * @param name The output name
     * @param roi The region of interest to export
     * @param iTransformCopies The ITransform instances to create additional export images
     */
    void startExport(QPoint resolution, QString path, QString name, QRect roi, std::vector<ITransform*> iTransformCopies, LogFile *logFile);

public slots:
    /**
     * @brief [slot] slot_abort stops the export.
     */
    void slot_abort();

    /**
     * @brief [slot] slot_finished is invoked from the export thread on finish.
     */
    void slot_finished();

signals:
    /**
     * @brief [signal] sig_exportStarted is emitted after an export started.
     */
    void sig_exportStarted();

    /**
     * @brief [signal] sig_exportAborted is emitted after an export was aborted.
     */
    void sig_exportAborted();

    /**
     * @brief [signal] sig_exportFinished is emitted after an export finished.
     * @param result Is @a 0 if export finished without problems, greater @a 0 otherwise
     */
    void sig_exportFinished(int result);

private:
    void closeThread();
    volatile bool m_stopped = false;
    DataManager* m_dataManager;
    ExportThread* m_exportThread;
    QObject* m_parent;
    QString m_exportPath;

    QPoint m_boundaries = QPoint(0, 0);
};

#endif // EXPORTEXECUTOR_H
