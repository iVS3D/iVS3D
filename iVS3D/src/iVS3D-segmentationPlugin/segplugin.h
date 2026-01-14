#pragma once

#include <NeuralNetFactory.h>

#include <QCoreApplication>
#include <QLabel>
#include <QObject>
#include <QVBoxLayout>
#include <QWidget>
#include <memory>
#include <optional>

#include "ibase.h"
#include "imask.h"
#include "ipreview.h"
#include "segloader.h"
#include "segsession.h"
#include "settingswidget.h"

using namespace segmentationplugin;

class SegmentationPlugin : public IBase, public IMask, public IPreview {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "iVS3D.IMask")
    Q_PLUGIN_METADATA(IID "iVS3D.IPreview")
    Q_PLUGIN_METADATA(IID "iVS3D.IBase")
    Q_INTERFACES(IMask IBase IPreview)

   public:
    using IBase::IBase;

    QString getName() const override {
        return tr("Segmentation Plugin with Preview");
    }

    std::shared_ptr<QWidget> getSettingsWidget(QWidget* parent) override {
        // Lazy load the model loader and settings widget
        std::optional<QString> errorMsg = std::nullopt;
        if (!m_loader) {
            try {
                m_loader = std::make_shared<ModelLoader>(m_modelFolder);
                auto models =
                    m_loader->getModels();  // Trigger loading to catch errors
                if (models.isEmpty()) {
                    errorMsg = tr("No models found in the specified folder.");
                }
                m_currentInfo = models[0];  // Select the first model by default
            } catch (const std::exception& e) {
                errorMsg = QString("Failed to load models: %1").arg(e.what());
                m_loader = nullptr;
            }
        }

        if (!m_settingsWidget) {
            m_settingsWidget = std::make_shared<SettingsWidget>(
                parent,
                m_loader ? m_loader->getModels() : QVector<ModelInfo>());
            m_settingsWidget->setOverlayAlpha(0.5f);  // default alpha
            connect(m_settingsWidget.get(), &SettingsWidget::modelChanged, this,
                    &SegmentationPlugin::onModelChanged);
            connect(m_settingsWidget.get(),
                    &SettingsWidget::overlayAlphaChanged, [this](float alpha) {
                        m_overlayAlpha =
                            alpha;  // does not require cache invalidation
                        emit updatePreview(false);
                    });
            connect(m_settingsWidget.get(),
                    &SettingsWidget::classSelectionChanged, this,
                    &SegmentationPlugin::onClassSelectionChanged);
        }

        if (errorMsg) {
            m_settingsWidget->displayError(*errorMsg);
        }

        return m_settingsWidget;
    }

    QMap<QString, QVariant> getSettings() const override {
        QMap<QString, QVariant> settings;
        if (m_currentInfo) {
            settings["modelName"] = m_currentInfo->name;

            QStringList selectedClasses;
            for (const auto& cls : m_currentInfo->classes) {
                if (cls.selected) {
                    selectedClasses.append(cls.name);
                }
            }
            settings["selectedClasses"] = selectedClasses;
        }
        settings["overlayAlpha"] = m_overlayAlpha;
        return settings;
    }

    void setSettings(const QMap<QString, QVariant>& settings) override {
        if (settings.contains("modelName")) {
            QString modelName = settings["modelName"].toString();
            onModelChanged(modelName);
        }
        if (settings.contains("selectedClasses") && m_currentInfo) {
            QStringList selectedClasses =
                settings["selectedClasses"].toStringList();
            QVector<bool> selectionFlags(m_currentInfo->classes.size(), false);
            for (int i = 0; i < m_currentInfo->classes.size(); ++i) {
                if (selectedClasses.contains(m_currentInfo->classes[i].name)) {
                    selectionFlags[i] = true;
                }
            }
            onClassSelectionChanged(selectionFlags);
        }
        if (settings.contains("overlayAlpha")) {
            m_overlayAlpha = settings["overlayAlpha"].toFloat();
            if (m_settingsWidget) {
                m_settingsWidget->setOverlayAlpha(m_overlayAlpha);
            }
        }
    }

    std::unique_ptr<IMaskComputeSession> createMaskComputeSession(
        const QMap<QString, QVariant>& settings) override;

    VisualizationResult generatePreview(const PreviewData& data) override;

    void onCudaChanged(bool enabled) override {
        m_useCuda = enabled;
        if (m_currentInfo) {
            m_currentSession = nullptr;  // force reload with new cuda setting
            m_cache = std::nullopt;      // Clear cache
            emit updatePreview(true);
        }
    }

   private slots:
    void onModelChanged(const QString& modelName) {
        m_currentInfo = std::nullopt;
        m_currentSession = nullptr;  // force reload with new model

        if (auto modelInfo = m_loader->getModelByName(modelName)) {
            m_settingsWidget->setModel(*modelInfo);
            m_settingsWidget->displaySettings();
            m_currentInfo = *modelInfo;
            // Do something with the selected model
        } else {
            m_settingsWidget->displayError(tr("Selected model not found."));
        }
        m_cache = std::nullopt;  // Clear entire cache on model change
        emit updatePreview();
    }

    void onClassSelectionChanged(const QVector<bool>& selectedClasses) {
        if (m_cache) {
            m_cache->maskImage =
                cv::Mat();  // Invalidate only mask image in cache
        }  // Segmentation and colorized image can remain
        assert(selectedClasses.size() == m_currentInfo->classes.size());
        for (size_t i = 0; i < selectedClasses.size(); ++i)
            m_currentInfo->classes[i].selected = selectedClasses[i];

        if (m_currentSession) m_currentSession->setModelInfo(*m_currentInfo);
        emit updatePreview(false);
    }

   private:
    QString m_modelFolder = QCoreApplication::applicationDirPath() +
                            "/plugins/resources/neural_network_models";
    std::shared_ptr<ModelLoader> m_loader;
    std::shared_ptr<SettingsWidget> m_settingsWidget;

    bool m_useCuda = false;
    float m_overlayAlpha = 0.5f;

    std::optional<ModelInfo> m_currentInfo;
    std::unique_ptr<SegmentationSession> m_currentSession;

    // cache for the last inference result (if any and valid)
    struct SegmentationCache {
        uint idx;
        cv::Mat image;
        NN::Tensor inferenceTensor;
        cv::Mat colorizedImage =
            cv::Mat();                  // optional: can be empty if invalid
        cv::Mat maskImage = cv::Mat();  // optional: can be empty if invalid
    };
    std::optional<SegmentationCache> m_cache;
};