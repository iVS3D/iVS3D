#include "signalobject.h"

signalObject::signalObject(QObject *parent)
{
    (void) parent;
}

void signalObject::newMetaData()
{
    emit sig_newMetaData();
}
