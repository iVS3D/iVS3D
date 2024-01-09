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
    explicit HistoryItem(QWidget *parent = nullptr, QString name = "");
    ~HistoryItem();

private:
    Ui::HistoryItem *ui;
};

#endif // HISTORYITEM_H
