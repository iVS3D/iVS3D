#ifndef PROGRESSDISPLAY_H
#define PROGRESSDISPLAY_H

#include <QObject>

/**
 * @class ProgressDisplay
 *
 * @ingroup View
 *
 * @brief The ProgressDisplay class is an interface for classes that can display progression in any form.
 *        Most of the time it is used to display the progress of a thread.
 *
 * @author Dominic Zahn
 *
 * @date 2021/09/28
 */
class ProgressDisplay
{
public:
    /**
     * @brief [slot] slot_displayProgress is used to display a progression
     * @param progress [0, 100] as percentage
     * @param currentOperation as a QString, which is currently running
     */
    virtual void slot_displayProgress(int progress, QString currentOperation) = 0;
    /**
     * @brief slot_displayMessage can show a message without specific format
     * @param message as a QString, which will be displayed
     */
    virtual void slot_displayMessage(QString message) = 0;
};

#endif // PROGRESSDISPLAY_H
