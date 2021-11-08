#include "modelinputiteratorfactory.h"

ModelInputIterator *ModelInputIteratorFactory::createIterator(ModelInputIteratorFactory::IteratorType t)
{
    ModelInputIterator *it;
    switch(t){
        case IteratorType::Images:      it = new ImageIterator(); break;
        case IteratorType::Keyframes:   it = new KeyframeIterator(); break;
        default:                        it = new ImageIterator();
    }
    return it;
}
