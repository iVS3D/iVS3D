#include "pluginthread.h"

PluginRunner::PluginRunner(QObject* parent) : QObject(parent) {}

void PluginRunner::requestPreview(const PreviewRequest& request) {
    if (!request.plugin.hasPreview()) return;  // no preview to compute
    if (request.id != m_latestRequest.loadRelaxed()) {
        return;  // discard outdated request
    }
    emit previewStarted(request.id);  // emit signal that preview generation has started
    VisualizationResult result =
        request.plugin.preview->generatePreview({request.idx, request.img});
    emit previewFinished({request.idx, result});
}

void PluginRunner::requestMask(const MaskRequest& request) {
    if (!request.plugin.hasMask()) {
        MaskGenerationResult result;
        result.imageIndex = request.imageIndex;
        result.mask = cv::Mat();
        result.maskRecordId = request.maskRecordId;
        result.success = false;
        result.errorMessage = "Plugin does not support mask generation";
        emit maskFinished(result);
        return;
    }
    
    const auto maskResult = request.plugin.mask->generateMask({request.imageIndex, request.image, request.exportDir});
    
    MaskGenerationResult result;
    result.imageIndex = request.imageIndex;
    result.maskRecordId = request.maskRecordId;
    
    if (maskResult) {
        result.mask = *maskResult;
        result.success = true;
        result.errorMessage = "";
    } else {
        result.mask = cv::Mat();
        result.success = false;
        result.errorMessage = maskResult.error().message;
    }
    
    emit maskFinished(result);
}

PluginThread::PluginThread(const QVector<PluginHandle>& pluginHandles,
                           QObject* parent)
    : QObject(parent) {
    m_thread = std::make_unique<QThread>(this);
    m_runner = std::make_unique<PluginRunner>(nullptr);

    // Move all plugins and the runner to the worker thread
    for (const PluginHandle& handle : pluginHandles) {
        handle.base->moveToThread(m_thread.get());
    }
    m_runner->moveToThread(m_thread.get());

    // forward the previewFinished signal from the runner to the PluginThread's
    // own signal
    connect(
        m_runner.get(), &PluginRunner::previewFinished, this,
        [=](const PreviewResult& result) {
            emit previewStateChanged(PreviewState::Idle);  // update state to idle when preview is finished 
            emit previewFinished(result);
        },
        Qt::QueuedConnection);
    connect(m_runner.get(), &PluginRunner::previewStarted, this,
            [=](const RequestId& id) {
                emit previewStateChanged(PreviewState::Processing);  // update state to processing when preview starts
            },
            Qt::QueuedConnection);

    // Forward mask finished signal from runner to PluginThread
    connect(m_runner.get(), &PluginRunner::maskFinished, this,
            [=](const MaskGenerationResult& result) {
                emit maskFinished(result);
            },
            Qt::QueuedConnection);

    // start the worker thread
    m_thread->start();
}

PluginThread::~PluginThread() {
    // Signal the thread to stop
    m_thread->quit();
    
    // Wait for the thread to finish before destroying it
    if (!m_thread->wait(5000)) {  // 5 second timeout
        m_thread->terminate();
        m_thread->wait();
    }
}

void PluginThread::requestPreview(const PluginHandle& plugin,
                                  const PreviewData& request) {
    if (!plugin.hasPreview()) return;  // no preview to compute
    RequestId id = ++m_counter;  // generate a new unique ID for this request
    m_runner->setLatestRequestId(
        id);  // update the latest request ID in the runner

    PreviewRequest previewRequest{id, request.index, request.image, plugin};
    QMetaObject::invokeMethod(
        m_runner.get(), "requestPreview",
        Qt::QueuedConnection,
        Q_ARG(PreviewRequest, previewRequest));
}

void PluginThread::requestMask(const MaskRequest& request) {
    QMetaObject::invokeMethod(
        m_runner.get(), "requestMask",
        Qt::QueuedConnection,
        Q_ARG(MaskRequest, request));
}
