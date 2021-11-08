#ifndef MODELINPUTITERATORFACTORY_H
#define MODELINPUTITERATORFACTORY_H

#include "controller/ModelInputIterator.h"  // all iterators to instanciate
#include "controller/imageiterator.h"
#include "controller/keyframeiterator.h"

/**
 * @class ModelInputIteratorFactory
 *
 * @ingroup Controller
 *
 * @brief The ModelInputIteratorFactory class is used to create ModelInputIterator instances.
 *
 * @author Dominik Wüst
 *
 * @date 2021/02/10
 */
class ModelInputIteratorFactory
{
    Q_ENUMS(IteratorType)
public:
    /**
     * @enum IteratorType
     * @brief The IteratorType enum represents possible iterators to create
     */
    enum IteratorType{
        Images,     /**< iterate all images */
        Keyframes   /**< iterate keyframes only */
    };
    /**
     * @brief create a ModelInputIterator of given IteratorType
     * @param t the iterator type
     * @return a pointer to the new ModelInputIterator
     */
    static ModelInputIterator *createIterator(IteratorType t);
};

#endif // MODELINPUTITERATORFACTORY_H
