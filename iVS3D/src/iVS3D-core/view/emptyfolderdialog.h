#ifndef EMPTYFOLDERDIALOG_H
#define EMPTYFOLDERDIALOG_H

#include <QDialog>

namespace Ui {
class EmptyFolderDialog;
}

/**
 * @class EmptyFolderDialog
 *
 * @ingroup View
 *
 * @brief The EmptyFolderDialog class gets opened if the export was started in a non empty folder, requiring user input on how to handle this situation
 * The user has three options:
 * 1) "Delete and continue", deleting all files inside the selected output-folder (QDir::removeRecursively()) and exporting after. QDialog::exec() returns "1" == accepted().
 * 2) "Ignore", ignoring any existing files and overwriting them if necessary. QDialog::exec() returns "2".
 * 3) "Abort", stops the export and aborts it completely. QDialog::exec() returns "0" == rejected().
 *
 * @author Lennart Ruck
 *
 * @date 2021/04/14
 */
class EmptyFolderDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief EmptyFolderDialog constructor that sets up ui elements
     * @param parent parent ui parent that holds this widget
     */
    explicit EmptyFolderDialog(QWidget *parent = 0, const QString &path = "");
    ~EmptyFolderDialog();

private slots:
    void on_YesButton_clicked();

    void on_IgnoreButton_clicked();

    void on_AbortButton_clicked();

private:
    Ui::EmptyFolderDialog *ui;
};

#endif // EMPTYFOLDERDIALOG_H
