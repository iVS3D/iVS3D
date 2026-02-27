#ifndef MASKCOMMAND_H
#define MASKCOMMAND_H

#include <QDir>
#include <QMutex>
#include <QWaitCondition>
#include <QString>
#include <atomic>
#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <tl/expected.hpp>

#include "imageprocessor.h"
#include "maskstack.h"
#include "pluginthread.h"

/**
 * @class MaskCommand
 * @brief Generates masks asynchronously via PluginThread and writes them to disk.
 *
 * Unlike TransformCommand which processes synchronously, MaskCommand:
 * - Sends async mask generation requests to PluginThread
 * - Blocks in execute() until the mask for the current image arrives
 * - Tracks pending mask count to prevent overwhelming the plugin thread
 * - Writes generated masks to disk
 *
 * @author Dominik Wüst
 * @date February 2026
 */
class MaskCommand : public ImageCommand {

public:
    /**
     * @brief Constructs a MaskCommand for async mask generation.
     * @param record The MaskRecord containing plugin name and settings
     * @param exportResolution The resolution for the export
     * @param roi The region of interest
     * @param folder The output folder for masks
     * @param pluginThread The PluginThread for async processing
     */
    MaskCommand(const MaskRecord* record, Resolution exportResolution, ROI roi,
                QString folder, PluginThread* pluginThread);

    /**
     * @brief Requests mask generation and waits for result before writing to disk.
     * @param ctx The image context
     * @return Error message if any, std::nullopt on success
     */
    std::optional<QString> execute(ImageContext& ctx) override;

    /**
     * @brief Receives completed mask from PluginThread (to be called by external connection).
     * @param result The mask generation result
     */
    void onMaskFinished(const MaskGenerationResult& result);

private:
    const MaskRecord* m_record;
    Resolution m_exportResolution;
    ROI m_roi;
    QString m_folder;
    PluginThread* m_pluginThread;
    QString m_pluginName;
    cv::Size m_exportSize;
    bool m_initialized = false;

    // Async mask handling
    // s_pendingMasks: Tracks the number of mask generation requests currently in flight.
    //                 This prevents the export pipeline from overwhelming the plugin thread
    //                 with requests faster than they can be processed.
    // MAX_PENDING_MASKS: Maximum allowed concurrent mask requests (3).
    //                    When this limit is reached, execute() blocks until masks complete.
    //                    This value balances latency (low = more blocking) vs. throughput.
    //                    Typical GPU operations take 100-500ms, so 3 concurrent requests
    //                    keeps a reasonable buffer without excessive memory usage.
    static std::atomic<int> s_pendingMasks;
    static constexpr int MAX_PENDING_MASKS = 3;  // Limit concurrent mask requests

    // Thread-safe mask result storage (using std::mutex for condition_variable compatibility)
    std::mutex m_resultMutex;
    std::map<uint, cv::Mat> m_maskResults;          // imageIndex -> mask
    std::map<uint, QString> m_maskErrors;           // imageIndex -> error message (if any)
    std::condition_variable m_maskReady;             // Signal when a mask arrives

    /**
     * @brief Waits for a specific mask to arrive from PluginThread.
     * 
     * Blocks the calling thread until the mask for the specified image arrives
     * via onMaskFinished() callback, or until the timeout expires.
     * 
     * @param imageIndex The image index to wait for
     * @param timeoutMs Timeout in milliseconds (default 30000ms = 30 seconds).
     *                   This provides a safety window to detect hung plugin operations.
     *                   30 seconds is sufficient for most GPU and CPU-based mask
     *                   generation but will catch genuine failures (crashes, GPU errors).
     * @return Expected with the mask on success, or unexpected with error message on failure
     */
    tl::expected< cv::Mat, QString > waitForMask(uint imageIndex, int timeoutMs = 30000);
};

#endif  // MASKCOMMAND_H
