#include "imageiterator.h"

unsigned int ImageIterator::getNext(ModelInputPictures *mip, unsigned int currentIdx, unsigned int stepsize)
{
    currentIdx += stepsize;
    if(currentIdx >= mip->getPicCount() - 1){
        currentIdx = mip->getPicCount() - 1;
    }
    return currentIdx;
}

unsigned int ImageIterator::getPrevious(ModelInputPictures *mip, unsigned int currentIdx, unsigned int stepsize)
{
    (void)mip;
    if(currentIdx > stepsize){
        return  currentIdx - stepsize;
    } else {
        return 0;
    }
}

unsigned int ImageIterator::getFirst(ModelInputPictures *mip)
{
    (void) mip;
    return 0;
}

unsigned int ImageIterator::getLast(ModelInputPictures *mip)
{
    return mip->getPicCount() - 1;
}

bool ImageIterator::isFirst(ModelInputPictures *mip, unsigned int currentIdx)
{
    (void) mip;
    return currentIdx == 0;
}

bool ImageIterator::isLast(ModelInputPictures *mip, unsigned int currentIdx)
{
    return currentIdx >= (mip->getPicCount() - 1);
}
