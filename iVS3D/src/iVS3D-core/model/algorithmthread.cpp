#include "algorithmthread.h"

template<typename outType>
AlgorithmThread<outType>::AlgorithmThread(Progressable *receiver, Reader *reader, const std::vector<uint> images, volatile bool *stopped, int pluginIdx, QMap<QString, QVariant> buffer, bool useCuda)
    : m_receiver(receiver), m_pluginIdx(pluginIdx) , m_reader(reader), m_images(images), m_stopped(stopped), m_buffer(buffer), m_useCuda(useCuda)
{
    m_logFile = LogManager::instance().createLogFile(AlgorithmManager::instance().getPluginNameToIndex(m_pluginIdx), true);
    m_logFile->setSettings(AlgorithmManager::instance().getSettings(m_pluginIdx));
    m_logFile->setInputInfo(m_images);
}
template AlgorithmThread<QMap<QString, QVariant>>::AlgorithmThread(Progressable *receiver, Reader *reader, const std::vector<uint> images, volatile bool *stopped, int pluginIdx, QMap<QString, QVariant> buffer, bool useCuda);
template AlgorithmThread<std::vector<uint>>::AlgorithmThread(Progressable *receiver, Reader *reader, const std::vector<uint> images, volatile bool *stopped, int pluginIdx, QMap<QString, QVariant> buffer, bool useCuda);

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

template<typename outType>
QVariant AlgorithmThread<outType>::getBuffer()
{
    return AlgorithmManager::instance().getBuffer(m_pluginIdx);
}
template QVariant AlgorithmThread<QMap<QString, QVariant>>::getBuffer();
template QVariant AlgorithmThread<std::vector<uint>>::getBuffer();

template<typename outType>
QString AlgorithmThread<outType>::getBufferName()
{
    return AlgorithmManager::instance().getBufferName(m_pluginIdx);
}
template QString AlgorithmThread<QMap<QString, QVariant>>::getBufferName();
template QString AlgorithmThread<std::vector<uint>>::getBufferName();
