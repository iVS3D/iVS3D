#ifndef AUTOMATICLISTWIDGET_H
#define AUTOMATICLISTWIDGET_H

#include <QListWidget>
#include <QDropEvent>

/**
 * @class AutomaticListWidget
 *
 * @ingroup View
 *
 * @brief The AutomaticListWidget overrides a QListWidget to detect drag and drop events
 *
 * @author Daniel Brommer
 *
 * @date 2021/07/06
 */

class AutomaticListWidget : public QListWidget
{
    Q_OBJECT
public:
    /**
     * @brief AutomaticListWidget Standart constructor
     * @param parent Pointer to the parent widget
     */
    AutomaticListWidget(QWidget *parent = nullptr);

public slots:
    /**
     * @brief dropEvent Overrides the dropEvent
     * @param event Pointer to the event
     */
    void dropEvent(QDropEvent *event);

signals:
    /**
     * @brief sig_itemDrop Signals that a drop event occured
     */
    void sig_itemDrop();
};

#endif // AUTOMATICLISTWIDGET_H
