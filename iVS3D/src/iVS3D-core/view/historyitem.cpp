#include "historyitem.h"
#include "ui_historyitem.h"

HistoryItem::HistoryItem(QWidget *parent, QString name)
    : QWidget(parent)
    , ui(new Ui::HistoryItem)
{
    ui->setupUi(this);
    ui->label->setText(name);
    ui->pushButton->setText("EDIT");
}

HistoryItem::~HistoryItem()
{
    delete ui;
}
