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
    m_resolution = parseResolution(resolution);


    QString roiString = exportSettings.find(stringContainer::ROI).value().toString();
    QStringList roiSplit = roiString.split(stringContainer::ROISpliter);

    m_roi = *new QRect(roiSplit[0].toInt(), roiSplit[1].toInt(), roiSplit[2].toInt(), roiSplit[3].toInt());

    m_useCrop = exportSettings.find(stringContainer::UseROI).value().toBool();

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

QPoint noUIExport::parseResolution(QString resolutionString)
{
    QStringList resolutionList = resolutionString.split("x");
    int width = resolutionList[0].toInt();
    int height = resolutionList[1].toInt();
    return QPoint(width, height);
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

    //If Use Crop is checked the current roi is used
    if (m_useCrop) {
        m_exportExec->startExport(m_resolution, m_path, outputName, m_roi, iTransformCopies, m_logFile);
    }
    //Otherwise roi won't be used (-> 0x0 Rect) this wont override m_roi
    else {
       m_exportExec->startExport(m_resolution, m_path, outputName, QRect(0,0,0,0), iTransformCopies, m_logFile);
    }

}

void noUIExport::slot_exportFinished(int result)
{
    if (result == -1) {
        m_receiver->slot_displayMessage("Export failed. Maybe the path is invalid");
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
