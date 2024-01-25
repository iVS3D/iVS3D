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

    void addEntry(QString name, QString data = "");

    void clear();

    int getSize();
    /**
     * @brief removeItemsAfter removes all items after the given row
     * @param row
     */
    void removeItemsAfter(int row);


    void selectItem(int row);

    QString getItemString(int row);

    void exitEditMode(int row);

    void clearSelection();

    void pendingAfter(int row);


public slots:

    void slot_itemClicked(QListWidgetItem *item);

    void slot_editClicked(int index);

signals:

    void sig_rowClicked(int row);

    void sig_clearClicked();

    void sig_editClicked(int row);

private slots:
    void on_button_clear_clicked();

private:
    Ui::OperationStack *ui;
    QList<HistoryItem*> t;
};

#endif // OPERATIONSTACK_H
