#ifndef BLUR_H
#define BLUR_H

/** @defgroup BlurPlugin BlurPlugin
 *
 * @ingroup Plugin
 *
 * @brief Calculates keyframes based on blurrines
 */

#include <QComboBox>
#include <QCoreApplication>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QLayout>
#include <QMap>
#include <QObject>
#include <QSpinBox>
#include <QString>
#include <QTranslator>
#include <QWidget>
#include <algorithm>

#include "ibase.h"
#include "iselection.h"
#include "ipreview.h"

#include "BlurAlgorithm.h"
#include "blurlaplacian.h"
#include "blursobel.h"
#include "blurtenengrad.h"

#define DESCRIPTION_STYLE                                               \
    "color: rgb(58, 58, 58); border-left: 6px solid  rgb(58, 58, 58); " \
    "border-top-right-radius: 5px; border-bottom-right-radius: 5px; "   \
    "background-color: lightblue;"
#define WINDOW_SIZE "Window size"
#define LOCAL_DEVIATION "Local deviation"
#define USED_BLUR "Blur"

/**
 * @class Blur
 *
 * @ingroup BlurPlugin
 *
 * @brief The Blur class selects keyframes based on blur/sharpness values
 * computed by the selected `BlurAlgorithm`.
 *
 * @author Daniel Brommer
 *
 * @date 2021/02/19
 */

class Blur : public PLUG::IBase, public PLUG::ISelection, public PLUG::IPreview {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "iVS3D.IBase")
    Q_INTERFACES(PLUG::IBase PLUG::ISelection PLUG::IPreview)

   public:
    /**
     * @brief Blur Constructor which creates an instance of every BlurAlgorithm
     * and stets standard values (WindowSize = 30, LocalDeviation = 95)
     */
    Blur();
    ~Blur() override;

    // IBase interface
    PLUG::SettingsWidgetResult getSettingsWidget() override;
    /**
     * @brief getName Returns the plugin Name
     * @return "Blur"
     */
    QString getName() const override;
    QMap<QString, QVariant> getSettings() const override;
    PLUG::ApplySettingsResult applySettings(const QMap<QString, QVariant>& settings) override;
    void activate() override;
    void deactivate() override;
    void onCudaChanged(bool enabled) override;
    PLUG::InputLoadedResult onInputLoaded(const PLUG::InputData& input) override;
    void onIndexChanged(uint index) override;

    // IPreview interface
    VIS::VisualizationResult generatePreview(const PLUG::PreviewData& data) override;

    // ISelection interface
    PLUG::SelectionResult selectImages(const PLUG::SelectionData& data,
                                 volatile bool& cancelFlag) override;

signals:
    void syncSettingsWidget(const QString& algorithmName,
                            int windowSize,
                            double localDeviation,
                            const QString& infoText);

private slots:
    void slot_blurAlgoChanged(const QString& name);
    void slot_wsChanged(int ws);
    void slot_ldChanged(double ld);

private:
    std::unique_ptr<QWidget> createSettingsWidget();
    std::vector<uint> sampleKeyframes(Reader* reader,
                                      volatile bool& cancelFlag,
                                      const std::vector<uint>& indices);
    QString progressMessage(int curr, int total) const;
    QString currentInfoText() const;
    void invalidateCache();

    Reader* m_reader = nullptr;
    QComboBox* m_comboBoxBlur = nullptr;
    QSpinBox* m_spinBoxWS = nullptr;
    QDoubleSpinBox* m_spinBoxLD = nullptr;
    QLabel* m_infoLabel = nullptr;
    BlurAlgorithm* m_usedBlur = nullptr;
    int m_windowSize = 30;
    double m_localDeviation = 5.0;
    std::vector<BlurAlgorithm*> m_blurAlgorithms = {};

    // Local cache (invalidated on algorithm/input change)
    std::vector<double> m_cachedBlurValues = {};
    uint m_cachedPicCount = 0;
    QString m_cachedAlgoName;
    uint m_currentIndex = 0;
    bool m_useCuda = false;
};

#endif  // BLUR_H
