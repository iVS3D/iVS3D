#ifndef IMAGEITERATOR_H
#define IMAGEITERATOR_H

#include "ModelInputIterator.h"

/**
 * @class ImageIterator
 *
 * @ingroup Controller
 *
 * @brief The ImageIterator class is used to iterate given ModelInputPictures instance mip. Handles boundaries and
 * and allows for easy boundary checks. Every image is iterated.
 *
 * @author Dominik Wüst
 *
 * @date 2021/02/11
 */
class ImageIterator : public ModelInputIterator
{
public:
    /**
     * @brief steps for stepsize images forward from given currentIdx without leaving the boundaries of ModelInputPictures mip.
     * @param mip the ModelInputPictures instance to iterate
     * @param currentIdx the index to start from
     * @param stepsize the number of images to step from currentIdx
     * @return the new index which is min(currentIdx+stepsize, mip.getPicCount()-1)
     *
     * @see ModelInputPictures::getPicCount
     */
    unsigned int getNext(ModelInputPictures *mip, unsigned int currentIdx, unsigned int stepsize = 1) override;
    /**
     * @brief steps for stepsize images backward from given currentIdx without leaving the boundaries of ModelInputPictures mip.
     * @param mip the ModelInputPictures instance to iterate
     * @param currentIdx the index to start from
     * @param stepsize the number of images to step from currentIdx
     * @return the new index which is max(currentIdx-stepsize, 0)
     */
    unsigned int getPrevious(ModelInputPictures *mip, unsigned int currentIdx, unsigned int stepsize = 1) override;
    /**
     * @brief get index of first image in ModelInputPictures instance mip. First is 0.
     * @param mip the ModelInputPictures to iterate
     * @return 0
     */
    unsigned int getFirst(ModelInputPictures *mip) override;
    /**
     * @brief get index of last image in ModelINputPictures instance mip. Last is mip.getPicCount()-1.
     * @param mip the modelInputPictures to iterate
     * @return mip.getPicCount()-1
     *
     * @see ModelInputPictures::getPicCount
     */
    unsigned int getLast(ModelInputPictures *mip) override;
    /**
     * @brief check if currentIdx is first index in ModelInputPictures mip.
     * @param mip the ModelInputPictures to iterate
     * @param currentIdx index to check
     * @return @a true if currentIdx is first index, @a false otherwise
     */
    bool isFirst(ModelInputPictures *mip, unsigned int currentIdx) override;
    /**
     * @brief check if currentIdx is last index in ModelInputPictures mip.
     * @param mip the ModelInputPictures to iterate
     * @param currentIdx index to check
     * @return @a true if currentIdx is first index, @a false otherwise
     */
    bool isLast(ModelInputPictures *mip, unsigned int currentIdx) override;
};

#endif // IMAGEITERATOR_H
