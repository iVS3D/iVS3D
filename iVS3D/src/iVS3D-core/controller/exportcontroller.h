#ifndef EXPORTCONTROLLER_H
#define EXPORTCONTROLLER_H

#include "DataManager.h"
#include "applicationsettings.h"
#include "modelalgorithm.h"
#include "exportexecutor.h"
#include "view/outputwidget.h"
#include "view/reconstructdialog.h"
#include "view/cropexport.h"
#include "view/emptyfolderdialog.h"
#include "plugin/transformmanager.h"
#include "logfile.h"
#include "logmanager.h"
#include "stringcontainer.h"
#include "model/metaData/gpsreader.h"

#if defined(Q_OS_LINUX)
    #include "colmapwrapper.h"
#endif

#include <QObject>
#include <QDebug>
#include <QFileDialog>
#include <QProcess>
#include <QCoreApplication>
#include <QSettings>
#include <QTextStream>

#include <QElapsedTimer>


// --- default resolutions in dropbox ---
#define RESOLUTION_LIST "2560 x 1440 (QHD)|1920 x 1080 (FHD)|1280 x 720 (HD)|1280 x 1024 (HD*)|640 x 480"

/**
 * @class ExportController
 *
 * @ingroup Controller
 *
 * @brief The ExportController class is managing signals and slots for the start of an export and opening reconstruction software.
 * The class manages the output widget on the bottom right plus the reconstruct dialog, having signals for all relevant events
 * triggered by that widget and dialog.
 *
 * @author Lennart Ruck
 *
 * @date 2021/02/08
 */
class ExportController : public QObject
{
    Q_OBJECT
public:
#if defined(Q_OS_LINUX)
    /**
     * @brief ExportController is created with a prepared outputWidget and dataManager pointer
     * Connects to GUI, initializes member pointers
     * @param outputWidget
     * @param dataManager
     */
    ExportController(OutputWidget *outputWidget, DataManager *dataManager, lib3d::ots::ColmapWrapper *colmap);
#elif defined(Q_OS_WIN)
    /**
     * @brief ExportController is created with a prepared outputWidget and dataManager pointer
     * Connects to GUI, initializes member pointers
     * @param outputWidget
     * @param dataManager
     */
    ExportController(OutputWidget *outputWidget, DataManager *dataManager);
#endif

    /**
     * @brief ExportController::~ExportController
     */
    ~ExportController();
    /**
     * @brief getOutputSettings returns the currently set output settings
     * @return QMap with the current output settings
     */
    QMap<QString, QVariant> getOutputSettings();
    /**
     * @brief setOutputSettings sets output settings
     * @param QMap with the output settings
     */
    void setOutputSettings(QMap<QString, QVariant> settings);


signals:
    /**
     * @brief sig_hasStatusMessage is used to display given Text in the status bar on the bottom left
     * @param message wanted output as QString
     */
    void sig_hasStatusMessage(QString message);
    /**
     * @brief sig_stopPlay signal stops video playback. It is used to avoid simultaneous access to model-data
     */
    void sig_stopPlay();
    /**
     * @brief sig_exportFinished is emitted if an export finished
     */
    void sig_exportFinished();
    /**
     * @brief sig_exportStarted is emitted if export started
     */
    void sig_exportStarted();
    /**
     * @brief sig_exportAborted is emitted if export aborted
     */
    void sig_exportAborted();

public slots:
    /**
     * @brief slot_export starts export and creates a projectfile in the output folder
     * It prepares various Strings needed, plus creates a AlgorithmExecutor and connects its signals&slots before it starts the export itself.
     */
    void slot_export();

    /**
     * @brief slot_cropExport creates a cropExport-Dialog
     */
    void slot_cropExport();

    /**
     * @brief slot_closeCropExport cleans up remaining connection of cropExport-Dialog and saves the cropped value
     */
    void slot_closeCropExport(int result);

    /**
     * @brief slot_reconstruct opens reconstruct dialog
     * Gathers various information needed for the dialog before it opens it.
     */
    void slot_reconstruct();
    /**
     * @brief slot_resolutionChange gets triggered if user alters the (output) resolution and changes it in the model accordingly
     * @param res new resolution input by the user
     */
    void slot_resolutionChange(const QString &res);
    /**
     * @brief slot_outputPathChanged gets triggered if user changes the outputpath manually (without the browse function)
     * @param path new path input by the user
     */
    void slot_outputPathChanged(QString path);
    /**
     * @brief slot_exportAborted triggered by ExportExecutor, cancelling the export
     * Disconnects ExportExecutor, removes progress bar and shows export results
     */
    void slot_exportAborted();
    /**
     * @brief slot_exportFinished triggered by ExportExecutor upon finishing export
     * Disconnects ExportExecutor, removes progress bar and shows export results
     * @param result unused (error code 0=OK, 1=stopped, -1=export failed)
     */
    void slot_exportFinished(int result);
    /**
    * @brief slot_showExportSettings triggered by AutomaticExecSettings shows the given settings and
    * saves the internal
    * @param QMap containing the export settings
    */
    void slot_showExportSettings(QMap<QString, QVariant> exportSettings);
    /**
     * @brief slot_onKeyframesChanged triggered by MIP if keyframes change
     */
    void slot_onKeyframesChanged();
    /**
     * @brief slot_nextImageOnPlayer triggered when a new image is shown on the video player
     * @param image mat of the image
     * @param id id of the image
     */
    void slot_nextImageOnPlayer(uint idx);

    /**
     * @brief slot_altitudeChanged is triggered when the altitude has been changed by the user
     * @param altitude set altitude
     */
    void slot_altitudeChanged(double altitude);

private:
    /**
     * @brief parseResolution parses resolution QString into QPoint (width x heigt)
     * @param resolutionString resolution to parse
     * @return returns (width,height) of given String if QString was valid, otherwise returns (-1,-1)
     */
    QPoint parseResolution(QString resolutionString);
    /**
     * @brief validateResolution compares if resolution is smaller or equal than the input resolution
     * @param resolution resolution to compare the input resolution to
     * @return returns true if given resolution is smaller or equal to input resolution
     */
    bool validateResolution(QPoint resolution);
    /**
     * @brief startReconstruct handles starting reconstruct software, preparing its start-arguments, creating batch-files and project-file
     * for Colmap:
     * - creates batch files which can be executed manually which hold given start arguments for later use
     * - creates project.ini file for colmap (and empty database.db)
     * - starts colmap if startarguments start with "gui" otherwise starts explorer at path of batch files which can then be executed for the desired result
     * @return returns true if everything succeeded, returns false if an error occurs at any point.
     */
    bool startReconstruct();

    void deleteExportFolder(QString path);

    bool createDatabaseFile(QString defaultpath, QString targetpath);
    bool createProjectFile(QString defaultpath, QString targetpath, QMap<QString, QString> projectsettings);
    bool createShortcutplusBatch(QString reconstructDir, QString startargs, QString exportDir);

    DataManager *m_dataManager;
    ExportExecutor *m_exportExec;
    OutputWidget *m_outputWidget;
    ReconstructDialog *m_reconstructDialog;
    QPoint m_resolution;
    QString m_path;
    QRect m_roi = QRect(0,0,0,0);
    CropExport* m_cropDialog;
    //Key is name of the project, value is path to export
    QMap<QString, QString> m_currentExports;
    // export runtime
    QElapsedTimer m_timer;
    uint m_imageOnPlayerId = 0;

#if defined(Q_OS_LINUX)
    lib3d::ots::ColmapWrapper *m_colmap;
#endif

};

#endif // EXPORTCONTROLLER_H
