#include "progresswidget.h"
#include "ui_progresswidget.h"

#include <QListWidgetItem>
#include <QFont>
#include <QColor>

ProgressWidget::ProgressWidget(QWidget *parent, bool showAbort) :
    QWidget(parent),
    ui(new Ui::ProgressWidget)
{
    ui->setupUi(this);
    ui->pushButton_abort->setVisible(showAbort);
    ui->listWidget_warnings->setWordWrap(true);
    ui->listWidget_warnings->setTextElideMode(Qt::ElideNone);
    ui->listWidget_warnings->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->listWidget_warnings->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

ProgressWidget::~ProgressWidget()
{
    delete ui;
}

void ProgressWidget::slot_displayProgress(int progress, QString currentOperation)
{
    if (progress == -1) {
        // algorithm has no progress update
        ui->progressBar->setMinimum(100);
        ui->progressBar->setMaximum(100);
        ui->progressBar->setValue(100);
    } else {
        // algorithm returns a progress
        ui->progressBar->setValue(progress);
    }

    // change text
    ui->label_currentOperation->setText(currentOperation);
}

void ProgressWidget::slot_displayMessage(QString message)
{
    appendLogEntry(message, false);
}

void ProgressWidget::slot_displayWarning(QString warning)
{
    appendLogEntry(warning, true);
}

void ProgressWidget::slot_clearWarnings()
{
    ui->listWidget_warnings->clear();
}

void ProgressWidget::appendLogEntry(const QString& text, bool isWarning)
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        return;
    }

    const QString entry = isWarning ? tr("⚠ %1").arg(trimmed) : trimmed;

    auto* item = new QListWidgetItem(entry);
    item->setFlags(item->flags() & ~Qt::ItemIsSelectable);

    if (isWarning) {
        QFont f = item->font();
        f.setBold(true);
        item->setFont(f);
        item->setForeground(QBrush(QColor("#B00020")));
        item->setBackground(QBrush(QColor("#FFF3CD")));
    }

    ui->listWidget_warnings->addItem(item);
    ui->listWidget_warnings->scrollToBottom();
}

void ProgressWidget::on_pushButton_abort_clicked()
{
     ui->label_currentOperation->setText(tr("Aborting"));
     ui->progressBar->setValue(100);
    emit sig_abort();
}
