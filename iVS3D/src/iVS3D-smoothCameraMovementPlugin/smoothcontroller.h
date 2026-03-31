#ifndef CONTROLLER_H
#define CONTROLLER_H

/** @defgroup StationaryCameraPlugin StationaryCameraPlugin
 *
 * @ingroup Plugin
 *
 * @brief Plugin to remove keyframes if no camera movement was detected.
 */

#include <QCoreApplication>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QLayout>
#include <QObject>
#include <QPoint>
#include <QTranslator>
#include <QVariant>
#include <QWidget>
#include <algorithm>
#include <future>
#include <memory>
#include <opencv2/core/mat.hpp>
#include <opencv2/video.hpp>
#include <vector>

#include "factory.h"
#include "flowcalculator.h"
#include "ibase.h"
#include "imagegatherer.h"
#include "iselection.h"
#include "reader.h"

#define PLUGIN_NAME QObject::tr("Smooth camera movement")
// widget
#define SELECTOR_LABEL_TEXT QObject::tr("Movement Threshold")
#define SELECTOR_DESCRIPTION                                               \
    QObject::tr(                                                           \
        "A resolution dependend threshold, that specifies when there was " \
        "enough movement to set a new keyframe.")
#define DESCRIPTION_STYLE                                               \
    "color: rgb(58, 58, 58); border-left: 6px solid  rgb(58, 58, 58); " \
    "border-top-right-radius: 5px; border-bottom-right-radius: 5px; "   \
    "background-color: lightblue;"
// settings
#define SETTINGS_SELECTOR_THRESHOLD "Selector threshold"
// log file
#define LF_OPT_FLOW_TOTAL "Flow calculation"
#define LF_SELECT_FRAMES "Selection of keyframes"
#define LF_CE_TYPE_ADDITIONAL_INFO "Additional Computation Information"
#define LF_CE_VALUE_USED_BUFFERED "Used buffered values"
#define LF_CE_TYPE_DEBUG "Debug Information"
#define LF_CE_NAME_FLOWVALUE "Flow value"
#define LF_TIMER_BUFFER "Update Buffer"
#define LF_TIMER_CORE "Core Computation"
#define LF_TIMER_SELECTION "Keyframe selection"

/**
 * @class CameraMovement
 *
 * @ingroup StationaryCameraPlugin
 *
 * @brief Implements smooth camera movement keyframe selection using `PLUG::IBase` + `PLUG::ISelection`.
 *
 * @author Dominic Zahn
 *
 * @date 2022/3/13
 */
class CameraMovement : public PLUG::IBase, public PLUG::ISelection {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "iVS3D.IBase")
    Q_INTERFACES(PLUG::IBase PLUG::ISelection)

   public:
    CameraMovement();
    ~CameraMovement() override = default;

    // IBase
    PLUG::SettingsWidgetResult getSettingsWidget() override;
    QString getName() const override;
    QMap<QString, QVariant> getSettings() const override;
    PLUG::ApplySettingsResult applySettings(
        const QMap<QString, QVariant>& settings) override;
    PLUG::InputLoadedResult onInputLoaded(
        const PLUG::InputData& input) override;
    void onCudaChanged(bool enabled) override;

    // ISelection
    PLUG::SelectionResult selectImages(const PLUG::SelectionData& data,
                                       volatile bool& cancelFlag) override;

   signals:
    void syncSettingsWidget(double selectorThreshold);

   private slots:
    void slot_selectorThresholdChanged(double value);

   private:
    std::unique_ptr<QWidget> createSettingsWidget();
    void reportProgress(const QString& op, int progress);

   private:
    double m_selectorThreshold = 2.0;
    bool m_useCuda = false;
    Reader* m_reader = nullptr;
    QPoint m_inputResolution = QPoint(0, 0);
    cv::SparseMat m_bufferMat;

    QDoubleSpinBox* m_selectorThresholdSpinBox = nullptr;
};

#endif  // CONTROLLER_H
