#ifndef PROGRESSWIDGET_H
#define PROGRESSWIDGET_H

#include <QWidget>
#include "model/progressdisplay.h"

namespace Ui {
class ProgressWidget;
}
/**
 * @class ProgressWidget
 *
 * @ingroup View
 *
 * @brief The ProgressWidget class widget inside the ProgressDialog that holds the concrete progress bar and abort button
 *
 * @author Lennart Ruck
 *
 * @date 2021/04/14
 */
class ProgressWidget : public QWidget, ProgressDisplay
{
    Q_OBJECT

public:
    /**
     * @brief ProgressWidget constructor that sets up ui elements
     * @param parent ui parent that holds this widget
     * @param showAbort visibility of abort button, @a true = show button, @a false = hide button
     */
    explicit ProgressWidget(QWidget *parent = nullptr, bool showAbort = true);
    ~ProgressWidget();

signals:
    /**
     * @brief sig_abort signals user pressing abort button
     */
    void sig_abort();

public slots:
    /**
     * @brief slot_progress updates progress bar with new given data
     * @param progress [0..100] percentage of total progress
     * @param currentOperation currently executed task
     */
    void slot_displayProgress(int progress, QString currentOperation);
    /**
     * @brief slot_displayMessage can show a message without specific format
     * @param message as a QString, which will be displayed
     */
    void slot_displayMessage(QString message);

private slots:
    void on_pushButton_abort_clicked();

private:
    Ui::ProgressWidget *ui;
};

#endif // PROGRESSWIDGET_H
