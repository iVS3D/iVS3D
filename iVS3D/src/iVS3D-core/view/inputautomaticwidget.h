#ifndef INPUTAUTOMATICWIDGET_H
#define INPUTAUTOMATICWIDGET_H

#include "infowidget.h"
#include "automaticwidget.h"

#include <QWidget>


namespace Ui {
class InputAutomaticWidget;
}
/**
 * @class InputAutomaticWidget
 *
 * @ingroup View
 *
 * @brief The InputAutomaticWidget class is used to show the InfoWidget and the AutomaticWidget in one widget
 *
 * @author Daniel Brommer
 *
 * @date 2021/07/19
 */

class InputAutomaticWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief InputAutomaticWidget Combines both widget
     * @param parent Pointer to parent
     * @param info Pointer to the InfoWidget
     * @param automatic Pointer to the AutomaticWidget
     */
    explicit InputAutomaticWidget(QWidget *parent = nullptr, InfoWidget *info = nullptr, AutomaticWidget *automatic = nullptr);
    ~InputAutomaticWidget();


private:
    Ui::InputAutomaticWidget *ui;


};

#endif // INPUTAUTOMATICWIDGET_H
