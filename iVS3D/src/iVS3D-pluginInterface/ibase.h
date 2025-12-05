#pragma once

#include <QObject>
#include "iVS3D-pluginInterface_global.h"


/**
 * @interface IBase 
 * 
 * @ingroup Plugin
 * 
 * @brief The IBase interface provides a base class for all plugin interfaces in iVS3D. It inherits from QObject
 * to enable signal-slot communication and common functionality across all plugins. All plugin signals need to be
 * declared in this interface to ensure they are available in derived plugin classes.
 */
class IBase : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;
    virtual ~IBase() {};

    virtual QString getName() const = 0;
    virtual QWidget* getSettingsWidget(QWidget* parent) = 0;

    virtual void initialize() {}
    virtual void activate() {}
    virtual void deactivate() {}

    /**
     * @brief onCudaChanged is called when the CUDA usage setting is changed in iVS3D.
     * @param enabled Indicates whether CUDA is enabled (true) or disabled (false).
     * 
     * Plugins can override this method to adjust their behavior based on the CUDA setting.
     */
    virtual void onCudaChanged(bool enabled) {}

signals:
    /**
     * @brief [signal] updatePreview() can be emitted when the plugin requests an update of the preview visualization.
     * 
     * This signal notifies the system that the preview needs to be regenerated, typically due to changes in
     * plugin settings or data. Plugins implementing preview functionality should emit this signal whenever
     * the preview visualization needs to be updated.
     */
    void updatePreview();
    void updateSettingsBuffer(QMap<QString, QVariant> buffer);
    void updateSelectedFrames(std::vector<uint> selectedFrames);
    void updateProgress(int progress, QString message = QString());
};

Q_DECLARE_INTERFACE(IBase, "iVS3D.IBase")