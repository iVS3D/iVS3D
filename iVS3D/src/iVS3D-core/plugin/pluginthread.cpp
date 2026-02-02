#include "pluginthread.h"

PluginRunner::PluginRunner(QObject* parent) : QObject(parent) {}

void PluginRunner::requestPreview(const PreviewRequest& request) {
    if (!request.plugin.hasPreview()) return;  // no preview to compute
    if (request.id != m_latestRequest.loadRelaxed()) {
        qDebug() << "[THREADING] Discarding outdated preview request with ID" << request.id << "for plugin" << request.plugin.name();
        return;  // discard outdated request
    }
    
    qDebug() << "[THREADING] PluginRunner generating preview with ID" << request.id << "for plugin" << request.plugin.name();
    VisualizationResult result =
        request.plugin.preview->generatePreview({request.idx, request.img});
    emit previewFinished({request.idx, result});
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
        [=](const PreviewResult& result) { emit previewFinished(result); },
        Qt::QueuedConnection);

    // start the worker thread
    m_thread->start();
    qDebug() << "[THREADING] PluginThread started with" << pluginHandles.size() << "plugins.";
}

void PluginThread::requestPreview(const PluginHandle& plugin,
                                  const PreviewData& request) {
    if (!plugin.hasPreview()) return;  // no preview to compute
    RequestId id = ++m_counter;  // generate a new unique ID for this request
    m_runner->setLatestRequestId(
        id);  // update the latest request ID in the runner

    qDebug() << "[THREADING] Requesting preview with ID" << id << "for plugin" << plugin.name();

    PreviewRequest previewRequest{id, request.index, request.image, plugin};
    QMetaObject::invokeMethod(
        m_runner.get(), "requestPreview",
        Qt::QueuedConnection,
        Q_ARG(PreviewRequest, previewRequest));
}
