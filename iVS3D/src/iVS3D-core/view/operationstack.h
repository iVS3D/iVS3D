#ifndef OPERATIONSTACK_H
#define OPERATIONSTACK_H

#include <QWidget>
#include <QListWidget>
#include "ui_operationstack.h"
#include "historyitem.h"

namespace Ui {
class OperationStack;
}

class OperationStack : public QWidget
{
    Q_OBJECT

public:
    explicit OperationStack(QWidget *parent = nullptr);
    ~OperationStack();

    void addEntry(QString name);

    void clear();

    int getSize();
    /**
     * @brief removeItemsAfter removes all items after the given row
     * @param row
     */
    void removeItemsAfter(int row);


    void selectItem(int row);

    QString getItemString(int row);


public slots:

    void slot_itemClicked(QListWidgetItem *item);

signals:

    void sig_rowClicked(int row);

    void sig_clearClicked();

private slots:
    void on_button_clear_clicked();

private:
    Ui::OperationStack *ui;
    QList<HistoryItem*> t;
};

#endif // OPERATIONSTACK_H
