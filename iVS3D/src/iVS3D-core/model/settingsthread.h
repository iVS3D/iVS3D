#ifndef SETTINGSTHREAD_H
#define SETTINGSTHREAD_H

#include "algorithmthread.h"

/**
 * @class SettingsThread
 *
 * @ingroup model
 *
 * @brief The SettingsThread class is a derivived class of Algorithmthread and is used to execute the generateSettings computation
 *        of a plugin in a seperate thread.
 *
 * @author Dominic Zahn
 *
 * @date 2021/08/17
 */
class SettingsThread : public AlgorithmThread<QMap<QString, QVariant>>
{
public:
    /**
     * @brief SettingsThread Constructor which takes all parameters used by IAlgorithm
     * @param parameters according to IAlgorithm
     */
    SettingsThread(Progressable *receiver, Reader *reader, const std::vector<uint> &images, volatile bool *stopped, int pluginIdx, QMap<QString, QVariant> buffer, bool useCuda);
protected:
    /**
     * @brief QThread mehtod which executes the generateSettings method of the specified plugin
     */
    void run() override;
};

#endif // SETTINGSTHREAD_H
