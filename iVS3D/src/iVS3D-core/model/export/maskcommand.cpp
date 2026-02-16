#include "maskcommand.h"

#include <QRegularExpression>
#include <thread>
#include <chrono>

#include "pluginmanager.h"

std::atomic<int> MaskCommand::s_pendingMasks(0);

MaskCommand::MaskCommand(const MaskRecord* record, Resolution exportResolution,
                         ROI roi, QString folder, PluginThread* pluginThread)
    : m_record(record), m_exportResolution(exportResolution), m_roi(roi),
      m_folder(folder), m_pluginThread(pluginThread) {
    
    // Compute cropped export resolution
    if (!roi.isDefault()) {
        auto rect = roi.cropAsCvRect(exportResolution);
        m_exportSize = rect.size();
    } else {
        m_exportSize = exportResolution.toCvSize();
    }

    // Get the plugin handle once in constructor (expensive operation)
    auto pluginOpt = PluginManager::instance().getPluginByName(m_record->pluginName);
    if (pluginOpt && pluginOpt->hasMask()) {
        m_pluginHandle = *pluginOpt;
        
        // Apply stored settings to the plugin (expensive, do once)
        if (!m_record->pluginSettings.isEmpty()) {
            auto result = m_pluginHandle.base->applySettings(m_record->pluginSettings);
            // Note: Settings application errors are handled during execute() when mask fails
        }
    }
}

std::optional<QString> MaskCommand::execute(ImageContext& ctx) {
    // Check current pending mask count and wait if necessary to avoid overwhelming the plugin thread.
    // MAX_PENDING_MASKS limits the number of concurrent mask generation requests in flight.
    // This prevents excessive queuing and memory usage when the plugin thread is slower than
    // the image processing pipeline.
    int currentPending = s_pendingMasks.load(std::memory_order_acquire);
    while (currentPending >= MAX_PENDING_MASKS) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        currentPending = s_pendingMasks.load(std::memory_order_acquire);
    }

    // Increment the counter atomically to claim a slot
    s_pendingMasks.fetch_add(1, std::memory_order_release);

    // Verify plugin handle was loaded and initialized successfully in constructor
    if (!m_pluginHandle.hasMask()) {
        s_pendingMasks.fetch_sub(1, std::memory_order_release);
        return QString("ERROR: Mask plugin '%1' not available").arg(m_record->pluginName);
    }

    // Prepare the mask generation request with the image and plugin handle.
    // Plugin settings have already been applied in the constructor.
    MaskRequest maskRequest;
    maskRequest.imageIndex = ctx.index;
    maskRequest.image = ctx.image;
    maskRequest.exportDir = QDir(m_folder);
    maskRequest.plugin = m_pluginHandle;
    maskRequest.maskRecordId = m_record->id;

    // Send the async request to the plugin thread
    if (m_pluginThread) {
        m_pluginThread->requestMask(maskRequest);
    } else {
        s_pendingMasks.fetch_sub(1, std::memory_order_release);
        return "ERROR: PluginThread not available for mask generation";
    }

    // Block until the mask arrives or timeout expires (default 30 seconds).
    // The timeout provides a safety net to detect hung plugin operations.
    // If the mask doesn't arrive within this window, an error is returned.
    auto maskResult = waitForMask(ctx.index);
    s_pendingMasks.fetch_sub(1, std::memory_order_release);

    if (!maskResult) {
        return maskResult.error();
    }

    cv::Mat mask = maskResult.value();

    // Resize mask to export resolution if needed (nearest-neighbor to preserve mask integrity)
    if (mask.cols != m_exportSize.width || mask.rows != m_exportSize.height) {
        cv::resize(mask, mask, m_exportSize, 0, 0, cv::INTER_NEAREST);
    }

    // Replace the image context with the generated mask so that downstream processors
    // (e.g., WriteToDiskCommand) will write it to disk
    ctx.image = mask;

    return std::nullopt;
}

void MaskCommand::onMaskFinished(const MaskGenerationResult& result) {
    {
        std::lock_guard<std::mutex> locker(m_resultMutex);
        if (result.success) {
            m_maskResults[result.imageIndex] = result.mask.clone();
        } else {
            // Store error message to be retrieved by execute()
            m_maskErrors[result.imageIndex] = result.errorMessage;
        }
    }
    // Wake up any waiting threads
    m_maskReady.notify_all();
}

tl::expected< cv::Mat, QString > MaskCommand::waitForMask(uint imageIndex, int timeoutMs) {
    std::unique_lock<std::mutex> locker(m_resultMutex);

    // Wait for the specific mask result to arrive via the onMaskFinished callback.
    // timeoutMs: Maximum time to wait in milliseconds (default 30000ms = 30 seconds).
    // This timeout ensures the export doesn't hang indefinitely if a plugin malfunctions.
    // A 30-second timeout is generous for most mask generation operations but catches
    // genuine failures (e.g., plugin crash, GPU error) without excessive wait.
    auto timeout = std::chrono::milliseconds(timeoutMs);
    if (!m_maskReady.wait_for(locker, timeout, [this, imageIndex]() {
        // Wait for either a mask result or an error message to arrive
        return m_maskResults.find(imageIndex) != m_maskResults.end() ||
               m_maskErrors.find(imageIndex) != m_maskErrors.end();
    })) {
        return tl::unexpected(QString("ERROR: Timeout waiting for mask for image at index %1").arg(imageIndex));
    }

    // Check if an error message arrived
    if (m_maskErrors.find(imageIndex) != m_maskErrors.end()) {
        QString error = m_maskErrors[imageIndex];
        m_maskErrors.erase(imageIndex);
        return tl::unexpected(error);
    }

    // Retrieve the mask if it was successfully generated
    cv::Mat result;
    if (m_maskResults.find(imageIndex) != m_maskResults.end()) {
        result = m_maskResults[imageIndex];
        m_maskResults.erase(imageIndex);
    }
    return result;
}