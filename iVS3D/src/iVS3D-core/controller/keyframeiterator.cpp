#include "keyframeiterator.h"

unsigned int KeyframeIterator::getNext(ModelInputPictures* mip,
                                       unsigned int currentIdx,
                                       unsigned int stepsize) {
    return mip->getNextKeyframe(currentIdx, stepsize);
}

unsigned int KeyframeIterator::getPrevious(ModelInputPictures* mip,
                                           unsigned int currentIdx,
                                           unsigned int stepsize) {
    return mip->getPreviousKeyframe(currentIdx, stepsize);
}

unsigned int KeyframeIterator::getFirst(ModelInputPictures* mip) {
    return mip->isKeyframe(0) ? 0 : mip->getNextKeyframe(0, 1);
}

unsigned int KeyframeIterator::getLast(ModelInputPictures* mip) {
    return mip->isKeyframe(mip->getImageCount() - 1)
               ? mip->getImageCount() - 1
               : mip->getPreviousKeyframe(mip->getImageCount() - 1, 1);
}

bool KeyframeIterator::isFirst(ModelInputPictures* mip,
                               unsigned int currentIdx) {
    return getFirst(mip) >= currentIdx;
}

bool KeyframeIterator::isLast(ModelInputPictures* mip,
                              unsigned int currentIdx) {
    return getLast(mip) <= currentIdx;
}
