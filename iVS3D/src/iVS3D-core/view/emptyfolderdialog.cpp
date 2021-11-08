#include "emptyfolderdialog.h"
#include "ui_emptyfolderdialog.h"

EmptyFolderDialog::EmptyFolderDialog(QWidget *parent, const QString &path) :
    QDialog(parent),
    ui(new Ui::EmptyFolderDialog)
{
    ui->setupUi(this);
    ui->outputLabel->setText("Selected folder: \"" + path + "\"");
}

EmptyFolderDialog::~EmptyFolderDialog()
{
    delete ui;
}

void EmptyFolderDialog::on_YesButton_clicked()
{
    done(1);

}

void EmptyFolderDialog::on_IgnoreButton_clicked()
{
    done(2);
}

void EmptyFolderDialog::on_AbortButton_clicked()
{
    done(0);
}
