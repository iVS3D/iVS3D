#pragma once

#include <QWidget>
#include <QSlider>
#include <QLabel>

#include <ModelManager.h>
#include <ModelSettingsWidget.h>

class SegmentationSettingsWidget : public QWidget {
    Q_OBJECT

public:
    explicit SegmentationSettingsWidget(MCFG::ModelManager& manager,
                                        QWidget* parent = nullptr);

    MCFG::ModelSettingsWidget* modelSettingsWidget() const { return m_modelSettingsWidget; }

    void setOverlayAlpha(float alpha);
    float overlayAlpha() const;

signals:
    void overlayAlphaChanged(float overlayAlpha);

public slots:
    void applyPluginSettings(const QString& selectedModelName,
                             float overlayAlpha);

private:
    MCFG::ModelSettingsWidget* m_modelSettingsWidget = nullptr;
    QSlider* m_alphaSlider = nullptr;
    QLabel* m_alphaValue = nullptr;
};
