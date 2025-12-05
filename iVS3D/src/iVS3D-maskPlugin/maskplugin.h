#pragma once

#include <QLabel>
#include <QObject>
#include <QVBoxLayout>
#include <QWidget>

#include "ibase.h"
#include "imask.h"
#include "ipreview.h"

class MaskPlugin : public IBase, public IMask, public IPreview {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "iVS3D.IMask")
    Q_PLUGIN_METADATA(IID "iVS3D.IPreview")
    Q_PLUGIN_METADATA(IID "iVS3D.IBase")
    Q_INTERFACES(IMask IBase IPreview)

public:
    using IBase::IBase;

    QString getName() const override {
        return tr("Simple Mask and Preview Plugin");
    }

    QWidget* getSettingsWidget(QWidget* parent) override {
        // No settings for this simple mask and preview plugin
        if (!m_settingsWidget) {
            m_settingsWidget = new QWidget(parent);
            m_settingsWidget->setLayout(new QVBoxLayout(m_settingsWidget));
        }
        return m_settingsWidget;
    }

    MaskResult generateMask(const MaskData& data) override;

    Visualization generatePreview(const PreviewData& data) override;

private:
    QWidget* m_settingsWidget = nullptr;
};
