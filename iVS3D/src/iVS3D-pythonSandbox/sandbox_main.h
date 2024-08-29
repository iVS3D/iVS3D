#ifndef SANDBOX_H
#define SANDBOX_H

/** @defgroup SandboxPlugin SandboxPlugin
 *
 *  @ingroup
 *
 * @brief Plugin that runs python files or a live python executor.
 */

#include <QObject>
#include <QWidget>
#include <QTranslator>
#include <QCoreApplication>
#include <QProcess>
#include <iostream>

#include "ialgorithm.h"
#include "reader.h"
#include "progressable.h"
#include "signalobject.h"

/**
 * @class SandboxMain
 *
 * @ingroup SandboxPlugin
 *
 * @brief The SandboxMain class
 *
 * @author Dominic Zahn
 *
 * @date 2024/08/29
 */
class SandboxMain : public IAlgorithm {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "iVS3D.IAlgorithm") // implement interface as plugin, use the iid as identifier
    Q_INTERFACES(IAlgorithm)    // declare this as implementation of IAlgorithm interface

public:
    SandboxMain();
    ~SandboxMain();
    QWidget *getSettingsWidget(QWidget *parent) override;
    /**
     * @brief sampleImages Create an keyframe list with indices to every nth sharp image. Sharp images are specified by index in sharpImages list.
     * The algorithm reports progress to the Progressable *receiver by calling Progressable::slot_makeProgress. Setting the *stopped bool to @a true
     * exits the algorithm.
     *
     * @param imageList is a preselection of frames
     * @param receiver is a progressable, which displays the already made progress
     * @param stopped Pointer to a bool indication if user wants to stop the computation
     * @param useCuda defines if the compution should run on graphics card
     * @param logFile can be used to protocoll progress or problems
     * @return A list of indices, which represent the selected keyframes.
     */
    std::vector<uint> sampleImages(const std::vector<unsigned int> &imageList, Progressable *receiver, volatile bool *stopped, bool useCuda, LogFileParent* logFile) override;
    QString getName() const override;
    void initialize(Reader *reader, QMap<QString,QVariant> buffer, signalObject *sigObj) override;
    void setSettings(QMap<QString,QVariant> settings) override;
    QMap<QString,QVariant> generateSettings(Progressable *receiver, bool useCuda, volatile bool* stopped) override;
    QMap<QString,QVariant> getSettings() override;

private slots:

private:

};

#endif
