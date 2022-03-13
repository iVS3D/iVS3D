#include "progressdialog.h"
#include "ui_progressdialog.h"

ProgressDialog::ProgressDialog(QWidget *parent, bool showAbort) :
    QDialog(parent),
    ui(new Ui::ProgressDialog)
{
    ui->setupUi(this);

    if(showAbort){
        QObject::connect(ui->bt_cancel, &QPushButton::clicked, this, &ProgressDialog::slot_btCancel);
    } else {
        ui->bt_cancel->setVisible(false);
    }
    setSizeGripEnabled(false);
    //adjustSize();
    setWindowFlags(windowFlags() & ~Qt::WindowCloseButtonHint);
}

ProgressDialog::~ProgressDialog()
{
    delete ui;
}

void ProgressDialog::slot_displayProgress(int progress, QString currentOperation)
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
    ui->label_currOperation->setText(currentOperation);
}

void ProgressDialog::slot_displayMessage(QString message)
{
    ui->label_currOperation->setText(message);
}

void ProgressDialog::slot_btCancel()
{    
    ui->label_currOperation->setText("Aborting");
    ui->progressBar->setValue(100);
    ui->bt_cancel->setEnabled(false);
    setEnabled(false);
    emit sig_abort();
}
