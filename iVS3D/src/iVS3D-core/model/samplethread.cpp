#include "samplethread.h"

SampleThread::SampleThread(Progressable *receiver, Reader *reader, const std::vector<uint> &images, volatile bool *stopped, int pluginIdx, QMap<QString, QVariant> buffer, bool useCuda)
    : AlgorithmThread<std::vector<uint>>(receiver, reader, images, stopped, pluginIdx, buffer, useCuda)
{
}

void SampleThread::run()
{
    m_output = AlgorithmManager::instance().sample(m_reader, m_images, m_receiver, m_stopped, m_pluginIdx, m_buffer, m_useCuda, m_logFile);
    m_logFile->setResultsInfo(m_output);
    m_result = 0;
}
