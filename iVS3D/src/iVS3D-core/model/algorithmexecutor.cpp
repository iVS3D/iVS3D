#include "algorithmexecutor.h"

AlgorithmExecutor::AlgorithmExecutor(DataManager *dataManager)
{
    m_dataManager = dataManager;
}


int AlgorithmExecutor::startSampling(int pluginIdx, bool onlyKeyframes, bool useBounds)
{
    // gather data that is needed for sampling
    ALGO_DATA preparedData = prepareAlgoStart(pluginIdx, onlyKeyframes, useBounds);

    if (preparedData.images.size() == 0) {
       slot_abort();
       return -1;
    }

    m_sampleThread = new SampleThread(this, preparedData.mip->getReader(), preparedData.images, &m_stopped, m_pluginIndex, preparedData.bufferData, preparedData.useCuda);
    //Use a direct Connection when no ui is used
    if (qApp->property(stringContainer::UIIdentifier).toBool()) {
        QObject::connect(m_sampleThread, &QThread::finished, this, &AlgorithmExecutor::slot_pluginFinished);
    }
    else {
        QObject::connect(m_sampleThread, &QThread::finished, this, &AlgorithmExecutor::slot_pluginFinished, Qt::DirectConnection);
    }
    m_sampleThread->start();
    m_currentThread = m_sampleThread;
    return 0;
}

int AlgorithmExecutor::startGenerateSettings(int pluginIdx, bool onlyKeyframes, bool useBounds)
{
    // gather data that is needed for generating the settings
    ALGO_DATA preparedData = prepareAlgoStart(pluginIdx, onlyKeyframes, useBounds);
    m_settingsThread = new SettingsThread(this, preparedData.mip->getReader(), preparedData.images, &m_stopped, m_pluginIndex, preparedData.bufferData, preparedData.useCuda);

    //Use a direct Connection when no ui is used
    if (qApp->property(stringContainer::UIIdentifier).toBool()) {
        QObject::connect(m_settingsThread, &QThread::finished, this, &AlgorithmExecutor::slot_pluginFinished);
    }
    else {
        QObject::connect(m_settingsThread, &QThread::finished, this, &AlgorithmExecutor::slot_pluginFinished, Qt::DirectConnection);
    }
    m_settingsThread->start();
    m_currentThread = m_settingsThread;
    return 0;
}

void AlgorithmExecutor::slot_abort()
{
    if (!m_currentThread) {
        emit sig_algorithmAborted();
        return;
    }
    disconnect(m_currentThread, &QThread::finished, this, &AlgorithmExecutor::slot_pluginFinished);
    m_stopped = true;
    m_currentThread->wait();
    delete m_currentThread;
    m_currentThread = nullptr;
    emit sig_algorithmAborted();
}



void AlgorithmExecutor::slot_pluginFinished()
{
    if (m_currentThread == m_sampleThread) {
        // if sampling finished set new parameters in DataManager
        std::vector<uint> keyframes = m_sampleThread->getOutput();
        QString message = AlgorithmManager::instance().getPluginNameToIndex(m_pluginIndex) + " extracted " + QString::number(keyframes.size()) + " images";
        slot_displayMessage(message);
        m_dataManager->getModelInputPictures()->updateMIP(keyframes);
        QString bufferName = AlgorithmManager::instance().getBufferName(m_pluginIndex);
        QString pluginName = AlgorithmManager::instance().getPluginNameToIndex(m_pluginIndex);
        QVariant buffer = m_sampleThread->getBuffer();
        m_dataManager->getModelAlgorithm()->addPluginBuffer(pluginName, bufferName, buffer);
    } else if (m_currentThread == m_settingsThread) {
        // if generateSettings finished
        QMap<QString, QVariant> generatedSettings = m_settingsThread->getOutput();
        QString pluginName = AlgorithmManager::instance().getPluginNameToIndex(m_pluginIndex);
        QString message("Generated settings for " + pluginName + ": ");
        slot_displayMessage(message);
        QMapIterator<QString, QVariant> iter(generatedSettings);
        while(iter.hasNext()) {
           iter.next();
           QString setting = "     " + iter.key() + " = " + iter.value().toString();
           slot_displayMessage(setting);
        }


        AlgorithmManager::instance().setSettings(m_pluginIndex, generatedSettings);
    }

    emit sig_pluginFinished();
}

ALGO_DATA AlgorithmExecutor::prepareAlgoStart(int pluginIdx, bool onlyKeyframes, bool useBounds)
{
    ALGO_DATA preparedData;
    m_pluginIndex = pluginIdx;
    m_stopped = false;

    // Read mip
    preparedData.mip = m_dataManager->getModelInputPictures();

    // Select images based on onlyKeframes & useBounds
    if (useBounds) {
        //Bounds are used
        QPoint boundaries = preparedData.mip->getBoundaries();
        if(onlyKeyframes){
            //All keyframes inbound
            preparedData.images = preparedData.mip->getAllKeyframes(boundaries);
        } else {
            //All images inbound
            preparedData.images.reserve(preparedData.mip->getPicCount());
            for(int i = boundaries.x(); i <= boundaries.y(); i++){
                preparedData.images.push_back(i);
            }
        }
    }
    else {
        //Bounds aren't used
        if(onlyKeyframes) {
          preparedData.images = preparedData.mip->getAllKeyframes();
        }
        else {
            preparedData.images.reserve(preparedData.mip->getPicCount());
            for(unsigned int i = 0; i < preparedData.mip->getPicCount(); i++){
                preparedData.images.push_back(i);
            }
        }
    }

    // Read bufferData
    QString pluginName = AlgorithmManager::instance().getPluginNameToIndex(m_pluginIndex);
    QMap<QString, QMap<QString, QVariant>> buffer = m_dataManager->getModelAlgorithm()->getPluginBuffer();

    QMapIterator<QString, QMap<QString, QVariant>> mapIt(buffer);
    while(mapIt.hasNext()) {
        mapIt.next();
        if (mapIt.key().compare(pluginName) == 0) {
            preparedData.bufferData = mapIt.value();
            break;
        }
    }

    // Read useCuda
    preparedData.useCuda = ApplicationSettings::instance().getUseCuda();
    return preparedData;
}


