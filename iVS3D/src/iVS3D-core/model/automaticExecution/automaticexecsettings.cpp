#include "automaticexecsettings.h"


AutomaticExecSettings::AutomaticExecSettings()
{

}

QList<QPair<QString, QMap<QString, QVariant>>> AutomaticExecSettings::getPluginList()
{
    return m_algoList;
}

QStringList AutomaticExecSettings::getPluginNames()
{
    QStringList usedPlugins;
    QString currentName;
    for (QPair<QString, QMap<QString, QVariant>> entry : m_algoList) {
        QString name;
        name = AlgorithmManager::instance().getPluginNameFromFileName(entry.first);
        if (entry.second.isEmpty()) {
            name.append("  -  Generated settings");
        }
        else {
            name.append(" - ");
            currentName = name;
            QMapIterator<QString, QVariant> iter(entry.second);
            while(iter.hasNext()) {
               iter.next();
               QString identifier = iter.key() + " = " + iter.value().toString() + "; ";
               if (iter.value().toString() == "") {
                   continue;
               }
               name.append(identifier);
            }
            name.chop(2);

        }
        usedPlugins.append(name);
    }
    return usedPlugins;
}

void AutomaticExecSettings::setExportController(ExportController* exportController)
{
    m_exportController =  exportController;
}

void AutomaticExecSettings::loadPluginList(QString path)
{
    m_algoList.clear();
    //Open file
    QFile file(path);
    bool openSuccess = file.open(QIODevice::ReadOnly | QIODevice::Text);
    if (!openSuccess) {
        return;
    }

    QString data = file.readAll();
    file.close();
    //Get JsonDocoment from file
    QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8());
    QJsonObject fullObject = doc.object();

    for (int i = 0; i < INT_MAX; i++) {
        //Find Objects in correct order
        QJsonObject::iterator iter = fullObject.find(QString::number(i));
        if (iter == fullObject.end()) {
            break;
        }
        //Step into the value of the found object
        QJsonObject innerObject = iter.value().toObject();
        //value of the innerObject is the settings map
        QVariant settingsObject = innerObject.begin().value().toObject();
        //Key of the innerObject is the plugin name
        QString pluginName = QString::fromStdString(innerObject.begin().key().toStdString());
        QMap<QString, QVariant> pluginSettings = settingsObject.toMap();
        QPair<QString, QMap<QString, QVariant>> pair = QPair<QString, QMap<QString, QVariant>>(pluginName, pluginSettings);
        m_algoList.append(pair);
    }

    //dont use ui, if command line is active
    if (QCoreApplication::arguments().size() < 2) {
        updateShownAlgoList();
    }
}


void AutomaticExecSettings::slot_addAuto(int idx, bool generate)
{
    QString pluginName = AlgorithmManager::instance().getPluginFileNameToIndex(idx);
    QMap<QString, QVariant> settings;
    if (generate) {
        settings = QMap<QString, QVariant>();
    }
    else {
        settings = AlgorithmManager::instance().getSettings(idx);
    }
    QPair<QString, QMap<QString, QVariant>> newEntry = QPair<QString, QMap<QString, QVariant>>(pluginName, settings);
    m_algoList.append(newEntry);
    updateShownAlgoList();

}

void AutomaticExecSettings::slot_addAutoOutput()
{
    QString pluginName = stringContainer::Export;
    QMap<QString, QVariant> settings = m_exportController->getOutputSettings();
    QPair<QString, QMap<QString, QVariant>> newEntry = QPair<QString, QMap<QString, QVariant>>(pluginName, settings);
    m_algoList.append(newEntry);
    updateShownAlgoList();
}

void AutomaticExecSettings::slot_removedPlugin(int row)
{
    m_algoList.removeAt(row);
    updateShownAlgoList();
}


void AutomaticExecSettings::slot_saveConfiguration(QString path)
{
    QJsonObject fullSave;
    int i = 0;
    QListIterator<QPair<QString, QVariantMap>> iterator(m_algoList);
    while (iterator.hasNext()) {
        QPair<QString, QVariantMap> pair = iterator.next();
        QJsonObject current;
        QString name = pair.first;
        current.insert(name, QJsonObject::fromVariantMap(pair.second));
        fullSave.insert(QString::number(i), current);
        i++;
    }
    if (path.right(5).toLower() != ".json") {
        path.append(".json");
    }
    QFile file(path);
    QJsonDocument doc(fullSave);
    if (!file.open(QFile::WriteOnly | QFile::Text | QFile::Truncate)) {
        return;
    }
    file.write(doc.toJson());
    file.close();
    return;
}

void AutomaticExecSettings::updateShownAlgoList()
{
    emit sig_updatedSelectedPlugins(getPluginNames());
}

void AutomaticExecSettings::slot_loadConfiguration(QString path)
{
    loadPluginList(path);
}

void AutomaticExecSettings::slot_doubleClickedItem(int row)
{
    //Signal ExportController to show the export settings
    if (m_algoList[row].first.compare(stringContainer::Export) == 0) {
        emit sig_showExportSettings(m_algoList[row].second);
        int iTransfromIndex = AlgorithmManager::instance().getAlgorithmCount() + 1;
        emit sig_setAlgorithm(iTransfromIndex);
        return;
    }

    int idx = AlgorithmManager::instance().getIndexFromFileName(m_algoList[row].first);

    //If the settings map is empty (-> generated settings) don't update the plugins settings
    if (!m_algoList[row].second.isEmpty()) {
       AlgorithmManager::instance().setSettings(idx, m_algoList[row].second);
    }
    emit sig_setAlgorithm(idx);

}

void AutomaticExecSettings::slot_autoOrderChanged(int first, int second)
{
    m_algoList.swap(first, second);
    updateShownAlgoList();
}

