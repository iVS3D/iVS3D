#include "signalobject.h"

signalObject::signalObject(QObject *parent)
{
    (void) parent;
}

void signalObject::newMetaData()
{
    emit sig_newMetaData();
}

void signalObject::selectedImageIndex(uint index)
{
    emit sig_selectedImageIndex(index);
}

void signalObject::keyframesChanged(std::vector<uint> keyframes)
{
    emit sig_keyframesChanged(keyframes);
}
