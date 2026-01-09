#pragma once

#include <QLabel>
#include <QObject>
#include <QVBoxLayout>
#include <QWidget>
#include <QCoreApplication>

#include <memory>
#include <optional>

#include "ibase.h"
#include "imask.h"
#include "ipreview.h"

#include "nnloader.h"
#include "settingswidget.h"
#include <NeuralNetFactory.h>

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

    QWidget* getSettingsWidget(QWidget* parent) override {

        // Lazy load the model loader and settings widget
        std::optional<QString> errorMsg = std::nullopt;
        if (!m_loader) {
            try {
                m_loader = std::make_shared<NNLoader>(m_modelFolder);
                auto models = m_loader->getModels(); // Trigger loading to catch errors
                if (models.isEmpty()) {
                    errorMsg = tr("No models found in the specified folder.");
                }
                m_currentModel = {models[0], nullptr}; // Select the first model by default
            } catch (const std::exception& e) {
                errorMsg = QString("Failed to load models: %1").arg(e.what());
                m_loader = nullptr;
            }
        }

        if (!m_settingsWidget) {    
            m_settingsWidget = std::make_shared<SettingsWidget>(m_loader ? m_loader->getModels() : QVector<ModelInfo>());
            m_settingsWidget->setOverlayAlpha(0.5f); // default alpha
            connect(m_settingsWidget.get(), &SettingsWidget::modelChanged, this, &SegmentationPlugin::onModelChanged);
            connect(m_settingsWidget.get(), &SettingsWidget::overlayAlphaChanged, [this](float alpha){
                m_overlayAlpha = alpha; // does not require cache invalidation
                emit updatePreview();
            });
            connect(m_settingsWidget.get(), &SettingsWidget::classSelectionChanged, this, &SegmentationPlugin::onClassSelectionChanged);
        }

        if (errorMsg) {
            m_settingsWidget->displayError(*errorMsg);
        }

        return m_settingsWidget.get();
    }

    MaskResult generateMask(const MaskData& data) override;

    Visualization generatePreview(const PreviewData& data) override;

    void onCudaChanged(bool enabled) override {
        m_useCuda = enabled;
        if (m_currentModel) {
            m_currentModel->model = nullptr; // force reload with new cuda setting
            m_cache = std::nullopt; // Clear cache
            emit updatePreview();
        }
    }


private slots:
    void onModelChanged(const QString& modelName) {
        m_currentModel = std::nullopt;
            
        if (auto modelInfo = m_loader->getModelByName(modelName)) {
            m_settingsWidget->setModel(*modelInfo);
            m_settingsWidget->displaySettings();
            m_currentModel = CurrentModel{*modelInfo, nullptr};
            // Do something with the selected model
        } else {
            m_settingsWidget->displayError(tr("Selected model not found."));
        }
        emit updatePreview();
        m_cache = std::nullopt; // Clear entire cache on model change
    }

    void onClassSelectionChanged(const QVector<bool>& selectedClasses) {
        if (m_cache) {
            m_cache->maskImage = cv::Mat(); // Invalidate only mask image in cache
        }                                   // Segmentation and colorized image can remain
        assert(selectedClasses.size() == m_currentModel->info.classes.size());
        for (size_t i = 0; i < selectedClasses.size(); ++i)
            m_currentModel->info.classes[i].selected = selectedClasses[i];
        emit updatePreview();
    }

private:
    std::optional<QString> ensureModelLoaded();
    tl::expected<NN::Tensor, NN::NeuralError> runInference(const cv::Mat& image);
    tl::expected<cv::Mat, NN::NeuralError> runColorization(const NN::Tensor& inferenceTensor);
    tl::expected<cv::Mat, NN::NeuralError> runMasking(const NN::Tensor& inferenceTensor);

    QString m_modelFolder = QCoreApplication::applicationDirPath() + "/plugins/resources/neural_network_models";
    std::shared_ptr<NNLoader> m_loader;
    std::shared_ptr<SettingsWidget> m_settingsWidget;
    
    bool m_useCuda = false;
    float m_overlayAlpha = 0.5f;

    // state of the current model (if any)
    struct CurrentModel{
        segmentationplugin::ModelInfo info;
        NN::NeuralNetPtr model = nullptr;
    };
    std::optional<CurrentModel> m_currentModel;

    // cache for the last inference result (if any and valid)
    struct SegmentationCache {
        uint idx;
        cv::Mat image;
        NN::Tensor inferenceTensor;
        cv::Mat colorizedImage = cv::Mat(); // optional: can be empty if invalid
        cv::Mat maskImage = cv::Mat(); // optional: can be empty if invalid
    };
    std::optional<SegmentationCache> m_cache;
};
