#ifndef ISERIALIZABLE_H
#define ISERIALIZABLE_H


#include <QVariant>

/**
 * @class ISerializable
 *
 * @ingroup Model
 *
 * @brief The ISerializable class ensures that inherit classes can be transformed to and from text
 *
 * @author Daniel Brommer
 *
 * @date 2021/02/08
 *
 */
class ISerializable
{
public:
    virtual ~ISerializable() {}
    /**
     * @brief toText generates a QVariant which contains the models data
     * @return generated QVariant
     */
    virtual QVariant toText()= 0;
    /**
     * @brief fromText recreates the given model form a QVariant
     * @param data contains essential model information which are used to recreate the model
     */
    virtual void fromText(QVariant data)= 0;
};

#endif // ISERIALIZABLE_H
