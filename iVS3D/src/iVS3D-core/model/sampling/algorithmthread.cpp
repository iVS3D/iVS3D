#include "algorithmthread.h"

template<typename outType>
AlgorithmThread<outType>::AlgorithmThread(Progressable *receiver, const std::vector<uint> images, volatile bool *stopped, int pluginIdx, bool useCuda)
    : m_receiver(receiver), m_pluginIdx(pluginIdx) , m_images(images), m_stopped(stopped), m_useCuda(useCuda)
{
    m_logFile = LogManager::instance().createLogFile(AlgorithmManager::instance().getPluginFileNameToIndex(m_pluginIdx), true);
    m_logFile->setSettings(AlgorithmManager::instance().getSettings(m_pluginIdx));
    m_logFile->setInputInfo(m_images);
}
template AlgorithmThread<QMap<QString, QVariant>>::AlgorithmThread(Progressable *receiver, const std::vector<uint> images, volatile bool *stopped, int pluginIdx, bool useCuda);
template AlgorithmThread<std::vector<uint>>::AlgorithmThread(Progressable *receiver, const std::vector<uint> images, volatile bool *stopped, int pluginIdx, bool useCuda);

template<typename outType>
outType AlgorithmThread<outType>::getOutput()
{
    return m_output;
}
template std::vector<uint> AlgorithmThread<std::vector<uint>>::getOutput();
template QMap<QString, QVariant> AlgorithmThread<QMap<QString, QVariant>>::getOutput();

template<typename outType>
int AlgorithmThread<outType>::getResult()
{
    return m_result;
}
template int AlgorithmThread<QMap<QString, QVariant>>::getResult();
template int AlgorithmThread<std::vector<uint>>::getResult();
