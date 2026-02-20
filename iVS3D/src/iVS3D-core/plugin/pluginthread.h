#pragma once

#include <QThread>
#include <QObject>
#include <QAtomicInteger>
#include <QDir>
#include <queue>
#include <memory>

#include "pluginhandle.h"

using RequestId = quint64;

struct PreviewRequest {
    RequestId id;   // unique ID for this request
    uint idx;       // Index of the image in the sequence
    cv::Mat img;    // The image to create a preview for
    PluginHandle plugin; // The plugin to use for generating the preview
};
Q_DECLARE_METATYPE(PreviewRequest)

struct PreviewResult {
    uint idx;       // Index of the image in the sequence
    VisualizationResult visualization; // The result of the visualization
};
Q_DECLARE_METATYPE(PreviewResult)

struct MaskRequest {
    RequestId id;               // unique ID for this request
    uint imageIndex;            // Index of the image in the sequence
    cv::Mat image;              // The image to create a mask for
    QDir exportDir;             // Directory where the generated mask will be saved
    PluginHandle plugin;        // The plugin to use for mask generation
    int maskRecordId;           // ID of the MaskRecord for tracking
};
Q_DECLARE_METATYPE(MaskRequest)

struct MaskGenerationResult {
    uint imageIndex;            // Index of the image in the sequence
    cv::Mat mask;               // The generated mask
    int maskRecordId;           // ID of the MaskRecord
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
 * PluginRunner lives in a separate worker thread and is responsible for executing plugin code asynchronously. 
 * It receives requests to execute plugin code via slots and emits signals with results when done.
 */
class PluginRunner : public QObject {
    Q_OBJECT
public:
    PluginRunner(QObject* parent = nullptr);

    void setLatestRequestId(RequestId id) {
        m_latestRequest.storeRelaxed(id);
    }

public slots:
    void requestPreview(const PreviewRequest& request);
    void requestMask(const MaskRequest& request);

signals:
    void previewStarted(const RequestId& id);
    void previewFinished(const PreviewResult& result);
    void maskFinished(const MaskGenerationResult& result);

private:
    QAtomicInteger<RequestId> m_latestRequest{0};
};


class PluginThread : public QObject {
    Q_OBJECT

public:
    PluginThread(const QVector<PluginHandle>& pluginHandles, QObject* parent = nullptr);
    ~PluginThread();

    void requestPreview(const PluginHandle& plugin, const PreviewData& request);
    void requestMask(const MaskRequest& request);

signals:
    void previewFinished(const PreviewResult& result);
    void previewStateChanged(const PreviewState& state);
    void maskFinished(const MaskGenerationResult& result);

private:
    std::unique_ptr<QThread> m_thread;
    std::unique_ptr<PluginRunner> m_runner;
    RequestId m_counter{0};
    QVector<PluginHandle> m_plugins;
};