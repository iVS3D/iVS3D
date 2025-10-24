#include "exportcontroller.h"

ExportController::ExportController(OutputWidget *outputWidget,
                                   DataManager *dataManager,
                                   lib3d::ots::ColmapWrapper *colmap) {
    m_exportExec = nullptr;
    m_reconstructDialog = nullptr;
    m_currentExports.clear();

    m_outputWidget = outputWidget;
    m_dataManager = dataManager;
    m_colmap = colmap;

    m_altitude_original = 0.0;
    m_altitude_current = 0.0;

    connect(m_outputWidget, &OutputWidget::sig_reconstruct, this,
            &ExportController::slot_reconstruct);
    connect(m_outputWidget, &OutputWidget::sig_export, this,
            &ExportController::slot_export);
    connect(m_outputWidget, &OutputWidget::sig_pathChanged, this,
            &ExportController::slot_outputPathChanged);
    connect(m_outputWidget, &OutputWidget::sig_resChanged, this,
            &ExportController::slot_exportResolutionChanged);
    connect(m_outputWidget, &OutputWidget::sig_altitudeChanged, this,
            &ExportController::slot_altitudeChanged);

    connect(m_dataManager->getModelInputPictures(),
            &ModelInputPictures::sig_mipChanged, this,
            &ExportController::slot_onKeyframesChanged);

    m_outputWidget->setEnabled(true);

    // set standard (input) path
    m_path = m_dataManager->getModelInputPictures()->getPath();
    if (!m_dataManager->getModelInputPictures()->getReader()->isDir()) {
        QStringList pathList = m_path.split("/");
        pathList.removeLast();
        m_path = pathList.join("/");
    }
    m_path += "/export";
    m_outputWidget->setOutputPath(m_path);
    m_outputWidget->enableReconstruct(false);

    // set standard resolution
    m_originalResolution = m_dataManager->getModelInputPictures()
                               ->getReaderParams()
                               ->getOriginalResolution();
    m_workingResolution = m_dataManager->getModelInputPictures()
                              ->getReaderParams()
                              ->getWorkingResolution();
    m_exportResolution = m_originalResolution;  // default export resolution is
                                                // original resolution

    // set the default output format depending on the input type (images /
    // video)
    if (m_dataManager->getModelInputPictures()->getReader()->isDir()) {
        m_outputWidget->enableFormat(EXPORT_FORMAT_SAME_AS_INPUT, true);
        m_outputWidget->setOutputFormat(EXPORT_FORMAT_SAME_AS_INPUT);
    } else {
        m_outputWidget->enableFormat(EXPORT_FORMAT_SAME_AS_INPUT, false);
        m_outputWidget->setOutputFormat("png");
    }
}

ExportController::~ExportController() {
    disconnect(m_outputWidget, &OutputWidget::sig_reconstruct, this,
               &ExportController::slot_reconstruct);
    disconnect(m_outputWidget, &OutputWidget::sig_export, this,
               &ExportController::slot_export);
    disconnect(m_outputWidget, &OutputWidget::sig_pathChanged, this,
               &ExportController::slot_outputPathChanged);

    m_outputWidget->setEnabled(false);

    m_outputWidget->setOutputPath("");
    m_currentExports.clear();
}

QMap<QString, QVariant> ExportController::getOutputSettings() {
    QMap<QString, QVariant> settings;

    settings.insert(stringContainer::Resolution, m_exportResolution.toString());

    bool use_roi = m_roi.has_value() && !m_roi->isDefault();
    settings.insert(stringContainer::UseROI, use_roi);

    if (use_roi) settings.insert(stringContainer::ROI, m_roi->toQRectF());

    std::vector<bool> useItransform =
        m_outputWidget->getSelectedITransformMasks();
    QList<QVariant> iTransformSettings;
    QList<QVariant> useItransformVariant;
    int idx = 0;
    for (bool use : useItransform) {
        useItransformVariant.append(use);
        iTransformSettings.append(
            TransformManager::instance().getSettings(idx));
    }
    settings.insert(stringContainer::UseITransform, useItransformVariant);
    settings.insert(stringContainer::ITransformSettings, iTransformSettings);

    settings.insert(stringContainer::OutputFormat,
                    m_outputWidget->getExportFormat());
    return settings;
}

void ExportController::setOutputSettings(QMap<QString, QVariant> settings) {
    if (settings.contains(stringContainer::OutputPath)) {
        m_path = settings.find(stringContainer::OutputPath).value().toString();
        m_outputWidget->setOutputPath(m_path);
    }

    QList<QVariant> useItransformVariant =
        settings.find(stringContainer::UseITransform).value().toList();
    std::vector<bool> selection;
    for (QVariant useItransform : useItransformVariant) {
        selection.push_back(useItransform.toBool());
    }
    m_outputWidget->setSelectedITransformMasks(selection);

    if (settings.contains(stringContainer::Resolution)) {
        QString resolution =
            settings.find(stringContainer::Resolution).value().toString();
        Resolution res;
        if (res.fromString(resolution)) {
            // check if the resolution is valid (not larger than original
            // resolution)
            ReaderParams rp;
            rp.initialize(m_originalResolution);
            bool valid = rp.setWorkingResolution(res);
            if (!valid) {
                qDebug() << "Given export resolution " << resolution
                         << " is not valid, using original resolution "
                         << m_originalResolution.toString() << " instead.";
                res = m_originalResolution;
            }
            m_exportResolution = res;
            m_outputWidget->setResolution(resolution);
        }
    }

    if (settings.contains(stringContainer::OutputFormat)) {
        QString format =
            settings.find(stringContainer::OutputFormat).value().toString();
        m_outputWidget->setOutputFormat(format);
    }

    QList<QVariant> iTransformSettingsList =
        settings.find(stringContainer::ITransformSettings).value().toList();
    int idx = 0;
    for (QVariant var : iTransformSettingsList) {
        QMap<QString, QVariant> iTransformSettings = var.toMap();
        TransformManager::instance().setSettings(iTransformSettings, idx);
        idx++;
    }
    updateFormatOptions();
}

void ExportController::setOriginalAltitude(double altitude) {
    m_altitude_original = altitude;
    m_altitude_current = m_altitude_original;
    updateFormatOptions();
    m_outputWidget->setOutputFormat(
        m_dataManager->getModelInputPictures()->getReader()->isDir()
            ? EXPORT_FORMAT_SAME_AS_INPUT
            : "png");
}

void ExportController::slot_reconstruct() {
    emit sig_stopPlay();
    ApplicationSettings as = ApplicationSettings::instance();

    // get reconstruct tools
    QMap<QString, QString> reconstructMap = as.getReconstructPath();
    // build Stringlist with reconstruct tool names
    int reconstructMapSize = reconstructMap.size();
    QStringList reconstructtoolList;
    for (int i = 0; i < reconstructMapSize; i++) {
        QString tempKey = reconstructMap.begin().key();
        reconstructtoolList.push_back(tempKey);
        reconstructMap.remove(tempKey);
    }

    QStringList exportList;
    for (QString exportName : m_currentExports.keys()) {
        exportList.push_back(exportName);
    }

    m_reconstructDialog =
        new ReconstructDialog(m_outputWidget, exportList, reconstructtoolList);
    if (m_reconstructDialog->exec()) {
        if (startReconstruct()) {
            m_reconstructDialog->close();
        }
    }
}

void ExportController::slot_export() {
    // use Working res, ROI, etc from MIP reader, but do not modify the export
    // resolution!
    auto readerParams =
        m_dataManager->getModelInputPictures()->getReaderParams();
    m_originalResolution = readerParams->getOriginalResolution();
    m_workingResolution = readerParams->getWorkingResolution();
    m_roi = std::nullopt;
    if (readerParams->getUseRoi() && !readerParams->getRoi().isDefault()) {
        m_roi = readerParams->getRoi();
    }

    m_lfExport = LogManager::instance().createLogFile("Export", false);
    m_lfExport->setSettings(getOutputSettings());

    if (m_path.endsWith("/")) {
        m_path.chop(1);
    }

    emit sig_stopPlay();

    // creating export Directory if necessary
    QDir exportDir;
    if (!exportDir.mkpath(m_path)) {
        qDebug() << "Couldn't create Export Directory " << m_path;
        emit sig_hasStatusMessage(
            QString(tr("Couldn't create Export Directory: %1")).arg(m_path));
        return;
    }

    QString outputName;
    // adding images folder to export if necessary
    // split path into folders, look at last foldername, replace "images" with
    // nothing if that string is NOT empty, it means the last foldername WASN'T
    // "images", so we are creating it
    if (!m_path.split("/").last().replace("images", "").isEmpty()) {
        outputName = m_path.split("/").last();
        m_path.append("/images");
        if (!QDir(m_path).exists()) {
            QDir().mkdir(m_path);
        }
    } else {
        QStringList temp = m_path.split("/");
        Q_ASSERT(temp.length() >= 2);
        outputName = temp[temp.length() - 2];
    }

    // create string without /images
    QString pathWOimages = "";
    QStringList outputPathBits = m_path.split("/");
    outputPathBits.removeLast();
    for (int i = 0; i < outputPathBits.length(); i++) {
        pathWOimages.append(outputPathBits[i]);
        if (i < outputPathBits.length() - 1) {
            pathWOimages.append("/");
        }
    }

    // check if export folder is empty
    QStringList exportEntries = QDir(pathWOimages).entryList();
    bool wipeDir = false;
    if (exportEntries.length() > 3) {
        // we have existing data
        wipeDir = true;
    } else if (exportEntries.contains("images", Qt::CaseSensitive)) {
        QString imagesDir = pathWOimages;
        imagesDir.append("/images");
        if (QDir(imagesDir).entryList().length() > 2) {
            // images folder is not empty
            wipeDir = true;
        }
    }

    // prepare iTransformNames
    QStringList iTransformNames =
        TransformManager::instance().getTransformList();

    if (wipeDir) {
        EmptyFolderDialog *emptyFolderD =
            new EmptyFolderDialog(m_outputWidget, pathWOimages);

        // if input path is same as output path, disable delete button
        QString inputPath = m_dataManager->getModelInputPictures()->getPath();
        QDir inputDir(inputPath);
        QDir outputDir(m_path);
        if (inputDir.absolutePath() == outputDir.absolutePath()) {
            emptyFolderD->setDeleteButtonEnabled(false);
        }
        int result = emptyFolderD->exec();
        switch (result) {
            case 0:
                // abort
                qDebug() << "User aborted export from empty folder dialog";
                return;
            case 1: {
                // user wants to delete and continue
                // Delete export images
                deleteExportFolder(pathWOimages);
                if (!QDir(m_path).exists()) {
                    QDir().mkpath(m_path);
                }
                break;
            }
            case 2:
                // ignore
                break;
            default:
                // this shouldn't occur
                return;
        }
    }

    // add Itransform folders
    std::vector<bool> iTransformUsed =
        m_outputWidget->getSelectedITransformMasks();
    if (iTransformNames.length() != (int)iTransformUsed.size()) {
        // this shouldn't happen!
        qDebug()
            << "count of iTransformNames doesn't match iTransformUsed list";
        return;
    }
    std::vector<ITransform *> iTransformCopies;
    for (uint i = 0; i < unsigned(iTransformNames.length()); ++i) {
        // check if itransform has been selected to export
        if (!iTransformUsed[i]) {
            continue;
        }
        iTransformCopies.push_back(
            TransformManager::instance().getTransform(i)->copy());
    }

    if (m_exportExec != nullptr) {
        delete m_exportExec;
    }
    m_exportExec = new ExportExecutor(this, m_dataManager);
    // connect GUI to export executor to display progress and result or to abort
    // export
    connect(m_exportExec, &ExportExecutor::sig_exportAborted, this,
            &ExportController::slot_exportAborted);
    connect(m_exportExec, &ExportExecutor::sig_exportFinished, this,
            &ExportController::slot_exportFinished);
    connect(m_exportExec, &ExportExecutor::sig_progress, m_outputWidget,
            &OutputWidget::slot_displayProgress);
    connect(m_outputWidget, &OutputWidget::sig_abort, m_exportExec,
            &ExportExecutor::slot_abort);
    m_outputWidget->showProgress();  // swap OutputWidget to display progress

    m_dataManager->createProject(
        outputName, pathWOimages + "/" + outputName + "-project.json");

    ExportConfig config;
    config.name = outputName;
    config.destination = m_path;
    config.format = m_outputWidget->getExportFormat();
    config.original_resolution = m_originalResolution;
    config.working_resolution = m_workingResolution;
    config.export_resolution = m_exportResolution;
    config.roi = m_roi;
    config.transformations = iTransformCopies;
    config.copy_images =
        canCopyImages() &&
        (m_outputWidget->getExportFormat() == EXPORT_FORMAT_SAME_AS_INPUT);

    // start export
    m_exportExec->startExport(config, m_lfExport);
    emit sig_exportStarted();
    m_timer = QElapsedTimer();
    m_timer.start();
    if (m_path.endsWith("/images")) {
        m_path.chop(7);  // remove /images at the end
    }
}

void ExportController::slot_outputPathChanged(QString path) {
    path.replace("\\", "/");
    m_path = path;
}

void ExportController::slot_exportAborted() {
    auto duration_ms = m_timer.elapsed();
    emit sig_hasStatusMessage(tr("Export aborted after ") +
                              QString::number(duration_ms) + tr("ms"));
    // disconnect GUI to export executor
    disconnect(m_exportExec, &ExportExecutor::sig_exportAborted, this,
               &ExportController::slot_exportAborted);
    disconnect(m_exportExec, &ExportExecutor::sig_exportFinished, this,
               &ExportController::slot_exportFinished);
    disconnect(m_exportExec, &ExportExecutor::sig_progress, m_outputWidget,
               &OutputWidget::slot_displayProgress);
    disconnect(m_outputWidget, &OutputWidget::sig_abort, m_exportExec,
               &ExportExecutor::slot_abort);
    m_outputWidget->showExportOptions();  // swap OutputWidget to display result
    emit sig_exportAborted();
}

void ExportController::slot_exportFinished(ExportResult result) {
    // disconnect GUI to export executor
    disconnect(m_exportExec, &ExportExecutor::sig_exportAborted, this,
               &ExportController::slot_exportAborted);
    disconnect(m_exportExec, &ExportExecutor::sig_exportFinished, this,
               &ExportController::slot_exportFinished);
    disconnect(m_exportExec, &ExportExecutor::sig_progress, m_outputWidget,
               &OutputWidget::slot_displayProgress);
    disconnect(m_outputWidget, &OutputWidget::sig_abort, m_exportExec,
               &ExportExecutor::slot_abort);
    m_outputWidget->showExportOptions();  // swap OutputWidget to display result

    if (result.type == ExportResultType::Aborted) {
        emit sig_exportFinished(QVariantMap());
        emit sig_hasStatusMessage(tr("Export aborted by the user."));
        return;
    }

    if (result.type == ExportResultType::Failed) {
        emit sig_exportFinished(QVariantMap());
        QMessageBox::critical(nullptr, tr("Error during export"),
                              tr("Export failed: %1").arg(result.errorMessage));
        return;
    }

    auto duration_ms = m_timer.elapsed();
    if (result.type == ExportResultType::PartialSuccess) {
        emit sig_hasStatusMessage(
            tr("Export finished after ") + QString::number(duration_ms) +
            tr("ms") + tr(" with ") + QString::number(result.brokenImages) +
            (result.brokenImages == 1 ? tr(" broken image.")
                                      : tr(" broken images.")));
    }
    if (result.type == ExportResultType::Success) {
        emit sig_hasStatusMessage(tr("Export finished after ") +
                                  QString::number(duration_ms) + tr("ms"));
    }

    // now we have an export, so enable reconstruct
    m_outputWidget->enableReconstruct(true);

    // Save current exportPath and name
    m_currentExports.insert(m_path.split("/").last(), m_path);
    m_colmap->setLocalPresetSequence(m_path.split("/").last(),
                                     m_path + "/images");
    emit sig_exportFinished(getOutputSettings());
}

void ExportController::slot_showExportSettings(
    QMap<QString, QVariant> exportSettings) {
    setOutputSettings(exportSettings);
}

void ExportController::slot_onKeyframesChanged() {
    m_outputWidget->enableExport(
        m_dataManager->getModelInputPictures()->getKeyframeCount(true) > 0);
}

void ExportController::slot_nextImageOnPlayer(uint idx) {
    m_imageOnPlayerId = idx;
}

bool ExportController::startReconstruct() {
    // get data from GUI
    QMap<QString, QString> reconstructtools =
        ApplicationSettings::instance().getReconstructPath();
    QString executablePath =
        reconstructtools.take(m_reconstructDialog->getReconstructtool());
    QString exportName = m_reconstructDialog->getExportName();
    QString startargs = m_reconstructDialog->getStartArguments();
    // bool createProject = m_reconstructDialog->getCreateProject();

    QString exportPath = m_currentExports.find(exportName).value();
    qDebug() << "ExportPath:" << exportPath;

    // get Itransforms and create maskPath
    QStringList iTransformNames =
        TransformManager::instance().getTransformList();
    std::vector<ITransform *> iTransformCopies;
    QString maskPath = exportPath;
    // mask path in project.ini file (for COLMAP) is set for the first
    // iTransform that has a masks folder
    bool maskPathIsSet = false;
    std::vector<bool> iTransformUsed =
        m_outputWidget->getSelectedITransformMasks();
    if (iTransformUsed.size() != iTransformNames.length()) {
        // this shouldn't happen
        qDebug() << "start reconstruct failed, because .getTransformList() and "
                    "getSelectedITransformMasks() didn't return Lists with the "
                    "same size";
    }

    maskPath.append("/masks");
    maskPathIsSet = QDir(maskPath).exists();

    // boolean for whether it starts colmap gui or explorer
    bool colmapGUI = false;
    if (startargs.contains("gui", Qt::CaseSensitive)) {
        colmapGUI = true;
    }

    // boolean for whether it starts colmap automatic reconstruction
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

    if (colmapGUI) {
        c_args << "--database_path" << exportPath + "/database.db";
        if (maskPathIsSet) {
            c_args << "--ImageReader.mask_path" << maskPath;
            c_args << "--StereoFusion.mask_path" << maskPath;
        }
    }

    c_args << "--image_path" << exportPath + "/images";

    qint64 pid;
    if (colmapGUI) {
        // QProcess::startDetached("/home/dominik/Downloads/iVS3D-1.1.9-linux-x64/iVS3D-core");
        QProcess::startDetached(executablePath, c_args, exportPath, &pid);
        emit sig_hasStatusMessage(
            "start of Reconstruction Software successful");
        qDebug() << "PID: " << pid;
        return true;
    } else {
        // QProcess::startDetached("x-terminal-emulator", QStringList() << "-e"
        // << "bash -c 'echo $PATH; read'");
        qDebug() << executablePath;
        QString colmap_cmd = executablePath + " " + c_args.join(" ");
        QProcess::startDetached(
            "x-terminal-emulator",
            QStringList() << "-e" << ("bash -c '" + colmap_cmd + "; read'"),
            exportPath, &pid);
        qDebug() << "PID: " << pid;
        return true;
    }
}

void ExportController::deleteExportFolder(QString path) {
    QDir toDelete(path);
    toDelete.removeRecursively();
}

bool ExportController::createDatabaseFile(QString defaultpath,
                                          QString targetpath) {
    if (QFile::exists(defaultpath)) {
        if (QFile::exists(targetpath)) {
            QFile::remove(targetpath);
        }
        if (QFile::copy(defaultpath, targetpath)) {
            return true;
        } else {
            qDebug() << "couldn't copy database";
        }
    } else {
        qDebug() << "default database doesnt exist";
    }
    return false;
}

bool ExportController::createProjectFile(
    QString defaultpath, QString targetpath,
    QMap<QString, QString> projectsettings) {
    if (QFile::exists(defaultpath)) {
        // see if projectfile already exists
        if (QFile::exists(targetpath)) {
            QFile::remove(targetpath);
        }
        if (QFile::copy(defaultpath, targetpath)) {
            // project file copied to target folder successful
            // adjusting project file
            QSettings *settings =
                new QSettings(targetpath, QSettings::IniFormat);
            QStringList keyList = projectsettings.keys();
            for (int i = 0; i < projectsettings.size(); i++) {
                QString maskPathCheck = keyList[i];
                if (maskPathCheck.replace("mask_path", "").isEmpty()) {
                    // set mask_path
                    settings->beginGroup("ImageReader");
                    settings->setValue(keyList[i],
                                       projectsettings.value(keyList[i]));
                    settings->endGroup();
                } else {
                    settings->setValue(keyList[i],
                                       projectsettings.value(keyList[i]));
                }
            }
            settings->sync();

            // changing project file
            QFile *projectiniFile = new QFile(targetpath);
            if (projectiniFile->open(QIODevice::ReadWrite | QIODevice::Text)) {
                QTextStream in(projectiniFile);
                QString line = in.readLine();
                QStringList lines;
                while (!line.isNull()) {
                    if (line.contains("[General]") || line.isEmpty()) {
                        // these get sorted out
                    } else {
                        lines << line;
                    }
                    line = in.readLine();
                }
                in.flush();
                projectiniFile->flush();
                projectiniFile->close();
                // lines contain only data to be written again
                // now delete project file and create a new one with lines
                QFile::remove(targetpath);
                QFile *projectini = new QFile(targetpath);
                if (projectini->open(QIODevice::ReadWrite | QIODevice::Text)) {
                    // created and opened project file
                    QTextStream out(projectini);
                    out.seek(0);
                    for (QStringList::Iterator it = lines.begin();
                         it != lines.end(); it++) {
                        out << *it << "\n";
                    }
                    out.flush();
                }
                projectini->close();
                return true;
            }
        } else {
        }
    } else {
    }
    return false;
}

void ExportController::slot_altitudeChanged(double altitude) {
    m_altitude_current = altitude;
    updateFormatOptions();
}

void ExportController::slot_roiChanged(std::optional<ROI> roi) {
    m_roi = roi;
    updateFormatOptions();
}

bool ExportController::canCopyImages() {
    if (!m_dataManager->getModelInputPictures()->getReader()->isDir())
        return false;
    if (abs(m_altitude_current - m_altitude_original) > 1e-2)
        return false;  // check if altitude has changed within two digits
    if (!(m_originalResolution == m_exportResolution)) return false;
    if (m_roi.has_value() && !m_roi->isDefault()) return false;
    return true;
}

void ExportController::updateFormatOptions() {
    m_outputWidget->enableFormat(EXPORT_FORMAT_SAME_AS_INPUT, canCopyImages());
}

bool ExportController::createShortcutplusBatch(QString reconstructDir,
                                               QString startargs,
                                               QString exportDir) {
    // create Strings
    QString batchPath = exportDir + "/colmap-with-startargs.bat";
    QString shortcutPath = exportDir + "/start_colmap-with-startargs.bat";
    QStringList batchLines = {};

    // change "/" into "\" for batchfile
    QString reconstructDirReverse = "";
    QStringList reconstructDirList = reconstructDir.split("/");
    QString exportDirReverse = "";
    QStringList exportDirList = exportDir.split("/");
    for (int i = 0;
         i < std::max(reconstructDirList.length(), exportDirList.length());
         i++) {
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

    // 1st batch line "cd reconstructdir"
    batchLines.append("cd " + reconstructDirReverse);

    // 2nd batch line "colmap startargs"
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

    // 3rd batch line "pause"
    batchLines.append("pause");

    // batch file copy and write
    if (QFile::exists(batchPath)) {
        // batch already exists (maybe old)
        QFile::remove(batchPath);
    }
    QFile *batchFile = new QFile(batchPath);
    if (batchFile->open(QIODevice::ReadWrite | QIODevice::Text)) {
        // created and opened project file
        QTextStream out(batchFile);
        out.seek(0);
        for (QStringList::Iterator it = batchLines.begin();
             it != batchLines.end(); it++) {
            out << *it << "\n";
        }
        out.flush();
    } else {
        // couldn't open batch file
        return false;
    }
    batchFile->close();

    // shortcut file copy
    if (QFile::exists(shortcutPath)) {
        // shortcut already exists (maybe old)
        QFile::remove(shortcutPath);
    }
    QFile *shortcutFile = new QFile(shortcutPath);
    if (shortcutFile->open(QIODevice::ReadWrite | QIODevice::Text)) {
        // created and opened project file
        QTextStream out(shortcutFile);
        out.seek(0);
        out << "@echo off\n";
        out << "cmd.exe /K colmap-with-startargs.bat";
        out.flush();
    } else {
        // couldn't open shortcut file
        return false;
    }
    shortcutFile->close();

    return true;
}

void ExportController::slot_exportResolutionChanged(QString resolution) {
    // check if given string is a valid resolution
    Resolution res;
    if (res.fromString(resolution)) {
        // check if the resolution is valid (not larger than original
        // resolution)
        ReaderParams rp;
        rp.initialize(m_originalResolution);
        bool valid = rp.setWorkingResolution(res);
        if (valid) {
            m_exportResolution = res;
        }
        m_outputWidget->setResolutionValid(valid);
        m_outputWidget->enableExport(valid);
    }
    updateFormatOptions();
}
