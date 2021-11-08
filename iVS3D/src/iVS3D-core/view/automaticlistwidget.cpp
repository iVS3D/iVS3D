#include "automaticlistwidget.h"

AutomaticListWidget::AutomaticListWidget(QWidget *parent) : QListWidget(parent)
{

}

void AutomaticListWidget::dropEvent(QDropEvent *event)
{
    QListWidget::dropEvent(event);
    emit sig_itemDrop();
}
