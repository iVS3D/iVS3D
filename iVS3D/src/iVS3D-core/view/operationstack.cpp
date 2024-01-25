
#include "operationstack.h"


OperationStack::OperationStack(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OperationStack)
{
    ui->setupUi(this);
    connect(ui->listWidget, &QListWidget::itemClicked, this, &OperationStack::slot_itemClicked);

}

OperationStack::~OperationStack()
{
    delete ui;
}

void OperationStack::addEntry(QString name, QString data)
{
    QString number = "(" + QString::number(getSize()) + ")  ";
    HistoryItem* newItem = new HistoryItem(ui->listWidget, name, data, getSize());
    t.append(newItem);
    QListWidgetItem* listItem = new QListWidgetItem();
    listItem->setData(Qt::UserRole, name);
    ui->listWidget->addItem(listItem);
    listItem->setSizeHint(newItem->size());
    ui->listWidget->setItemWidget(listItem, newItem);
    ui->listWidget->setCurrentItem(listItem);
    connect(newItem, &HistoryItem::sig_editClicked, this , &OperationStack::slot_editClicked);
}

void OperationStack::clear()
{
    removeItemsAfter(0); // keep the first element which is import
}

int OperationStack::getSize()
{
    return ui->listWidget->count();
}

void OperationStack::removeItemsAfter(int row)
{
    while (row < getSize()) {
        auto item = ui->listWidget->takeItem(getSize() - 1);
        HistoryItem* toDelete = t.takeLast();
        delete toDelete;
        delete item;
    }
}

void OperationStack::selectItem(int row)
{
    auto selected_item = ui->listWidget->item(row);
    ui->listWidget->setCurrentItem(selected_item);
}

QString OperationStack::getItemString(int row)
{
    return ui->listWidget->item(row)->data(Qt::UserRole).toString();
}

void OperationStack::exitEditMode(int row)
{
    t.at(row)->exitEditNode();
}

void OperationStack::clearSelection()
{
    ui->listWidget->selectionModel()->clear();
}

void OperationStack::pendingAfter(int row)
{
    for(int i = row; i < getSize(); ++i) {
        t.at(i)->toPending();
    }
}


void OperationStack::slot_itemClicked(QListWidgetItem *item)
{
    int row = ui->listWidget->row(item);
    emit sig_rowClicked(row);
}

void OperationStack::slot_editClicked(int index)
{
    emit sig_editClicked(index);
}

void OperationStack::on_button_clear_clicked()
{
    emit sig_clearClicked();
}

