#include "stackcontroller.h"

StackController::StackController(OperationStack* opStack, History* mipHistory, SamplingWidget* samplingWidget)
{
    m_opStack = opStack;
    m_history = mipHistory;
    m_samplingWidget = samplingWidget;
    connect(m_opStack, &OperationStack::sig_rowClicked, this, &StackController::slot_rowClicked);
    m_opStack->addEntry("Loaded input");
}

void StackController::select()
{
    m_opStack->selectItem(m_history->getCurrentIndex());
}

void StackController::deleteInvalidFuture()
{
    if (m_history->getCurrentIndex() < m_opStack->getSize() - 1) {
        //Dont delete the first element (Loaded input)
        if (m_history->getCurrentIndex() == 0) {
            m_opStack->removeItemsAfter(m_history->getCurrentIndex() + 1);
            return;
        }
        m_opStack->removeItemsAfter(m_history->getCurrentIndex());
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
    m_history->restoreState(row);
    QString itemString = m_opStack->getItemString(row);
    if(m_algoSettings.contains(itemString)) {
        QPair<int, QMap<QString, QVariant>> algoData = m_algoSettings.value(itemString);
        AlgorithmManager::instance().setSettings(algoData.first, algoData.second);
        m_samplingWidget->setAlgorithm(algoData.first);
    }
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
