#ifndef PROGRESSDIALOG_H
#define PROGRESSDIALOG_H

#include <QDialog>
#include "model/progressdisplay.h"

namespace Ui {
class ProgressDialog;
}
/**
 * @class ProgressDialog
 *
 * @ingroup View
 *
 * @brief The ProgressDialog class is the ui dialog holding the progress bar and abort/cancel button
 *
 * @author Lennart Ruck
 *
 * @date 2021/04/14
 */
class ProgressDialog : public QDialog, public ProgressDisplay
{
    Q_OBJECT

public:
    /**
     * @brief ProgressDialog constructor configuring ui elements and connects cancel button when applicable
     * @param parent ui parent to return to after dialog is closed
     * @param showAbort visibility of the abort button, @a true = show button, @a false = hide button
     */
    explicit ProgressDialog(QWidget *parent = nullptr, bool showAbort = true);
    ~ProgressDialog();

signals:
    /**
     * @brief sig_abort signals that user pressed the abort button
     */
    void sig_abort();

public slots:
    /**
     * @brief slot_progress updates progress bar with new given data
     * @param progress [0..100] percentage of total progress
     * @param currentOperation currently executed task
     */
    void slot_displayProgress(int progress, QString currentOperation) override;
    void slot_displayMessage(QString message) override;

private slots:
    void slot_btCancel();

private:
    Ui::ProgressDialog *ui;
};

#endif // PROGRESSDIALOG_H
