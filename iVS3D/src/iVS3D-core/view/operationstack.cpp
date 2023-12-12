#include "operationstack.h"
#include "ui_operationstack.h"

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
    QListWidgetItem* newItem = new QListWidgetItem(number + name);
    ui->listWidget->addItem(newItem);
    ui->listWidget->setCurrentItem(newItem);
}

void OperationStack::clear()
{
    ui->listWidget->clear();
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

void OperationStack::slot_itemClicked(QListWidgetItem *item)
{
    int row = ui->listWidget->row(item);
    emit sig_rowClicked(row);
}
