#pragma once

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QFrame>
#include <QSpacerItem>
#include <QMap>

#include "maskstack.h"

/**
 * @class MaskStackView
 * @brief UI widget to display and manage a stack of mask configurations.
 *
 * Features:
 * - Shows plugin name, resolution, ROI (if not default), and settings summary
 * - Per-entry remove buttons
 * - Clear-all button
 * - Selection emits a signal for restoring state
 * - Setters to refresh the entire list or update incrementally
 */
class MaskStackView : public QWidget {
    Q_OBJECT
public:
    explicit MaskStackView(QWidget* parent = nullptr);
    ~MaskStackView() override = default;

    /**
     * @brief Replace the current list with the given records.
     */
    void setRecords(const QVector<MaskRecord>& records);

    /**
     * @brief Append a single record to the list.
     */
    void addRecord(const MaskRecord& record);

    /**
     * @brief Remove a record by ID.
     */
    void removeRecordById(int id);

    /**
     * @brief Clear all records from the list.
     */
    void clearRecords();

signals:
    /**
     * @brief Request to remove a record with given id.
     */
    void sig_removeRecord(int id);

    /**
     * @brief Request to clear all records.
     */
    void sig_clearAll();

    /**
     * @brief Emitted when a record is selected (clicked) by the user.
     */
    void sig_recordSelected(int id);

private:
    struct ItemWidgets {
        QWidget* container = nullptr;
        QLabel* title = nullptr;
        QLabel* resolutionLabel = nullptr;
        QLabel* details = nullptr;
        QPushButton* removeButton = nullptr;
        QPushButton* expandButton = nullptr;
        bool isExpanded = false;
        int id = -1;
    };

    QListWidget* m_list = nullptr;
    QPushButton* m_clearButton = nullptr;
    QMap<int, ItemWidgets> m_itemWidgets;

    // Helpers
    QWidget* createListItem(const MaskRecord& record, ItemWidgets& outWidgets);
    QString formatDetails(const MaskRecord& record) const;
    void toggleItemExpanded(int id);

    void connectSelectionSignals();
};
