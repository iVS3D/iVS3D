#pragma once

#include <QObject>
#include <QVector>
#include <QString>
#include <QMap>
#include <QVariant>
#include <QRect>
#include <QSize>
#include <memory>

#include "resolution.h"
#include "roi.h"

/**
 * @struct MaskRecord
 * @brief Contains all information needed to generate a mask at export time.
 *
 * This record stores the complete state of a mask generation configuration,
 * allowing it to be restored later during export.
 *
 * @author Dominik Wüst
 * @date January 2026
 */
struct MaskRecord {
    /**
     * @brief Name of the IMask plugin used for this mask
     */
    QString pluginName;

    /**
     * @brief Settings from the plugin that were active when mask was added
     * Serializable QMap that can be passed to plugin->setSettings()
     */
    QMap<QString, QVariant> pluginSettings;

    QString pluginSettingsString;

    /**
     * @brief Working resolution at the time the mask was configured
     * The images will be resized to this resolution during mask generation
     */
    Resolution workingResolution;

    /**
     * @brief Region of Interest (ROI) at the time the mask was configured
     * The images will be cropped to this ROI after resizing to working resolution
     */
    ROI roi;

    /**
     * @brief Unique identifier for this record (index in stack)
     */
    int id;

    /**
     * @brief Human-readable description for UI display
     * @return Formatted string showing plugin name and configuration
     */
    QString getDisplayName() const {
        return QString("[%1] %2 (%3x%4)")
            .arg(id)
            .arg(pluginName)
            .arg(workingResolution.toString());
    }
};

/**
 * @class MaskStack
 * @brief Manages a stack of mask generation records for later export.
 *
 * This class maintains a list of mask generation configurations. Each entry
 * represents a call to "Add Mask" by the user with specific plugin settings.
 * At export time, the application will iterate through this stack and generate
 * masks for each image using the stored configurations.
 *
 * Features:
 * - Add new mask records when user clicks "Add Mask"
 * - Remove individual records
 * - Clear entire stack
 * - Retrieve records by index
 * - Query size and empty status
 * - Emits signals when stack changes (for UI synchronization)
 *
 * @author Dominik Wüst
 * @date January 2026
 */
class MaskStack : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Construct an empty mask stack
     * @param parent The parent QObject
     */
    explicit MaskStack(QObject* parent = nullptr);

    /**
     * @brief Add a new mask record to the stack
     * @param record The MaskRecord to add
     * @note Emits sig_recordAdded with the new record
     * @note Emits sig_stackChanged
     */
    void addRecord(const MaskRecord& record);

    /**
     * @brief Remove a record at the given index
     * @param index The index of the record to remove (0-based)
     * @return true if removal was successful, false if index is out of range
     * @note Emits sig_recordRemoved with the removed record if successful
     * @note Emits sig_stackChanged if successful
     */
    bool removeRecord(int index);

    /**
     * @brief Remove a record by its ID
     * @param id The ID of the record to remove
     * @return true if removal was successful, false if ID not found
     * @note Emits sig_recordRemoved with the removed record if successful
     * @note Emits sig_stackChanged if successful
     */
    bool removeRecordById(int id);

    /**
     * @brief Clear all records from the stack
     * @note Emits sig_stackCleared
     * @note Emits sig_stackChanged
     */
    void clear();

    /**
     * @brief Get the number of records in the stack
     * @return Number of records
     */
    int size() const;

    /**
     * @brief Check if the stack is empty
     * @return true if stack contains no records, false otherwise
     */
    bool isEmpty() const;

    /**
     * @brief Get a record by index
     * @param index The index of the record (0-based)
     * @return Pointer to the record, or nullptr if index is out of range
     */
    const MaskRecord* getRecord(int index) const;

    /**
     * @brief Get all records in the stack
     * @return A copy of the internal records vector
     */
    QVector<MaskRecord> getAllRecords() const;

    /**
     * @brief Get a record by its ID
     * @param id The ID of the record
     * @return Pointer to the record, or nullptr if ID not found
     */
    const MaskRecord* getRecordById(int id) const;

signals:
    /**
     * @brief Emitted when a new record is added to the stack
     * @param record The newly added record
     */
    void sig_recordAdded(const MaskRecord& record);

    /**
     * @brief Emitted when a record is removed from the stack
     * @param record The removed record
     */
    void sig_recordRemoved(const MaskRecord& record);

    /**
     * @brief Emitted when the entire stack is cleared
     */
    void sig_stackCleared();

    /**
     * @brief Emitted whenever the stack contents change
     * (after add, remove, or clear operations)
     */
    void sig_stackChanged();

private:
    QVector<MaskRecord> m_records;  // All mask records in order
    int m_nextId = 0;               // Counter for unique record IDs
};
