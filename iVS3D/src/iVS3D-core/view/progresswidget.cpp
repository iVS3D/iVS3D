#include "progresswidget.h"
#include "ui_progresswidget.h"

ProgressWidget::ProgressWidget(QWidget *parent, bool showAbort) :
    QWidget(parent),
    ui(new Ui::ProgressWidget)
{
    ui->setupUi(this);
    ui->pushButton_abort->setVisible(showAbort);
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
    ui->label_message->setText(message);
}

void ProgressWidget::on_pushButton_abort_clicked()
{
    emit sig_abort();
}
