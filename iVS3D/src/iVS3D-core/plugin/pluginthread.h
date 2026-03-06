#pragma once

#include <QAtomicInteger>
#include <QDir>
#include <QHash>
#include <QObject>
#include <QThread>
#include <memory>
#include <queue>

#include "pluginhandle.h"

using RequestId = quint64;

struct PreviewRequest {
    RequestId id;        // unique ID for this request
    uint idx;            // Index of the image in the sequence
    cv::Mat img;         // The image to create a preview for
    QString pluginName;  // The plugin to use for generating the preview
};
Q_DECLARE_METATYPE(PreviewRequest)

struct PreviewResult {
    uint idx;                           // Index of the image in the sequence
    VIS::VisualizationResult visualization;  // The result of the visualization
};
Q_DECLARE_METATYPE(PreviewResult)

struct MaskRequest {
    RequestId id;        // unique ID for this request
    uint imageIndex;     // Index of the image in the sequence
    cv::Mat image;       // The image to create a mask for
    QDir exportDir;      // Directory where the generated mask will be saved
    QString pluginName;  // The plugin to use for mask generation
    int maskRecordId;    // ID of the MaskRecord for tracking
};
Q_DECLARE_METATYPE(MaskRequest)

struct SelectionRequest {
    RequestId id;
    QString pluginName;
    PLUG::SelectionData data;
};
Q_DECLARE_METATYPE(SelectionRequest)

struct SelectionResultData {
    RequestId id;
    QString pluginName;
    bool success = false;
    bool cancelled = false;
    std::vector<uint> selectedIndices;
    QString errorMessage;
};
Q_DECLARE_METATYPE(SelectionResultData)

struct MaskGenerationResult {
    uint imageIndex;   // Index of the image in the sequence
    cv::Mat mask;      // The generated mask
    int maskRecordId;  // ID of the MaskRecord
    bool success = true;
    QString errorMessage;
};
Q_DECLARE_METATYPE(MaskGenerationResult)

enum class PreviewState {
    Idle,
    Processing,
};
Q_DECLARE_METATYPE(PreviewState)

/**
 * PluginRunner lives in a separate worker thread and is responsible for
 * executing plugin code asynchronously. It receives requests to execute plugin
 * code via slots and emits signals with results when done.
 */
class PluginRunner : public QObject {
    Q_OBJECT
   public:
    PluginRunner(const QHash<QString, PluginHandle>& plugins,
                 QObject* parent = nullptr);

    void setLatestRequestId(RequestId id) { m_latestRequest.storeRelaxed(id); }

    bool setActivePlugin(const QString& pluginName);
    void clearActivePlugin();
    PLUG::ApplySettingsResult applyPluginSettings(
        const QString& pluginName, const QMap<QString, QVariant>& settings);
    QMap<QString, QVariant> getPluginSettings(const QString& pluginName) const;
    QString getPluginSettingsString(const QString& pluginName) const;
    void enableCuda(bool useCuda);
    void onInputLoaded(Reader* reader);
    void resetSelectionCancelFlag();
    void cancelSelectionDirect();

   public slots:
    void onMetaDataLoaded(const PLUG::InputMetaData& inputMetaData);
    void onIndexChanged(uint index);
    void onSelectedImagesChanged(const std::vector<uint>& selectedImages);
    void requestPreview(const PreviewRequest& request);
    void requestMask(const MaskRequest& request);
    void requestSelection(const SelectionRequest& request);
    void cancelSelection();

   signals:
    void previewStarted(const RequestId& id);
    void previewFinished(const PreviewResult& result);
    void maskFinished(const MaskGenerationResult& result);
    void selectionFinished(const SelectionResultData& result);
    void activePluginUpdatePreview(bool clearOldPreview);
    void activePluginUpdateProgress(int progress, const QString& message);
    void activePluginUpdateSelectedImages(
        const std::vector<uint>& selectedImages);

   private:
    const PluginHandle* findPlugin(const QString& pluginName) const;

    QHash<QString, PluginHandle> m_plugins;
    QString m_activePluginName;
    volatile bool m_selectionCancelFlag = false;
    QAtomicInteger<RequestId> m_latestRequest{0};
};

class PluginThread : public QObject {
    Q_OBJECT

   public:
    PluginThread(const QHash<QString, PluginHandle>& pluginHandles,
                 QObject* parent = nullptr);
    ~PluginThread();

    void requestPreview(const QString& pluginName, const PLUG::PreviewData& request);
    void requestMask(const MaskRequest& request);
    void requestSelection(const QString& pluginName, const PLUG::SelectionData& data);
    void cancelSelection();

    bool setActivePlugin(const QString& pluginName);
    void clearActivePlugin();

    PLUG::ApplySettingsResult applyPluginSettings(
        const QString& pluginName, const QMap<QString, QVariant>& settings);
    QMap<QString, QVariant> getPluginSettings(const QString& pluginName) const;
    QString getPluginSettingsString(const QString& pluginName) const;

    void enableCuda(bool useCuda);
    void onInputLoaded(Reader* reader);

    public slots:
    void onMetaDataLoaded(const PLUG::InputMetaData& inputMetaData);
    void onIndexChanged(uint index);
    void onSelectedImagesChanged(const std::vector<uint>& selectedImages);

   signals:
    void previewFinished(const PreviewResult& result);
    void previewStateChanged(const PreviewState& state);
    void maskFinished(const MaskGenerationResult& result);
    void selectionFinished(const SelectionResultData& result);
    void activePluginUpdatePreview(bool clearOldPreview);
    void activePluginUpdateProgress(int progress, const QString& message);
    void activePluginUpdateSelectedImages(
        const std::vector<uint>& selectedImages);
    void notifyMetaDataLoaded(const PLUG::InputMetaData& inputMetaData);
    void notifyIndexChanged(uint index);
    void notifySelectedImagesChanged(const std::vector<uint>& selectedImages);

   private:
    std::unique_ptr<QThread> m_thread;
    std::unique_ptr<PluginRunner> m_runner;
    RequestId m_counter{0};
    QHash<QString, PluginHandle> m_plugins;
};