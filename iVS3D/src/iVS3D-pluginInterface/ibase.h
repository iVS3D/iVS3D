#pragma once

#include <QMap>
#include <QObject>
#include <QStringList>
#include <QVariant>
#include <QWidget>
#include <memory>
#include <tl/expected.hpp>
#include <vector>

#include "ierror.h"

class Reader;
class MetaData;

/**
 * @defgroup Plugin PluginInterface
 * @brief Interfaces and types for iVS3D plugins.
 *
 * @details
 * This module defines the base interface `IBase` for all plugins, as well as
 * common types and result aliases used across different plugin categories (e.g.
 * selection, masking, preview).
 *
 * @see @ref plugin_interface_doc "PluginInterface.md" for a comprehensive
 * overview of the plugin API and integration guidelines.
 */

/**
 * @namespace PLUG
 * @brief Plugin interface namespace containing common plugin contracts and
 * helper types.
 * @ingroup Plugin
 *
 * For a full overview of the plugin API, see
 * @ref plugin_interface_doc "PluginInterface.md".
 */
namespace PLUG {

/**
 * @typedef SettingsWidgetResult
 * @brief Type alias for the result of a settings widget creation operation,
 * which can be either a successful unique_ptr to a QWidget or an Error
 * indicating failure.
 */
using SettingsWidgetResult = tl::expected<std::unique_ptr<QWidget>, Error>;

/**
 * @typedef ApplySettingsResult
 * @brief Type alias for the result of applying settings to a plugin, which can
 * be either a successful void result or an Error indicating failure.
 */
using ApplySettingsResult = tl::expected<void, Error>;

/**
 * @typedef InputLoadedResult
 * @brief Type alias for the result of handling an input loaded event, which can
 * be either a successful void result or an Error indicating failure.
 */
using InputLoadedResult = tl::expected<void, Error>;

/**
 * @typedef MetaDataLoadedResult
 * @brief Type alias for the result of handling a metadata-loaded event,
 * either success (`void`) or an `Error`.
 */
using MetaDataLoadedResult = tl::expected<void, Error>;

/**
 * @struct InputData
 * @brief Struct to encapsulate data related to the input loaded event in iVS3D
 */
struct InputData {
    ::Reader* reader = nullptr;
};

/**
 * @struct InputMetaData
 * @brief Struct containing metadata context that has been loaded for the
 * currently opened input.
 */
struct InputMetaData {
    ::MetaData* metaData = nullptr;
};

/**
 * @interface IBase
 *
 * @ingroup Plugin
 *
 * @brief The IBase interface provides a base class for all plugin interfaces in
 * iVS3D. It inherits from QObject to enable signal-slot communication and
 * common functionality across all plugins. All plugin signals need to be
 * declared in this interface to ensure they are available in derived plugin
 * classes.
 *
 * For usage and extension guidance see
 * @ref plugin_interface_doc "PluginInterface.md".
 *
 * @date 2025/12/05
 * @author Dominik Wüst
 */
class IBase : public QObject {
    Q_OBJECT

   public:
    using QObject::QObject;
    virtual ~IBase() {};

    /**
     * @brief getName returns the name of the plugin which will be displayed in
     * the iVS3D interface.
     * @return The name of the plugin as a QString.
     */
    virtual QString getName() const = 0;

    /**
     * @brief getSettingsWidget creates and returns a settings QWidget for this
     * plugin.
     *
     * Ownership contract:
     *  - The plugin must allocate and return a std::unique_ptr<QWidget>.
     *  - Returning the unique_ptr transfers ownership to the caller.
     *  - The plugin must NOT keep ownership of, or store a pointer to, the
     * returned widget.
     *  - The caller (core application) owns the widget and may assign a
     * QObject/QWidget parent.
     *
     * Threading contract:
     *  - The widget is part of the UI and must only be accessed from the UI
     * thread.
     *  - Plugins may run in worker threads and therefore must not directly
     * access the widget after handing it over.
     *  - If a plugin needs to reflect state changes in the UI later (e.g. from
     * applySettings), it must do so via Qt signals/slots connected during
     * widget creation.
     *
     * @return A unique_ptr to the created settings QWidget or an Error if the
     * widget could not be created.
     */
    virtual SettingsWidgetResult getSettingsWidget() = 0;

    /**
     * @brief getSettings retrieves the current settings of the plugin as a map
     * of key-value pairs. The settings are stored in the history within iVS3D
     * and in combination with applySettings allow for saving and restoring
     * plugin configurations.
     *
     * @see applySettings
     * @return A QMap containing the plugin settings as key-value pairs.
     */
    virtual QMap<QString, QVariant> getSettings() const = 0;

    /**
     * @brief getSettingsString is a helper method that converts the plugin
     * settings into a human-readable string format.
     *
     * @details This is used to display the settings in the
     * iVS3D interface and to store them in the history. The default
     * implementation converts the settings map into a semicolon-separated list
     * of key=value pairs.
     */
    virtual QString getSettingsString() const {
        QMap<QString, QVariant> settings = getSettings();
        QStringList settingsList;
        for (auto it = settings.constBegin(); it != settings.constEnd(); ++it) {
            settingsList.append(it.key() + "=" + it.value().toString());
        }
        return settingsList.join(";");
    }

    /**
     * @brief applySettings applies the provided settings to the plugin. This
     * method is used to restore plugin configurations from the history within
     * iVS3D.
     *
     * @see getSettings
     * @param settings A QMap containing the plugin settings as key-value pairs.
     */
    virtual ApplySettingsResult applySettings(
        const QMap<QString, QVariant>& settings) = 0;

    /**
     * @brief activate is called when the plugin is activated in iVS3D.
     * Plugins can override this method to perform any necessary setup when
     * they become active.
     */
    virtual void activate() {}

    /**
     * @brief deactivate is called when the plugin is deactivated in iVS3D.
     * Plugins can override this method to perform any necessary cleanup when
     * they are no longer active.
     */
    virtual void deactivate() {}

    /**
     * @brief onCudaChanged is called when the CUDA usage setting is changed in
     * iVS3D.
     * @param enabled Indicates whether CUDA is enabled (true) or disabled
     * (false).
     *
     * Plugins can override this method to adjust their behavior based on the
     * CUDA setting.
     */
    virtual void onCudaChanged(bool enabled) {}

    /**
     * @brief onInputLoaded is called when a new input video or image set is
     * loaded.
     *
     * Plugins can override this method to reset caches or initialize state that
     * depends on the current input.
     */
    virtual InputLoadedResult onInputLoaded(const InputData& input) {
        (void)input;
        return {};
    }

    /**
     * @brief onMetaDataLoaded is called whenever metadata was loaded or
     * refreshed for the currently active input.
     *
     * Typical use cases:
     * - refresh plugin-internal caches that depend on metadata,
     * - enable/disable metadata-dependent features,
     * - precompute derived values from loaded metadata tracks.
     *
     * Threading contract:
     * - This callback is invoked on the plugin worker thread.
     * - Implementations must not directly manipulate UI widgets here.
     *
     * Error handling:
     * - Return an `Error` if the plugin cannot consume the provided metadata.
     * - iVS3D logs such errors and continues operating.
     *
     * @param inputMetaData Metadata context for the currently loaded input.
     * @return `MetaDataLoadedResult` indicating success or failure.
     */
    virtual MetaDataLoadedResult onMetaDataLoaded(
        const InputMetaData& inputMetaData) {
        (void)inputMetaData;
        return {};
    }

    /**
     * @brief onIndexChanged is called when the currently displayed frame index
     * changes in the viewer.
     *
     * Plugins can use this notification to update index-dependent internal
     * state (for example temporal caches, active-frame labels, or lazy loading
     * windows).
     *
     * Threading contract:
     * - Called on the plugin worker thread.
     *
     * @param index New current frame index.
     */
    virtual void onIndexChanged(uint index) { (void)index; }

    /**
     * @brief onSelectedImagesChanged is called when the current keyframe /
     * selected-image list changed.
     *
     * This keeps plugins synchronized with interactive edits, sampling results,
     * and undo/redo restores that modify the selected image set.
     *
     * Threading contract:
     * - Called on the plugin worker thread.
     *
     * @param selectedImages Updated sorted list of selected image indices.
     */
    virtual void onSelectedImagesChanged(
        const std::vector<uint>& selectedImages) {
        (void)selectedImages;
    }

   signals:
    /**
     * @brief [signal] updatePreview(bool clearOldPreview) can be emitted when
     * the plugin requests an update of the preview visualization.
     *
     * This signal notifies the system that the preview needs to be regenerated,
     * typically due to changes in plugin settings or data. Plugins implementing
     * preview functionality should emit this signal whenever the preview
     * visualization needs to be updated.
     *
     * The optional parameter `clearOldPreview` indicates whether the existing
     * preview should be cleared before generating a new one. If set to false,
     * the old preview will be retained until the new preview is ready to be
     * displayed, this can reduce flickering in some scenarios.
     */
    void updatePreview(bool clearOldPreview = true);

    /**
     * @brief [signal] updateSelectedImages(std::vector<uint> selectedImages)
     * can be emitted when the plugin wants to update the selection of images in
     * the video player. iVS3D will only handle this signal if the plugin is
     * currently active.
     *
     * This signal notifies the system to change the currently selected images
     * in the video player to the specified list of image indices.
     *
     * @see activate, deactivate
     *
     * @param selectedImages A vector containing the indices of the images to
     * be selected in the video player.
     */
    void updateSelectedImages(std::vector<uint> selectedImages);

    /**
     * @brief [signal] updateProgress(int progress, QString message) can be
     * emitted to inform iVS3D about the progress of a long-running operation.
     *
     * This signal allows the plugin to communicate its current progress to the
     * iVS3D interface, which can then display this information to the user.
     * The progress value should be in the range of 0 to 100, representing the
     * percentage of completion. An optional message can provide additional
     * context about the operation's status.
     *
     * This signal is only effective when the plugin is active.
     *
     * @see activate, deactivate
     *
     * @param progress An integer value between 0 and 100 indicating the
     * percentage of completion.
     * @param message An optional QString providing additional information about
     * the progress status.
     */
    void updateProgress(int progress, QString message = QString());

    void encounteredError(Error error);
};

}  // namespace PLUG

Q_DECLARE_INTERFACE(PLUG::IBase, "iVS3D.IBase")
Q_DECLARE_METATYPE(PLUG::InputMetaData)
