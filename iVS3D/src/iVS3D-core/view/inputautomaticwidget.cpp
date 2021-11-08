#include "inputautomaticwidget.h"
#include "ui_inputautomaticwidget.h"
#include <QSplitter>

InputAutomaticWidget::InputAutomaticWidget(QWidget *parent, InfoWidget *info, AutomaticWidget *automatic) :
    QWidget(parent),
    ui(new Ui::InputAutomaticWidget)
{
    ui->setupUi(this);
    auto s = new QSplitter(Qt::Vertical, this);
    s->addWidget(info);
    s->addWidget(automatic);
    ui->verticalLayout->addWidget(s);
}



InputAutomaticWidget::~InputAutomaticWidget()
{
    delete ui;
}

