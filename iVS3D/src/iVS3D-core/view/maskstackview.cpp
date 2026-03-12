#include "maskstackview.h"

#include <QListWidgetItem>
#include <QStyle>
#include <QStyleOptionViewItem>
#include <QMap>

namespace {
constexpr int kMaxSettingsLength = 120;  // Truncate long settings strings
}

MaskStackView::MaskStackView(QWidget* parent)
    : QWidget(parent)
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(4);

    // Header with title and clear button
    auto* headerLayout = new QHBoxLayout();
    auto* titleLabel = new QLabel(tr("Masks to export"));
    m_clearButton = new QPushButton("✕");
    m_clearButton->setToolTip(tr("Remove all saved masks"));
    m_clearButton->setMaximumWidth(28);
    m_clearButton->setMaximumHeight(28);
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(m_clearButton);
    mainLayout->addLayout(headerLayout);

    m_list = new QListWidget(this);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    m_list->setSpacing(2);
    m_list->setUniformItemSizes(false);
    m_list->setAlternatingRowColors(true);
    mainLayout->addWidget(m_list, 1);

    connect(m_clearButton, &QPushButton::clicked, this, [this]() {
        emit sig_clearAll();
    });

    connectSelectionSignals();
}

void MaskStackView::connectSelectionSignals()
{
    connect(m_list, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        bool ok = false;
        int id = item->data(Qt::UserRole).toInt(&ok);
        if (ok) {
            emit sig_recordSelected(id);
        }
    });
}

QString MaskStackView::formatDetails(const MaskRecord& record) const
{
    return record.pluginSettingsString;
}

QWidget* MaskStackView::createListItem(const MaskRecord& record, ItemWidgets& outWidgets)
{
    auto* container = new QWidget();
    auto* layout = new QVBoxLayout(container);
    layout->setContentsMargins(3, 1, 3, 1);
    layout->setSpacing(0);

    // First line: Expand button, plugin name, and remove button
    auto* topRow = new QHBoxLayout();
    topRow->setSpacing(6);
    topRow->setContentsMargins(0, 0, 0, 0);
    
    // Expand/collapse button indicator
    auto* expandBtn = new QPushButton("▶");
    expandBtn->setToolTip(tr("Expand/collapse details"));
    expandBtn->setCursor(Qt::PointingHandCursor);
    expandBtn->setMaximumWidth(20);
    expandBtn->setMaximumHeight(20);
    expandBtn->setMinimumWidth(20);
    expandBtn->setMinimumHeight(20);
    expandBtn->setStyleSheet(
        "QPushButton { "
        "  border: none; "
        "  background-color: transparent; "
        "  padding: 0px; "
        "  margin: 0px; "
        "  font-size: 10px; "
        "} "
        "QPushButton:hover { "
        "  background-color: palette(button); "
        "}"
    );
    
    // Title with bold font
    auto* title = new QLabel(record.pluginName);
    QFont titleFont = title->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 1);
    title->setFont(titleFont);
    
    // Resolution and ROI indicator (hidden by default)
    QString resolutionText = record.workingResolution.toString();
    if (!record.roi.isDefault()) {
        resolutionText += " (ROI)";
    }
    auto* resolutionLabel = new QLabel(resolutionText);
    QFont resFont = resolutionLabel->font();
    resFont.setPointSize(resFont.pointSize() - 1);
    resolutionLabel->setFont(resFont);
    resolutionLabel->setStyleSheet("color: palette(mid-light);");
    resolutionLabel->setVisible(false);

    // Remove button with better sizing
    auto* removeBtn = new QPushButton("✕");
    removeBtn->setToolTip(tr("Remove this mask"));
    removeBtn->setCursor(Qt::PointingHandCursor);
    removeBtn->setMaximumWidth(24);
    removeBtn->setMaximumHeight(24);
    removeBtn->setMinimumWidth(24);
    removeBtn->setMinimumHeight(24);
    removeBtn->setStyleSheet(
        "QPushButton { "
        "  border: none; "
        "  background-color: transparent; "
        "  padding: 2px; "
        "  border-radius: 4px; "
        "} "
        "QPushButton:hover { "
        "  background-color: palette(button); "
        "}"
    );
    
    topRow->addWidget(expandBtn, 0);
    topRow->addWidget(title, 1);
    topRow->addWidget(resolutionLabel, 0);
    topRow->addStretch();
    topRow->addWidget(removeBtn, 0);

    // Details section (hidden by default)
    QString detailsText = formatDetails(record);
    auto* detail = new QLabel(detailsText);
    detail->setWordWrap(true);
    QFont detailFont = detail->font();
    detailFont.setPixelSize(int(detailFont.pixelSize() * 0.95));
    detail->setFont(detailFont);
    detail->setStyleSheet("color: palette(mid);");
    detail->setVisible(false);

    layout->addLayout(topRow);
    layout->addWidget(detail);

    outWidgets.container = container;
    outWidgets.title = title;
    outWidgets.resolutionLabel = resolutionLabel;
    outWidgets.details = detail;
    outWidgets.removeButton = removeBtn;
    outWidgets.expandButton = expandBtn;
    outWidgets.isExpanded = false;
    outWidgets.id = record.id;

    // Connect expand button
    connect(expandBtn, &QPushButton::clicked, this, [this, recordId = record.id]() {
        toggleItemExpanded(recordId);
    });

    // Connect remove button
    connect(removeBtn, &QPushButton::clicked, this, [this, record]() {
        emit sig_removeRecord(record.id);
    });

    return container;
}

void MaskStackView::setRecords(const QVector<MaskRecord>& records)
{
    m_list->clear();
    for (const auto& record : records) {
        addRecord(record);
    }
}

void MaskStackView::addRecord(const MaskRecord& record)
{
    auto* item = new QListWidgetItem(m_list);
    item->setData(Qt::UserRole, record.id);

    ItemWidgets widgets;
    QWidget* widget = createListItem(record, widgets);
    item->setSizeHint(widget->sizeHint());
    m_list->addItem(item);
    m_list->setItemWidget(item, widget);

    // Store widgets reference for expansion toggle
    m_itemWidgets[record.id] = widgets;
}

void MaskStackView::removeRecordById(int id)
{
    for (int i = 0; i < m_list->count(); ++i) {
        QListWidgetItem* item = m_list->item(i);
        if (item && item->data(Qt::UserRole).toInt() == id) {
            delete m_list->takeItem(i);
            m_itemWidgets.remove(id);
            return;
        }
    }
}

void MaskStackView::toggleItemExpanded(int id)
{
    if (!m_itemWidgets.contains(id)) {
        return;
    }
    
    ItemWidgets& widgets = m_itemWidgets[id];
    widgets.isExpanded = !widgets.isExpanded;
    
    // Update visibility
    widgets.resolutionLabel->setVisible(widgets.isExpanded);
    widgets.details->setVisible(widgets.isExpanded);
    
    // Update expand button indicator
    widgets.expandButton->setText(widgets.isExpanded ? "▼" : "▶");
    
    // Recalculate size hint for list item
    for (int i = 0; i < m_list->count(); ++i) {
        QListWidgetItem* item = m_list->item(i);
        if (item && item->data(Qt::UserRole).toInt() == id) {
            item->setSizeHint(widgets.container->sizeHint());
            break;
        }
    }
}

void MaskStackView::clearRecords()
{
    m_list->clear();
    m_itemWidgets.clear();
}
