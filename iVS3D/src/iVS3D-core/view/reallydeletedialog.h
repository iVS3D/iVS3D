#ifndef REALLYDELETEDIALOG_H
#define REALLYDELETEDIALOG_H

#include <QDialog>

namespace Ui {
class ReallyDeleteDialog;
}
/**
 * @class ReallyDeleteDialog
 *
 * @ingroup View
 *
 * @brief The ReallyDeleteDialog class is handling the pop up window confirming to deselect all currently selected keyframes
 *
 * @author Dominik Wüst
 *
 * @date 2021/04/14
 */
class ReallyDeleteDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief ReallyDeleteDialog constructor that sets up ui elements
     * @param parent parent ui parent that holds this widget
     */
    explicit ReallyDeleteDialog(QWidget *parent = nullptr);
    ~ReallyDeleteDialog();

private slots:
    void on_pushButton_2_clicked();

    void on_pushButton_clicked();

private:
    Ui::ReallyDeleteDialog *ui;
};

#endif // REALYDELETEDIALOG_H
