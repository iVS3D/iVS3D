#ifndef OPERATIONSTACK_H
#define OPERATIONSTACK_H

#include <QWidget>
#include <QListWidget>

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

public slots:

    void slot_itemClicked(QListWidgetItem *item);

signals:

    void sig_rowClicked(int row);

private:
    Ui::OperationStack *ui;
};

#endif // OPERATIONSTACK_H
