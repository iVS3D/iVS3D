#ifndef APPLICATIONSETTINGS_H
#define APPLICATIONSETTINGS_H

#include <QDebug>
#include <QLocale>
#include <QMap>
#include <QObject>
#include <QSettings>
#include <QVariant>

#include "cvmat_qmetadata.h"
/**
 * @class ApplicationSettings
 *
 * @ingroup Model
 *
 * @brief The ApplicationSettings class is a singleton which saves all settings
 * for iVS3D. These are: Paths to the reconstruction softwares, the standard
 * input path, the plugin path, layout style and if dark style is active
 *
 * @author Daniel Brommer
 *
 * @date 2021/01/30
 */

class ApplicationSettings {
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
    void setStandardInputPath(const QString& standardInput);
    /**
     * @brief Disable sanity checks for ots product configuration.
     *
     * @param disableChecks @a true will disbale checks and allow for
     * (potentially) faulty configurations. Only use if you know what you are
     * doing!
     */
    void setDisableChecks(bool disableChecks);
    /**
     * @brief Sets the darkstyle
     *
     * @param dark @a true will toggle darkstyle
     */
    void setColorTheme(ColorTheme theme);
    /**
     * @brief setUseCuda Sets use cuda boolean
     * @param useCuda @a true if cuda is used, @a false otherwise
     */
    void setUseCuda(bool useCuda);
    /**
     * @brief setCreateLogs Sets createLogs
     * @param createLogs @a true if log files should be created, @a false if no
     * log files should be created
     */
    void setCreateLogs(bool createLogs);
    /**
     * @brief setInterpolateMetaData Sets interpolateMetaData
     * @param interpolateMetaData @a true if missing meta data should be
     * interpolated, @a false if not
     */
    void setInterpolateMetaData(bool interpolateMetaData);

    /**
     * @brief Returns all saved reconstruction softwares
     *
     * @return QMap with the name of the reconstruction software as key and it's
     * path as value
     */
    QMap<QString, QString> getReconstructPath();
    /**
     * @brief Returns the standard input path
     *
     * @return QString with the current input path
     */
    QString getStandardInputPath();
    /**
     * @brief Returns wether sanity checks in ots product configuration are
     * disabled
     *
     * @return @a true, if checks are disabled
     */
    bool getDisableChecks();
    /**
     * @brief Returns which darkstyle is saved in the settings file
     *
     * @return @a true if darkstyle is saved
     */
    ColorTheme getColorTheme();
    /**
     * @brief Returns if use CUDA is enabled
     * @return @a true if cuda is enabled
     */
    bool getUseCuda();

    /**
     * @brief The CUDA_ERR_CODE enum has an error code if CUDA is not available.
     */
    enum CUDA_ERR_CODE {
        NO_ERR = 0,        /// indicates that cuda is available
        NO_GPU_FOUND = 1,  /// no GPU was detected
        CC_MISSMATCH =
            2,  /// Compute Capability of the GPU is not supported by the build
        BUILT_WITHOUT_CUDA = 3,  /// iVS3D was compiled without CUDA
        UNKNOWN_ERR = 4          /// any other error occured with CUDA
    };

    /**
     * @brief Returns if the opencv backend supports CUDA
     * @param err_code (optional) pointer, if not null, the error code is stored
     * there if cuda is not available
     * @return @a true if cuda is supported
     */
    bool getCudaAvailable(CUDA_ERR_CODE* err_code = nullptr) const;
    /**
     * @brief Returns if create Logs is enabled
     * @return @a true if logging is enabled and log files will be created
     */
    bool getCreateLogs();
    /**
     * @brief Returns if interpolate meta data is enabled
     * @return @a true if interpolation is enabled
     */
    bool getInterpolateMetaData();
    /**
     * @brief Returns a list of available locales for translating the
     * application.
     * @return The list of QLocale
     */
    const QList<QLocale> getAvailableLocales();
    /**
     * @brief Returns the currently selected locale.
     * @return The locale for translation.
     */
    QLocale getLocale();
    /**
     * @brief setLocale selects a locale for translation. The local has to be
     * supported by the application. Available locales can be accessed by using
     * @a getAvailableLocales(). If the selected locale is not supported, then
     * the current locale is not changed.
     *
     * IMPORTANT: The locale is only applied when the applications starts next
     * time!
     * @param locale The locale to use for translation
     * @return Returns @a true if the locale is supported and @a false
     * otherwise.
     */
    bool setLocale(QLocale locale);

    /**
     * @brief setDefaultPluginName setter for the plugin which is shown on
     * startup
     * @return Returns the name of the on startup shown plugin as a QString
     */
    QString getDefaultPluginName();
    /**
     * @brief getDefaultPluginName getter for the plugin which is shown on
     * startup
     * @param name of the plugin, which is shown on startup as a QString
     */
    void setDefaultPluginName(const QString& pluginName);

    /**
     * @brief get_forceBackupVideoReader shows if the backup reader is always
     used
     * @return if true the backup OpenCV more robust reader is always used
     * otherwise it is only used when the ffmpeg frame perfect custom reader
     * fails

     */
    bool get_forceBackupVideoReader();
    /**
     * @brief get_forceBackupVideoReader allows the user to not use the default
     * video reader, which is frame perfect but not as robust as the backup
     * OpenCV based reader
     * @param v if true the backup OpenCV more robust reader is always used
     * otherwise it is only used when the ffmpeg frame perfect custom reader
     * fails
     */
    void set_forceBackupVideoReader(bool v);

   private:
    ApplicationSettings();
    QMap<QString, QString> m_reconstructPath;
    QString m_standardInputPath;
    bool m_disableChecks;
    ColorTheme m_colorTheme;
    bool m_useCuda;
    bool m_createLogs;
    bool m_interpolateMetaData;
    void loadSettings();
    void saveSettings();
    QLocale m_locale;
    QString m_defaultPluginName;
    bool m_forceBackupVideoReader;
};

#endif  // APPLICATIONSETTINGS_H
