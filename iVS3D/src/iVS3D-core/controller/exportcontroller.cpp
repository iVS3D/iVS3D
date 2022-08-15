#include "exportcontroller.h"

ExportController::ExportController(OutputWidget *outputWidget, DataManager *dataManager, lib3d::ots::ColmapWrapper *colmap) : m_roi(0,0,0,0)
{

    m_exportExec = nullptr;
    m_reconstructDialog = nullptr;
    m_cropDialog = nullptr;
    m_currentExports.clear();

    m_outputWidget = outputWidget;
    m_dataManager = dataManager;
    m_colmap = colmap;

    connect(m_outputWidget, &OutputWidget::sig_reconstruct, this, &ExportController::slot_reconstruct);
    connect(m_outputWidget, &OutputWidget::sig_export, this, &ExportController::slot_export);
    connect(m_outputWidget, &OutputWidget::sig_cropExport, this, &ExportController::slot_cropExport);
    connect(m_outputWidget, &OutputWidget::sig_resChanged, this, &ExportController::slot_resolutionChange);
    connect(m_outputWidget, &OutputWidget::sig_pathChanged, this, &ExportController::slot_outputPathChanged);

    connect(m_dataManager->getModelInputPictures(), &ModelInputPictures::sig_mipChanged, this, &ExportController::slot_onKeyframesChanged);

    m_outputWidget->setEnabled(true);

    // set standard (input) resolution
    m_resolution = m_dataManager->getModelInputPictures()->getInputResolution();
    QStringList resList = QString(RESOLUTION_LIST).split("|");
    resList.push_front(QString::number(m_resolution.x()) + " x " + QString::number(m_resolution.y()) + " (input res)");
    m_outputWidget->setResolutionList(resList, 0);

    // set standard (input) path
    m_path = m_dataManager->getModelInputPictures()->getPath();
    if(!m_dataManager->getModelInputPictures()->getReader()->isDir()){
        QStringList pathList = m_path.split("/");
        pathList.removeLast();
        m_path = pathList.join("/");
    }
    m_path += "/export";
    m_outputWidget->setOutputPath(m_path);
    m_outputWidget->setCropStatus(false);

    m_outputWidget->enableReconstruct(false);

}

ExportController::~ExportController()
{
    disconnect(m_outputWidget, &OutputWidget::sig_reconstruct, this, &ExportController::slot_reconstruct);
    disconnect(m_outputWidget, &OutputWidget::sig_export, this, &ExportController::slot_export);
    disconnect(m_outputWidget, &OutputWidget::sig_resChanged, this, &ExportController::slot_resolutionChange);
    disconnect(m_outputWidget, &OutputWidget::sig_pathChanged, this, &ExportController::slot_outputPathChanged);

    m_outputWidget->setEnabled(false);

    m_outputWidget->setResolutionList(QStringList(""), 0);
    m_outputWidget->setOutputPath("");
    m_currentExports.clear();
}

QMap<QString, QVariant> ExportController::getOutputSettings()
{
    QMap<QString, QVariant> settings;

    QString resolution = QString::number(m_resolution.x()) + stringContainer::ROISpliter + QString::number(m_resolution.y());
    settings.insert(stringContainer::Resolution, resolution);

    QString roi = QString::number(m_roi.x()) + stringContainer::ROISpliter + QString::number(m_roi.y()) + "x" + QString::number(m_roi.width()) +"x" + QString::number(m_roi.height());
    settings.insert(stringContainer::ROI, roi);

    settings.insert(stringContainer::UseROI, m_outputWidget->getCropStatus());

    std::vector<bool> useItransform = m_outputWidget->getSelectedITransformMasks();
    QList<QVariant> iTransformSettings;
    QList<QVariant> useItransformVariant;
    int idx = 0;
    for (bool use : useItransform) {
        useItransformVariant.append(use);
        iTransformSettings.append(TransformManager::instance().getSettings(idx));
    }
    settings.insert(stringContainer::UseITransform, useItransformVariant);
    settings.insert(stringContainer::ITransformSettings, iTransformSettings);
    return settings;

}

void ExportController::setOutputSettings(QMap<QString, QVariant> settings)
{
    if(settings.contains(stringContainer::OutputPath)){
        m_path = settings.find(stringContainer::OutputPath).value().toString();
        m_outputWidget->setOutputPath(m_path);
    }

    QString resolution = settings.find(stringContainer::Resolution).value().toString();
    m_resolution = parseResolution(resolution);
    m_outputWidget->setResolution(resolution);

    QString roiString = settings.find(stringContainer::ROI).value().toString();
    QStringList roiSplit = roiString.split(stringContainer::ROISpliter);
    m_roi = *new QRect(roiSplit[0].toInt(), roiSplit[1].toInt(), roiSplit[2].toInt(), roiSplit[3].toInt());

    bool useCrop = settings.find(stringContainer::UseROI).value().toBool();
    m_outputWidget->setCropStatus(useCrop);

    QList<QVariant> useItransformVariant = settings.find(stringContainer::UseITransform).value().toList();
    std::vector<bool> selection;
    for (QVariant useItransform : useItransformVariant) {
        selection.push_back(useItransform.toBool());
    }
    m_outputWidget->setSelectedITransformMasks(selection);

    QList<QVariant> iTransformSettingsList = settings.find(stringContainer::ITransformSettings).value().toList();
    int idx = 0;
    for (QVariant var : iTransformSettingsList) {
        QMap<QString, QVariant> iTransformSettings = var.toMap();
        TransformManager::instance().setSettings(iTransformSettings, idx);
        idx++;
    }
}

void ExportController::slot_reconstruct()
{
    emit sig_stopPlay();
    ApplicationSettings as = ApplicationSettings::instance();

    //get reconstruct tools
    QMap<QString, QString> reconstructMap = as.getReconstructPath();
    //build Stringlist with reconstruct tool names
    int reconstructMapSize = reconstructMap.size();
    QStringList reconstructtoolList;
    for (int i = 0; i < reconstructMapSize; i++) {
        QString tempKey = reconstructMap.begin().key();
        reconstructtoolList.push_back(tempKey);
        reconstructMap.remove(tempKey);
    }

    QStringList exportList;
    for(QString exportName : m_currentExports.keys()){
        exportList.push_back(exportName);
    }

    m_reconstructDialog = new ReconstructDialog(m_outputWidget, exportList, reconstructtoolList);
    if(m_reconstructDialog->exec()){
        if (startReconstruct()) {
            m_reconstructDialog->close();
        }
    }
}

void ExportController::slot_export()
{
    LogFile *lfExport = LogManager::instance().createLogFile("Export", false);
    lfExport->setSettings(getOutputSettings());

    if (m_path.endsWith("/")) {
        m_path.chop(1);
    }

    emit sig_stopPlay();

    //creating export Directory if necessary
    QDir exportDir;
    if (!exportDir.mkpath(m_path)) {
        qDebug() << "Couldn't create Export Directory " << m_path;
        emit sig_hasStatusMessage(QString("Couldn't create Export Directory: %1").arg(m_path));
        return;
    }

    QString outputName;
    //adding images folder to export if necessary
    //split path into folders, look at last foldername, replace "images" with nothing
    //if that string is NOT empty, it means the last foldername WASN'T "images", so we are creating it
    if (!m_path.split("/").last().replace("images", "").isEmpty()) {
        outputName = m_path.split("/").last();
        m_path.append("/images");
        if (!QDir(m_path).exists()) {
            QDir().mkdir(m_path);
        }
    }
    else {
        QStringList temp = m_path.split("/");
        Q_ASSERT(temp.length() >= 2);
        outputName = temp[temp.length() - 2];
    }

    //create string without /images
    QString pathWOimages = "";
    QStringList outputPathBits = m_path.split("/");
    outputPathBits.removeLast();
    for (int i = 0; i < outputPathBits.length(); i++) {
        pathWOimages.append(outputPathBits[i]);
        if (i < outputPathBits.length() - 1) {
            pathWOimages.append("/");
        }
    }

    //check if export folder is empty
    QStringList exportEntries = QDir(pathWOimages).entryList();
    bool wipeDir = false;
    if (exportEntries.length() > 3) {
        //we have existing data
        wipeDir = true;
    }
    else if (exportEntries.contains("images", Qt::CaseSensitive)) {
        QString imagesDir = pathWOimages;
        imagesDir.append("/images");
        if (QDir(imagesDir).entryList().length() > 2) {
            //images folder is not empty
            wipeDir = true;
        }
    }

    //prepare iTransformNames
    QStringList iTransformNames = TransformManager::instance().getTransformList();

    if(wipeDir) {
        EmptyFolderDialog *emptyFolderD = new EmptyFolderDialog(m_outputWidget, pathWOimages);
        int result = emptyFolderD->exec();
        switch (result) {
            case 0:
                //abort
                qDebug() << "User aborted export from empty folder dialog";
                return;
            case 1:
            {
                //user wants to delete and continue
                //Delete export images
                deleteExportFolder(pathWOimages);
                if (!QDir(m_path).exists()) {
                    QDir().mkpath(m_path);
                }
                break;
            }
            case 2:
                //ignore
                break;
            default:
                //this shouldn't occur
                return;
        }
    }

    //add Itransform folders
    std::vector<bool> iTransformUsed = m_outputWidget->getSelectedITransformMasks();
    if (iTransformNames.length() != (int)iTransformUsed.size()) {
        //this shouldn't happen!
        qDebug() << "count of iTransformNames doesn't match iTransformUsed list";
        return;
    }
    std::vector<ITransform*> iTransformCopies;
    for (uint i = 0; i < unsigned(iTransformNames.length()); ++i) {
        //check if itransform has been selected to export
        if (!iTransformUsed[i]) {
            continue;
        }
        iTransformCopies.push_back(TransformManager::instance().getTransform(i)->copy());
        QString iTransformDir = pathWOimages;
        iTransformDir.append("/").append(iTransformCopies[i]->getName());
        if (!QDir(iTransformDir).exists()) {
            QDir().mkdir(iTransformDir);
        }
    }


    if (m_exportExec != nullptr) {
        delete m_exportExec;
    }
    m_exportExec = new ExportExecutor(this, m_dataManager);
    // connect GUI to export executor to display progress and result or to abort export
    connect(m_exportExec, &ExportExecutor::sig_exportAborted, this, &ExportController::slot_exportAborted);
    connect(m_exportExec, &ExportExecutor::sig_exportFinished, this, &ExportController::slot_exportFinished);
    connect(m_exportExec, &ExportExecutor::sig_progress, m_outputWidget, &OutputWidget::slot_displayProgress);
    connect(m_outputWidget, &OutputWidget::sig_abort, m_exportExec, &ExportExecutor::slot_abort);
    m_outputWidget->showProgress(); // swap OutputWidget to display progress

    m_dataManager->createProject(outputName, pathWOimages + "/" + outputName + "-project.json");

    //If Use Crop is checked the current roi is used
    if (m_outputWidget->getCropStatus()) {
        m_exportExec->startExport(m_resolution, m_path, outputName, m_roi, iTransformCopies, lfExport);
    }
    //Otherwise roi won't be used (-> 0x0 Rect) this wont override m_roi
    else {
       m_exportExec->startExport(m_resolution, m_path, outputName, QRect(0,0,0,0), iTransformCopies, lfExport);
    }


    emit sig_exportStarted();
    m_timer = QElapsedTimer();
    m_timer.start();
    if (m_path.endsWith("/images")) {
        m_path.chop(7); //remove /images at the end
    }

}

void ExportController::slot_cropExport()
{
    const cv::Mat* img = m_dataManager->getModelInputPictures()->getPic(1);
    m_cropDialog = new CropExport(m_outputWidget, img, m_roi);
    connect(m_cropDialog, &CropExport::finished, this, &ExportController::slot_closeCropExport);
    m_cropDialog->open();
    m_cropDialog->triggerResize();

}

void ExportController::slot_closeCropExport(int result)
{
    if (result == 1) {
        m_outputWidget->setCropStatus(true);
        QRect currentRoi = m_cropDialog->getROI();
        //Don't update roi, if no roi has been drawn
        if (currentRoi.size() != QSize(1,1)) {
            m_roi = currentRoi;
        }

    }

    disconnect(m_cropDialog, &CropExport::finished, this, &ExportController::slot_closeCropExport);
    m_cropDialog->deleteLater();
}


void ExportController::slot_outputPathChanged(QString path)
{
    path.replace("\\", "/");
    m_path = path;
}

void ExportController::slot_resolutionChange(const QString &res)
{
    m_resolution = parseResolution(res);
    m_outputWidget->setResolutionValid(validateResolution(m_resolution));
}

void ExportController::slot_exportAborted()
{
    auto duration_ms = m_timer.elapsed();
    emit sig_hasStatusMessage("Export aborted after " + QString::number(duration_ms) + "ms");
    // disconnect GUI to export executor
    disconnect(m_exportExec, &ExportExecutor::sig_exportAborted, this, &ExportController::slot_exportAborted);
    disconnect(m_exportExec, &ExportExecutor::sig_exportFinished, this, &ExportController::slot_exportFinished);
    disconnect(m_exportExec, &ExportExecutor::sig_progress, m_outputWidget, &OutputWidget::slot_displayProgress);
    disconnect(m_outputWidget, &OutputWidget::sig_abort, m_exportExec, &ExportExecutor::slot_abort);
    m_outputWidget->showExportOptions(); // swap OutputWidget to display result
    emit sig_exportAborted();
}

void ExportController::slot_exportFinished(int result)
{
    // disconnect GUI to export executor
    disconnect(m_exportExec, &ExportExecutor::sig_exportAborted, this, &ExportController::slot_exportAborted);
    disconnect(m_exportExec, &ExportExecutor::sig_exportFinished, this, &ExportController::slot_exportFinished);
    disconnect(m_exportExec, &ExportExecutor::sig_progress, m_outputWidget, &OutputWidget::slot_displayProgress);
    disconnect(m_outputWidget, &OutputWidget::sig_abort, m_exportExec, &ExportExecutor::slot_abort);
    m_outputWidget->showExportOptions(); // swap OutputWidget to display result

    if (result == -1) {
        emit sig_hasStatusMessage("Export failed. Maybe the path is invalid");
        emit sig_exportFinished();
        return;
    }

    auto duration_ms = m_timer.elapsed();
    emit sig_hasStatusMessage("Export finished after " + QString::number(duration_ms) + "ms");

    // now we have an export, so enable reconstruct
    m_outputWidget->enableReconstruct(true);

    // save log file
    if (ApplicationSettings::instance().getCreateLogs()) {
        LogManager::instance().print();
    }
    //Save current exportPath and name
    m_currentExports.insert(m_path.split("/").last(), m_path);
    m_colmap->setLocalPresetSequence(m_path.split("/").last(), m_path + "/images");

    emit sig_exportFinished();
}

void ExportController::slot_showExportSettings(QMap<QString, QVariant> exportSettings)
{
    setOutputSettings(exportSettings);
}

void ExportController::slot_onKeyframesChanged()
{
    m_outputWidget->enableExport(m_dataManager->getModelInputPictures()->getKeyframeCount(true) > 0);
}

QPoint ExportController::parseResolution(QString resolutionString)
{
    //remove spaces
    resolutionString = resolutionString.simplified();
    //split at x
    QStringList xSplitList = resolutionString.split(stringContainer::ROISpliter);

    int width = -1;
    int height = -1;
    if(xSplitList.size() <= 1) {
        //we dont have a x to split between
    }
    else {
        bool oneInteger = false;

        //DETERMINE Width
        //create space split list
        QStringList spaceSplitList = xSplitList[0].split(" ");
        //iterate over x split Strings which are split by a space
        for (int n = 0; n < spaceSplitList.size(); n++) {
            //remove all but numbers in string
            spaceSplitList[n].replace(QRegExp("[^\\d]"), "");
            //parse the leftover String
            int parseTemp = spaceSplitList[n].toInt();
            if (parseTemp > 0) {
                if (!oneInteger) {
                    oneInteger = true;
                    width = parseTemp;
                }
                else {
                    //more than one number in one x-section
                    return QPoint(-1, -1);
                }
            }
        }
        //catch if no number is inside x String
        if (!oneInteger){
            return QPoint(-1, -1);
        }
        oneInteger = false;
        //DETERMINE Height
        //create space split list
        spaceSplitList = xSplitList[1].split(" ");
        //iterate over x split Strings which are split by a space
        for (int n = 0; n < spaceSplitList.size(); n++) {
            //remove all but numbers in string
            spaceSplitList[n].replace(QRegExp("[^\\d]"), "");
            //parse the leftover String
            int parseTemp = spaceSplitList[n].toInt();
            if (parseTemp > 0) {
                if (!oneInteger) {
                    oneInteger = true;
                    height = parseTemp;
                }
                else {
                    //more than one number in one y-section
                    return QPoint(-1, -1);
                }
            }
        }
        //catch if no number is inside x String
        if (!oneInteger){
            return QPoint(-1, -1);
        }
    }

    return QPoint(width, height);
}

bool ExportController::validateResolution(QPoint resolution)
{
    ModelInputPictures *mip = m_dataManager->getModelInputPictures();
    Q_ASSERT(mip != nullptr);
    QPoint inputRes = mip->getInputResolution();
    QPoint difRes = inputRes - resolution;
    if (resolution == QPoint(-1, -1) || difRes.x() < 0 || difRes.y() < 0){
        return false;
    }
    return true;
}
#if defined(Q_OS_WIN)
bool ExportController::startReconstruct()
{
    //get data from GUI
    ApplicationSettings as = ApplicationSettings::instance();
    QMap<QString, QString> reconstructtools = as.getReconstructPath();
    QString executablePath = reconstructtools.take(m_reconstructDialog->getReconstructtool());
    QString exportName = m_reconstructDialog->getExportName();
    QString startargs = m_reconstructDialog->getStartArguments();
    bool createProject = m_reconstructDialog->getCreateProject();



    //make reconstructdir String
    //this path leads to the reconstruct executable (colmap.bat)
    QStringList extractWorkDir = executablePath.split("/");
    extractWorkDir.removeLast();
    QString reconstructdir;
    for (int i = 0; i < extractWorkDir.length(); i++) {
        reconstructdir.append(extractWorkDir[i]);
        if (i < extractWorkDir.length() - 1) {
            reconstructdir.append("/");
        }
    }

    //exportpath creation
    //this path leads to the selected export

    QString exportPath = m_currentExports.find(exportName).value();
    QStringList exportDirList = exportPath.split("/");
    QString exportDir = "";
    for (int i = 0; i < exportDirList.length() - 1; i++) {
        exportDir.append(exportDirList[i]);
        if (i < exportDirList.length() - 2) {
            exportDir.append("/");
        }
    }

    //get Itransforms and create maskPath
    QStringList iTransformNames = TransformManager::instance().getTransformList();
    std::vector<ITransform*> iTransformCopies;
    QString maskPath = exportPath;
    //mask path in project.ini file (for COLMAP) is set for the first iTransform that has a masks folder
    bool maskPathIsSet = false;
    std::vector<bool> iTransformUsed = m_outputWidget->getSelectedITransformMasks();
    if (iTransformUsed.size() != iTransformNames.length()) {
        //this shouldn't happen
        qDebug () << "start reconstruct failed, because .getTransformList() and getSelectedITransformMasks() didn't return Lists with the same size";
    }
    for (uint i = 0; i < unsigned(iTransformNames.length()); ++i) {
        if (!iTransformUsed[i]) {
            continue;
        }
        iTransformCopies.push_back(TransformManager::instance().getTransform(i)->copy());
        maskPath = exportPath;
        maskPath.append("/").append(iTransformCopies[i]->getName());
        QStringList iTransformOutputNames = iTransformCopies[i]->getOutputNames();
        for (int j = 0; j < iTransformOutputNames.length(); ++j) {
            QString maskCheck = iTransformOutputNames[j];
            if (!maskPathIsSet && maskCheck.replace("masks", "").isEmpty()) {
                maskPath.append("/").append(iTransformOutputNames[j]);
                maskPathIsSet = true;
            }
        }
    }

    //project file creation
    QString projectiniPath = "";
    if (createProject) {
        QString databasePath = exportDir + "/database.db";
        QString imagesPath = exportPath;
        projectiniPath = exportDir + "/" + exportName + "-project.ini";
        QString defaultFilePath = QCoreApplication::applicationDirPath() + "/colmap";

        //database Copy
        if (!createDatabaseFile(defaultFilePath + "/defaultdatabase.db", databasePath)) {
            qDebug() << "can't continue, Failed to copy database";
            return false;
        }

        //project Copy
        QMap<QString, QString> settingsmap;
        settingsmap.insert("image_path", imagesPath);
        settingsmap.insert("database_path", databasePath);
        if (maskPathIsSet) {
            settingsmap.insert("mask_path", maskPath);
        }
        if (!createProjectFile(defaultFilePath + "/defaultproject.ini", projectiniPath, settingsmap)) {
            qDebug() << "can't continue, Failed to create projectfile";
            return false;
        }
    }

    //boolean for whether it starts colmap gui or explorer
    bool colmapGUI = false;
    if (startargs.contains("gui", Qt::CaseSensitive)) {
        colmapGUI = true;
    }

    //boolean for whether it starts colmap automatic reconstruction
    bool autoreconstruct = false;
    if (startargs.contains("automatic_reconstructor", Qt::CaseSensitive)) {
        autoreconstruct = true;
    }

    //cmd.exe startarg
    QString cmdargs = "";
    if (executablePath.contains(".bat", Qt::CaseSensitive) || executablePath.contains(".exe", Qt::CaseSensitive)) {
        //executable is a batch or executable file

        //start args for cmd
        //approach with "cd C:/.." command first and then "colmap --startargs .. --project_path .."
        cmdargs.append("/K \"cd " + reconstructdir);
        cmdargs.append("&&colmap ");

        cmdargs.append(startargs);

        if (createProject) {
            cmdargs.append(" --project_path ");
            cmdargs.append(projectiniPath);
        }

        if (autoreconstruct) {
            cmdargs.append(" --workspace_path ");
            cmdargs.append(exportDir);
            cmdargs.append(" --image_path ");
            cmdargs.append(exportDir + "/images");
        }

        cmdargs.append("\"");

        executablePath = "cmd.exe " + cmdargs;

        //cmd shortcut plus batch file creation
        QString batchStartargs = startargs;
        if (createProject && colmapGUI) {
            batchStartargs.append(" --project_path ");
            batchStartargs.append(projectiniPath);
        }
        if (autoreconstruct) {
            batchStartargs.append(" --workspace_path ");
            batchStartargs.append(exportDir);
            batchStartargs.append(" --image_path ");
            QString imagespath = exportDir;
            imagespath.append("/images");
            batchStartargs.append(imagespath);
        }
        if (!createShortcutplusBatch(reconstructdir, batchStartargs, exportDir)) {
            //creation of shortcut and or batchfile failed
            qDebug() << "creation of shortcut and or batchfile failed";
        }

    }
    else {
        //executable is not a .bat nor .exe file
        qDebug() << "executable is neither a .exe nor a .bat file";
        emit sig_hasStatusMessage("executable is neither a .exe nor a .bat file!");
        return false;
    }


    QString debugargs = cmdargs;

    //starts process
    QProcess reconstructProcess;
    if (colmapGUI) {
        if (/*reconstructProcess.startDetached(executablePath)*/QProcess::startDetached(executablePath, QStringList(""))) {
            emit sig_hasStatusMessage("start of Reconstruction Software successful");
            return true;
        }
        else {
            qDebug() << "Couldn't start colmap gui process!";
            emit sig_hasStatusMessage("failed to start Reconstruction Software!");
        }
    }
    else {

        QStringList exportDirectoryList = exportDir.split("/");
        QString exportDirReverse = "";
        for (int i = 0; i < exportDirectoryList.length(); i++) {
            exportDirReverse.append(exportDirectoryList[i]);
            if (i < exportDirectoryList.length() - 1) {
                exportDirReverse.append("\\");
            }
        }
        if (reconstructProcess.startDetached("explorer.exe " + exportDirReverse)) {
            emit sig_hasStatusMessage("start of explorer successful");
            return true;
        }
        else {
            qDebug() << "Couldn't start explorer process!";
            emit sig_hasStatusMessage("failed to start Reconstruction Software!");
        }
    }

    return false;
}

#elif defined(Q_OS_LINUX)

bool ExportController::startReconstruct()
{
    //get data from GUI
    QMap<QString, QString> reconstructtools = ApplicationSettings::instance().getReconstructPath();
    QString executablePath = reconstructtools.take(m_reconstructDialog->getReconstructtool());
    QString exportName = m_reconstructDialog->getExportName();
    QString startargs = m_reconstructDialog->getStartArguments();
    //bool createProject = m_reconstructDialog->getCreateProject();

    QString exportPath = m_currentExports.find(exportName).value();
    qDebug() << "ExportPath:" << exportPath;

    //get Itransforms and create maskPath
    QStringList iTransformNames = TransformManager::instance().getTransformList();
    std::vector<ITransform*> iTransformCopies;
    QString maskPath = exportPath;
    //mask path in project.ini file (for COLMAP) is set for the first iTransform that has a masks folder
    bool maskPathIsSet = false;
    std::vector<bool> iTransformUsed = m_outputWidget->getSelectedITransformMasks();
    if (iTransformUsed.size() != iTransformNames.length()) {
        //this shouldn't happen
        qDebug () << "start reconstruct failed, because .getTransformList() and getSelectedITransformMasks() didn't return Lists with the same size";
    }
    for (uint i = 0; i < unsigned(iTransformNames.length()); ++i) {
        if (!iTransformUsed[i]) {
            continue;
        }
        iTransformCopies.push_back(TransformManager::instance().getTransform(i)->copy());
        maskPath = exportPath;
        maskPath.append("/").append(iTransformCopies[i]->getName());
        QStringList iTransformOutputNames = iTransformCopies[i]->getOutputNames();
        for (int j = 0; j < iTransformOutputNames.length(); ++j) {
            QString maskCheck = iTransformOutputNames[j];
            if (!maskPathIsSet && maskCheck.replace("masks", "").isEmpty()) {
                maskPath.append("/").append(iTransformOutputNames[j]);
                maskPathIsSet = true;
                break;
            }
        }
    }

    //boolean for whether it starts colmap gui or explorer
    bool colmapGUI = false;
    if (startargs.contains("gui", Qt::CaseSensitive)) {
        colmapGUI = true;
    }

    //boolean for whether it starts colmap automatic reconstruction
    bool autoreconstruct = false;
    if (startargs.contains("automatic_reconstructor", Qt::CaseSensitive)) {
        autoreconstruct = true;
    }

    QStringList c_args;
    c_args.push_back(startargs);

    if (autoreconstruct) {
        c_args << "--workspace_path" << exportPath;
        c_args << "--mask_path" << maskPath;
    }

    if (colmapGUI){
        c_args << "--database_path" << exportPath + "/database.db";
        if (maskPathIsSet) {
            c_args << "--ImageReader.mask_path" << maskPath;
            c_args << "--StereoFusion.mask_path" << maskPath;
        }
    }

    c_args << "--image_path" << exportPath + "/images";

    qint64 pid;
    if(colmapGUI){
        //QProcess::startDetached("/home/dominik/Downloads/iVS3D-1.1.9-linux-x64/iVS3D-core");
        QProcess::startDetached(executablePath, c_args, exportPath, &pid);
        emit sig_hasStatusMessage("start of Reconstruction Software successful");
        qDebug() << "PID: " << pid;
        return true;
    } else {
        //QProcess::startDetached("x-terminal-emulator", QStringList() << "-e" << "bash -c 'echo $PATH; read'");
        qDebug() << executablePath;
        QString colmap_cmd = executablePath + " " + c_args.join(" ");
        QProcess::startDetached("x-terminal-emulator", QStringList() << "-e" << ("bash -c '" + colmap_cmd + "; read'"), exportPath, &pid);
        qDebug() << "PID: " << pid;
        return true;
    }
}
#endif

void ExportController::deleteExportFolder(QString path)
{
    QDir toDelete(path);
    toDelete.removeRecursively();
}

bool ExportController::createDatabaseFile(QString defaultpath, QString targetpath)
{
    if(QFile::exists(defaultpath)) {
        if (QFile::exists(targetpath)) {
            QFile::remove(targetpath);
        }
        if (QFile::copy(defaultpath, targetpath)) {
            return true;
        }
        else {
            qDebug() << "couldn't copy database";
        }
    }
    else {
        qDebug() << "default database doesnt exist";
    }
    return false;
}

bool ExportController::createProjectFile(QString defaultpath, QString targetpath, QMap<QString, QString> projectsettings)
{
    if (QFile::exists(defaultpath)) {
        //see if projectfile already exists
        if (QFile::exists(targetpath)) {
            QFile::remove(targetpath);
        }
        if (QFile::copy(defaultpath, targetpath)) {
            //project file copied to target folder successful
            //adjusting project file
            QSettings *settings = new QSettings(targetpath, QSettings::IniFormat);
            QStringList keyList = projectsettings.keys();
            for (int i = 0; i < projectsettings.size(); i++) {
                QString maskPathCheck = keyList[i];
                if (maskPathCheck.replace("mask_path", "").isEmpty()) {
                    //set mask_path
                    settings->beginGroup("ImageReader");
                    settings->setValue(keyList[i], projectsettings.value(keyList[i]));
                    settings->endGroup();
                }
                else {
                    settings->setValue(keyList[i], projectsettings.value(keyList[i]));
                }
            }
            settings->sync();

            //changing project file
            QFile *projectiniFile = new QFile(targetpath);
            if (projectiniFile->open(QIODevice::ReadWrite | QIODevice::Text)) {
                QTextStream in(projectiniFile);
                QString line = in.readLine();
                QStringList lines;
                while (!line.isNull()) {
                    if(line.contains("[General]") || line.isEmpty()) {
                        //these get sorted out
                    }
                    else {
                        lines << line;
                    }
                    line = in.readLine();
                }
                in.flush();
                projectiniFile->flush();
                projectiniFile->close();
                //lines contain only data to be written again
                //now delete project file and create a new one with lines
                QFile::remove(targetpath);
                QFile *projectini = new QFile(targetpath);
                if (projectini->open(QIODevice::ReadWrite | QIODevice::Text)) {
                    //created and opened project file
                    QTextStream out(projectini);
                    out.seek(0);
                    for (QStringList::Iterator it = lines.begin(); it != lines.end(); it++) {
                        out << *it << "\n";
                    }
                    out.flush();
                }
                projectini->close();
                return true;
            }
        }
        else {
        }
    }
    else {
    }
    return false;
}

bool ExportController::createShortcutplusBatch(QString reconstructDir, QString startargs, QString exportDir)
{
    //create Strings
    QString batchPath = exportDir + "/colmap-with-startargs.bat";
    QString shortcutPath = exportDir + "/start_colmap-with-startargs.bat";
    QStringList batchLines = {};

    //change "/" into "\" for batchfile
    QString reconstructDirReverse = "";
    QStringList reconstructDirList = reconstructDir.split("/");
    QString exportDirReverse = "";
    QStringList exportDirList = exportDir.split("/");
    for (int i = 0; i < std::max(reconstructDirList.length(), exportDirList.length()); i++) {
        if (i < reconstructDirList.length()) {
            reconstructDirReverse.append(reconstructDirList[i]);
            if (i < reconstructDirList.length() - 1) {
                reconstructDirReverse.append("\\");
            }
        }
        if (i < exportDirList.length()) {
            exportDirReverse.append(exportDirList[i]);
            if (i < exportDirList.length() - 1) {
                exportDirReverse.append("\\");
            }
        }
    }

    //1st batch line "cd reconstructdir"
    batchLines.append("cd " + reconstructDirReverse);

    //2nd batch line "colmap startargs"
    QStringList startargsSplitSpace = startargs.split(" ");
    for (int i = 0; i < startargsSplitSpace.length(); i++) {
        if (startargsSplitSpace[i].contains("database.db", Qt::CaseSensitive)) {
            startargsSplitSpace[i] = exportDirReverse + "\\database.db";
        }
        if (startargsSplitSpace[i].contains("images", Qt::CaseSensitive)) {
            startargsSplitSpace[i] = exportDirReverse + "\\images";
        }
    }
    QString startargsAltered = "";
    for (int i = 0; i < startargsSplitSpace.length(); i++) {
        startargsAltered.append(startargsSplitSpace[i]);
        if (i < startargsSplitSpace.length() - 1) {
            startargsAltered.append(" ");
        }
    }
    batchLines.append("colmap " + startargsAltered);

    //3rd batch line "pause"
    batchLines.append("pause");

    //batch file copy and write
    if (QFile::exists(batchPath)) {
        //batch already exists (maybe old)
        QFile::remove(batchPath);
    }
    QFile *batchFile = new QFile(batchPath);
    if (batchFile->open(QIODevice::ReadWrite | QIODevice::Text)) {
        //created and opened project file
        QTextStream out(batchFile);
        out.seek(0);
        for (QStringList::Iterator it = batchLines.begin(); it != batchLines.end(); it++) {
            out << *it << "\n";
        }
        out.flush();
    }
    else {
        //couldn't open batch file
        return false;
    }
    batchFile->close();

    //shortcut file copy
    if (QFile::exists(shortcutPath)) {
        //shortcut already exists (maybe old)
        QFile::remove(shortcutPath);
    }
    QFile *shortcutFile = new QFile(shortcutPath);
    if (shortcutFile->open(QIODevice::ReadWrite | QIODevice::Text)) {
        //created and opened project file
        QTextStream out(shortcutFile);
        out.seek(0);
        out << "@echo off\n";
        out << "cmd.exe /K colmap-with-startargs.bat";
        out.flush();
    }
    else {
        //couldn't open shortcut file
        return false;
    }
    shortcutFile->close();

    return true;
}
