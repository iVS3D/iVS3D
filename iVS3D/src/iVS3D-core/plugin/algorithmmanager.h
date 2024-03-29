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
#include <QMap>

#include <QThread>

#include "../iVS3D-pluginInterface/iVS3D-pluginInterface_global.h"
#include "../iVS3D-pluginInterface/ialgorithm.h"
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

class AlgorithmManager : public QObject
{
    Q_OBJECT
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
    std::vector<uint> sample(std::vector<uint> sharpImages, Progressable* receiver, volatile bool* stopped, int idx, bool useCuda, LogFileParent *logFile);
    ///**
    // * @brief getAlgorithmList returns the loaded plugins
    // * @return QStringList with the names of the loaded plugins
    // */
    //QStringList getAlgorithmList();
    QStringList getAlgorithmNames();

    /**
     * @brief getIndexToPluginName Takes the index of a plugin and returns it's translated name.
     * @param index Index of the plugin
     * @return (possibly) translated name of the plugin
     */
    QString getPluginNameToIndex(int index);
    /**
     * @brief getPluginFileNameToIndex Takes the index of a plugin and returns it's file name.
     * @param index Index of the plugin
     * @return Name of the plugin file
     */
    QString getPluginFileNameToIndex(int index);   
    /**
     * @brief getPluginNameFromFileName Takes the name of a plugin file and returns it's translated name.
     * @param FileName nmae of the plugin file
     * @return (possibly) translated name of the plugin
     */
    QString getPluginNameFromFileName(QString FileName);
    /**
     * @brief getIndexFromFileName Takes the name of a plugin file and returns its index.
     * @param FileName nmae of the plugin file
     * @return index Index of the plugin
     */
    int getIndexFromFileName(QString FileName);
    /**
     * @brief getAlgorithmCount returns the number of loaded IAlgorithms
     * @return The number of loaded IAlgorithms
     */
    int getAlgorithmCount();
    /**
     * @brief initializePlugins calls the initialize method on all loaded IAlgorithms
     * @param reader The reader needed for the initialize method according to IAlgorithm
     */
    void initializePlugins(Reader *reader, QMap<QString, QMap<QString,QVariant>> allBuffer);
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
    QMap<QString, QVariant> generateSettings(int idx, Progressable *receiver, bool useCuda, volatile bool* stopped);
    /**
     * @brief getSettings gets the settings of the indexed plugin
     * @param idx Index of the plugin
     * @return Settings map
    */
    QMap<QString, QVariant> getSettings(int idx);



    void notifyNewMetaData();
    void notifySelectedImageIndex(uint index);
    void notifyKeyframesChanged(std::vector<uint> keyframes);
    void notifyUpdateBuffer(QString pluginName, QMap<QString, QVariant> buffer);


private slots:
    void slot_updateKeyframes(std::vector<uint> keyframes);
    void slot_updateBuffer(QMap<QString, QVariant> buffer);

signals:
    void sig_updateKeyframe(QString pluginName, std::vector<uint> keyframes);
    void sig_updateBuffer(QString pluginName, QMap<QString, QVariant> buffer);

private:
    std::vector<IAlgorithm*> m_algorithmList;
    AlgorithmManager();
    void loadPlugins();
    signalObject* m_sigObj;
    QStringList m_pluginFileNames;
};

#endif // ALGORITHMMANAGER_H
