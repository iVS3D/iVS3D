#ifndef MAINWINDOW_H
#define MAINWINDOW_H

/** @defgroup View View
 *
 * @ingroup View
 *
 * @brief manages Gui elements and dialogs for user interaction.
 */

#include <QMainWindow>
#include <QSplitter>
#include <QTimer>
#include <QDesktopWidget>
#include <QLayout>
#include <QApplication>
#include <QSplitter>
#include <QMimeData>
#include <QDockWidget>
#include <QFrame>

#include "view/videoplayer.h"
#include "view/timeline.h"
#include "view/infowidget.h"
#include "view/samplingwidget.h"
#include "view/automaticwidget.h"
#include "view/outputwidget.h"
#include "view/helpdialog.h"
#include "view/about.h"
#include "view/darkstyle/DarkStyle.h"

#include "stringcontainer.h"

#define UI_LAYOUT_VERSION "1.0.0"

//for debug
#include <opencv2/core.hpp>
#include <QDebug>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

/**
 * @class MainWindow
 *
 * @ingroup View
 *
 * @brief The MainWindow class holds all major GUI-components (widgets), has status bar on the bottom left and menu bar on the top left.
 * GUI-components: VideoPlayer, Timeline, Input(-Widget), Sampling(-Widget) and Output(-Widget).
 * the status bar is used by nearly all classes to easily display status or other vital information to the user.
 * the menu bar holds many buttons to open inputs, open and save projects, change various settings and display about-information.
 *
 * @author Lennart Ruck
 *
 * @date 2021/02/08
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * @brief MainWindow sets up all UI elements on the main-window, connects signals&slots for the various widgets
     *
     * @param parent gui-parent (here it is a nullptr)
     * @param dark @a true if darkmode enabled
     * @param cuda @a -1 if cuda is not available, @a 0 if disabled, @a 1 if enabled
     * @param horizontal @a true if horizontal layout, vertical otherwise
     * @param algorithmList holds identifier(names) of all loaded plugins(algorithms)
	 * @param transformList holds identifier(names) of all loaded transform plugins
     */
    MainWindow(QWidget *parent = nullptr, bool dark = false, int cuda = -1, bool createLog = false,  bool interpolateMetaData = true, QStringList algorithmList = QStringList(tr("no algorithm")), QStringList transformList = QStringList(""), QWidget *otsWidget = nullptr);

    /**
      * @brief delete members and disconnect connections
      */
    ~MainWindow();
    /**
     * @brief showProjectTitle used to show loaded project in the titlebar of the main-window
     *
     * @param title project file path or default "not saved"
     */
    void showProjectTitle(const QString &title = tr("not saved"));
    /**
     * @brief getInputWidget getter for the widget which displays all input data
     *
     * @return Pointer to an Infowidget which displays all input data
     */
    InfoWidget *getInputWidget();
    /**
     * @brief getVideoPlayer getter for the element which lets the user interact with the displayed video/images
     *
     * @return Pointer to the Videoplayer
     */
    VideoPlayer *getVideoPlayer();
    /**
     * @brief getSamplingWidget getter for the element which is the ui for every algorithm
     *
     * @return Pointer to the SamplingWidget
     */
    SamplingWidget *getSamplingWidget();
    /**
     * @brief getOutputWidget getter for the element which is used to move through a video/image-list
     *
     * @return Pointer to the Timeline
     */
    Timeline *getTimeline();
    /**
     * @brief getOutputWidget getter for the element which displays export and output data
     *
     * @return Pointer to the OutputWidget
     */
    OutputWidget *getOutputWidget();
    /**
     * @brief getAutoWidget getter for the element which displays the automatic sampling
     *
     * @return Pointer to the AutomaticWidget
     */
    AutomaticWidget *getAutoWidget();
    /**
     * @brief enableSaveProject enables "Save project" button in the menu-bar
     *
     * @param status true = enable
     */
    void enableSaveProject(bool status);
    /**
     * @brief enableOpenMetaData enables "Open Meta Data" button in the menu-bar
     *
     * @param status true = enable
     */
    void enableOpenMetaData(bool status);
    /**
     * @brief enableUndo enables "undo" button in the menu-bar
     *
     * @param status true = enable
     */
    void enableUndo(bool status);
    /**
     * @brief enableRedo enables "redo" button in the menu-bar
     *
     * @param status true = enable
     */
    void enableRedo(bool status);
    /**
     * @brief enableTools enables tool buttons in the menu-bar
     *
     * @param status true = enable
     */
    void enableTools(bool status);

    void addOtsWindow(QWidget *otsWidget);

    void addSettingsAction(QAction *action);

    void enableInputButtons(bool status);

    void enableExportPath(bool status);

    bool getInputEnabled();


signals:
    /**
     * @brief sig_openProject "Open Project" in menu-bar
     */
    void sig_openProject();
    /**
     * @brief sig_saveProject "Save Project" in menu-bar
     */
    void sig_saveProject();
    /**
     * @brief sig_saveProjectAs "Save Project As" in menu-bar
     */
    void sig_saveProjectAs();
    /**
     * @brief sig_openInputFolder "Open Input Folder" in menu-bar
     */
    void sig_openInputFolder();
    /**
     * @brief sig_openInputVideo "Open Input Video" in menu-bar
     */
    void sig_openInputVideo();
    /**
     * @brief sig_openMetaData "Open Meta Data" in menu-bar
     */
    void sig_openMetaData();
    /**
     * @brief sig_changeDefaultInputPath "Change default input path" in menu-bar
     */
    void sig_changeDefaultInputPath();
    /**
     * @brief sig_changeReconstructPath "Add Reconstruct Path" in menu-bar
     */
    void sig_changeReconstructPath();
    /**
     * @brief sig_openVideoDragAndDrop emitted by drag&drop event
     * When a drag&drop event occurs this signal is used to initiate opening the dropped filepath as input/project
     * @param filePath path dropped by the drag&drop event
     */
    void sig_openVideoDragAndDrop(QString filePath);
    /**
     * @brief sig_changeLayoutStyle "Change layout style" in menu-bar
     *
     * @param horizontal layout is in horizontal mode
     */
    void sig_changeLayoutStyle(bool horizontal);
    /**
     * @brief sig_changeDarkStyle "Use DarkStyle" in menu-bar
     *
     * @param dark layout is in dark mode
     */
    void sig_changeDarkStyle(bool dark);
    /**
     * @brief sig_changeUseCuda is emitted if the useCuda option is toggled
     * @param useCuda @a true if cuda is enabled
     */
    void sig_changeUseCuda(bool useCuda);
    /**
     * @brief sig_changeCreateLogFile is emitted if the createLogFile option is toggled
     * @param createLog @a true if createLogFile is enabled
     */
    void sig_changeCreateLogFile(bool createLog);

    /**
     * @brief sig_changeInterpolateMetaData is emitted if the interpolate meta data option is toggled
     * @param interpolate @a true if interpolate is enabled
     */
    void sig_changeInterpolateMetaData(bool interpolate);

    void sig_undo();

    void sig_redo();

    void sig_resetBoundaries();

    void sig_deleteAllKeyframes();

    void sig_deleteKeyframesBoundaries();

    void sig_toggleKeyframe();

    void sig_quit();


public slots:
    /**
     * @brief slot_displayStatusMessage displays given Text in the status bar on the bottom left
     * @param message text to display
     */
    void slot_displayStatusMessage(QString message);


private slots:
    void on_actionOpen_Project_triggered();
    void on_actionSave_Project_triggered();
    void on_actionSave_Project_As_triggered();
    void on_actionOpen_Input_triggered();
    void on_actionSet_Reconstructiontool_Path_triggered();
    void on_actionSet_Input_Path_triggered();
    void on_actionInfo_triggered();
    void on_actionOpen_Input_Video_triggered();
    void on_actionHelp_triggered();
    void on_actionuse_DarkStyle_toggled(bool);
    void on_actionUse_CUDA_triggered();
    void on_actionLicence_triggered();
    void on_actionCreate_log_file_triggered();
    void on_actionOpen_Meta_Data_triggered();
    void on_actionInterpolate_missing_meta_data_triggered();

    void on_actionUndo_triggered();

    void on_actionRedo_triggered();

    void on_actionReset_Boundaries_triggered();

    void on_actionDelete_All_Keyframes_triggered();

    void on_actionDelete_Keyframes_triggered();

private:
    QFrame *addFrame(QWidget *w);

    Ui::MainWindow *ui;
    //delete this: QWidget *layout_widget;
    VideoPlayer *m_videoplayer;
    InfoWidget *m_inputWidget;
    SamplingWidget *m_samplingWidget;
    OutputWidget *m_outputWidget;
    Timeline *m_timeline;
    AutomaticWidget *m_autoWidget;
    const QString m_appName = "intelligent Video Sampler 3D";
    std::vector<uint> generateKeyframes(uint totalFrames, uint keyframeCount);
    void dropEvent(QDropEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void readSettings();
    void closeEvent(QCloseEvent *event) override;
    QDockWidget *m_outputDock;
    QDockWidget *m_reconstructDock;
};
#endif // MAINWINDOW_H
