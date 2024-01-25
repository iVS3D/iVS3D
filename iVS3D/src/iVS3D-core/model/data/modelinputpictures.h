#ifndef MODELINPUTPICTURES_H
#define MODELINPUTPICTURES_H

#include "reader.h"
#include "readerfactory.h"
#include "ISerializable.h"
#include "stringcontainer.h"
#include "concurrentreader.h"
#include "metadatamanager.h"
#include "algorithmmanager.h"

#include <QObject>
#include <QPoint>
#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/core/utils/filesystem.hpp>
#include <QByteArray>
#include <QVector>
#include <QVariant>
#include <QJsonObject>
#include <QVector>
#include <QStringList>


/**
 * @class ModelInputPictures
 *
 * @ingroup Model
 *
 * @brief The ModelInputPictures class is responsible for saving all the Data regarding the input. It implements the interface ISerializable to
 * be able to save all its current Data
 *
 * @author Daniel Brommer
 *
 * @date 2021/01/28
 */

class ModelInputPictures: public QObject, public ISerializable
{
Q_OBJECT



public:
    /**
     * @class Memento
     *
     * @ingroup Model
     *
     * @brief The Memento class is used to store and restore the keyframe list of mip for undo and redo.
     *
     * @author Dominik Wuest
     *
     * @date 2022/05/16
     */
    class Memento {
        friend class ModelInputPictures;

    public:
        /**
         * @brief getSnapshotDate returns the QDateTime object with the exact creation time and date of the memento
         * @return The date and time of creation
         */
        QDateTime getSnapshotDate();

        int getNumImages();

    private:
        Memento(std::vector<uint> state);
        std::vector<uint> getState();

        std::vector<uint> m_state;
        QDateTime m_dateTime;
    };

    /**
     * @brief ModelInputPictures Constructor, which uses inputPath to import the frames.
     *
     * @param inputPath Path to the file or directory to be opend
     */
    explicit ModelInputPictures(QString inputPath);
    /**
     * @brief ModelInputPictures Constructor, which creates an empty class. toText can be used on this instance.
     */
    explicit ModelInputPictures();
    /**
     * @brief Sets the frame with index as a keyframe
     *
     * @param index Index of the Keyframe to be added
     */
    void addKeyframe(unsigned int index);
    /**
     * @brief Checks if the indexed frame is a keyframe
     *
     * @param index Index of the frame to be checked
     * @return @a true if the frame is a keyframe @a false otherwise
     */
    bool isKeyframe(unsigned int index);
    /**
     * @brief Updates the indices of all keyframes
     *
     * @param keyframes Index vector with the indices of the keyframes
     */
    void updateMIP(const std::vector<unsigned int> &keyframes);
    /**
     * @brief Removes the keyframe with the given index
     *
     * @param index Index of the keyframe to be removed
     */
    void removeKeyframe(unsigned int index);
    /**
     * @brief Returns the frame with the given index
     *
     * @param index Index of the frame
     * @return Mat pointer to the frame
     */
    const cv::Mat* getPic(unsigned int index);
    /**
     * @brief Returns the number of keyframes
     *
     * @param inBound   true: returns only the amount of keyframes between the currently set boundaries
     *                  false: returns the total amount of keyframes
     * @return Number of keyframes
     */
    unsigned int getKeyframeCount(bool inBound);
    /**
     * @brief Returns the number of frames
     *
     * @return Number of frames
     */
    unsigned int getPicCount();
    /**
     * @brief Returns the index vector containing the keyframes
     *
     * @param inBound   true: returns only the keyframes that are between the currently set boundaries
     *                  false: returns all keyframes
     * @return Keyframe vector
     */
    std::vector<unsigned int> getAllKeyframes(bool inBound);
    /**
     * @brief Returns the input resolution
     *
     * @return Input resolution
     */
    QPoint getInputResolution();
    /**
     * @brief Returns the stepsize-next keyframe to a given index
     *
     * @param index Index of the current frame
     * @param stepsize Number of keyframes to be skipped
     * @return The index of the stepsize-next keyframe if it exists, otherwise it will return the last keyframe
     */
    unsigned int getNextKeyframe(unsigned int index, unsigned int stepsize);
    /**
     * @brief Returns the stepsize-next keyframe to a given index
     *
     * @param index Index of the current frame
     * @param stepsize Number of keyframes to be skipped
     * @return The index of the stepsize-next keyframe if it exists, otherwise it will return the first keyframe
     */
    unsigned int getPreviousKeyframe(unsigned int index, unsigned int stepsize);
    /**
     * @brief Returns the input Path
     *
     * @return QString with the inputPath
     */
    QString getPath();
    /**
     * @brief Returns the current Reader as a DelayedCopyReader
     *
     * @return A DelayedCopyReader
     */
    Reader *getReader();
    /**
     * @brief createConcurrentReader creates an object from the class ConcurrentReader which can be used to access pictures in a parallel way
     *
     * @return object of the class ConcurrentReader
     */
    ConcurrentReader *createConcurrentReader();
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
    /**
     * @brief Returns the current boundaries
     *
     * @return The current boundaries
     */
    QPoint getBoundaries();
    /**
     * @brief Set the current boundaries
     *
     * @param boundaries The new boundaries
     */
    void setBoundaries(QPoint boundaries);
    /**
     * @brief loadMetaData Loads the given meta data for the imported Video
     * @param path Paths the the meta data to load
     * @return how many MetaDataReader have succesfully loaded the meta data
     */
    int loadMetaData(QStringList paths);    
    /**
     * @brief loadMetaDataImages Tries to extract meta data from the imported images
     * @return how many MetaDataReader have succesfully loaded meta data
     */
    int loadMetaDataImages();
    Memento *save();
    void restore(Memento *m);

signals:
    /**
     * @brief Signal, which is emitted, when the keyframe vector changes
     *
     */
    void sig_mipChanged();


private:
    Reader* m_reader = nullptr;
    std::vector<unsigned int> m_keyframes;
    QString m_inputPath;
    QPoint m_inputResolution;
    cv::Mat m_currentMat;
    QPoint m_boundaries;
    MetaDataManager* m_metaDataManager = nullptr;

    void setResolution();
    std::vector<unsigned int> splitString(QString string);



};

#endif // MODELINPUTPICTURES_H
