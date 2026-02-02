#include "maskstackview.h"

#include <QListWidgetItem>
#include <QStyle>
#include <QStyleOptionViewItem>

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
    m_clearButton->setStyleSheet("QPushButton { font-size: 14px; }");
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
    layout->setContentsMargins(4, 2, 4, 2);
    layout->setSpacing(1);

    // First line: Plugin name, resolution, ROI indicator, and remove button
    auto* topRow = new QHBoxLayout();
    topRow->setSpacing(12);
    
    auto* title = new QLabel(record.pluginName);
    
    // Resolution in smaller font
    QString resolutionText = record.workingResolution.toString();
    if (!record.roi.isDefault()) {
        resolutionText += " (ROI)";
    }
    auto* resolutionLabel = new QLabel(resolutionText);
    QFont f = resolutionLabel->font();
    f.setPointSize(f.pointSize() - 2);
    resolutionLabel->setFont(f);

    auto* removeBtn = new QPushButton("✕");
    removeBtn->setToolTip(tr("Remove this mask"));
    removeBtn->setCursor(Qt::PointingHandCursor);
    removeBtn->setMaximumWidth(18);
    removeBtn->setMaximumHeight(18);
    
    removeBtn->setStyleSheet(QString("QPushButton { font-size: 12px; }"));
    
    topRow->addWidget(title);
    topRow->addWidget(resolutionLabel);
    topRow->addStretch();
    topRow->addWidget(removeBtn);

    // Second line: Settings in even smaller font
    auto* detail = new QLabel(formatDetails(record));
    detail->setWordWrap(true);
    detail->setFont(f);

    layout->addLayout(topRow);
    if (!formatDetails(record).isEmpty()) {
        layout->addWidget(detail);
    }

    outWidgets.container = container;
    outWidgets.title = title;
    outWidgets.details = detail;
    outWidgets.removeButton = removeBtn;
    outWidgets.id = record.id;

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
}

void MaskStackView::removeRecordById(int id)
{
    for (int i = 0; i < m_list->count(); ++i) {
        QListWidgetItem* item = m_list->item(i);
        if (item && item->data(Qt::UserRole).toInt() == id) {
            delete m_list->takeItem(i);
            return;
        }
    }
}

void MaskStackView::clearRecords()
{
    m_list->clear();
}
