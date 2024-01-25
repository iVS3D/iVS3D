#include "stackcontroller.h"
#include "automaticexecsettings.h"
#include "automaticexecutor.h"

StackController::StackController(OperationStack* opStack, History* mipHistory, SamplingWidget* samplingWidget, ExportController* exportController, DataManager* dm)
{
    m_opStack = opStack;
    m_history = mipHistory;
    m_samplingWidget = samplingWidget;
    m_exportController = exportController;
    m_dm = dm;
    connect(m_opStack, &OperationStack::sig_rowClicked, this, &StackController::slot_rowClicked);
    connect(m_opStack, &OperationStack::sig_clearClicked, this, &StackController::slot_clearClicked);
    connect(m_opStack, &OperationStack::sig_editClicked, this , &StackController::slot_editClicked);
    connect(m_samplingWidget, &SamplingWidget::sig_exitEditMode, this, &StackController::slot_exitEditMode);
    connect(m_samplingWidget, &SamplingWidget::sig_startEdit, this, &StackController::slot_startEdit);
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
    int index = m_history->getCurrentIndex() - 1;
    if (index == -1) {
        m_opStack->clearSelection();
        return;
    }
    m_opStack->selectItem(index);
}

void StackController::deleteInvalidFuture()
{
    int currentIndex = m_history->getCurrentIndex() - 1;
    if (currentIndex < m_opStack->getSize() - 1) {
        m_opStack->removeItemsAfter(currentIndex);
    }
}

void StackController::slot_toggleKeyframe(uint idx, bool isNowKeyframe)
{
    deleteInvalidFuture();
    if (isNowKeyframe) {
        m_opStack->addEntry("Add keyframe " + QString::number(idx));
    }
    else {
        m_opStack->addEntry("Remove keyframe " + QString::number(idx));
    }
}

void StackController::slot_deleteKeyframes()
{
    deleteInvalidFuture();
    m_opStack->addEntry("Delete all keyframes");
}

void StackController::slot_deleteAllKeyframes()
{
    deleteInvalidFuture();
    m_opStack->addEntry("Reset all keyframes");
}

void StackController::slot_rowClicked(int row)
{   
    QString itemString = m_opStack->getItemString(row);
    if(m_algoSettings.contains(itemString)) {
        QPair<int, QMap<QString, QVariant>> algoData = m_algoSettings.value(itemString);
        //-1 == Export
        if (algoData.first == -1) {
            m_exportController->setOutputSettings(algoData.second);
        }
        //Regular sampling
        else {
            AlgorithmManager::instance().setSettings(algoData.first, algoData.second);
            m_samplingWidget->setAlgorithm(algoData.first);
        }

    }
    //State 0 in history is state 1 in the Stack (Input is not shown in the stack, but is present in the history)
    m_history->restoreState(row + 1);
}

void StackController::slot_clearClicked()
{
    m_algoSettings.clear();
    m_opStack->removeItemsAfter(0);
    m_samplingWidget->exitEditMode();
    m_history->clear();
    m_history->restoreState(0);
}

void StackController::slot_algorithmFinished(int index)
{
    deleteInvalidFuture();
    QMap<QString, QVariant> settings = AlgorithmManager::instance().getSettings(index);
    QString name;
    name = AlgorithmManager::instance().getPluginNameToIndex(index);
    if (settings.isEmpty()) {
        name.append("  -  Generated settings");
    }
    else {
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

    }
    int numImages = m_history->getCurrentNumImages();
    m_opStack->addEntry(name, "Sampled " + QString::number(numImages) + " images");
    m_algoSettings.insert(name, {index, settings});
}

void StackController::slot_keyframesChangedByPlugin(QString pluginName)
{
    deleteInvalidFuture();
    m_opStack->addEntry("Keyframes changed by plugin " + pluginName);
}

void StackController::slot_exportFinished(QMap<QString, QVariant> settings)
{
    deleteInvalidFuture();
    QString name = "Export";
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

    int numImages = m_history->getCurrentNumImages();
    m_opStack->addEntry(name, "Exported " + QString::number(numImages) + " images");
    m_opStack->addEntry(name);
    m_algoSettings.insert(name, {-1, settings});
}

void StackController::slot_editClicked(int row)
{
    m_opStack->selectItem(row);
    slot_rowClicked(row);
    if (row == m_rowInEditMode) {
        m_opStack->exitEditMode(m_rowInEditMode);
        m_samplingWidget->exitEditMode();
        m_rowInEditMode = -1;
        return;
    }
    else if (m_rowInEditMode != -1) {
        m_opStack->exitEditMode(m_rowInEditMode);
    }
    m_samplingWidget->toEditMode();
    m_rowInEditMode = row;

}

void StackController::slot_exitEditMode()
{
    if (m_rowInEditMode != -1) {
        m_opStack->exitEditMode(m_rowInEditMode);
        m_rowInEditMode = -1;
    }
}

void StackController::slot_startEdit()
{
    //m_opStack->exitEditMode(m_rowInEditMode);

    m_opStack->pendingAfter(m_rowInEditMode);
    // Restore the state BEFORE the edited plugin.
    // Index 0 in history is input, index 0 in this class is the first stackitem, therefore m_rowInEditMode - 1 is not needed to get the previous state
    m_history->restoreState(m_rowInEditMode);
    AutomaticExecSettings* execSettings = new AutomaticExecSettings();
    //Added Settings of the edited entry
    QString itemString = m_opStack->getItemString(m_rowInEditMode);
    QPair<int, QMap<QString, QVariant>> algoData = m_algoSettings.value(itemString);
    QMap<QString, QVariant> editedSettings = AlgorithmManager::instance().getSettings(algoData.first);
    QString editedPlugin = AlgorithmManager::instance().getPluginFileNameToIndex(algoData.first);
    execSettings->manualAdd({editedPlugin, editedSettings});

    //Add Settings and plugin names of item in the stack after the edited item
    for (int i = m_rowInEditMode + 1; i < m_opStack->getSize(); ++i) {
        QString itemString = m_opStack->getItemString(i);
        if(m_algoSettings.contains(itemString)) {
            QPair<int, QMap<QString, QVariant>> algoData = m_algoSettings.value(itemString);
            //-1 == Export
            if (algoData.first == -1) {
                QMap<QString, QVariant> settings = algoData.second;
                QString name = stringContainer::Export;
                execSettings->manualAdd({name, settings});
            }
            //Regular sampling
            else {
                QMap<QString, QVariant> settings = algoData.second;
                QString pluginName = AlgorithmManager::instance().getPluginFileNameToIndex(algoData.first);
                execSettings->manualAdd({pluginName, settings});
            }

        }
    }
    AutomaticExecutor* autoExec = new AutomaticExecutor(m_dm, execSettings);
    autoExec->setExportController(m_exportController);
    autoExec->slot_startAutomaticExec();
    m_rowInEditMode = -1;
}
