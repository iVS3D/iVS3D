#ifndef APPLICATIONSETTINGS_H
#define APPLICATIONSETTINGS_H

#include "stringcontainer.h"
#include <QObject>
#include <QMap>
#include <QVariant>
#include <QSettings>

#include "opencv2/core.hpp"
#ifdef WITH_CUDA
#include "opencv2/core/cuda.hpp"
#endif

/**
 * @class ApplicationSettings
 *
 * @ingroup Model
 *
 * @brief The ApplicationSettings class is a singleton which saves all settings for iVS3D. These are: Paths to the reconstruction softwares,
 * the standard input path, the plugin path, layout style and if dark style is active
 *
 * @author Daniel Brommer
 *
 * @date 2021/01/30
 */

class ApplicationSettings
{
public:  
    /**
     * @brief Returns the singleton instance of this class
     *
     * @return Instance of this class
     */
    static ApplicationSettings& instance();
    /**
     * @brief Saves an reconstruction software
     *
     * @param name Name of the reconstruction software
     * @param path Path of the reconstruction software
     */
    void addReconstructPath(QString name, QString path);
    /**
     * @brief Removes the given reconstruction software
     *
     * @param name Name of the reconstruction software to be removed
     */
    bool removeReconstructPath(QString name);
    /**
     * @brief Saves the standard input path
     *
     * @param standardInput Path of the standard input
     */
    void setStandardInputPath(const QString &standardInput);
    /**
     * @brief Sets the darkstyle
     *
     * @param dark @a true will toggle darkstyle
     */
    void setDarkStyle(bool dark);
    /**
     * @brief setUseCuda Sets use cuda boolean
     * @param useCuda @a true if cuda is used, @a false otherwise
     */
    void setUseCuda(bool useCuda);
    /**
     * @brief setCreateLogs Sets createLogs
     * @param createLogs @a true if log files should be created, @a false if no log files should be created
     */
    void setCreateLogs(bool createLogs);

    /**
     * @brief Returns all saved reconstruction softwares
     *
     * @return QMap with the name of the reconstruction software as key and it's path as value
     */
    QMap<QString, QString> getReconstructPath();
    /**
     * @brief Returns the standard input path
     *
     * @return QString with the current input path
     */
    QString getStandardInputPath();
    /**
     * @brief Returns which darkstyle is saved in the settings file
     *
     * @return @a true if darkstyle is saved
     */
    bool getDarkStyle();
    /**
     * @brief Returns if darksytle is currently toggled
     *
     * @return @a true if darkstyle is active
     */
    bool getActiveStyle();
    /**
     * @brief Returns if use CUDA is enabled
     * @return @a true if cuda is enabled
     */
    bool getUseCuda();
    /**
     * @brief Returns if the opencv backend supports CUDA
     * @return @a true if cuda is supported
     */
    bool getCudaAvailable() const;
    /**
     * @brief Returns if create Logs is enabled
     * @return @a true if logging is enabled and log files will be created
     */
    bool getCreateLogs();

private:
    ApplicationSettings();
    QMap<QString, QString> m_reconstructPath;
    QString m_standardInputPath;
    bool m_darkStyle;
    bool m_activeStyle;
    bool m_useCuda;
    bool m_createLogs;
    void loadSettings();
    void saveSettings();
};



#endif // APPLICATIONSETTINGS_H
