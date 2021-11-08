#include "progressable.h"

void Progressable::slot_makeProgress(int progress, QString currentOperation)
{
    emit sig_progress(progress, currentOperation);
}

void Progressable::slot_displayMessage(QString message)
{
    emit sig_message(message);
}
