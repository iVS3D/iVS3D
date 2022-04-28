#ifndef SIGNALOBJECT_H
#define SIGNALOBJECT_H

#include <QObject>
#include <QMap>
#include <QVariant>
#include <QPoint>

class signalObject : public QObject
{
    Q_OBJECT
public:
    signalObject(QObject *parent = nullptr);
    void newMetaData();
    void selectedImageIndex(uint index);
    void keyframesChanged(std::vector<uint> keyframes);
    void boundariesChanged(QPoint boundaries);
    void updateBuffer(QString pluginName, QMap<QString, QVariant> buffer);

signals:
    /**
     * @brief sig_newMetaData notification about updated meta data. Access Reader object to fetch the new meta data if needed.
     */
    void sig_newMetaData();
    /**
     * @brief sig_selectedImageIndex is emitted if a new image is selected in the core ui.
     * @param index of the newly selected image
     */
    void sig_selectedImageIndex(uint index);
    /**
     * @brief sig_keyframesChanged is emitted if the keyframes list changes.
     * @param keyframes are the newly selected keyframes
     */
    void sig_keyframesChanged(std::vector<uint> keyframes);
    /**
     * @brief sig_boundariesChanged is emitted if the timeline boundaries are moved.
     * @param boundaries begin end end boundary
     */
    void sig_boundariesChanged(QPoint boundaries);
    /**
     * @brief sig_updateBuffer is emitted if new buffer has been loaded
     * @param pluginName name of the plugin whose buffer is emited
     * @param buffer the buffer itself
     */
    void sig_updateBuffer(QString pluginName, QMap<QString, QVariant> buffer);
};

#endif // SIGNALOBJECT_H
