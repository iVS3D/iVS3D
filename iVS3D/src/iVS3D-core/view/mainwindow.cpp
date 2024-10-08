#include "mainwindow.h"
#include "ui_mainwindow.h"


#include <QDesktopWidget>
#include <QLayout>
#include <QWindow>
#include <QHBoxLayout>
#include <QApplication>
#include <QSplitter>
#include "view/about.h"
#include "view/licencedialog.h"
#include "view/darkstyle/DarkStyle.h"
#include <QMimeData>

//for debug
#include <opencv2/core.hpp>
#include <QDebug>



MainWindow::MainWindow(QWidget *parent, ColorTheme theme, int cuda, bool createLog, bool interpolateMetaData, QList<QLocale> locales, QLocale selectedLocale, QStringList algorithmList, QStringList transformList, QWidget *otsWidget)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

#if defined(Q_OS_WIN)
    addSettingsAction(ui->actionSet_Reconstructiontool_Path);
#endif

    // --- create GUI Elements VideoPlayer, Timeline and Info-Widgets with Buttons
    // --- for sampling and export of images or 3d-reconstruction

    // all GUI elements have main window as parent
    m_videoplayer = new VideoPlayer(this, theme);
    m_timeline = new Timeline(this);
    m_inputWidget = new InfoWidget(this, "Input", theme);
    m_samplingWidget = new SamplingWidget(this, algorithmList, transformList);
    m_outputWidget = new OutputWidget(this, "Output", transformList);
    m_autoWidget = new AutomaticWidget(this);
    m_autoWidget->setVisible(false);

    // initialize the layout of GUI elements
    m_videoplayer->addWidgetToLayout(m_timeline); // timeline is part of videoplayer-layout
    setCentralWidget(m_videoplayer);
    setDockNestingEnabled(true);
    QDockWidget *dock;

    dock = new QDockWidget(tr("Input"), this);
    dock->setObjectName("Input");
    dock->setAllowedAreas(
                Qt::BottomDockWidgetArea |
                Qt::TopDockWidgetArea |
                Qt::LeftDockWidgetArea |
                Qt::RightDockWidgetArea);

    dock->setWidget(addFrame(m_inputWidget));
    addDockWidget(Qt::BottomDockWidgetArea, dock);
    ui->menuView->addAction(dock->toggleViewAction());

#ifdef HIDE_INPUT_WIDGET
    dock->setVisible(false);
#endif

    /*QDockWidget *d2 = dock;
    dock = new QDockWidget(tr("Operation Stack"), this);
    dock->setObjectName("Stack");
    dock->setAllowedAreas(
                Qt::BottomDockWidgetArea |
                Qt::TopDockWidgetArea |
                Qt::LeftDockWidgetArea |
                Qt::RightDockWidgetArea);
    dock->setWidget(addFrame(m_opStack));
    addDockWidget(Qt::BottomDockWidgetArea, dock);
    ui->menuView->addAction(dock->toggleViewAction());
    splitDockWidget(d2,dock,Qt::Vertical);*/

#ifdef HIDE_INPUT_WIDGET
    dock->setVisible(false);
#endif

    dock = new QDockWidget(tr("Image selection"), this);
    dock->setObjectName("Image selection");
    dock->setAllowedAreas(
                Qt::BottomDockWidgetArea |
                Qt::TopDockWidgetArea |
                Qt::LeftDockWidgetArea |
                Qt::RightDockWidgetArea);
    dock->setWidget(addFrame(m_samplingWidget));
    addDockWidget(Qt::BottomDockWidgetArea, dock);
    ui->menuView->addAction(dock->toggleViewAction());

    dock = new QDockWidget(tr("Export"), this);
    dock->setObjectName("Export");
    dock->setAllowedAreas(
                Qt::BottomDockWidgetArea |
                Qt::TopDockWidgetArea |
                Qt::LeftDockWidgetArea |
                Qt::RightDockWidgetArea);
    dock->setWidget(addFrame(m_outputWidget));
    m_outputDock = dock;
    addDockWidget(Qt::BottomDockWidgetArea, dock);
    ui->menuView->addAction(dock->toggleViewAction());

    if(otsWidget) {
        addOtsWindow(otsWidget);
    }



    // store default layout state and add action to reset to this default state
    const auto defaultDockLayout = saveState();
    ui->menuView->addAction(tr("Reset Layout"), [this,defaultDockLayout](){
        this->restoreState(defaultDockLayout);
    }, QKeySequence(Qt::CTRL | Qt::Key_L));

    // read layout and geometry from last session if available
    readSettings();
    m_outputDock->raise();

    // connect input widget
    connect(m_inputWidget, &InfoWidget::sig_openFolderPressed, this, &MainWindow::on_actionOpen_Input_triggered);
    connect(m_inputWidget, &InfoWidget::sig_openVideoPressed, this, &MainWindow::on_actionOpen_Input_Video_triggered);
    connect(m_inputWidget, &InfoWidget::sig_openMetaPressed, this, &MainWindow::on_actionOpen_Meta_Data_triggered);

    // connect videoplayer widget
    connect(m_videoplayer, &VideoPlayer::sig_deleteAllKeyframes, this, &MainWindow::on_actionDelete_All_Keyframes_triggered);

    this->showProjectTitle();


    //This is to ensure opencv is incorperated correctly
    cv::Mat myMat;

    // --- initialize GUI elements with placeholders
    m_inputWidget->setInfo(QMap<QString,QString>());

    // --- disable all GUI elements
    // --- until some image data is loaded
    m_timeline->setEnabled(false);
    m_videoplayer->setEnabled(false);
    m_inputWidget->setEnabled(true);
    m_samplingWidget->setEnabled(false);
    m_outputWidget->setEnabled(false);
    //m_opStack->setEnabled(false);

    // --- use dark style if selected
    this->setColorTheme(theme);

    if(cuda < 0){
        ui->actionUse_CUDA->setEnabled(false);
        QString err_msg;
        switch (cuda) {
        case -1: err_msg = tr("No GPU found"); break;
        case -2: err_msg = tr("Compute Capability not supported"); break;
        case -3: err_msg = tr("Built without CUDA"); break;
        default:
            err_msg = tr("Unknown error"); break;
        }
        ui->actionUse_CUDA->setText(tr("CUDA not available: ") + err_msg);
    } else {
        ui->actionUse_CUDA->setEnabled(true);
        ui->actionUse_CUDA->setChecked(cuda);
    }

    // set create log file
    ui->actionCreate_log_file->setChecked(createLog);
    // set interpolate meta data
    ui->actionInterpolate_missing_meta_data->setChecked(interpolateMetaData);

    //enable Drag and Drop
    setAcceptDrops(true);
    //disable automatic execution
    ui->actionStartAutoExec->setEnabled(false);

    ui->actionOpen_Input->setIcon(QIcon(theme == DARK ? ":/icons/openFolderIconW" : ":/icons/openFolderIconB"));
    ui->actionOpen_Input_Video->setIcon( QIcon(theme == DARK ? ":/icons/openVideoIconW"  : ":/icons/openVideoIconB"));
    ui->actionOpen_Meta_Data->setIcon( QIcon(theme == DARK ? ":/icons/openMetaIconW"  : ":/icons/openMetaIconB"));

    QMenu *languageMenu = new QMenu(tr("Language"), this);

    // Create an action group for exclusive selection
    QActionGroup *actionGroup = new QActionGroup(this);
    actionGroup->setExclusive(true);

    // Add actions for each available locale
    for (const QLocale &locale : locales) {
        QString name = locale.nativeLanguageName();
        if(name.contains("English")){
            name = "English";
        }
        QAction *action = new QAction(name, this);
        action->setData(locale);
        action->setCheckable(true);

        // Highlight the selected language
        if (locale.language() == selectedLocale.language()) {
            action->setChecked(true);
        }

        connect(action, &QAction::triggered, this, &MainWindow::on_changeLanguage);
        actionGroup->addAction(action);
        languageMenu->addAction(action);
    }

    ui->menuSettings->addMenu(languageMenu);
}

MainWindow::~MainWindow()
{
    delete ui;
    delete m_videoplayer;
    delete m_timeline;
    delete m_inputWidget;
    delete m_samplingWidget;
    delete m_outputWidget;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QSettings settings(stringContainer::settingsCompany, stringContainer::settingsProgramm);
    settings.setValue("layoutVersion", QString(UI_LAYOUT_VERSION));
    settings.setValue("windowGeometry", QVariant(geometry()));
    settings.setValue("windowState", saveState());
    settings.setValue("maximized", this->isMaximized());
    emit sig_quit();
    QMainWindow::closeEvent(event);
}

void MainWindow::readSettings()
{
    QSettings settings(stringContainer::settingsCompany, stringContainer::settingsProgramm);
    if(
        settings.value("layoutVersion").isNull()                                                // no version -> very old layout file, dont use!
        || settings.value("layoutVersion").value<QString>().compare(QString(UI_LAYOUT_VERSION)) != 0     // version of stored layout does not match current layout, dont use!
        ){
        qDebug() << "Found incompatible layout file. Falling back to default!";
        return;
    }
    if(!settings.value("windowGeometry").isNull()){
        resize(settings.value("windowGeometry").value<QRect>().size());
        restoreState(settings.value("windowState").toByteArray());
        if(settings.value("maximized").toBool()) this->setWindowState(this->windowState() | Qt::WindowMaximized);
    }
}

void MainWindow::showProjectTitle(const QString &title)
{
    this->setWindowTitle(m_appName + " [" + title + "]");
}

void MainWindow::slot_displayStatusMessage(QString message)
{
    statusBar()->showMessage(message);
}


VideoPlayer *MainWindow::getVideoPlayer()
{
    return m_videoplayer;
}


Timeline *MainWindow::getTimeline()
{
    return m_timeline;
}

OutputWidget *MainWindow::getOutputWidget()
{
    return m_outputWidget;
}

AutomaticWidget *MainWindow::getAutoWidget()
{
    return m_autoWidget;
}


InfoWidget *MainWindow::getInputWidget()
{
    return m_inputWidget;
}


SamplingWidget *MainWindow::getSamplingWidget()
{
    return m_samplingWidget;
}


void MainWindow::enableSaveProject(bool status)
{
    ui->actionSave_Project->setEnabled(status);
    ui->actionSave_Project_As->setEnabled(status);
}

void MainWindow::enableOpenMetaData(bool status)
{
    ui->actionOpen_Meta_Data->setEnabled(status);
    m_inputWidget->enableOpenMetaData(status);
}

void MainWindow::enableUndo(bool status)
{
    ui->actionUndo->setEnabled(status);
}

void MainWindow::enableRedo(bool status)
{
    ui->actionRedo->setEnabled(status);
}

void MainWindow::enableTools(bool status)
{
    ui->actionDelete_All_Keyframes->setEnabled(status);
    ui->actionDelete_Keyframes->setEnabled(status);
    ui->actionReset_Boundaries->setEnabled(status);
}

void MainWindow::addOtsWindow(QWidget *otsWidget)
{
    QDockWidget *dock = new QDockWidget(tr("3D-Reconstruction"), this);
    dock->setObjectName("3D-Reconstruction");
    dock->setAllowedAreas(
                Qt::BottomDockWidgetArea |
                Qt::TopDockWidgetArea |
                Qt::LeftDockWidgetArea |
                Qt::RightDockWidgetArea);
    dock->setWidget(addFrame(otsWidget));
    m_reconstructDock = dock;
    addDockWidget(Qt::RightDockWidgetArea, dock);
    tabifyDockWidget(m_outputDock, dock);
    m_outputDock->raise();
    ui->menuView->insertAction(m_outputDock->toggleViewAction(), dock->toggleViewAction());
}

void MainWindow::addSettingsAction(QAction *action)
{
    ui->menuSettings->insertAction(ui->actionSet_Input_Path, action);
}

void MainWindow::enableInputButtons(bool status)
{
    QString tooltip = status ? QString() : tr("input has been passed as a start argument. Thus it can not be changed!");
    this->m_inputWidget->enableOpenImages(status, tooltip);
    this->m_inputWidget->enableOpenVideo(status, tooltip);
    this->m_inputWidget->enableOpenMetaData(status, tooltip);

    auto enableAction = [status, tooltip](QAction *action){
        action->setEnabled(status);
        if(status == false) action->setToolTip(tooltip); // if disabled -> show reason in tooltip
    };

    enableAction(ui->actionOpen_Input);
    enableAction(ui->actionOpen_Input_Video);
    enableAction(ui->actionOpen_Meta_Data);
    enableAction(ui->actionOpen_Project);
}

void MainWindow::enableExportPath(bool status)
{
    this->m_outputWidget->enableExportPathChange(status);
}

bool MainWindow::getInputEnabled()
{
    return ui->actionOpen_Input->isEnabled();
}

void MainWindow::setColorTheme(ColorTheme theme)
{
    // assign a color palette for each theme
    QApplication::setStyle(theme==ColorTheme::DARK ? new DarkStyle : QStyleFactory::create("Fusion"));
    // update icon colors according to the theme
    ui->actionOpen_Input->setIcon(QIcon(theme == DARK ? ":/icons/openFolderIconW" : ":/icons/openFolderIconB"));
    ui->actionOpen_Input_Video->setIcon( QIcon(theme == DARK ? ":/icons/openVideoIconW"  : ":/icons/openVideoIconB"));
    ui->actionOpen_Meta_Data->setIcon( QIcon(theme == DARK ? ":/icons/openMetaIconW"  : ":/icons/openMetaIconB"));
    ui->actionRedo->setIcon(QIcon(theme == DARK ? ":/icons/redoIconW"  : ":/icons/redoIconB"));
    ui->actionUndo->setIcon(QIcon(theme == DARK ? ":/icons/undoIconW"  : ":/icons/undoIconB"));
    ui->actionDelete_All_Keyframes->setIcon(QIcon(theme == DARK ? ":/icons/resetIconW"  : ":/icons/resetIconB"));

    // notify children to update colors as well
    m_inputWidget->setColorTheme(theme);
    m_videoplayer->setColorTheme(theme);
}

OperationStack *MainWindow::getOpStack()
{
    return m_inputWidget->getOpStack();
}

void MainWindow::on_actionOpen_Project_triggered()
{
    emit sig_openProject();
}

void MainWindow::on_actionSave_Project_triggered()
{
    emit sig_saveProject();
}

void MainWindow::on_actionSave_Project_As_triggered()
{
    emit sig_saveProjectAs();
}

void MainWindow::on_actionOpen_Input_triggered()
{
    emit sig_openInputFolder();
}

void MainWindow::on_actionSet_Reconstructiontool_Path_triggered()
{
    emit sig_changeReconstructPath();
}

void MainWindow::on_actionSet_Input_Path_triggered()
{
    emit sig_changeDefaultInputPath();
}

void MainWindow::on_actionInfo_triggered()
{
    //open About Dialog
    QDialog *aboutDialog = new About(this);
    aboutDialog->exec();
}

void MainWindow::on_actionOpen_Input_Video_triggered()
{
    emit sig_openInputVideo();
}

void MainWindow::on_actionHelp_triggered()
{
    //open Help Dialog
    QDialog *help = new helpDialog(this);
    help->exec();
}


std::vector<uint> MainWindow::generateKeyframes(uint totalFrames, uint keyframeCount)
{
    std::vector<uint> keyframes;
    // reset vector
    keyframes.clear();

    while (keyframeCount != 0) {
        uint index = rand() % totalFrames;
        if (keyframes.empty() || std::find(keyframes.begin(), keyframes.end(), index) == keyframes.end()) {
            // the frame is not a keyframe
            keyframes.push_back(index);
            keyframeCount--;
        }
    }

    std::sort(keyframes.begin(), keyframes.end());

    return keyframes;
}

void MainWindow::on_actionToggleTheme_triggered()
{
    //QApplication::setStyle(checked ? new DarkStyle : QStyleFactory::create("Fusion"));
    emit sig_toggleTheme();
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const QList<QUrl> &url = event->mimeData()->urls();
    if (url.length() <= 0) {
        return;
    }
    QString filePath = url[0].toLocalFile();
    qDebug() << "Dropped file:" << filePath;
    emit sig_openVideoDragAndDrop(filePath);
}

void MainWindow::on_actionUse_CUDA_triggered()
{
    emit sig_changeUseCuda(ui->actionUse_CUDA->isChecked());
}

void MainWindow::on_actionLicence_triggered()
{
    //open licence Dialog
    QDialog *licence = new LicenceDialog(this);
    licence->exec();
}

void MainWindow::on_actionCreate_log_file_triggered()
{
    emit sig_changeCreateLogFile(ui->actionCreate_log_file->isChecked());
}

void MainWindow::on_actionOpen_Meta_Data_triggered()
{
    emit sig_openMetaData();
}

void MainWindow::on_actionInterpolate_missing_meta_data_triggered()
{
    emit sig_changeInterpolateMetaData(ui->actionInterpolate_missing_meta_data->isChecked());
}

void MainWindow::on_actionUndo_triggered()
{
    emit sig_undo();
}

void MainWindow::on_actionRedo_triggered()
{
    emit sig_redo();
}

void MainWindow::on_actionReset_Boundaries_triggered()
{
    emit sig_resetBoundaries();
}

void MainWindow::on_actionDelete_All_Keyframes_triggered()
{
    emit sig_deleteAllKeyframes();
}

void MainWindow::on_actionDelete_Keyframes_triggered()
{
    emit sig_deleteKeyframesBoundaries();
}

void MainWindow::on_changeLanguage()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (action && action->isChecked()) {
        // Retrieve the selected locale from the action's data
        QLocale selectedLocale = action->data().toLocale();
        QString msg = "Selected Language: " + selectedLocale.nativeLanguageName();
        slot_displayStatusMessage(msg);
        emit sig_selectLanguage(selectedLocale);

        // the language matches the currently active language -> no restart required
        if (selectedLocale.language() == qApp->property("translation").toLocale().language()){
            return;
        }

        // Otherwise ask the user to restart the application
        QMessageBox msgBox(this);
        msgBox.setWindowTitle(tr("Language Change"));
        msgBox.setText(tr("The application needs to be restarted for the language change to take effect. Make sure to save your project before restarting!"));

        // Change the text of the OK button
        msgBox.setButtonText(QMessageBox::Ok, tr("Restart Now"));
        // Add a "Restart Later" button
        msgBox.addButton(tr("Restart Later"), QMessageBox::RejectRole);


        int result = msgBox.exec();

        if (result == QMessageBox::Ok) {
            emit sig_restart();
        }
    }
}

QFrame *MainWindow::addFrame(QWidget *w)
{
     QLayout *l = new QHBoxLayout();
     l->addWidget(w);
     QFrame *f = new QFrame();
     f->setFrameShape(QFrame::Box);
     //QPalette* palette = new QPalette();
     //palette->setColor(QPalette::Foreground,QColor(0,0,0,50));
     //f->setPalette(*palette);
     f->setObjectName("myObject");
     f->setStyleSheet("#myObject { border: 1px solid rgba(0,0,0,0.3); }");
     f->setLayout(l);
     return f;
}


