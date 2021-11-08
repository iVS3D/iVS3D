#ifndef KEYFRAMEITERATOR_H
#define KEYFRAMEITERATOR_H

#include "ModelInputIterator.h"

/**
 * @class KeyframeIterator
 *
 * @ingroup Controller
 *
 * @brief The KeyframeIterator class is used to iterate given ModelInputPictures instance mip. Handles boundaries of mip.
 * KeyframeIterator only iterates indices marked as keyframes in ModelInputPictures.
 *
 * @see ModelInputPictures::getAllKeyframes
 *
 * @author Dominik Wüst
 *
 * @date 2021/02/11
 */
class KeyframeIterator : public ModelInputIterator
{
public:
    /**
     * @brief steps for stepsize keyframes forward from given currentIdx without leaving the boundaries of ModelInputPictures mip.
     * @param mip the ModelInputPictures instance to iterate
     * @param currentIdx the index to start from
     * @param stepsize the number of keyframes to step from currentIdx
     * @return the index of keyframe which is stepsize keyframes forward from currentIdx
     */
    unsigned int getNext(ModelInputPictures *mip, unsigned int currentIdx, unsigned int stepsize = 1) override;
    /**
     * @brief steps for stepsize keyframes backward from given currentIdx without leaving the boundaries of ModelInputPictures mip.
     * @param mip the ModelInputPictures instance to iterate
     * @param currentIdx the index to start from
     * @param stepsize the number of keyframes to step from currentIdx
     * @return the index of keyframe which is stepsize keyframes backward from currentIdx
     */
    unsigned int getPrevious(ModelInputPictures *mip, unsigned int currentIdx, unsigned int stepsize = 1) override;
    /**
     * @brief get index of first keyframe in ModelInputPictures mip.
     * @param mip the ModelInputPictures instance to iterate
     * @return 0 if mip.isKeyframe(0), mip.getNextKeyframe(0,1) otherwise
     *
     * @see ModelInputPictures::isKeyframe
     * @see ModelInputPictures::getNextKeyframe
     */
    unsigned int getFirst(ModelInputPictures *mip) override;
    /**
     * @brief get index of last keyframe in ModelInputPictures mip.
     * @param mip the ModelInputPictures instance to iterate
     * @return mip.getPicCount()-1 if mip.isKeyframe(mip.getPicCount()-1), mip.getPreviousKeyframe(mip.getPicCount()-1,1) otherwise
     *
     * @see ModelInputPictures::isKeyframe
     * @see ModelInputPictures::getPreviousKeyframe
     * @see ModelInputPictures::getPicCount
     */
    unsigned int getLast(ModelInputPictures *mip) override;
    /**
     * @brief check if currentIdx is first keyframe index in ModelInputPictures mip.
     * @param mip the ModelInputPictures to iterate
     * @param currentIdx index to check
     * @return @a true if currentIdx is first keyframe index, @a false otherwise
     */
    bool isFirst(ModelInputPictures *mip, unsigned int currentIdx) override;
    /**
     * @brief check if currentIdx is last keyframe index in ModelInputPictures mip.
     * @param mip the ModelInputPictures to iterate
     * @param currentIdx index to check
     * @return @a true if currentIdx is last keyframe index, @a false otherwise
     */
    bool isLast(ModelInputPictures *mip, unsigned int currentIdx) override;
};

#endif // KEYFRAMEITERATOR_H
