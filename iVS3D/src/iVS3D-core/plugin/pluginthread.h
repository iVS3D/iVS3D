#pragma once

#include <QThread>
#include <QObject>
#include <QAtomicInteger>

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

enum class PreviewState {
    Idle,
    Processing,
};
Q_DECLARE_METATYPE(PreviewState)

/**
 * PluginRunner lives in a separate worker thread and is responsible for executing plugin code asynchronously. 
 * It receives requests to execute plugin code via the request_preview slot, executes the plugin's generatePreview 
 * function, and emits a signal with the result when done. It also keeps track of the
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

signals:
    void previewStarted(const RequestId& id);
    void previewFinished(const PreviewResult& result);

private:
    QAtomicInteger<RequestId> m_latestRequest{0};  // track the latest request ID to ensure only the most recent request is processed
};


class PluginThread : public QObject {
    Q_OBJECT

public:
    PluginThread(const QVector<PluginHandle>& pluginHandles, QObject* parent = nullptr);
    ~PluginThread();

    void requestPreview(const PluginHandle& plugin, const PreviewData& request);

signals:
    void previewFinished(const PreviewResult& result);
    void previewStateChanged(const PreviewState& state);

private:

    std::unique_ptr<QThread> m_thread;
    std::unique_ptr<PluginRunner> m_runner;
    RequestId m_counter{0};  // counter to assign unique IDs to each request
};