#include "controller.h"


Controller::Controller(QString inputPath, QString settingsPath, QString outputPath, QString logPath)
    #if defined(Q_OS_LINUX)
        : m_colmapWrapper(new lib3d::ots::ColmapWrapper)
    #endif
{
    m_videoPlayerController = nullptr;
    m_algorithmController = nullptr;
    QStringList algorithms = AlgorithmManager::instance().getAlgorithmNames();
    QStringList transforms = TransformManager::instance().getTransformList();
    int useCuda = -1;
    if(ApplicationSettings::instance().getCudaAvailable()){
        useCuda = ApplicationSettings::instance().getUseCuda();
    }

    QWidget *otsWidget = nullptr;
#if defined(Q_OS_LINUX)
    const auto otsTheme = ApplicationSettings::instance().getDarkStyle() ? lib3d::ots::ui::ETheme::DARK : lib3d::ots::ui::ETheme::LIGHT;

    otsWidget = new QWidget;
    otsWidget->setLayout(new QVBoxLayout);
    //otsWidget->layout()->addWidget(m_colmapWrapper->getOrCreateUiControlsFactory()->createSettingsPushButton());
    otsWidget->layout()->addWidget(m_colmapWrapper->getOrCreateUiControlsFactory()->createViewWidget(nullptr));
    otsWidget->layout()->addWidget(m_colmapWrapper->getOrCreateUiControlsFactory()->createNewProductPushButton(otsTheme, nullptr));

#endif
    bool interpolateMetaData = ApplicationSettings::instance().getInterpolateMetaData();
    m_mainWindow = new MainWindow(
                nullptr,
                ApplicationSettings::instance().getDarkStyle(),
                useCuda,
                ApplicationSettings::instance().getCreateLogs(),
                interpolateMetaData,
                algorithms,
                transforms,
                otsWidget
                );

    m_mainWindow->enableUndo(false);
    m_mainWindow->enableRedo(false);
    m_mainWindow->enableTools(false);

#if defined(Q_OS_LINUX)
    m_mainWindow->addSettingsAction(m_colmapWrapper->getOrCreateUiControlsFactory()->createSettingsAction(otsTheme, nullptr));

    m_colmapWrapper->getOrCreateUiControlsFactory()->updateIconTheme(otsTheme);
#endif

    if(AlgorithmManager::instance().getAlgorithmCount() + TransformManager::instance().getTransformCount() >0){
        displayPluginSettings();
    }
    TransformManager::instance().enableCuda(ApplicationSettings::instance().getUseCuda());

    LogManager::instance().toggleLog(ApplicationSettings::instance().getCreateLogs());

    m_dataManager = new DataManager();
    m_exportController = nullptr;

    m_openExec = nullptr;

    connect(m_mainWindow, &MainWindow::sig_openProject, this, &Controller::slot_openProject);
    connect(m_mainWindow, &MainWindow::sig_openInputFolder, this, &Controller::slot_openInputFolder);
    connect(m_mainWindow, &MainWindow::sig_openInputVideo, this, &Controller::slot_openInputVideo);
    connect(m_mainWindow, &MainWindow::sig_openVideoDragAndDrop, this, &Controller::slot_openVideoDragAndDrop);
    connect(m_mainWindow, &MainWindow::sig_saveProjectAs, this, &Controller::slot_saveProjectAs);
    connect(m_mainWindow, &MainWindow::sig_saveProject, this, &Controller::slot_saveProject);
    connect(m_mainWindow, &MainWindow::sig_changeReconstructPath, this, &Controller::slot_addReconstructPath);
    connect(m_mainWindow, &MainWindow::sig_changeDefaultInputPath, this, &Controller::slot_changeDefaultInputPath);
    connect(m_mainWindow, &MainWindow::sig_changeDarkStyle, this, &Controller::slot_changeDarkStyle);
    connect(m_mainWindow, &MainWindow::sig_changeUseCuda, this, &Controller::slot_changeUseCuda);
    connect(m_mainWindow, &MainWindow::sig_changeCreateLogFile, this, &Controller::slot_changeCreateLogFile);
    connect(m_mainWindow, &MainWindow::sig_openMetaData, this, &Controller::slot_openMetaData);
    connect(m_mainWindow, &MainWindow::sig_undo, this, &Controller::slot_undo);
    connect(m_mainWindow, &MainWindow::sig_redo, this, &Controller::slot_redo);
#if defined(Q_OS_LINUX)
    connect(m_mainWindow, &MainWindow::sig_quit, m_colmapWrapper->getOrCreateUiControlsFactory(), &lib3d::ots::ui::ColmapWrapperControlsFactory::onQuit);
#endif
    connect(m_mainWindow, &MainWindow::sig_changeInterpolateMetaData, this, &Controller::slot_changeInterpolateMetaData);

    connect(this, &Controller::sig_hasStatusMessage, m_mainWindow, &MainWindow::slot_displayStatusMessage);

    m_automaticController = new AutomaticController(m_mainWindow->getOutputWidget(), m_mainWindow->getAutoWidget(), m_mainWindow->getSamplingWidget(), m_dataManager);

    m_mainWindow->show();


    if (logPath != nullptr && !logPath.isEmpty()) {
        LogManager::instance().setLogDirectory(logPath);
    }

    if (inputPath != nullptr && !inputPath.isEmpty()) {
        m_mainWindow->enableInputButtons(false);
        m_timer = QElapsedTimer();
        m_timer.start();
        createOpenMessage(m_dataManager->open(inputPath));
    }

    if(outputPath != nullptr && !outputPath.isEmpty()){
        m_mainWindow->enableExportPath(false);
        m_mainWindow->getOutputWidget()->setOutputPath(outputPath);
    }

    //Disable 'create files for' widget when no transform plugins are found
    m_mainWindow->getOutputWidget()->enableCreateFilesWidget(TransformManager::instance().getTransformCount() != 0);
    MetaDataManager::instance().interpolateMissingMetaData(interpolateMetaData);

}

Controller::~Controller()
{
    #if defined(Q_OS_LINUX)
        delete m_colmapWrapper;
    #endif
    TransformManager::instance().exit();
}

void Controller::slot_openInputFolder()
{
    if(m_exporting){
        QMessageBox msgBox;
        msgBox.setText(tr("Wait for export to finish before importing new images."));
        msgBox.exec();
        return;
    }
    QString folderPath = QFileDialog::getExistingDirectory(m_mainWindow, tr("Choose Folder"), ApplicationSettings::instance().getStandardInputPath(), QFileDialog::DontUseNativeDialog);
    if (folderPath == nullptr) {
        emit sig_hasStatusMessage(tr("Input canceled"));
        return;
    }
    m_timer = QElapsedTimer();
    m_timer.start();
    createOpenMessage(m_dataManager->open(folderPath));
}

void Controller::slot_openInputVideo()
{
    if(m_exporting){
        QMessageBox msgBox;
        msgBox.setText(tr("Wait for export to finish before importing a new video."));
        msgBox.exec();
        return;
    }
    QString selectedFilter = "";
    QString folderPath = QFileDialog::getOpenFileName(m_mainWindow, tr("Choose Video"), ApplicationSettings::instance().getStandardInputPath(), "*.mp4 *.mov *.avi", &selectedFilter, QFileDialog::DontUseNativeDialog);
    if (folderPath == nullptr) {
        emit sig_hasStatusMessage(tr("Input canceled"));
        return;
    }

    if(m_openExec){
        delete m_openExec;
    }
    m_openExec = new OpenExecutor(folderPath, m_dataManager);
    connect(m_openExec, &OpenExecutor::sig_finished, this, &Controller::slot_openFinished);

    m_timer = QElapsedTimer();
    m_timer.start();
    m_openExec->open();
}

void Controller::slot_openVideoDragAndDrop(QString filePath)
{
    if(m_isImporting) {
        return;
    }


    if(m_exporting){
        QMessageBox msgBox;
        msgBox.setText(tr("Wait for export to finish before importing new files."));
        msgBox.exec();
        return;
    }
    if (filePath.endsWith(".mp4", Qt::CaseInsensitive) ||
            filePath.endsWith(".mov", Qt::CaseInsensitive) ||
            filePath.endsWith(".avi", Qt::CaseInsensitive) ||
            QDir(filePath).exists() ||
            filePath.endsWith(".json", Qt::CaseInsensitive))
    {
        if(m_openExec){
            delete m_openExec;
        }
        m_isImporting = true;
        m_openExec = new OpenExecutor(filePath, m_dataManager);
        connect(m_openExec, &OpenExecutor::sig_finished, this, &Controller::slot_openFinished);
        m_timer = QElapsedTimer();
        m_timer.start();
        m_openExec->open();
    }
}

void Controller::slot_openProject()
{
    if(m_exporting){
        QMessageBox msgBox;
        msgBox.setText(tr("Wait for export to finish before importing new project."));
        msgBox.exec();
        return;
    }
    QString selectedFilter = "";
    QString folderPath = QFileDialog::getOpenFileName(m_mainWindow, tr("Choose project file"), ApplicationSettings::instance().getStandardInputPath(), "*.json", &selectedFilter, QFileDialog::DontUseNativeDialog);

    if (folderPath != nullptr) {

        if(m_openExec){
            delete m_openExec;
        }
        m_openExec = new OpenExecutor(folderPath, m_dataManager);
        connect(m_openExec, &OpenExecutor::sig_finished, this, &Controller::slot_openFinished);

        m_timer = QElapsedTimer();
        m_timer.start();
        m_openExec->open();
    }
    else {
       emit sig_hasStatusMessage(tr("Input canceled"));
    }
}

void Controller::slot_saveProjectAs()
{
    QString selectedFilter = "";
    QString projectPath = QFileDialog::QFileDialog::getSaveFileName (m_mainWindow, tr("Save project"), ApplicationSettings::instance().getStandardInputPath(), "*.json", &selectedFilter, QFileDialog::DontUseNativeDialog);
    if (projectPath == nullptr) {
        emit sig_hasStatusMessage(tr("Input canceled"));
        return;
    }

    if (0 != QString::compare(projectPath.split(".").last(), "json", Qt::CaseSensitive)) {
        projectPath += ".json";
    }
    m_dataManager->saveProjectAs(getNameFromPath(projectPath, ".json"), projectPath);
    m_mainWindow->showProjectTitle(m_dataManager->getProjectPath());
    emit sig_hasStatusMessage(tr("Project saved"));
}

void Controller::slot_saveProject()
{
    if (m_dataManager->isProjectLoaded()) {
        m_dataManager->saveProject();
        emit sig_hasStatusMessage(tr("Project saved"));
        return;
    }
    slot_saveProjectAs();
}

void Controller::slot_addReconstructPath()
{
    ReconstructionToolsDialog rtd(m_mainWindow);
    rtd.exec();
}


void Controller::slot_changeDefaultInputPath()
{
    QString folderPath = QFileDialog::getExistingDirectory(m_mainWindow, tr("Choose standard input path"), ApplicationSettings::instance().getStandardInputPath(), QFileDialog::DontUseNativeDialog);
    if (folderPath == nullptr) {
        emit sig_hasStatusMessage(tr("Input canceled"));
        return;
    }
    ApplicationSettings::instance().setStandardInputPath(folderPath);

    emit sig_hasStatusMessage(tr("Standard input path changed"));
}

void Controller::slot_changeDarkStyle(bool dark)
{
    ApplicationSettings::instance().setDarkStyle(dark);
    emit sig_hasStatusMessage(tr("GUI changed to ") + QString((dark ? tr("dark") : tr("light"))) + tr(" style -- restart to activate changes"));
}

void Controller::slot_changeUseCuda(bool useCuda)
{
    ApplicationSettings::instance().setUseCuda(useCuda);
    if(!m_exporting){
        TransformManager::instance().enableCuda(ApplicationSettings::instance().getUseCuda());
    }
    emit sig_hasStatusMessage(useCuda ? tr("CUDA enabled") : tr("CUDA disabled"));
}

void Controller::slot_changeCreateLogFile(bool createLog)
{
    ApplicationSettings::instance().setCreateLogs(createLog);
    QString msg = tr("Create log files") + (QString)(createLog ? tr(" enabled") : tr(" disabled"));
    LogManager::instance().toggleLog(createLog);
    emit sig_hasStatusMessage(msg);
}

void Controller::slot_changeInterpolateMetaData(bool interpolate)
{
    ApplicationSettings::instance().setInterpolateMetaData(interpolate);
    QString msg = tr("Interpolating missing meta data") + (QString)(interpolate ? tr(" enabled") : tr(" disabled"));
    emit sig_hasStatusMessage(msg);
    MetaDataManager::instance().interpolateMissingMetaData(interpolate);
}

void Controller::slot_openMetaData()
{
    QString selectedFilter = "";
    QString filePath = QFileDialog::getOpenFileName(m_mainWindow, tr("Choose Meta Data"), ApplicationSettings::instance().getStandardInputPath(), "*.srt *.jpeg *.jpg", &selectedFilter, QFileDialog::DontUseNativeDialog);
    if (filePath == nullptr) {
        emit sig_hasStatusMessage(tr("Input canceled"));
        return;
    }
    int n = m_dataManager->getModelInputPictures()->loadMetaData(QStringList(filePath));
    if (n > 0) {
        AlgorithmManager::instance().notifyNewMetaData();
    }
    QString msg = tr("Loaded ") + QString::number(n) + tr(" meta data feature") + QString(n > 1 ? tr("s") : "");
    emit sig_hasStatusMessage(msg);

}

void Controller::slot_openFinished(int result)
{
    disconnect(m_openExec, &OpenExecutor::sig_finished, this, &Controller::slot_openFinished);

    delete m_openExec;
    m_openExec = nullptr;

    m_isImporting = false;

    createOpenMessage(result);
}

void Controller::slot_exportStarted()
{
    m_exporting = true;
    TransformManager::instance().enableCuda(false);
}

void Controller::slot_exportFinished()
{
    m_exporting = false;
    TransformManager::instance().enableCuda(ApplicationSettings::instance().getUseCuda());
}

void Controller::slot_undo()
{
    m_dataManager->getHistory()->undo();
}

void Controller::slot_redo()
{
    m_dataManager->getHistory()->redo();
}

void Controller::slot_historyChanged()
{
    m_mainWindow->enableRedo(m_dataManager->getHistory()->hasFuture());
    m_mainWindow->enableUndo(m_dataManager->getHistory()->hasPast());
}

void Controller::createOpenMessage(int numPics)
{
    auto duration_ms = m_timer.elapsed();
    if (numPics <= 0) {
        emit sig_hasStatusMessage(tr("No images found after ") + QString::number(duration_ms) + tr("ms"));
        onFailedOpen();
    }
    else {

        MetaData* md =  m_dataManager->getModelInputPictures()->getReader()->getMetaData();
        int metaDataCount = 0;
        if (md) {
            metaDataCount = md->loadAllMetaData().size();
        }

        if (metaDataCount != 0) {
            emit sig_hasStatusMessage(tr("Import of ") + QString::number(numPics) + tr(" images and ")
                                      + QString::number(metaDataCount) + tr(" meta data feature") + QString(metaDataCount > 1 ? tr("s") : "")
                                      + tr(" finished after ") + QString::number(duration_ms) + tr("ms"));
        }
        else {
            emit sig_hasStatusMessage(tr("Import of ") + QString::number(numPics) + tr(" images finished after ") + QString::number(duration_ms) + tr("ms"));
        }
        onSuccessfulOpen();
    }
}


QString Controller::getNameFromPath(QString path, QString dataFormat) {
    int slashIndex = path.lastIndexOf("/");
    //Cut chars from start to last /
    QString name = path.mid(slashIndex + 1);
    //Cut ending data format
    name.chop(dataFormat.size());
    return name;
}

void Controller::setInputWidgetInfo() {
    QMap<QString, QString> info;
    QString picCount = QString::number(m_dataManager->getModelInputPictures()->getPicCount());
    info.insert(tr("#Frames  "), picCount);
    QString x = QString::number(m_dataManager->getModelInputPictures()->getInputResolution().x());
    QString y = QString::number(m_dataManager->getModelInputPictures()->getInputResolution().y());
    QString resolution = x + " x " + y;
    info.insert(tr("Resolution  "), resolution);
    info.insert(stringContainer::inputPathIdentifier, m_dataManager->getModelInputPictures()->getPath());
    Reader* currentReader = m_dataManager->getModelInputPictures()->getReader();
    if(currentReader->getFPS() != -1) {
        info.insert(tr("FPS "), QString::number(currentReader->getFPS()));
        info.insert(tr("Video duration "), QString::number(currentReader->getVideoDuration()) + "s");
    }
    QStringList loadedMetaData = MetaDataManager::instance().availableMetaData();
    if (loadedMetaData.size() != 0) {
        for (int i = 0; i < loadedMetaData.size(); i++) {
            info.insert(tr("Loaded Meta Data") + " #" + QString::number(i + 1), loadedMetaData.at(i));
        }
    }
    m_mainWindow->getInputWidget()->setInfo(info);
}

void Controller::displayPluginSettings()
{
    SamplingWidget *samplingW = m_mainWindow->getSamplingWidget();
    QWidget *settingsW;
    if(AlgorithmManager::instance().getAlgorithmCount()>0){
        settingsW = AlgorithmManager::instance().getSettingsWidget(samplingW,0);
    }else {
        settingsW = TransformManager::instance().getSettingsWidget(samplingW,0);
    }
    samplingW->showAlgorithmSettings(settingsW);
}

void Controller::onFailedOpen()
{
    // --- called after image data has been loaded succesfully
    // --- setup the GUI with the new data
    m_mainWindow->enableSaveProject(false);
    m_mainWindow->enableUndo(false);
    m_mainWindow->enableRedo(false);
    QMap<QString, QString> info;
    m_mainWindow->getInputWidget()->setInfo(info);
    m_mainWindow->enableOpenMetaData(false);
    m_mainWindow->enableTools(false);
    // remove old controllers if existing
    if(m_videoPlayerController)
    {
        disconnect(m_videoPlayerController, &VideoPlayerController::sig_hasStatusMessage, m_mainWindow, &MainWindow::slot_displayStatusMessage);
        delete m_videoPlayerController;
        m_videoPlayerController = nullptr;
    }
    if(m_algorithmController) {
        disconnect(m_algorithmController, &AlgorithmController::sig_hasStatusMessage, m_mainWindow, &MainWindow::slot_displayStatusMessage);
        delete m_algorithmController;
        m_algorithmController = nullptr;
    }
    if (m_exportController)
    {
        disconnect(m_exportController, &ExportController::sig_hasStatusMessage, m_mainWindow, &MainWindow::slot_displayStatusMessage);
        delete m_exportController;
        m_exportController = nullptr;
    }
    m_automaticController->disableAutoWidget();

}

void Controller::onSuccessfulOpen()
{
    // --- called after image data has been loaded succesfully
    // --- setup the GUI with the new data
    m_mainWindow->enableSaveProject(true);
    m_mainWindow->enableUndo(false);
    m_mainWindow->enableRedo(false);
    if(m_dataManager->isProjectLoaded()){
        m_mainWindow->showProjectTitle(m_dataManager->getProjectPath());
        emit sig_hasStatusMessage(tr("Project ") + m_dataManager->getProjectName() + tr(" with ")
                                  + QString::number(m_dataManager->getModelInputPictures()->getPicCount()) + tr(" images loaded"));
    } else {
        m_mainWindow->showProjectTitle();
    }

    //init plugins and notify about current keyframes
    Reader* currentReader = m_dataManager->getModelInputPictures()->getReader();
    AlgorithmManager::instance().initializePlugins(currentReader, m_dataManager->getModelAlgorithm()->getPluginBuffer());
    AlgorithmManager::instance().notifyKeyframesChanged(m_dataManager->getModelInputPictures()->getAllKeyframes(false));
    if (currentReader->getMetaData() != nullptr && currentReader->getMetaData()->availableMetaData().size() > 0) {
        AlgorithmManager::instance().notifyNewMetaData();
    }

    // remove old controllers if existing
    if(m_videoPlayerController)
    {
        disconnect(m_videoPlayerController, &VideoPlayerController::sig_hasStatusMessage, m_mainWindow, &MainWindow::slot_displayStatusMessage);
        delete m_videoPlayerController;
    }
    if(m_algorithmController) {
        disconnect(m_algorithmController, &AlgorithmController::sig_hasStatusMessage, m_mainWindow, &MainWindow::slot_displayStatusMessage);
        delete m_algorithmController;
    }
    if (m_exportController)
    {
        disconnect(m_exportController, &ExportController::sig_hasStatusMessage, m_mainWindow, &MainWindow::slot_displayStatusMessage);
        delete m_exportController;
    }

    // --- create new controllers for video player, export and image sampling
    // --- using the new data (in dataManager) and connect to main window

    // AlgorithmController manages input widget and algorithm used widgets and delegates image sampling
    m_algorithmController = new AlgorithmController(m_dataManager, m_mainWindow->getSamplingWidget());
    connect(m_algorithmController, &AlgorithmController::sig_hasStatusMessage, m_mainWindow, &MainWindow::slot_displayStatusMessage);

    // VideoPlayerControler manages video player and timeline
    m_videoPlayerController = new VideoPlayerController(this, m_mainWindow->getVideoPlayer(), m_mainWindow->getTimeline(), m_dataManager, m_algorithmController);
    connect(m_videoPlayerController, &VideoPlayerController::sig_hasStatusMessage, m_mainWindow, &MainWindow::slot_displayStatusMessage);
    connect(m_mainWindow, &MainWindow::sig_deleteAllKeyframes, m_videoPlayerController, &VideoPlayerController::slot_deleteAllKeyframes);
    connect(m_mainWindow, &MainWindow::sig_deleteKeyframesBoundaries, m_videoPlayerController, &VideoPlayerController::slot_deleteKeyframes);
    connect(m_mainWindow, &MainWindow::sig_resetBoundaries, m_videoPlayerController, &VideoPlayerController::slot_resetBoundaries);

    connect(m_algorithmController, &AlgorithmController::sig_stopPlay, m_videoPlayerController, &VideoPlayerController::slot_stopPlay);

    // ExportController manages algorithm used widget and reconstruct widget and delegates export of images and 3d-reconstruction
    #if defined(Q_OS_LINUX)
        m_exportController = new ExportController(m_mainWindow->getOutputWidget(), m_dataManager, m_colmapWrapper);
    #elif defined(Q_OS_WIN)
        m_exportController = new ExportController(m_mainWindow->getOutputWidget(), m_dataManager);
    #endif
    connect(m_exportController, &ExportController::sig_hasStatusMessage, m_mainWindow, &MainWindow::slot_displayStatusMessage);
    connect(m_exportController, &ExportController::sig_stopPlay, m_videoPlayerController, &VideoPlayerController::slot_stopPlay);
    connect(m_exportController, &ExportController::sig_exportStarted, this, &Controller::slot_exportStarted);
    connect(m_exportController, &ExportController::sig_exportFinished, this, &Controller::slot_exportFinished);
    connect(m_exportController, &ExportController::sig_exportAborted, this, &Controller::slot_exportFinished);
    connect(m_videoPlayerController, &VideoPlayerController::sig_read, m_exportController, &ExportController::slot_nextImageOnPlayer);

    //AutoExecutor is used for the automatic Execution
    m_automaticController->setExporController(m_exportController);
    connect(m_automaticController->autoExec(), &AutomaticExecutor::sig_stopPlay, m_videoPlayerController, &VideoPlayerController::slot_stopPlay);
    connect(m_automaticController->autoExec(), &AutomaticExecutor::sig_hasStatusMessage, m_mainWindow, &MainWindow::slot_displayStatusMessage);


    setInputWidgetInfo(); // initialize input widget with information about new input data
    m_mainWindow->getSamplingWidget()->setAlgorithm(0);
    if(m_mainWindow->getInputEnabled()) m_mainWindow->enableOpenMetaData(true);
    m_mainWindow->enableTools(true);

    connect(m_dataManager->getHistory(), &History::sig_historyChanged, this, &Controller::slot_historyChanged);
}
