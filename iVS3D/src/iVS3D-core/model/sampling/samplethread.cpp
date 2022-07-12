#include "samplethread.h"

SampleThread::SampleThread(Progressable *receiver, const std::vector<uint> &images, volatile bool *stopped, int pluginIdx, bool useCuda)
    : AlgorithmThread<std::vector<uint>>(receiver, images, stopped, pluginIdx, useCuda)
{
}

void SampleThread::run()
{
    m_output = AlgorithmManager::instance().sample(m_images, m_receiver, m_stopped, m_pluginIdx, m_useCuda, m_logFile);
    m_logFile->setResultsInfo(m_output);
    m_result = 0;
}
