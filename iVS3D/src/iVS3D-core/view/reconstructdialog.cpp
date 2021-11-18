#include "reconstructdialog.h"
#include "ui_reconstructdialog.h"

ReconstructDialog::ReconstructDialog(QWidget *parent)  :
    QDialog(parent),
    ui(new Ui::ReconstructDialog)
{
    ui->setupUi(this);
}

ReconstructDialog::~ReconstructDialog()
{
    delete ui;
}

ReconstructDialog::ReconstructDialog(QWidget *parent, QStringList exportnameList, QStringList reconstructtoolList)  :
    QDialog(parent),
    ui(new Ui::ReconstructDialog)
{
    ui->setupUi(this);

    this->ui->selectExportComboBox->addItems(exportnameList);
    this->ui->selectReconstructtoolComboBox->addItems(reconstructtoolList);
}

QString ReconstructDialog::getExportName()
{
    return this->ui->selectExportComboBox->currentText();
}

QString ReconstructDialog::getReconstructtool()
{
    return this->ui->selectReconstructtoolComboBox->currentText();
}

QString ReconstructDialog::getStartArguments()
{
    return this->ui->startArgumentsLineEdit->text();
}

bool ReconstructDialog::getCreateProject()
{
    return this->ui->createprojectfileCheckBox->isChecked();
}


void ReconstructDialog::on_cancelbutton_clicked()
{
    emit sig_cancel();
    QDialog::reject();
}

void ReconstructDialog::on_reconstructButton_clicked()
{
    emit sig_reconstruct();
    QDialog::accept();
}
