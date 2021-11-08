#include "automaticwidget.h"
#include "ui_automaticwidget.h"



AutomaticWidget::AutomaticWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AutomaticWidget)
{
    ui->setupUi(this);
    //Moving the items with drag and drop must be synched with the automatiExecSettings
    ui->listWidget->setDragDropMode(QAbstractItemView::InternalMove);
    connect(ui->listWidget, &AutomaticListWidget::sig_itemDrop, this, &AutomaticWidget::slot_itemDrop);

}

AutomaticWidget::~AutomaticWidget()
{
    delete ui;
}

void AutomaticWidget::updateSelectedPlugins(QStringList pluginList)
{
    m_idList.clear();
    ui->listWidget->clear();
    int idx = 0;
    for (QString plugin : pluginList) {
       QListWidgetItem* item = new QListWidgetItem;
       item->setText(plugin);
       //Every item gets a QUuid to detect which items were swap with drag and drop
       QUuid qid = QUuid::createUuid();
       m_idList.append(qid);
       QVariant id = QVariant(qid);
       item->setData(Qt::UserRole, id);
       ui->listWidget->insertItem(idx, item);
       idx++;
    }
}

void AutomaticWidget::on_remove_clicked()
{
    QList<QListWidgetItem*> items = ui->listWidget->selectedItems();
    for(QListWidgetItem* item : items) {
        int row = ui->listWidget->row(item);
        delete ui->listWidget->takeItem(ui->listWidget->row(item));
        emit sig_removedPlugin(row);
    }
}

void AutomaticWidget::on_start_clicked()
{
    emit sig_startAutomaticExec();
}

void AutomaticWidget::on_save_clicked()
{
    emit sig_saveConfiguration();
}

void AutomaticWidget::on_load_clicked()
{
    emit sig_loadConfiguration();
}

void AutomaticWidget::on_listWidget_itemDoubleClicked(QListWidgetItem *item)
{
    int selectedRow = ui->listWidget->row(item);
    emit sig_doubleClickedItem(selectedRow);
}


void AutomaticWidget::slot_itemDrop()
{
    QList<QUuid> currentIds;
    int count = ui->listWidget->count();
    for (int i = 0; i < count; i++) {
        QVariant id = ui->listWidget->item(i)->data(Qt::UserRole);
        currentIds.append(id.toUuid());
    }


    QVector<int> order;
    for (int i = 0;i < count; i++) {
        if (i != m_idList.indexOf(currentIds[i])) {
            emit sig_autoOrderChanged(i, m_idList.indexOf(currentIds[i]));
            return;
        }
    }


}


