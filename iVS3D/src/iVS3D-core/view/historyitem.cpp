#include "historyitem.h"
#include "ui_historyitem.h"

HistoryItem::HistoryItem(QWidget *parent, QString name, QString data, int index)
    : QWidget(parent)
    , ui(new Ui::HistoryItem), m_index(index)
{
    ui->setupUi(this);
    ui->label_action->setText(name);
    ui->label_data->setText(data);
}

HistoryItem::~HistoryItem()
{
    delete ui;
}

void HistoryItem::exitEditNode()
{
    ui->btn_edit->setText("To edit mode");
}

void HistoryItem::toPending()
{
    ui->btn_edit->setText("Pending");
    ui->btn_edit->repaint();
}

void HistoryItem::on_btn_edit_clicked()
{
    if (ui->btn_edit->text().compare("To edit mode") == 0) {
        ui->btn_edit->setText("Exit edit mode");
    }
    else {
        exitEditNode();
    }
    emit sig_editClicked(m_index);
}


