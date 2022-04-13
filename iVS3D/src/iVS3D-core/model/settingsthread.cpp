#include "settingsthread.h"

SettingsThread::SettingsThread(Progressable *receiver, const std::vector<uint> &images, volatile bool *stopped, int pluginIdx, bool useCuda)
   : AlgorithmThread<QMap<QString, QVariant>>(receiver, images, stopped, pluginIdx, useCuda)
{
}

void SettingsThread::run()
{
    m_output = AlgorithmManager::instance().generateSettings(m_pluginIdx, m_receiver, m_useCuda, m_stopped);
    m_result = 0;
}
