#include "about.h"
#include "ui_about.h"

#ifndef IVS3D_VER
#define IVS3D_VER unknown
#endif

#ifndef IVS3D_DAT
#define IVS3D_DAT unknown
#endif

#define QUOTE(str) _QUOTE(str)
#define _QUOTE(str) #str

About::About(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::About)
{
    ui->setupUi(this);
    ui->l_date->setText(tr("build date: ") + QString(QUOTE(IVS3D_DAT)));
    ui->l_version->setText(tr("build version: ") + QString(QUOTE(IVS3D_VER)));
}

About::~About()
{
    delete ui;
}
