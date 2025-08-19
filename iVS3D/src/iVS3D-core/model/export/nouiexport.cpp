#include "nouiexport.h"

noUIExport::noUIExport(Progressable * receiver, QMap<QString, QVariant> exportSettings, DataManager* dm)
{
    m_logFile = LogManager::instance().createLogFile(stringContainer::Export, false);
    m_logFile->setSettings(exportSettings);

    //Overwrite export path from settings file if its provided via command line arguments
    if (qApp->property(stringContainer::OverwriteExport).toString().compare("") != 0) {
        m_path = qApp->property(stringContainer::OverwriteExport).toString();
    }
    else {
        m_path = exportSettings.find(stringContainer::OutputPath).value().toString();
    }


    QString resolution = exportSettings.find(stringContainer::Resolution).value().toString();
    Resolution output_res;
    if(output_res.fromString(resolution)){
        dm->getModelInputPictures()->getReaderParams()->setWorkingResolution(output_res);
    }


    QString roiString = exportSettings.find(stringContainer::ROI).value().toString();
    QStringList roiSplit = roiString.split(stringContainer::ROISpliter);

    if(exportSettings.find(stringContainer::UseROI).value().toBool()){
        ROI roi(QRect(roiSplit[0].toInt(), roiSplit[1].toInt(), roiSplit[2].toInt(), roiSplit[3].toInt()),dm->getModelInputPictures()->getReaderParams()->getOriginalResolution());
        dm->getModelInputPictures()->getReaderParams()->setRoi(roi);
        dm->getModelInputPictures()->getReaderParams()->setUseRoi(true);
    }

    QList<QVariant> useItransformVariant = exportSettings.find(stringContainer::UseITransform).value().toList();
    for (QVariant useItransform : useItransformVariant) {
        m_ITransfromSelection.push_back(useItransform.toBool());
    }
    m_dataManager = dm;

    QList<QVariant> iTransformSettingsList = exportSettings.find(stringContainer::ITransformSettings).value().toList();
    int idx = 0;
    for (QVariant var : iTransformSettingsList) {
        QMap<QString, QVariant> iTransformSettings = var.toMap();
        TransformManager::instance().setSettings(iTransformSettings, idx);
        idx++;
    }
    m_receiver = receiver;
}

void noUIExport::runExport()
{
    //creating export Directory if necessary
    QDir exportDir;
    exportDir.mkdir(m_path);

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


    //prepare iTransformNames
    QStringList iTransformNames = TransformManager::instance().getTransformList();


    std::vector<ITransform*> iTransformCopies;
    for (uint i = 0; i < unsigned(iTransformNames.length()); ++i) {
        //check if itransform has been selected to export
        if (!m_ITransfromSelection[i]) {
            continue;
        }
        iTransformCopies.push_back(TransformManager::instance().getTransform(i)->copy());
        QString iTransformDir = pathWOimages;
        iTransformDir.append("/").append(iTransformCopies[i]->getName());
        if (!QDir(iTransformDir).exists()) {
            QDir().mkdir(iTransformDir);
        }
    }


    m_exportExec = new ExportExecutor(this, m_dataManager);
    connect(m_exportExec, &ExportExecutor::sig_exportFinished, this, &noUIExport::slot_exportFinished, Qt::DirectConnection);

    connect(m_exportExec,&ExportExecutor::sig_progress, this, &noUIExport::slot_displayProgress, Qt::DirectConnection);
    connect(m_exportExec, &ExportExecutor::sig_message, this, &noUIExport::slot_displayMessage, Qt::DirectConnection);

    m_dataManager->createProject(outputName, pathWOimages + "/" + outputName + "-project.json");

    ExportConfig config;
    config.name = outputName;
    config.destination = m_path;
    config.transformations = iTransformCopies;
    config.format = "png";
    config.readerParams = *m_dataManager->getModelInputPictures()->getReaderParams();

    m_exportExec->startExport(config, m_logFile);
}

void noUIExport::slot_exportFinished(ExportResult result)
{
    if (result.type == ExportResultType::Failed) {
        m_receiver->slot_displayMessage(tr("Export failed: \n%1").arg(result.errorMessage));
    }
    disconnect(m_exportExec,&ExportExecutor::sig_progress, this, &noUIExport::slot_displayProgress);
    disconnect(m_exportExec, &ExportExecutor::sig_message, this, &noUIExport::slot_displayMessage);
    emit sig_exportFinished();
    delete m_exportExec;
}

void noUIExport::slot_displayMessage(QString message)
{
    m_receiver->slot_displayMessage(message);
}

void noUIExport::slot_displayProgress(int progress, QString currentProgress)
{
    m_receiver->slot_makeProgress(progress, currentProgress);
}
