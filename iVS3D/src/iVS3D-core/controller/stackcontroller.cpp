#include "stackcontroller.h"

StackController::StackController(OperationStack* opStack, History* mipHistory, SamplingWidget* samplingWidget, ExportController* exportController)
{
    m_opStack = opStack;
    m_history = mipHistory;
    m_samplingWidget = samplingWidget;
    m_exportController = exportController;
    connect(m_opStack, &OperationStack::sig_rowClicked, this, &StackController::slot_rowClicked);
    connect(m_opStack, &OperationStack::sig_clearClicked, this, &StackController::slot_clearClicked);
    m_opStack->addEntry("Loaded input");
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
    m_opStack->addEntry(name);
    m_algoSettings.insert(name, {index, settings});
}

void StackController::slot_keyframesChangedByPlugin(QString pluginName)
{
    deleteInvalidFuture();
    m_opStack->addEntry("Keyframes changed by plugin " + pluginName);
}

void StackController::slot_exportFinished(QMap<QString, QVariant> settings)
{
    //In case of the export, the history is updated BEFORE the stack is updated --> the m_history index has to be reduced by 1
    deleteInvalidFuture(1);
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


    m_opStack->addEntry(name);
    m_algoSettings.insert(name, {-1, settings});
}
