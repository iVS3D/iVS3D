#include "pluginthread.h"

#include <QMetaObject>

PluginRunner::PluginRunner(const QHash<QString, PluginHandle>& plugins,
                           QObject* parent)
    : QObject(parent), m_plugins(plugins) {}

const PluginHandle* PluginRunner::findPlugin(const QString& pluginName) const {
    auto it = m_plugins.find(pluginName);
    if (it == m_plugins.end()) {
        return nullptr;
    }
    return &it.value();
}

bool PluginRunner::setActivePlugin(const QString& pluginName) {
    if (pluginName == m_activePluginName) {
        return true;
    }

    const PluginHandle* oldPlugin = findPlugin(m_activePluginName);
    if (oldPlugin && oldPlugin->base) {
        disconnect(oldPlugin->base, nullptr, this, nullptr);
        oldPlugin->base->deactivate();
    }

    const PluginHandle* newPlugin = findPlugin(pluginName);
    if (!newPlugin || !newPlugin->base) {
        m_activePluginName.clear();
        return false;
    }

    connect(newPlugin->base, &IBase::updatePreview, this,
            &PluginRunner::activePluginUpdatePreview, Qt::DirectConnection);
    connect(newPlugin->base, &IBase::updateProgress, this,
            &PluginRunner::activePluginUpdateProgress, Qt::DirectConnection);
    connect(newPlugin->base, &IBase::updateSelectedImages, this,
            &PluginRunner::activePluginUpdateSelectedImages,
            Qt::DirectConnection);

    newPlugin->base->activate();
    m_activePluginName = pluginName;
    return true;
}

void PluginRunner::clearActivePlugin() {
    const PluginHandle* oldPlugin = findPlugin(m_activePluginName);
    if (oldPlugin && oldPlugin->base) {
        disconnect(oldPlugin->base, nullptr, this, nullptr);
        oldPlugin->base->deactivate();
    }
    m_activePluginName.clear();
}

ApplySettingsResult PluginRunner::applyPluginSettings(
    const QString& pluginName, const QMap<QString, QVariant>& settings) {
    const PluginHandle* plugin = findPlugin(pluginName);
    if (!plugin || !plugin->base) {
        return tl::unexpected(
            Error(ErrorCode::InvalidInput,
                  QString("Plugin '%1' not found").arg(pluginName)));
    }
    return plugin->base->applySettings(settings);
}

QMap<QString, QVariant> PluginRunner::getPluginSettings(
    const QString& pluginName) const {
    const PluginHandle* plugin = findPlugin(pluginName);
    if (!plugin || !plugin->base) {
        return {};
    }
    return plugin->base->getSettings();
}

QString PluginRunner::getPluginSettingsString(const QString& pluginName) const {
    const PluginHandle* plugin = findPlugin(pluginName);
    if (!plugin || !plugin->base) {
        return {};
    }
    return plugin->base->getSettingsString();
}

void PluginRunner::enableCuda(bool useCuda) {
    for (const auto& handle : m_plugins) {
        if (handle.base) {
            handle.base->onCudaChanged(useCuda);
        }
    }
}

void PluginRunner::onInputLoaded(Reader* reader) {
    for (const auto& handle : m_plugins) {
        if (!handle.base) {
            continue;
        }
        handle.base->onInputLoaded({reader}).map_error([&](const Error& err) {
            printf("[PluginThread] Error loading input for plugin %s: %s\n",
                   handle.name().toStdString().c_str(),
                   err.message.toStdString().c_str());
        });
    }
}

void PluginRunner::resetSelectionCancelFlag() { m_selectionCancelFlag = false; }

void PluginRunner::cancelSelectionDirect() { m_selectionCancelFlag = true; }

void PluginRunner::onMetaDataLoaded(const InputMetaData& inputMetaData) {
    for (const auto& handle : m_plugins) {
        if (!handle.base) {
            continue;
        }
        handle.base
            ->onMetaDataLoaded(inputMetaData)
            .map_error([&](const Error& err) {
                printf(
                    "[PluginThread] Error loading metadata for plugin %s: "
                    "%s\n",
                    handle.name().toStdString().c_str(),
                    err.message.toStdString().c_str());
            });
    }
}

void PluginRunner::onIndexChanged(uint index) {
    for (const auto& handle : m_plugins) {
        if (handle.base) {
            handle.base->onIndexChanged(index);
        }
    }
}

void PluginRunner::onSelectedImagesChanged(
    const std::vector<uint>& selectedImages) {
    for (const auto& handle : m_plugins) {
        if (handle.base) {
            handle.base->onSelectedImagesChanged(selectedImages);
        }
    }
}

void PluginRunner::requestPreview(const PreviewRequest& request) {
    const PluginHandle* plugin = findPlugin(request.pluginName);
    if (!plugin || !plugin->hasPreview()) return;  // no preview to compute
    if (request.id != m_latestRequest.loadRelaxed()) {
        return;  // discard outdated request
    }
    emit previewStarted(
        request.id);  // emit signal that preview generation has started
    VisualizationResult result =
        plugin->preview->generatePreview({request.idx, request.img});
    emit previewFinished({request.idx, result});
}

void PluginRunner::requestMask(const MaskRequest& request) {
    const PluginHandle* plugin = findPlugin(request.pluginName);
    if (!plugin || !plugin->hasMask()) {
        MaskGenerationResult result;
        result.imageIndex = request.imageIndex;
        result.mask = cv::Mat();
        result.maskRecordId = request.maskRecordId;
        result.success = false;
        result.errorMessage = "Plugin does not support mask generation";
        emit maskFinished(result);
        return;
    }

    const auto maskResult = plugin->mask->generateMask(
        {request.imageIndex, request.image, request.exportDir});

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

void PluginRunner::requestSelection(const SelectionRequest& request) {
    const PluginHandle* plugin = findPlugin(request.pluginName);
    if (!plugin || !plugin->hasSelection()) {
        SelectionResultData result;
        result.id = request.id;
        result.pluginName = request.pluginName;
        result.success = false;
        result.cancelled = false;
        result.errorMessage = "Plugin does not support selection";
        emit selectionFinished(result);
        return;
    }

    const auto selectionResult =
        plugin->selection->selectImages(request.data, m_selectionCancelFlag);

    SelectionResultData result;
    result.id = request.id;
    result.pluginName = request.pluginName;
    result.cancelled = m_selectionCancelFlag;
    if (selectionResult) {
        result.success = true;
        result.selectedIndices = selectionResult.value();
    } else {
        result.success = false;
        result.errorMessage = selectionResult.error().message;
    }
    emit selectionFinished(result);
}

void PluginRunner::cancelSelection() { m_selectionCancelFlag = true; }

PluginThread::PluginThread(const QHash<QString, PluginHandle>& pluginHandles,
                           QObject* parent)
    : QObject(parent), m_plugins(pluginHandles) {
    m_thread = std::make_unique<QThread>(this);
    m_runner = std::make_unique<PluginRunner>(m_plugins, nullptr);

    // Move all plugins and the runner to the worker thread
    for (const PluginHandle& handle : m_plugins) {
        if (handle.base) {
            handle.base->moveToThread(m_thread.get());
        }
    }
    m_runner->moveToThread(m_thread.get());

    // forward the previewFinished signal from the runner to the PluginThread's
    // own signal
    connect(
        m_runner.get(), &PluginRunner::previewFinished, this,
        [=](const PreviewResult& result) {
            emit previewStateChanged(
                PreviewState::Idle);  // update state to idle when preview is
                                      // finished
            emit previewFinished(result);
        },
        Qt::QueuedConnection);
    connect(
        m_runner.get(), &PluginRunner::previewStarted, this,
        [=](const RequestId& id) {
            emit previewStateChanged(
                PreviewState::Processing);  // update state to processing when
                                            // preview starts
        },
        Qt::QueuedConnection);

    // Forward mask finished signal from runner to PluginThread
    connect(
        m_runner.get(), &PluginRunner::maskFinished, this,
        [=](const MaskGenerationResult& result) { emit maskFinished(result); },
        Qt::QueuedConnection);

    connect(
        m_runner.get(), &PluginRunner::selectionFinished, this,
        [=](const SelectionResultData& result) {
            emit selectionFinished(result);
        },
        Qt::QueuedConnection);

    connect(m_runner.get(), &PluginRunner::activePluginUpdatePreview, this,
            &PluginThread::activePluginUpdatePreview, Qt::QueuedConnection);
    connect(m_runner.get(), &PluginRunner::activePluginUpdateProgress, this,
            &PluginThread::activePluginUpdateProgress, Qt::QueuedConnection);
    connect(m_runner.get(), &PluginRunner::activePluginUpdateSelectedImages,
            this, &PluginThread::activePluginUpdateSelectedImages,
            Qt::QueuedConnection);

        connect(this, &PluginThread::notifyMetaDataLoaded, m_runner.get(),
            &PluginRunner::onMetaDataLoaded, Qt::QueuedConnection);
        connect(this, &PluginThread::notifyIndexChanged, m_runner.get(),
            &PluginRunner::onIndexChanged, Qt::QueuedConnection);
        connect(this, &PluginThread::notifySelectedImagesChanged, m_runner.get(),
            &PluginRunner::onSelectedImagesChanged, Qt::QueuedConnection);

    // start the worker thread
    m_thread->start();
}

PluginThread::~PluginThread() {
    if (m_thread && m_thread->isRunning()) {
        // Move objects back to the current thread from within their own thread
        for (const PluginHandle& handle : m_plugins) {
            if (!handle.base) {
                continue;
            }
            QMetaObject::invokeMethod(
                handle.base,
                [this, base = handle.base]() {
                    base->moveToThread(this->thread());
                },
                Qt::BlockingQueuedConnection);
        }

        if (m_runner) {
            QMetaObject::invokeMethod(
                m_runner.get(),
                [this]() { m_runner->moveToThread(this->thread()); },
                Qt::BlockingQueuedConnection);
        }

        // Signal the thread to stop
        m_thread->quit();

        // Wait for the thread to finish before destroying it
        m_thread->wait(5000);  // 5 second timeout
    } else {
        // Thread already stopped, move directly
        for (const PluginHandle& handle : m_plugins) {
            if (handle.base) {
                handle.base->moveToThread(this->thread());
            }
        }
        if (m_runner) {
            m_runner->moveToThread(this->thread());
        }
    }
}

void PluginThread::requestPreview(const QString& pluginName,
                                  const PreviewData& request) {
    const auto pluginIt = m_plugins.find(pluginName);
    if (pluginIt == m_plugins.end() || !pluginIt->hasPreview())
        return;  // no preview to compute

    RequestId id = ++m_counter;  // generate a new unique ID for this request
    m_runner->setLatestRequestId(
        id);  // update the latest request ID in the runner

    PreviewRequest previewRequest{id, request.index, request.image, pluginName};
    QMetaObject::invokeMethod(m_runner.get(), "requestPreview",
                              Qt::QueuedConnection,
                              Q_ARG(PreviewRequest, previewRequest));
}

void PluginThread::requestMask(const MaskRequest& request) {
    QMetaObject::invokeMethod(m_runner.get(), "requestMask",
                              Qt::QueuedConnection,
                              Q_ARG(MaskRequest, request));
}

void PluginThread::requestSelection(const QString& pluginName,
                                    const SelectionData& data) {
    RequestId id = ++m_counter;
    m_runner->resetSelectionCancelFlag();
    SelectionRequest selectionRequest{id, pluginName, data};
    QMetaObject::invokeMethod(m_runner.get(), "requestSelection",
                              Qt::QueuedConnection,
                              Q_ARG(SelectionRequest, selectionRequest));
}

void PluginThread::cancelSelection() {
    m_runner->cancelSelectionDirect();
}

bool PluginThread::setActivePlugin(const QString& pluginName) {
    bool success = false;
    QMetaObject::invokeMethod(
        m_runner.get(),
        [this, &success, pluginName]() {
            success = m_runner->setActivePlugin(pluginName);
        },
        Qt::BlockingQueuedConnection);
    return success;
}

void PluginThread::clearActivePlugin() {
    QMetaObject::invokeMethod(
        m_runner.get(), [this]() { m_runner->clearActivePlugin(); },
        Qt::BlockingQueuedConnection);
}

ApplySettingsResult PluginThread::applyPluginSettings(
    const QString& pluginName, const QMap<QString, QVariant>& settings) {
    ApplySettingsResult result = {};
    QMetaObject::invokeMethod(
        m_runner.get(),
        [this, &result, pluginName, settings]() {
            result = m_runner->applyPluginSettings(pluginName, settings);
        },
        Qt::BlockingQueuedConnection);
    return result;
}

QMap<QString, QVariant> PluginThread::getPluginSettings(
    const QString& pluginName) const {
    QMap<QString, QVariant> settings;
    QMetaObject::invokeMethod(
        m_runner.get(),
        [this, &settings, pluginName]() {
            settings = m_runner->getPluginSettings(pluginName);
        },
        Qt::BlockingQueuedConnection);
    return settings;
}

QString PluginThread::getPluginSettingsString(const QString& pluginName) const {
    QString settingsString;
    QMetaObject::invokeMethod(
        m_runner.get(),
        [this, &settingsString, pluginName]() {
            settingsString = m_runner->getPluginSettingsString(pluginName);
        },
        Qt::BlockingQueuedConnection);
    return settingsString;
}

void PluginThread::enableCuda(bool useCuda) {
    QMetaObject::invokeMethod(
        m_runner.get(), [this, useCuda]() { m_runner->enableCuda(useCuda); },
        Qt::QueuedConnection);
}

void PluginThread::onInputLoaded(Reader* reader) {
    QMetaObject::invokeMethod(
        m_runner.get(), [this, reader]() { m_runner->onInputLoaded(reader); },
        Qt::QueuedConnection);
}

void PluginThread::onMetaDataLoaded(const InputMetaData& inputMetaData) {
    emit notifyMetaDataLoaded(inputMetaData);
}

void PluginThread::onIndexChanged(uint index) { emit notifyIndexChanged(index); }

void PluginThread::onSelectedImagesChanged(
    const std::vector<uint>& selectedImages) {
    emit notifySelectedImagesChanged(selectedImages);
}
