#ifndef CONTROLLER_H
#define CONTROLLER_H

/** @defgroup Controller Controller
 *
 * @ingroup Controller
 *
 * @brief manages interaction between model and view.
 */

#include "DataManager.h"
#include "openexecutor.h"
#include "applicationsettings.h"
#include "automaticexecutor.h"
#include "automaticexecsettings.h"

#include "view/mainwindow.h"
#include "view/progressdialog.h"
#include "view/reconstructiontoolsdialog.h"

#include "controller/exportcontroller.h"
#include "controller/algorithmcontroller.h"
#include "controller/videoplayercontroller.h"
#include "controller/automaticcontroller.h"

#if defined(Q_OS_LINUX)
    #include "colmapwrapper.h"
#endif

#include "plugin/algorithmmanager.h"
#include "plugin/transformmanager.h"

#include <QObject>
#include <QWidget>
#include <QFileDialog>
#include <QDialog>
#include <QMessageBox>
#include <QElapsedTimer>


/**
 * @class Controller
 *
 * @ingroup Controller
 *
 * @brief The Controller class is the main controller and the starting point of iVS3D. Its responsible for initializing the other controllers, the DataManager,
 * the MainWindow, AlgorithmManager and TransformManager. This includes creating the instances and connecting their signal & slots.
 * The Controller also handels user input on the menu bar.
 *
 * If a new Import was successfully loaded Controller will delete all existing controllers and creat new instances of them.
 *
 * @author Daniel Brommer
 *
 * @date 2021/03/5
 */

class Controller: public QObject
{
    Q_OBJECT
public:
    /**
     * @brief Controllers constructor which creates AlgorithmManager, TransformManager, DataManager and MainWindow and connecting existing signals
     */
    Controller(QString inputPath, QString settingsPath, QString outputPath, QString logPath);
    ~Controller();
public slots:
    /**
     * @brief slot_openInputFolder Slot is called when a folder should be loaded.
     * It will show a file dialog and use the DataManager to import the selected folder.
     */
    void slot_openInputFolder();
    /**
     * @brief slot_openInputVideo Slot is called when a video should be loaded.
     * It will show a file dialog and use the OpenExecutor to import the selected video.
     */
    void slot_openInputVideo();
    /**
     * @brief slot_openVideoDragAndDrop Slot is called when a file is drop on the VideoPlayer.
     * It will try to import the drop file with the OpenExecutor.
     */
    void slot_openVideoDragAndDrop(QString filePath);
    /**
     * @brief slot_openProject Slot is called when a project should be loaded
     * It will show a file dialog and use the OpenExecutor to import the selected project.
     */
    void slot_openProject();
    /**
     * @brief slot_saveProjectAs Slot is called when a project should be saved to a new location
     * It will show a file dialog and use the DataManager to save the current project.
     */
    void slot_saveProjectAs();
    /**
     * @brief slot_saveProject Slot is called when a project should be saved
     * If the project is saved for the firs time slot_saveProjectAs is called
     * Otherwise it will use the DataManager to save the current project.
     */
    void slot_saveProject();
    /**
     * @brief slot_addReconstructPath Slot is called when 'Manage reconstrution tools' is clicked. It will execute a ReconstructionToolsDialog.
     */
    void slot_addReconstructPath();
    /**
     * @brief slot_changeDefaultInputPath Slot is called when 'Set input Path' is clicked.
     * It will show a file dialog and save the selected folder in ApplicationSettings.
     */
    void slot_changeDefaultInputPath();
    /**
     * @brief slot_toggleColorTheme Slot is called when color theme is changed by the user. Toggles between different themes.
     */
    void slot_toggleColorTheme();
    /**
     * @brief slot_changeColorTheme Slot is called when a specific color theme is selected.
     * @param theme The selected color theme
     */
    void slot_changeColorTheme(ColorTheme theme);
    /**
     * @brief slot_changeUseCuda Slot is called when 'CUDA' is clicked.
     * @param useCuda @a true if CUDA is marked
     */
    void slot_changeUseCuda(bool useCuda);
    /**
     * @brief slot_changeUseCuda Slot is called when 'create log file' is clicked.
     * @param createLog @a true if create log file is marked
     */
    void slot_changeCreateLogFile(bool createLog);
    /**
     * @brief slot_changeInterpolateMetaData Slot is called when 'Interpolate missing meta data' is clicked.
     * @param interpolate @a true if missings meta data is interpolated
     */
    void slot_changeInterpolateMetaData(bool interpolate);
    /**
     * @brief slot_openMetaData Slot is called when Meta Data should be loaded
     */
    void slot_openMetaData();

signals:
    /**
     * @brief [signal] sig_hasStatusMessage() is emitted when the status message is updated.
     *
     * @param message QString with the new status
     */
    void sig_hasStatusMessage(QString message);

    /**
     * @brief [signal] sig_colorThemeChanged() is emitted when the color theme is changed.
     * @param theme the new color theme
     */
    void sig_colorThemeChanged(ColorTheme theme);

private slots:
    void slot_openFinished(int result);
    void slot_exportStarted();
    void slot_exportFinished();
    void slot_undo();
    void slot_redo();
    void slot_historyChanged();

private:
    VideoPlayerController *m_videoPlayerController;
    AlgorithmController* m_algorithmController;
    ExportController *m_exportController;
    AutomaticController* m_automaticController;
    MainWindow* m_mainWindow;
    DataManager* m_dataManager;
    OpenExecutor *m_openExec;
    #if defined(Q_OS_LINUX)
        lib3d::ots::ColmapWrapper *m_colmapWrapper;
        QAction *m_colmapWrapperSettingsAction;
        QPushButton *m_newProductPushButton;
    #endif
    bool m_exporting = false;
    //Prevents multiple drag and drops at the same time
    bool m_isImporting = false;


    void createOpenMessage(int numPics);
    QString getNameFromPath(QString path, QString dataFormat);
    /**
     * @brief onSuccessfulOpen is called if a new import was successfully opend
     *
     */
    void onSuccessfulOpen();
    void setInputWidgetInfo();
    void displayPluginSettings();
    void onFailedOpen();

    // plugin runtime
    QElapsedTimer m_timer;


};

#endif // CONTROLLER_H
