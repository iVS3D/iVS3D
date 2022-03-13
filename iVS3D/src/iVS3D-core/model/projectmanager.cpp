#include "projectmanager.h"


ProjectManager::ProjectManager()
{

}

bool ProjectManager::saveProject(ModelInputPictures *mip, ModelAlgorithm *ma)
{
    if (mip == nullptr || ma == nullptr){
        return false;
    }
    QVariant mipVariant = mip->toText();
    QVariant maVariant = ma->toText();

    QJsonObject all;
    QJsonObject mipJSON = mipVariant.toJsonObject();
    QJsonObject maJSON = maVariant.toJsonObject();
    all.insert(stringContainer::projectNameIdentifier, m_projectName);
    all.insert(stringContainer::mipIdentifier, mipJSON);
    all.insert(stringContainer::maIdentifier, maJSON);


    QFile file(m_projectPath);
    QJsonDocument doc(all);
    if (!file.open(QFile::WriteOnly | QFile::Text | QFile::Truncate)) {
        return false;
    }
    file.write(doc.toJson());
    file.close();
    return true;
}

bool ProjectManager::saveProjectAs(ModelInputPictures *mip, ModelAlgorithm *ma, QString path, const QString &name)
{
    if (0 != QString::compare(path.split(".").last(), "json", Qt::CaseSensitive)) {
        return false;
    }
    QString oldName = m_projectName;
    QString oldPath = m_projectPath;
    m_projectName = name;
    m_projectPath = path;
    if (!saveProject(mip, ma)) {
        m_projectName = oldName;
        m_projectPath = oldPath;
        return false;
    }
    return true;
}

bool ProjectManager::createProject(ModelInputPictures *mip, ModelAlgorithm *ma, const QString &path, const QString &name) {
    QString oldName = m_projectName;
    QString oldPath = m_projectPath;
    m_projectName = name;
    m_projectPath = path;
    if (!saveProject(mip, ma)) {
        m_projectName = oldName;
        m_projectPath = oldPath;
        return false;
    }
    m_projectName = oldName;
    m_projectPath = oldPath;
    return true;
}

bool ProjectManager::loadProject(ModelInputPictures *mip, ModelAlgorithm *ma, QString path)
{
    //Open project file
    QFile file(path);
    bool openSuccess = file.open(QIODevice::ReadOnly | QIODevice::Text);
    if (!openSuccess) {
        return false;
    }

    QString data = file.readAll();
    file.close();
    //Get JsonDocoment from file
    QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8());
    QJsonObject fullObject = doc.object();
    //Get project Name
    m_projectName = fullObject.find(stringContainer::projectNameIdentifier).value().toString();
    //Get the mip JsonObject and create mip with it
    QJsonObject mipJson = fullObject.find(stringContainer::mipIdentifier).value().toObject();
    mip->fromText(mipJson);
    //Get the ma JsonObject and create mip with it
    QJsonObject maVar = fullObject.find(stringContainer::maIdentifier).value().toObject();
    ma->fromText(maVar);

    m_projectPath = path;
    return true;
}

bool ProjectManager::isProjectLoaded()
{
    return m_projectName != "";
}

QString ProjectManager::getProjectName()
{
    return m_projectName;
}


QString ProjectManager::getProjectPath()
{
    return m_projectPath;
}

