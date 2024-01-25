#ifndef HISTORYITEM_H
#define HISTORYITEM_H

#include <QWidget>

namespace Ui {
class HistoryItem;
}

class HistoryItem : public QWidget
{
    Q_OBJECT

public:
    explicit HistoryItem(QWidget *parent = nullptr, QString name = "", QString data = "", int index = 0);
    ~HistoryItem();

    void exitEditNode();

    void toPending();

private slots:
    void on_btn_edit_clicked();

signals:

    void sig_editClicked(int index);


private:
    Ui::HistoryItem *ui;
    int m_index;
};

#endif // HISTORYITEM_H
