#include "algorithmexecutor.h"

AlgorithmExecutor::AlgorithmExecutor(ModelInputPictures *mip)
{
    m_mip = mip;
}


int AlgorithmExecutor::startSampling(int pluginIdx)
{
    // gather data that is needed for sampling
    ALGO_DATA preparedData = prepareAlgoStart(pluginIdx);

    if (preparedData.images.size() == 0) {
       slot_abort();
       return -1;
    }

    m_sampleThread = new SampleThread(this, preparedData.images, &m_stopped, m_pluginIndex, preparedData.useCuda);
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

int AlgorithmExecutor::startGenerateSettings(int pluginIdx)
{
    // gather data that is needed for generating the settings
    ALGO_DATA preparedData = prepareAlgoStart(pluginIdx);
    m_settingsThread = new SettingsThread(this, preparedData.images, &m_stopped, m_pluginIndex, preparedData.useCuda);

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
        m_mip->updateMIP(keyframes);

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

    emit sig_pluginFinished(m_pluginIndex);
}

ALGO_DATA AlgorithmExecutor::prepareAlgoStart(int pluginIdx)
{
    ALGO_DATA preparedData;
    m_pluginIndex = pluginIdx;
    m_stopped = false;

    // Read mip
    preparedData.mip = m_mip;

    // select images
    QPoint boundaries = preparedData.mip->getBoundaries();
    preparedData.images = preparedData.mip->getAllKeyframes(boundaries);
    if(preparedData.images.size() == 0){
        // no images selected jet, select all within boundaries
        preparedData.images.reserve(boundaries.manhattanLength());
        for(int i = boundaries.x(); i<= boundaries.y(); ++i){
            preparedData.images.push_back(i);
        }
    }

    // Read useCuda
    preparedData.useCuda = ApplicationSettings::instance().getUseCuda();
    return preparedData;
}


