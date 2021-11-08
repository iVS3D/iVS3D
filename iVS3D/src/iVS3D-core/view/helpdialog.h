#ifndef HELPDIALOG_H
#define HELPDIALOG_H

#include <QDialog>

namespace Ui {
class helpDialog;
}

/**
 * @class helpDialog
 *
 * @ingroup View
 *
 * @brief The helpDialog class holds key combination shortcuts for any action
 *
 * @ingroup View
 *
 * @author Dominik Wüst
 *
 * @date 2021/04/14
 */
class helpDialog : public QDialog
{
    Q_OBJECT

public:
    explicit helpDialog(QWidget *parent = nullptr);
    ~helpDialog();

private:
    Ui::helpDialog *ui;
};

#endif // HELPDIALOG_H
