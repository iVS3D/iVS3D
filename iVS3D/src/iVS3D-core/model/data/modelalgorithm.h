#ifndef MODELALGORITHM_H
#define MODELALGORITHM_H

#include "ISerializable.h"

#include <QObject>
#include <QMap>
#include <QJsonObject>


/**
 * @class ModelAlgorithm
 *
 * @ingroup Model
 *
 * @brief The ModelAlgorithm class is responsible for buffering data from the used plugins. It implements the interface ISerializable to
 * be able to save all its current Data
 *
 * @author Daniel Brommer
 *
 * @date 2021/03/03
 */

class ModelAlgorithm: public ISerializable
{
public:
    /**
     * @brief ModelInputPictures default Constructor
     *
     */
    ModelAlgorithm();
    /**
     * @brief Saves the buffer given buffer
     *
     * @param pluginName Name of the Plugin
     * @param bufferName Name of the buffer
     * @param value Buffered values as QVariant
     */
    void addPluginBuffer(QString pluginName, QString bufferName, QVariant value);
    /**
     * @brief Returns the whole buffer
     *
     * @return A QMap<QString, QMap<QString, QVariant>> where the outer map saves a inner map for every Plugin. The inner map contains the buffered value and
     * its name given by the plugin. This way a plugin is able to save multiple values. E.g. the Blur plugin can save blur values from the Laplacian blur filter
     * and blur values from the Sobel algorithm.
     *
     */
    QMap<QString, QMap<QString, QVariant>> getPluginBuffer();

    // ISerializable interface
    /**
     * @brief Saves this class to a QVariant
     *
     * @return QVariant containing important data from this class
     */
    QVariant toText() override;
    /**
     * @brief Reades its members from the given QVariant
     *
     * @param data QVariant containing this class data
     */
    void fromText(QVariant data) override;


private:
    QMap<QString, QMap<QString, QVariant>> m_pluginBuffer;



};

#endif // MODELALGORITHM_H
