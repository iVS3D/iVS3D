#ifndef PROGRESSABLE_H
#define PROGRESSABLE_H

#include <QObject>

/**
 * @interface Progressable
 *
 * @ingroup Model
 *
 * @brief The Progressable interface is used to by multithreaded actions such as sampling or exporting to report their progress.
 *
 * @author Patrick Binder
 *
 * @date 2021/02/05
 */
class Progressable: public QObject
{
    Q_OBJECT
public slots:
    /**
     * @brief [slot] slot_makeProgress used to delegate the progression to an attached view
     * @param progress [0..100] percentage of total progress
     * @param currentOperation operation that is currently executed
     */
    void slot_makeProgress(int progress, QString currentOperation);

    /**
     * @brief [slot] slot_displayMessage used to delegate a message to an attached view
     * @param message which should be displayed in the view
     */
    void slot_displayMessage(QString message);

signals:
    /**
     * @brief [signal] sig_progress is emitted if a thread makes a progress and wants to report it.
     * @param progress The progress in range [0,100]
     * @param currentOp The curtrently performed operation
     */
    void sig_progress(int progress, QString currentOp);
    /**
     * @brief [signal] sig_message is emitted if a thread wants to display a message.
     * @param message The message in form of a QString
     */
    void sig_message(QString message);
};

#endif // PROGRESSABLE_H
