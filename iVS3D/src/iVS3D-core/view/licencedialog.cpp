#include "licencedialog.h"
#include "ui_licencedialog.h"

LicenceDialog::LicenceDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LicenceDialog)
{
    ui->setupUi(this);
}

LicenceDialog::~LicenceDialog()
{
    delete ui;
}
