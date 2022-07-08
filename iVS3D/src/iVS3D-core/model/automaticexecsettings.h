#ifndef AUTOMATICEXECSETTINGS_H
#define AUTOMATICEXECSETTINGS_H

#include <QObject>
#include <QFileDialog>
#include <QJsonObject>
#include <QFile>
#include <QJsonDocument>

#include "controller/exportcontroller.h"
#include "plugin/algorithmmanager.h"



/**
 * @class AutomaticExecSettings
 *
 * @ingroup Model
 *
 * @brief The AutomaticExecSettings class is used to track the settings for the automatic execution
 *
 * @author Daniel Brommer
 *
 * @date 2021/06/14
 */

class AutomaticExecSettings : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief AutomaticExecSettings Constructor which creates neede connects
     * @param autoWidget Pointer to the AutomaticWidget
     * @param samplingWidget Pointer to the SamplingWidget
     * @param outputWidget Pointer to the OutputWidget
     */
    AutomaticExecSettings();
    /**
     * @brief getPluginList Returns the current automatic settings
     * @return QList with QPairs. First in the pair is name of the algorithm, second is a QMap with its settings
     */
    QList<QPair<QString, QMap<QString, QVariant>>> getPluginList();
    /**
     * @brief getPluginNames Returns QstringList with name of the alogrithm and its settings
     * @return QStringList with name + settings
     */
    QStringList getPluginNames();
    /**
     * @brief setExportController ExportController doesn't exists when this class is created, so it has to be set later
     * @param exportController Pointer to the ExportController
     */
    void setExportController(ExportController* exportController);
    /**
     * @brief loadPluginList Actually loads the settings from the given file
     * @param path Path to the JSON file
     */
    void loadPluginList(QString path);

public slots:
    /**
     * @brief slot_addAuto Connected to the SamplingWidget, used to add a plugin
     * @param idx Index of the selected plugin
     * @param generate Boolean wether settings should be generated or not
     */
    void slot_addAuto(int idx, bool generate);
    /**
     * @brief slot_addAutoOutput Connected to the OutputWidget, used to add a export
     */
    void slot_addAutoOutput();
    /**
     * @brief slot_removedPlugin Connected to the AutomaticWidget, called when the user removes an algorithm
     * @param row Index of the removed algorithm
     */
    void slot_removedPlugin(int row);
    /**
     * @brief slot_saveConfiguration Connected to the AutomaticController, called to save the current settings to a JSON file
     */
    void slot_saveConfiguration(QString path);
    /**
     * @brief slot_loadConfiguration Connected to the AutomaticController, called to load a existing JSON file
     */
    void slot_loadConfiguration(QString path);

    /**
     * @brief slot_doubleClickedItem Connected to the AutomaticWidget, called when a algorithm is double clicked to show its settings on the ui
     * @param row Index of the clicked algorithm
     */
    void slot_doubleClickedItem(int row);
    /**
     * @brief slot_autoOrderChanged Connected to the AutomaticWidget, called when algorithms are moved with the drag&drop feature
     * @param first first element to be swapped
     * @param second second element to be swapped
     */
    void slot_autoOrderChanged(int first, int second);

signals:
    /**
     * @brief sig_showExportSettings Connected to ExportController, used to set settings in the OutpuWidget
     * @param exportSettings Settings of the export
     */
    void sig_showExportSettings(QMap<QString, QVariant> exportSettings);

    void sig_setAlgorithm(int index);

    void sig_updatedSelectedPlugins(QStringList usedPlugins);


private:
    //algoList maps the plugin name with its selected settings
    QList<QPair<QString, QMap<QString, QVariant>>> m_algoList;

    ExportController* m_exportController = nullptr;

    void updateShownAlgoList();

};

#endif // AUTOMATICEXECSETTINGS_H
