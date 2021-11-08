#include "settingsthread.h"

SettingsThread::SettingsThread(Progressable *receiver, Reader *reader, const std::vector<uint> &images, volatile bool *stopped, int pluginIdx, QMap<QString, QVariant> buffer, bool useCuda)
   : AlgorithmThread<QMap<QString, QVariant>>(receiver, reader, images, stopped, pluginIdx, buffer, useCuda)
{
}

void SettingsThread::run()
{
    m_output = AlgorithmManager::instance().generateSettings(m_pluginIdx, m_receiver, m_buffer, m_useCuda, m_stopped);
    m_result = 0;
}
