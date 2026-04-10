#ifndef MODELINPUTPICTURES_H
#define MODELINPUTPICTURES_H

#include <qresultstore.h>

#include <QByteArray>
#include <QJsonObject>
#include <QObject>
#include <QPoint>
#include <QStringList>
#include <QVariant>
#include <QVector>
#include <memory>
#include <opencv2/core.hpp>
#include <opencv2/core/utils/filesystem.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/videoio.hpp>

#include "ISerializable.h"
#include "algorithmmanager.h"
#include "metadatamanager.h"
#include "reader.h"
#include "readerfactory.h"
#include "resolution.h"
#include "stringcontainer.h"

/**
 * @class ModelInputPictures
 *
 * @ingroup Model
 *
 * @brief The ModelInputPictures class is responsible for saving all the Data
 * regarding the input. It implements the interface ISerializable to be able to
 * save all its current Data
 *
 * @author Daniel Brommer
 *
 * @date 2021/01/28
 */

class ModelInputPictures : public QObject, public ISerializable {
    Q_OBJECT

   public:
    /**
     * @class Memento
     *
     * @ingroup Model
     *
     * @brief The Memento class is used to store and restore the keyframe list
     * of mip for undo and redo.
     *
     * @author Dominik Wuest
     *
     * @date 2022/05/16
     */
    class Memento {
        friend class ModelInputPictures;

       public:
        /**
         * @brief getSnapshotDate returns the QDateTime object with the exact
         * creation time and date of the memento
         * @return The date and time of creation
         */
        QDateTime getSnapshotDate();

       private:
        Memento(std::vector<uint> state);
        std::vector<uint> getState();

        std::vector<uint> m_state;
        QDateTime m_dateTime;
    };

    /**
     * @brief ModelInputPictures Constructor, which uses inputPath to import the
     * frames.
     *
     * @param inputPath Path to the file or directory to be opend
     */
    explicit ModelInputPictures(QString inputPath);
    /**
     * @brief ModelInputPictures Constructor, which creates an empty class.
     * toText can be used on this instance.
     */
    explicit ModelInputPictures();

    ~ModelInputPictures();
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
    void updateKeyframes(const std::vector<unsigned int>& keyframes);
    /**
     * @brief Removes the keyframe with the given index
     *
     * @param index Index of the keyframe to be removed
     */
    void removeKeyframe(unsigned int index);
    /**
     * @brief Returns the number of keyframes
     *
     * @param inBound   true: returns only the amount of keyframes between the
     * currently set boundaries false: returns the total amount of keyframes
     * @return Number of keyframes
     */
    unsigned int getKeyframeCount(bool inBound);

    /**
     * @brief Returns the index vector containing the keyframes
     *
     * @param inBound   true: returns only the keyframes that are between the
     * currently set boundaries false: returns all keyframes
     * @return Keyframe vector
     */
    std::vector<unsigned int> getAllKeyframes(bool inBound);

    /**
     * @brief Returns the stepsize-next keyframe to a given index
     *
     * @param index Index of the current frame
     * @param stepsize Number of keyframes to be skipped
     * @return The index of the stepsize-next keyframe if it exists, otherwise
     * it will return the last keyframe
     */
    unsigned int getNextKeyframe(unsigned int index, unsigned int stepsize);
    /**
     * @brief Returns the stepsize-next keyframe to a given index
     *
     * @param index Index of the current frame
     * @param stepsize Number of keyframes to be skipped
     * @return The index of the stepsize-next keyframe if it exists, otherwise
     * it will return the first keyframe
     */
    unsigned int getPreviousKeyframe(unsigned int index, unsigned int stepsize);
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
     * @brief Returns the number of frames
     *
     * @return Number of frames
     */
    unsigned int getImageCount();
    /**
     * @brief Returns the resolution as in form (width, height).
     * This is the resolution of the first image. If the resolution changes
     * during the data no gurantee is given.
     *
     * @return resolution (width,height) of the first image
     */
    Resolution getResolution();
    /**
     * @brief Setter for working resolution
     * The working resolution is used to for sampling in plugins and masks are
     * resized to this during export. This resolution needs to be smaller or the
     * same as the (original) resolution.
     *
     * @param new working resolution
     *
     * @return true on success
     */
    bool setWorkingResolution(Resolution r);
    /**
     * @brief Setter for working resolution
     * The working resolution is used to for sampling in plugins and masks are
     * resized to this during export. This resolution needs to be smaller or the
     * same as the (original) resolution.
     *
     * This version validates the string and uses the version with individual
     * inputs underneath.
     *
     * @return working resolution (width,height)
     */
    bool setWorkingResolution(QString resStr);
    /**
     * @brief Getter for working resolution
     * The working resolution is used to for sampling in plugins and masks
     * are resized to this during export.
     *
     * This version validates the string and uses the version with individual
     * inputs underneath.
     *
     * @return working resolution (width,height)
     */
    Resolution getWorkingResolution();

    /**
     * @brief Returns the input Path
     *
     * @return QString with the inputPath
     */
    QString getPath();
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
     * @brief loadMetaData Loads the given meta data for the imported Video
     * @param path Paths the the meta data to load
     * @return how many MetaDataReader have succesfully loaded the meta data
     */
    int loadMetaData(QStringList paths);
    /**
     * @brief loadMetaDataImages Tries to extract meta data from the
     * imported images
     * @return how many MetaDataReader have succesfully loaded meta data
     */
    int loadMetaDataImages();
    Memento* save();
    void restore(Memento* m);

    void setAltitude(double altitude);
    double getAltitude();

    std::unique_ptr<iReader> createNewReader();

   signals:
    /**
     * @brief Signal, which is emitted, when the keyframe vector changes
     *
     */
    void sig_mipChanged();

   private:
    // Keyframe Data
    std::vector<unsigned int> m_keyframes = {};
    QPoint m_boundaries = {-1, -1};

    // Image Data
    QString m_inputPath = "";
    Resolution m_resolution = {0, 0};
    Resolution m_workResolution = {0, 0};
    uint m_imageCount = 0;

    // Meta Data Management
    MetaDataManager* m_metaDataManager = nullptr;
    double m_altitude = 0;

    // helper functions
    std::vector<unsigned int> splitString(QString string);
};

#endif  // MODELINPUTPICTURES_H
