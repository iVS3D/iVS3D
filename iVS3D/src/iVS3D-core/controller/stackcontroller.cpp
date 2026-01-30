#include "stackcontroller.h"
#include "pluginmanager.h"

StackController::StackController(OperationStack* opStack, History* mipHistory, SamplingWidget* samplingWidget, ExportController* exportController)
{
    m_opStack = opStack;
    m_history = mipHistory;
    m_samplingWidget = samplingWidget;
    m_exportController = exportController;
    connect(m_opStack, &OperationStack::sig_rowClicked, this, &StackController::slot_rowClicked);
    connect(m_opStack, &OperationStack::sig_clearClicked, this, &StackController::slot_clearClicked);
    m_opStack->addEntry(tr("Loaded input"));
}

StackController::~StackController()
{
    m_opStack->clear();
    m_algoSettings.clear();
    disconnect(m_opStack, &OperationStack::sig_rowClicked, this, &StackController::slot_rowClicked);
    disconnect(m_opStack, &OperationStack::sig_clearClicked, this, &StackController::slot_clearClicked);
}

void StackController::select()
{
    m_opStack->selectItem(m_history->getCurrentIndex());
}

void StackController::deleteInvalidFuture(int exportFlag)
{
    int currentIndex = m_history->getCurrentIndex() - exportFlag;
    if (currentIndex < m_opStack->getSize() - 1) {
        //Dont delete the first element (Loaded input)
        if (currentIndex == 0) {
            m_opStack->removeItemsAfter(currentIndex + 1);
            return;
        }
        m_opStack->removeItemsAfter(currentIndex + 1);
    }
}

void StackController::slot_toggleKeyframe(uint idx, bool isNowKeyframe)
{
    deleteInvalidFuture();
    if (isNowKeyframe) {
        m_opStack->addEntry(QString(tr("Add image %1")).arg(QString::number(idx)));
    }
    else {
        m_opStack->addEntry(QString(tr("Remove image %1")).arg(QString::number(idx)));
    }
}

void StackController::slot_deleteKeyframes()
{
    deleteInvalidFuture();
    m_opStack->addEntry(tr("Reset selection"));
}

void StackController::slot_deleteAllKeyframes()
{
    deleteInvalidFuture();
    m_opStack->addEntry(tr("Reset selection"));
}

void StackController::slot_rowClicked(int row)
{   
    QString itemString = m_opStack->getItemString(row);
    if(m_algoSettings.contains(itemString)) {
        auto algoData = m_algoSettings.value(itemString);
        //-1 == Export
        if (algoData.pluginName == "Export") {
            m_exportController->setOutputSettings(algoData.pluginSettings);
        }
        //Regular sampling
        else if (PluginManager::instance().getPluginByName(algoData.pluginName).has_value()) {
            auto pluginHandle = PluginManager::instance().getPluginByName(algoData.pluginName).value();
            auto result = pluginHandle.base->applySettings(algoData.pluginSettings);
            assert(result.has_value()); // should not fail

            m_samplingWidget->setSelectedPlugin(algoData.pluginName);
            auto settingsResult = pluginHandle.base->getSettingsWidget(m_samplingWidget);
            assert(settingsResult.has_value()); // should not fail
            m_samplingWidget->showPluginSettings(settingsResult.value());
        }

    }
    m_history->restoreState(row);
}

void StackController::slot_clearClicked()
{
    m_algoSettings.clear();
    m_opStack->removeItemsAfter(1);
    m_history->clear();
    slot_rowClicked(0);
    select();
}

void StackController::addToStack(const PluginHandle& plugin)
{
    deleteInvalidFuture();
    QMap<QString, QVariant> settings = plugin.base->getSettings();
    QString uiText = plugin.name();

    uiText.append(" - ");
        QMapIterator<QString, QVariant> iter(settings);
        while(iter.hasNext()) {
           iter.next();
           QString identifier = iter.key() + " = " + iter.value().toString() + "; ";
           if (iter.value().toString() == "") {
               continue;
           }
           uiText.append(identifier);
        }
        uiText.chop(2);
    m_opStack->addEntry(uiText);
    m_algoSettings.insert(uiText, {plugin.name(), settings});
}

void StackController::slot_pluginFinished(QString name)
{
    deleteInvalidFuture();
    auto pluginResult = PluginManager::instance().getPluginByName(name);
    assert(pluginResult.has_value()); // should not fail
    auto pluginHandle = pluginResult.value();
    QMap<QString, QVariant> settings = pluginHandle.base->getSettings();
    QString uiText = name;

    uiText.append(" - ");
        QMapIterator<QString, QVariant> iter(settings);
        while(iter.hasNext()) {
           iter.next();
           QString identifier = iter.key() + " = " + iter.value().toString() + "; ";
           if (iter.value().toString() == "") {
               continue;
           }
           uiText.append(identifier);
        }
        uiText.chop(2);
    m_opStack->addEntry(uiText);
    m_algoSettings.insert(uiText, {name, settings});
}

void StackController::slot_keyframesChangedByPlugin(QString pluginName)
{
    deleteInvalidFuture();
    m_opStack->addEntry(QString(tr("Image selection changed by plugin %1")).arg(pluginName));
}

void StackController::slot_exportFinished(QMap<QString, QVariant> settings)
{
    if (settings.empty()) return; // nothing to display

    //In case of the export, the history is updated BEFORE the stack is updated --> the m_history index has to be reduced by 1
    deleteInvalidFuture(1);
    QString name = tr("Export");
    name.append(" - ");
    QMapIterator<QString, QVariant> iter(settings);
    while(iter.hasNext()) {
        iter.next();
        QString identifier = iter.key() + " = " + iter.value().toString() + "; ";
        if (iter.value().toString() == "") {
            continue;
        }
        name.append(identifier);
    }
    name.chop(2);

    m_opStack->addEntry(name);
    m_algoSettings.insert(name, {"Export", settings});
}
