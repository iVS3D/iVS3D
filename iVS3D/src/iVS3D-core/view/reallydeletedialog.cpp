#include "reallydeletedialog.h"
#include "ui_reallydeletedialog.h"

ReallyDeleteDialog::ReallyDeleteDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ReallyDeleteDialog)
{
    ui->setupUi(this);
}

ReallyDeleteDialog::~ReallyDeleteDialog()
{
    delete ui;
}

void ReallyDeleteDialog::on_pushButton_2_clicked()
{
    accept();
}

void ReallyDeleteDialog::on_pushButton_clicked()
{
    reject();
}
