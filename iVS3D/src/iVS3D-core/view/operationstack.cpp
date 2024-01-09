
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

void OperationStack::addEntry(QString name)
{
    QString number = "(" + QString::number(getSize() + 1) + ")  ";
    HistoryItem* newItem = new HistoryItem(ui->listWidget, name);
    t.append(newItem);
    QListWidgetItem* listItem = new QListWidgetItem();
    ui->listWidget->addItem(listItem);
    listItem->setSizeHint(newItem->size());
    ui->listWidget->setItemWidget(listItem, newItem);
    ui->listWidget->setCurrentItem(listItem);
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


void OperationStack::slot_itemClicked(QListWidgetItem *item)
{
    int row = ui->listWidget->row(item);
    emit sig_rowClicked(row);
}

void OperationStack::on_button_clear_clicked()
{
    emit sig_clearClicked();
}

