#ifndef ALGORITHMMANAGER_H
#define ALGORITHMMANAGER_H

/** @defgroup Plugin Plugin
 *
 * @ingroup Plugin
 *
 * @brief Manages IAlgorithm and ITransform plugins.
 */

#include <QDir>
#include <QCoreApplication>
#include <QPluginLoader>
#include <QDebug>

#include <QThread>

#include "ialgorithm.h"
#include "signalobject.h"

/**
 * @class AlgorithmManager
 *
 * @ingroup Plugin
 *
 * @brief The AlgorithmManager class is a singleton which is responisble for loading and executing the IAlgorithm plugins.
 *
 * @author Patrick Binder
 *
 * @date 2021/02/15
 */

class AlgorithmManager
{
public:
    /**
     * @brief instance singleton method
     * @return The one istance of this class
     */
    static AlgorithmManager &instance();
    /**
     * @brief getSettingsWidget Gets the Settings widget from the algorithm with the given index
     * @param parent The parent Widget
     * @param idx Index of the Algorithm
     * @return Settings widget of the indexed algorithm
     */
    QWidget* getSettingsWidget(QWidget* parent, uint idx);
    /**
     * @brief sample Executes the indexed algorithm
     * @param Parameters according to the IAlgorithm interface
     * @param idx Index of the Algorithm
     * @return Keyframe list calculated by the plugin
     */
    std::vector<uint> sample(Reader* images, std::vector<uint> sharpImages, Progressable* receiver, volatile bool* stopped, int idx, QMap<QString, QVariant> buffer, bool useCuda, LogFileParent *logFile);
    /**
     * @brief getAlgorithmList returns the loaded plugins
     * @return QStringList with the names of the loaded plugins
     */
    QStringList getAlgorithmList();
    /**
     * @brief getIndexToPluginName Takes the index of a plugin and returns it's name.
     * @param index Index of the plugin
     * @return Name of the plugin
     */
    QString getPluginNameToIndex(int index);
    /**
     * @brief getBuffer calls the getBuffer method on the indexed IAlgorithm
     * @param index Index of the plugin
     * @return QVariant with the buffer
     */
    QVariant getBuffer(int idx);
    /**
     * @brief getBufferName calls the getBufferName method on the indexed IAlgorithm
     * @param index Index of the plugin
     * @return QString with the buffer name
     */
    QString getBufferName(int idx);
    /**
     * @brief getAlgorithmCount returns the number of loaded IAlgorithms
     * @return The number of loaded IAlgorithms
     */
    int getAlgorithmCount();
    /**
     * @brief initializePlugins calls the initialize method on all loaded IAlgorithms
     * @param reader The reader needed for the initialize method according to IAlgorithm
     */
    void initializePlugins(Reader *reader);
    /**
     * @brief setSettings sets the settings of the indexed plugin
     * @param idx Index of the plugin
     * @param settings Settings map
    */
    void setSettings(int idx, QMap<QString, QVariant> settings);
    /**
     * @brief generateSettings returns the generated settings of the indexed plugin
     * @param idx Index of the plugin
     * @param receiver is a progressable, which displays the already made progress
     * @param buffer QVariant with the buffered data form last call to sampleImages
     * @param useCuda @a true if cv::cuda can be used
     * @param stopped Bool to indicated user stopped the generation of settings
    */
    QMap<QString, QVariant> generateSettings(int idx, Progressable *receiver, QMap<QString, QVariant> buffer, bool useCuda, volatile bool* stopped);
    /**
     * @brief getSettings gets the settings of the indexed plugin
     * @param idx Index of the plugin
     * @return Settings map
    */
    QMap<QString, QVariant> getSettings(int idx);

    IAlgorithm *getAlgo(int idx);

    void sigNewMetaData();

private:
    std::vector<IAlgorithm*> m_algorithmList;
    AlgorithmManager();
    void loadPlugins();
    signalObject* m_sigObj;
};

#endif // ALGORITHMMANAGER_H
