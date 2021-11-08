#ifndef MODELINPUTITERATOR_H
#define MODELINPUTITERATOR_H

#include "model/modelinputpictures.h"

/**
 * @interface ModelInputIterator
 *
 * @ingroup Controller
 *
 * @brief The ModelInputIterator class handles iteration over given ModelInputPictures. Handles boundarys and allows for easy boundary checks.
 * Different implementations of this interface allow to easily swap iteration policy, f.e. iterate keyframes only.
 *
 * @author Dominik Wüst
 *
 * @date 2021/02/11
 */
class ModelInputIterator {
public:
    /**
     * @brief steps for stepsize images forward from given currentIdx without leaving the boundarys of ModelInputPictures mip.
     * @param mip the ModelInputPictures instance to iterate
     * @param currentIdx the index to start from
     * @param stepsize the number of images to step from currentIdx
     * @return the new index which is stepsize images forward from currentIdx
     */
    virtual unsigned int getNext(ModelInputPictures *mip, unsigned int currentIdx, unsigned int stepsize = 1) = 0;
    /**
     * @brief steps for stepsize images backward from given currentIdx without leaving the boundarys of ModelInputPictures mip.
     * @param mip the ModelInputPictures instance to iterate
     * @param currentIdx the index to start from
     * @param stepsize the number of images to step from currentIdx
     * @return the new index which is stepsize images backward from currentIdx
     */
    virtual unsigned int getPrevious(ModelInputPictures *mip, unsigned int currentIdx, unsigned int stepsize = 1) = 0;
    /**
     * @brief get index of first image in ModelInputPicturesInstance mip.
     * @param mip the ModelInputPictures to iterate
     * @return the index of the first image
     */
    virtual unsigned int getFirst(ModelInputPictures *mip) = 0;
    /**
     * @brief get index of last image in ModelInputPicturesInstance mip.
     * @param mip the ModelInputPictures to iterate
     * @return the index of the first image
     */
    virtual unsigned int getLast(ModelInputPictures *mip) = 0;
    /**
     * @brief check if currentIdx is first index in ModelInputPictures mip.
     * @param mip the ModelInputPictures to iterate
     * @param currentIdx index to check
     * @return @a true if currentIdx is first index, @a false otherwise
     */
    virtual bool isFirst(ModelInputPictures *mip, unsigned int currentIdx) = 0;
    /**
     * @brief check if currentIdx is last index in ModelInputPictures mip.
     * @param mip the ModelInputPictures to iterate
     * @param currentIdx index to check
     * @return @a true if currentIdx is last index, @a false otherwise
     */
    virtual bool isLast(ModelInputPictures *mip, unsigned int currentIdx)= 0;
};

#endif // MODELINPUTITERATOR_H
