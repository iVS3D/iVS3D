#include "controller.h"
#include "roiselect.h"
#include "pluginthread.h"


Controller::Controller(QString inputPath, QString settingsPath, QString outputPath, QString logPath)
        : m_colmapWrapper(new lib3d::ots::ColmapWrapper)
{
    m_videoPlayerController = nullptr;
    m_pluginController = nullptr;
    m_stackController = nullptr;
    QStringList algorithms = AlgorithmManager::instance().getAlgorithmNames();
    QStringList transforms = TransformManager::instance().getTransformList();
    int useCuda = -1;
    ApplicationSettings::CUDA_ERR_CODE cuda_err_code;
    if(ApplicationSettings::instance().getCudaAvailable(&cuda_err_code)){
        useCuda = ApplicationSettings::instance().getUseCuda();
    } else {
        useCuda = -cuda_err_code;
    }

    QList<QLocale> locales = ApplicationSettings::instance().getAvailableLocales();
    QLocale selectedLocale = ApplicationSettings::instance().getLocale();

    QWidget *otsWidget = nullptr;
    const auto otsTheme = ApplicationSettings::instance().getColorTheme() == ColorTheme::DARK ? lib3d::ots::ui::ETheme::DARK : lib3d::ots::ui::ETheme::LIGHT;

    m_colmapWrapper->setChecksDisabled(ApplicationSettings::instance().getDisableChecks());

    otsWidget = new QWidget;
    otsWidget->setLayout(new QVBoxLayout);
    //otsWidget->layout()->addWidget(m_colmapWrapper->getOrCreateUiControlsFactory()->createSettingsPushButton());
    otsWidget->layout()->addWidget(m_colmapWrapper->getOrCreateUiControlsFactory()->createViewWidget(nullptr));
    m_newProductPushButton = m_colmapWrapper->getOrCreateUiControlsFactory()->createNewProductPushButton(otsTheme, nullptr);
    otsWidget->layout()->addWidget(m_newProductPushButton);

    bool interpolateMetaData = ApplicationSettings::instance().getInterpolateMetaData();
    m_mainWindow = new MainWindow(
                nullptr,
                ApplicationSettings::instance().getColorTheme(),
                useCuda,
                ApplicationSettings::instance().getCreateLogs(),
                interpolateMetaData,
                locales,
                selectedLocale,
                algorithms,
                transforms,
                otsWidget
                );

    m_mainWindow->getSamplingWidget()->setPluginList(PluginManager::instance().getPluginNames());

    m_mainWindow->enableUndo(false);
    m_mainWindow->enableRedo(false);
    m_mainWindow->enableTools(false);

    m_colmapWrapperSettingsAction = m_colmapWrapper->getOrCreateUiControlsFactory()->createSettingsAction(otsTheme, nullptr);
    m_mainWindow->addSettingsAction(m_colmapWrapperSettingsAction);

    m_colmapWrapper->getOrCreateUiControlsFactory()->updateIconTheme(otsTheme);

    TransformManager::instance().enableCuda(ApplicationSettings::instance().getUseCuda());
    PluginManager::instance().enableCuda(ApplicationSettings::instance().getUseCuda());

    LogManager::instance().toggleLog(ApplicationSettings::instance().getCreateLogs());

    m_dataManager = new DataManager();
    m_exportController = nullptr;

    m_openExec = nullptr;

    connect(m_mainWindow, &MainWindow::sig_openProject, this, &Controller::slot_openProject);
    connect(m_mainWindow, &MainWindow::sig_openInputFolder, this, &Controller::slot_openInputFolder);
    connect(m_mainWindow, &MainWindow::sig_openInputVideo, this, &Controller::slot_openInputVideo);
    connect(m_mainWindow, &MainWindow::sig_openVideoDragAndDrop, this, &Controller::slot_openDragAndDrop);
    connect(m_mainWindow, &MainWindow::sig_saveProjectAs, this, &Controller::slot_saveProjectAs);
    connect(m_mainWindow, &MainWindow::sig_saveProject, this, &Controller::slot_saveProject);
    connect(m_mainWindow, &MainWindow::sig_changeReconstructPath, this, &Controller::slot_addReconstructPath);
    connect(m_mainWindow, &MainWindow::sig_changeDefaultInputPath, this, &Controller::slot_changeDefaultInputPath);
    connect(m_mainWindow, &MainWindow::sig_toggleTheme, this, &Controller::slot_toggleColorTheme);
    connect(m_mainWindow, &MainWindow::sig_changeUseCuda, this, &Controller::slot_changeUseCuda);
    connect(m_mainWindow, &MainWindow::sig_changeCreateLogFile, this, &Controller::slot_changeCreateLogFile);
    connect(m_mainWindow, &MainWindow::sig_openMetaData, this, &Controller::slot_openMetaData);
    connect(m_mainWindow, &MainWindow::sig_undo, this, &Controller::slot_undo);
    connect(m_mainWindow, &MainWindow::sig_redo, this, &Controller::slot_redo);
    connect(m_mainWindow, &MainWindow::sig_selectLanguage, this, &Controller::slot_selectLanguage);
    connect(m_mainWindow, &MainWindow::sig_restart, this, &Controller::slot_restart);

    OutputWidget* outputWidget = m_mainWindow->getOutputWidget();
    connect(m_mainWindow->getSamplingWidget(), &SamplingWidget::sig_resChanged, this, &Controller::slot_workingResolutionChanged);
    connect(m_mainWindow->getVideoPlayer(), &VideoPlayer::sig_cropEdit, this, &Controller::slot_editCrop);
    connect(m_mainWindow->getVideoPlayer(), &VideoPlayer::sig_useCropChanged, this, &Controller::slot_useCropChanged);
    connect(outputWidget, &OutputWidget::sig_altitudeChanged, this, &Controller::slot_altitudeChanged);

    connect(m_mainWindow, &MainWindow::sig_quit, m_colmapWrapper->getOrCreateUiControlsFactory(), &lib3d::ots::ui::ColmapWrapperControlsFactory::onQuit);
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
    delete m_colmapWrapper;
    if (m_videoPlayerController) {
        delete m_videoPlayerController;
    }
    if (m_pluginController) {
        delete m_pluginController;
    }
    if (m_stackController) {
        delete m_stackController;
    }
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
    loadInputDataFromPath(folderPath);
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

    loadInputDataFromPath(folderPath);
}

void Controller::slot_openDragAndDrop(QString filePath)
{
    // prevent multiple imports at once
    if(m_isImporting) {
        return;
    }

    // prevent imports while exporting
    if(m_exporting){
        QMessageBox msgBox;
        msgBox.setText(tr("Wait for export to finish before importing new files."));
        msgBox.exec();
        return;
    }

    // handle video formats or folders
    if (loadInputDataFromPath(filePath)) {
        return;
    }

    // handle meta data
    bool fileHasSupportedExtension = false;
    for(const QString &extension : MetaDataManager::supportedFileExtensions()) {
        if (filePath.endsWith(extension)) {
            fileHasSupportedExtension = true;
            break;
        }
    }
    if(fileHasSupportedExtension) {
        loadMetaDataFromPath(filePath);
        return;
    }
    emit sig_hasStatusMessage(tr("Unable to import file: ") + filePath);
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

    if (folderPath == nullptr) {
        emit sig_hasStatusMessage(tr("Input canceled"));
        return;
    }
    loadInputDataFromPath(folderPath);
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

void Controller::slot_toggleColorTheme()
{
    if(ApplicationSettings::instance().getColorTheme() == ColorTheme::LIGHT){
        ApplicationSettings::instance().setColorTheme(ColorTheme::DARK);
    } else {
        ApplicationSettings::instance().setColorTheme(ColorTheme::LIGHT);
    }
    m_mainWindow->setColorTheme(ApplicationSettings::instance().getColorTheme());

    const auto otsTheme = ApplicationSettings::instance().getColorTheme() == ColorTheme::DARK ? lib3d::ots::ui::ETheme::DARK : lib3d::ots::ui::ETheme::LIGHT;
    m_colmapWrapper->getOrCreateUiControlsFactory()->updateIconTheme(otsTheme);
    // update icons of settings action and new product button
    m_colmapWrapper->getOrCreateUiControlsFactory()->createSettingsAction(otsTheme, m_mainWindow, m_colmapWrapperSettingsAction);
    m_colmapWrapper->getOrCreateUiControlsFactory()->createNewProductPushButton(otsTheme, m_mainWindow, m_newProductPushButton);
    // notify the plugins next!
}

void Controller::slot_changeColorTheme(ColorTheme theme)
{
    ApplicationSettings::instance().setColorTheme(theme);
    emit sig_hasStatusMessage(tr("GUI color theme changed to ") + QString((theme == ColorTheme::DARK ? tr("dark") : tr("light"))) + tr(" style"));
}

void Controller::slot_changeUseCuda(bool useCuda)
{
    ApplicationSettings::instance().setUseCuda(useCuda);
    if(!m_exporting){
        TransformManager::instance().enableCuda(ApplicationSettings::instance().getUseCuda());
    }
    PluginManager::instance().enableCuda(ApplicationSettings::instance().getUseCuda());
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
    QString filePath
        = QFileDialog::getOpenFileName(m_mainWindow,
                                       tr("Choose Meta Data"),
                                       ApplicationSettings::instance().getStandardInputPath(),
                                       "*.srt *.SRT *.gpx *.GPX *.jpeg *.jpg *.txt",
                                       &selectedFilter,
                                       QFileDialog::DontUseNativeDialog);
    if (filePath == nullptr) {
        emit sig_hasStatusMessage(tr("Input canceled"));
        return;
    }
    loadMetaDataFromPath(filePath);
}

void Controller::slot_openFinished(int result)
{
//    disconnect(m_openExec, &OpenExecutor::sig_finished, this, &Controller::slot_openFinished);
//    disconnect(m_inputProgressDialog, &ProgressDialog::rejected, m_openExec, &OpenExecutor::slot_abort);

    m_inputProgressDialog->close();
//    if (m_openExec) {
//        delete m_openExec;
//        m_openExec = nullptr;
//    }

//    if (m_inputProgressDialog) {
//        m_inputProgressDialog->close();
//        delete m_inputProgressDialog;
//        m_inputProgressDialog = nullptr;
//    }

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
    m_dataManager->getHistory()->slot_save();
}

void Controller::slot_undo()
{
    m_dataManager->getHistory()->undo();
    m_stackController->select();
}

void Controller::slot_redo()
{
    m_dataManager->getHistory()->redo();
    m_stackController->select();
}

void Controller::slot_historyChanged()
{
    m_mainWindow->enableRedo(m_dataManager->getHistory()->hasFuture());
    m_mainWindow->enableUndo(m_dataManager->getHistory()->hasPast());
}

void Controller::slot_selectLanguage(QLocale language)
{
    ApplicationSettings::instance().setLocale(language);
}

void Controller::slot_restart()
{
    // Quit the current instance
    QCoreApplication::quit();

    // Start a new instance of the application
    QProcess::startDetached(qApp->arguments()[0], qApp->arguments());
}

void Controller::slot_workingResolutionChanged(QString resolution)
{
    if(m_dataManager->getModelInputPictures()) {
        std::shared_ptr<ReaderParams> params = m_dataManager->getModelInputPictures()->getReaderParams();
        Resolution res;
        if (res.fromString(resolution)){
            bool valid = params->setWorkingResolution(res);
            m_mainWindow->getSamplingWidget()->setResolutionValid(valid);
            m_videoPlayerController->slot_mipChanged();
        }
    }
}

void Controller::slot_editCrop()
{
    if(m_dataManager->getModelInputPictures()) {
        std::shared_ptr<ReaderParams> params = m_dataManager->getModelInputPictures()->getReaderParams();
        uint current_idx = m_videoPlayerController->getImageIndexOnScreen();
        const cv::Mat* img = m_dataManager->getModelInputPictures()->getPic(current_idx);
        if (img->empty()) {
            QMessageBox *em = new QMessageBox();
            em->setText(tr("The selected frame is broken and can´t be cropped. Please select another frame to select a new region of intrest."));
            em->show();
            return;
        }
        Resolution imgResolution(*img);
        QRect roiRect = params->getRoi().cropAsQRect(imgResolution);
        CropExport dialog = CropExport(m_mainWindow, img, roiRect);

        if (dialog.exec() == QDialog::Accepted) {
            roiRect = dialog.getROI();
            //Don't update roi, if no roi has been drawn
            if (roiRect.size() != QSize(1,1)) {
                ROI newROI(roiRect, imgResolution);
                bool valid = params->setRoi(newROI);
                params->setUseRoi(valid);
                m_mainWindow->getVideoPlayer()->setCropStatus(valid);
                m_videoPlayerController->slot_mipChanged();
            }
            if (m_exportController) {
                m_exportController->slot_roiChanged(params->getUseRoi() ? std::optional<ROI>(params->getRoi()) : std::nullopt);
            }
        }
    }
}

void Controller::slot_useCropChanged(int checkstate)
{
    if(m_dataManager->getModelInputPictures()) {
        std::shared_ptr<ReaderParams> params = m_dataManager->getModelInputPictures()->getReaderParams();
        params->setUseRoi(checkstate != Qt::Unchecked);
        m_videoPlayerController->slot_mipChanged();
        if (m_exportController) {
            m_exportController->slot_roiChanged(params->getUseRoi() ? std::optional<ROI>(params->getRoi()) : std::nullopt);
        }
    }
}

void Controller::slot_altitudeChanged(double altitude)
{
    m_dataManager->getModelInputPictures()->setAltitude(altitude);
}

void Controller::slot_restorePluginSettings(int id) {

    const MaskRecord *record = m_stack->getRecordById(id);

    m_mainWindow->getSamplingWidget()->setResolution(record->workingResolution.toString());
    slot_workingResolutionChanged(record->workingResolution.toString());
    std::shared_ptr<ReaderParams> params = m_dataManager->getModelInputPictures()->getReaderParams();
    bool valid = params->setRoi(record->roi);
    params->setUseRoi(!record->roi.isDefault());
    m_mainWindow->getVideoPlayer()->setCropStatus(params->getUseRoi());
    m_videoPlayerController->slot_mipChanged();
    if (m_exportController) {
        m_exportController->slot_roiChanged(params->getUseRoi() ? std::optional<ROI>(params->getRoi()) : std::nullopt);
    }
    auto pluginHandle = PluginManager::instance().getPluginByName(record->pluginName);
    assert(pluginHandle.has_value());
    auto applySettingsResult = pluginHandle.value().base->applySettings(record->pluginSettings);
    if (!applySettingsResult) {
        QMessageBox msgBox;
        msgBox.setText(tr("Error restoring plugin settings for plugin '") + record->pluginName + tr("': ") + applySettingsResult.error().message);
        msgBox.exec();
    }
    if (m_pluginController) {
        m_mainWindow->getSamplingWidget()->setSelectedPlugin(record->pluginName);
        m_pluginController->slot_selectPlugin(record->pluginName);
    }
}

void Controller::createOpenMessage(int numPics)
{
    m_mainWindow->getVideoPlayer()->updateRoi();
    auto duration_ms = m_timer.elapsed();
    if (numPics <= 0) {
        emit sig_hasStatusMessage(tr("No images imported after ") + QString::number(duration_ms) + tr("ms"));
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

    auto readerParams = m_dataManager->getModelInputPictures()->getReaderParams();
    QString resolution = readerParams->getOriginalResolution().toString();

    QList<VideoPlayer::OverlayEntry> entries = {
        {m_dataManager->getModelInputPictures()->getPath(), false, Qt::ElideMiddle},  // Filepath
        {tr("General"), true},          // section header
        {resolution + tr(" pixels")},   // resolution
        {QString::number(m_dataManager->getModelInputPictures()->getPicCount()) + tr(" images")},   // image count
    };
    Reader* currentReader = m_dataManager->getModelInputPictures()->getReader();
    if(currentReader->getFPS() != -1) {
        entries.append({
            {tr("Video"), true},
            {QString::number(currentReader->getVideoDuration()) + tr(" seconds")},
            {QString::number(currentReader->getFPS()) + tr(" fps")}
        });
    }
    QStringList loadedMetaData = MetaDataManager::instance().availableMetaData();
    if (loadedMetaData.size() != 0) {
        entries.push_back({tr("Metadata"), true});
        for (int i = 0; i < loadedMetaData.size(); i++) {
            entries.push_back({" #" + QString::number(i + 1) + " " + loadedMetaData.at(i)});
        }
    }
    m_mainWindow->getVideoPlayer()->updateOverlayText(entries);

    // set standard (input) resolution
    QStringList resList = QString(RESOLUTION_LIST).split("|");
    resList.push_front(resolution + " (input res)");
    int resolutionIdx = 0;
    if(!(readerParams->getOriginalResolution() == readerParams->getWorkingResolution())){
        QString wRes = readerParams->getWorkingResolution().toString();
        auto it = std::find_if(resList.begin(), resList.end(),
                               [&wRes](const QString& res) { return res.startsWith(wRes); });

        resolutionIdx = (it != resList.end()) ? std::distance(resList.begin(), it) : -1;

        if(resolutionIdx < 0) {
            resList.push_back(wRes);
            resolutionIdx = resList.length()-1;
        }
    }
    m_mainWindow->getSamplingWidget()->setResolutionList(resList, resolutionIdx);
    m_mainWindow->getOutputWidget()->setResolutionList(resList, resolutionIdx);

    // set altitude (if available)
    setAltitude();
}

void Controller::onFailedOpen()
{
    // --- called after image data has been loaded succesfully
    // --- setup the GUI with the new data
    m_mainWindow->enableSaveProject(false);
    m_mainWindow->enableUndo(false);
    m_mainWindow->enableRedo(false);
    m_mainWindow->getOutputWidget()->setAltitudeVisible(false);
    m_mainWindow->getVideoPlayer()->updateOverlayText({});
    m_mainWindow->enableOpenMetaData(false);
    m_mainWindow->enableTools(false);
    // remove old controllers if existing
    if(m_videoPlayerController)
    {
        disconnect(m_videoPlayerController, &VideoPlayerController::sig_hasStatusMessage, m_mainWindow, &MainWindow::slot_displayStatusMessage);
        delete m_videoPlayerController;
        m_videoPlayerController = nullptr;
    }
    if(m_pluginController) {
        delete m_pluginController;
        m_pluginController = nullptr;
    }
    if (m_exportController)
    {
        disconnect(m_exportController, &ExportController::sig_hasStatusMessage, m_mainWindow, &MainWindow::slot_displayStatusMessage);
        delete m_exportController;
        m_exportController = nullptr;
    }
    m_automaticController->disableAutoWidget();

    QMessageBox msgBox;
    msgBox.setText(tr("Failed to load input data. Possible causes are:\n"
                      "- The selected folder does not contain any supported image or video files.\n"
                      "- The selected project file is corrupted or incompatible.\n"
                      "- The path or filenames contain invalid characters.\n\n"
                      "Please check the input and try again."));
    msgBox.exec();
}

uint Controller::loadMetaDataFromPath(QString path)
{
    int n = m_dataManager->getModelInputPictures()->loadMetaData(QStringList(path));
    if (n > 0) {
        AlgorithmManager::instance().notifyNewMetaData();
        //Update the info widget
        setInputWidgetInfo();
        QString msg = tr("Loaded ") + QString::number(n) + tr(" meta data feature") + QString(n > 1 ? tr("s") : "");
        emit sig_hasStatusMessage(msg);
        return n;
    } else {
        QString msg = tr("No meta data features were detected");
        emit sig_hasStatusMessage(msg);
        return 0;
    }
}

bool Controller::loadInputDataFromPath(QString path)
{
    if (m_videoPlayerController) {
        m_videoPlayerController->slot_stopPlay();
    }
    
    bool validDatasetPath =
            path.endsWith(".mp4", Qt::CaseInsensitive) ||
            path.endsWith(".mov", Qt::CaseInsensitive) ||
            path.endsWith(".avi", Qt::CaseInsensitive) ||
            QDir(path).exists() ||
            path.endsWith(".json", Qt::CaseInsensitive);
    if (!validDatasetPath) {
        return false;
    }

    if (m_openExec){
        delete m_openExec;
    }
    m_isImporting = true;
    m_openExec = new OpenExecutor(path, m_dataManager);
    if (m_inputProgressDialog) {
        delete m_inputProgressDialog;
    }
    m_inputProgressDialog = new ProgressDialog(m_mainWindow, false);
    m_inputProgressDialog->setModal(true);
    m_inputProgressDialog->slot_displayProgress(-1, tr("Importing dataset and metadata."));

    connect(m_inputProgressDialog, &ProgressDialog::rejected, m_openExec, &OpenExecutor::slot_abort);
    connect(m_openExec, &OpenExecutor::sig_finished, this, &Controller::slot_openFinished);
    m_timer = QElapsedTimer();
    m_timer.start();
    m_openExec->open();
    m_inputProgressDialog->show();
    return true;
}

void Controller::setAltitude()
{
    QList<MetaDataReader*> readers = MetaDataManager::instance().loadAllMetaData();
    for (MetaDataReader* reader : readers) {
        if (reader->getName().startsWith("GPS")) {
            GPSReader* gpsReader = dynamic_cast<GPSReader*>(reader);
            if (!gpsReader->hasAltitudeData()) {
                continue;
            }
            m_mainWindow->getOutputWidget()->setAltitudeVisible(true);
            QVariant gpsData = reader->getImageMetaData(0);
            QHash<QString, QVariant> gpsHash = gpsData.toHash();
            double altitude_abs = gpsHash.find("GPSAltitude").value().toDouble();
            double altitude = (gpsHash.find("GPSAltitudeRef").value().toString() == "0") ? altitude_abs : altitude_abs * -1;
            m_mainWindow->getOutputWidget()->setAltitude(altitude);
            m_exportController->setOriginalAltitude(altitude);
            return;
        }
    }
    m_mainWindow->getOutputWidget()->setAltitudeVisible(false);
    
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
    if(m_pluginController) {
        delete m_pluginController;
    }
    if (m_exportController) {
        disconnect(m_exportController, &ExportController::sig_hasStatusMessage, m_mainWindow, &MainWindow::slot_displayStatusMessage);
        delete m_exportController;
    }
    if (m_stackController) {
        delete m_stackController;
    }


    // --- create new controllers for video player, export and image sampling
    // --- using the new data (in dataManager) and connect to main window

    m_stack = std::make_shared<MaskStack>();
    
    auto pluginThread = std::make_shared<PluginThread>(PluginManager::instance().getPlugins(), this);

    // VideoPlayerControler manages video player and timeline
    m_videoPlayerController = new VideoPlayerController(this, m_mainWindow->getVideoPlayer(), m_mainWindow->getTimeline(), m_dataManager, pluginThread);
    connect(m_videoPlayerController, &VideoPlayerController::sig_hasStatusMessage, m_mainWindow, &MainWindow::slot_displayStatusMessage);
    connect(m_mainWindow, &MainWindow::sig_deleteAllKeyframes, m_videoPlayerController, &VideoPlayerController::slot_deleteAllKeyframes);
    connect(m_mainWindow, &MainWindow::sig_deleteKeyframesBoundaries, m_videoPlayerController, &VideoPlayerController::slot_deleteKeyframes);
    connect(m_mainWindow, &MainWindow::sig_resetBoundaries, m_videoPlayerController, &VideoPlayerController::slot_resetBoundaries);

    // ExportController manages algorithm used widget and reconstruct widget and delegates export of images and 3d-reconstruction
    m_exportController = new ExportController(m_mainWindow->getOutputWidget(), m_dataManager, m_colmapWrapper, m_stack);
    connect(m_exportController, &ExportController::sig_hasStatusMessage, m_mainWindow, &MainWindow::slot_displayStatusMessage);
    connect(m_exportController, &ExportController::sig_stopPlay, m_videoPlayerController, &VideoPlayerController::slot_stopPlay);
    connect(m_exportController, &ExportController::sig_exportStarted, this, &Controller::slot_exportStarted);
    connect(m_exportController, &ExportController::sig_exportFinished, this, &Controller::slot_exportFinished);
    connect(m_exportController, &ExportController::sig_exportAborted, this, &Controller::slot_exportFinished);
    //connect(m_videoPlayerController, &VideoPlayerController::sig_read, m_exportController, &ExportController::slot_nextImageOnPlayer);

    //AutoExecutor is used for the automatic Execution
    m_automaticController->setExporController(m_exportController);
    connect(m_automaticController->autoExec(), &AutomaticExecutor::sig_stopPlay, m_videoPlayerController, &VideoPlayerController::slot_stopPlay);
    connect(m_automaticController->autoExec(), &AutomaticExecutor::sig_hasStatusMessage, m_mainWindow, &MainWindow::slot_displayStatusMessage);


    setInputWidgetInfo(); // initialize input widget with information about new input data

    if(m_mainWindow->getInputEnabled()) m_mainWindow->enableOpenMetaData(true);
    m_mainWindow->enableTools(true);

    connect(m_dataManager->getHistory(), &History::sig_historyChanged, this, &Controller::slot_historyChanged);

    //Create the StackController
    m_stackController = new StackController(m_mainWindow->getOpStack(), m_dataManager->getHistory(), m_mainWindow->getSamplingWidget(), m_exportController);
    connect(m_videoPlayerController, &VideoPlayerController::sig_toggleKeyframe, m_stackController, &StackController::slot_toggleKeyframe);
    connect(m_videoPlayerController, &VideoPlayerController::sig_deleteAllKeyframes, m_stackController, &StackController::slot_deleteAllKeyframes);
    connect(m_videoPlayerController, &VideoPlayerController::sig_deleteKeyframes, m_stackController, &StackController::slot_deleteKeyframes);
    connect(m_exportController, &ExportController::sig_exportFinished, m_stackController, &StackController::slot_exportFinished);

    // AlgorithmController manages input widget and algorithm used widgets and delegates image sampling
    m_pluginController = new PluginController(m_dataManager, m_mainWindow->getSamplingWidget(), m_videoPlayerController, m_stackController, m_stack);
    connect(m_mainWindow->getOutputWidget()->getMaskStackView().get(), &MaskStackView::sig_recordSelected, this, &Controller::slot_restorePluginSettings);

    // update the working resolution, roi, etc
    std::shared_ptr<ReaderParams> params = m_dataManager->getModelInputPictures()->getReaderParams();
    m_mainWindow->getVideoPlayer()->setCropStatus(params->getUseRoi());
    QRect roi = params->getRoi().cropAsQRect(params->getOriginalResolution());
    m_mainWindow->getVideoPlayer()->updateRoi(params->getUseRoi()? roi : QRect());

    // default working resolution: 720p if input is larger
    #define DEFAULT_WORKING_RESOLUTION_HEIGHT 720
    if (params->getOriginalResolution().getHeight() > DEFAULT_WORKING_RESOLUTION_HEIGHT) {
        float aspect_ratio = static_cast<float>(params->getOriginalResolution().getWidth()) / params->getOriginalResolution().getHeight();
        m_mainWindow->getSamplingWidget()->setResolution(Resolution(static_cast<int>(DEFAULT_WORKING_RESOLUTION_HEIGHT * aspect_ratio), DEFAULT_WORKING_RESOLUTION_HEIGHT).toString());
    }
}
