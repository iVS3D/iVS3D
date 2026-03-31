#pragma once

#include <QObject>
#include <QWidget>
#include <QString>
#include <QMap>
#include <QVariant>
#include <memory>
#include <vector>

#include "ibase.h"
#include "iselection.h"

/**
 * @defgroup NthFramePlugin NthFramePlugin
 * @ingroup Plugin
 * @brief Plugin to select every Nth frame as keyframe.
 */

/**
 * @class NthFrame
 * @ingroup NthFramePlugin
 *
 * @brief The NthFrame class implements the IBase and ISelection plugin interfaces
 * to select every Nth frame as a keyframe. Users can configure the frame interval N
 * and optionally keep isolated images that would otherwise be skipped.
 *
 * @author Dominik Wüst
 * @date 2021/02/14
 */
class NthFrame : public PLUG::IBase, public PLUG::ISelection {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "iVS3D.IBase")
    Q_PLUGIN_METADATA(IID "iVS3D.ISelection")
    Q_INTERFACES(PLUG::IBase PLUG::ISelection)

public:
    /// Constructor initializing N to 1 and default settings.
    NthFrame();

    /// Destructor.
    ~NthFrame() = default;

    // IBase interface implementation
    QString getName() const override;
    PLUG::SettingsWidgetResult getSettingsWidget() override;
    QMap<QString, QVariant> getSettings() const override;
    PLUG::ApplySettingsResult applySettings(const QMap<QString, QVariant>& settings) override;
    void activate() override;
    void deactivate() override;
    void onCudaChanged(bool enabled) override;
    PLUG::InputLoadedResult onInputLoaded(const PLUG::InputData& input) override;

    // ISelection interface implementation
    PLUG::SelectionResult selectImages(const PLUG::SelectionData& data, volatile bool& cancelFlag) override;

signals:
    void syncSettingsWidget(uint n, bool keepIsolated);

private slots:
    void slot_nChanged(int n);
    void slot_checkboxToggled(bool checked);

private:
    std::unique_ptr<QWidget> createSettingsWidget();

    // Settings
    unsigned int m_n = 1;           ///< Frame interval (select every Nth frame)
    bool m_keepIsolated = true;     ///< Keep isolated images outside the N-frame pattern
    int m_fps = 30;                 ///< Frames per second from video (for default N calculation)
    uint m_numFrames = 0;           ///< Total frame count of the current input

    // UI components
};

