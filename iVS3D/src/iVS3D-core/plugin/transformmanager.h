#ifndef TRANSFORMMANAGER_H
#define TRANSFORMMANAGER_H

#include "itransform.h"
#include "itransformrequestdequeue.h"
#include "stringcontainer.h"
#include <QWidget>


#include <QObject>
#include <QDir>
#include <QCoreApplication>
#include <QPluginLoader>


/**
 * @interface TransformManager
 *
 * @ingroup Plugin
 *
 * @brief The TransformManager class manages transformation-plugins and provides them to the core application
 *
 * @author Dominik Wüst
 *
 * @date 2021/04/12
 */
class TransformManager : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief instance returns the singelton of this class
     * @return transformmanager instance
     */
    static TransformManager& instance();

    /**
     * @brief getSettingsWidget returns the settings-widget for the settings configuration
     * @param parent parentwidget
     * @param idx id of the plugin
     * @return widget with elements to configure all necessary settings of this plugin
     */
    QWidget *getSettingsWidget(QWidget* parent, uint idx);

    /**
     * @brief getTransformList gets a list of names all availabe transformation-plugins
     * @return stringlist with plugin names
     */
    QStringList getTransformList();

    /**
     * @brief getTransformCount gets the amount of available transformation-plugins
     * @return numer of list elements
     */
    int getTransformCount();

    /**
     * @brief getTransform gets a desyncroniced version of the plugin
     * @param id id of the plugin
     * @return plugin
     */
    ITransformRequestDequeue *getTransform(uint id);

    /**
     * @brief selectTransform signals that the selected plugin has changed
     * @param id id of the new plugin
     */
    void selectTransform(uint id);

    /**
     * @brief setTransformationEnabled signals that the user wants to activate the transformation preview on the gui
     * this has to be activated extra because calculating the transformaiton is usually expensive
     * @param enabled @a true if activating, @a false if deactivating
     */
    void setTransformationEnabled(bool enabled);

    /**
     * @brief isTransformEnabled returns if the transformaiton is currently active
     * @return @a true if active, @a false if deactive
     */
    bool isTransformEnabled();

    /**
     * @brief exit ends excecution of the thread that is excecuting the plugin and frees the plugins resources
     */
    void exit();

    /**
     * @brief enableCuda enables or disables the usage of CUDA for the transformation plugins
     * @param enabled @a true if CUDA should be used, @a false if not
     */
    void enableCuda(bool enabled);

    /**
     * @brief setter for plugin's settings
     * @param QMap with the settings
     * @param id id of the plugin
     */
    void setSettings(QMap<QString, QVariant> settings, uint idx);

    /**
     * @brief getter for plugin's settings
     * @param id id of the plugin
     * @return QMap with the settings
     */
    QMap<QString, QVariant> getSettings(uint idx);

signals:
    void sig_selectedTransformChanged(uint id);
    void sig_transformEnabledChanged(bool enabled);

private:
    /**
     * Contains and owns the transform plugin as well as the associated QPluginLoader.
     * This structure is used to manage the lifecycle of the plugin and uses its loader 
     * to free resources when the plugin is no longer needed.
     */
    struct TransformPlugin {
        ITransformRequestDequeue *transform;
        QPluginLoader *loader;
    };

    TransformManager();
    std::vector<TransformPlugin> m_transformList;
    void loadPlugins();
    void cleanup();
    bool m_transformationEnabled;
    QThread *m_transformThread;
};

#endif // TRANSFORMMANAGER_H
