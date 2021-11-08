#ifndef SAMPLETHREAD_H
#define SAMPLETHREAD_H

#include "algorithmthread.h"

/**
 * @class SampleThread
 *
 * @ingroup model
 *
 * @brief The SettingsThread class is a derivived class of Algorithmthread and is used to execute the sampling computation
 *        of a plugin in a seperate thread.
 *
 * @author Dominic Zahn
 *
 * @date 2021/08/17
 */
class SampleThread : public AlgorithmThread<std::vector<uint>>
{
public:
    /**
     * @brief SampleThread Constructor which takes all parameters used by IAlgorithm
     * @param parameters according to IAlgorithm
     */
    SampleThread(Progressable *receiver, Reader *reader, const std::vector<uint> &images, volatile bool *stopped, int pluginIdx, QMap<QString, QVariant> buffer, bool useCuda);
protected:
    /**
     * @brief QThread mehtod which executes the sampling method of the specified plugin
     */
    void run() override;
};

#endif // SAMPLETHREAD_H
