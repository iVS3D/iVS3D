#ifndef VIDEOPLAYERCONTROLLER_H
#define VIDEOPLAYERCONTROLLER_H

#include <QObject>  // used for signals and slots
#include <QTimer>   // used for periodic timer events to update displayed image

#include "plugin/algorithmmanager.h" //used to emit signal that index has changed

#include "model/DataManager.h"  // used to access image data for displaying and manipulation

#include "view/videoplayer.h"   // used to display images
#include "view/timeline.h"      // used to visualize keyframe distribution

#include "controller/ModelInputIterator.h"  // used to iterate over given image data
#include "controller/algorithmcontroller.h" // used to display transformed images

#include "controller/imageiterator.h"
#include "controller/modelinputiteratorfactory.h"
#include "view/reallydeletedialog.h"

/**
 * @class VideoPlayerController
 *
 * @ingroup Controller
 *
 * @brief The VideoPlayerController class manages visualization of image data and manual changes to keyframes.
 *
 * The image data is provided by the DataManager *dataManager. For visualization a VideoPlayer *player and a Timeline *timeline are used. The user can select
 * images and modify keyframes using Timeline and VideoPlayer functions which delegate the command to this VideoPlayerController using signals. This VideoPlayerController
 * is tasked with updating the data provided by dataManager as well as refreshing the view-elements (Timeline and VideoPlayer) periodically and after changes to the image data.
 *
 * @author Dominik Wüst
 *
 * @date 2021/02/10
 */
class VideoPlayerController : public QObject
{
    Q_OBJECT  // used to enable signals and slots for this class
    QThread workerThread;

public:
    /**
     * @brief Creates a VideoPlayerController instance.
     * Connects to the signals from VideoPlayer and Timeline and initializes both with image data from DataManager.
     * Initializes a timer to interrupt periodically.
     * @param parent a QWidget as parent in the Qt object hierarchy
     * @param player a VideoPlayer instance to display images
     * @param timeline a Timeline instance to visualize keyframe distribution
     * @param dataManager a DataManager Instance to access image data
     * @param algoController an AlgorithmController instance to transform images and preview them
     */
    explicit VideoPlayerController(QObject *parent = nullptr, VideoPlayer *player = nullptr, Timeline *timeline = nullptr, DataManager *dataManager = nullptr, AlgorithmController *algoController = nullptr);

    /**
     * @brief disconnects signals, deletes timer and VideoPlayerController instance.
     */
    ~VideoPlayerController();

    /**
     * @brief getImageIndexOnScreen getter for m_imageIndexOnScreen (only used in tests for now)
     * @return index of image currently displayed
     */
    unsigned int getImageIndexOnScreen();

public slots:
    /**
     * @brief [slot] slot_play() toggles automatic iteration over image sequence.
     */
    void slot_play();

    /**
     * @brief [slot] slot_showFirstImage() displays first image.
     */
    void slot_showFirstImage();

    /**
     * @brief [slot] slot_showLastImage() displays last image.
     */
    void slot_showLastImage();

    /**
     * @brief [slot] slot_showPreviousImage() displays image going one or more steps backward from current image.
     * @see VideoPlayerController::slot_changeStepSize to change stepsize.
     */
    void slot_showPreviousImage();

    /**
     * @brief [slot] slot_showNextImage() displays image going one or more steps forward from current image.
     * @see VideoPlayerController::slot_changeStepSize to change stepsize.
     */
    void slot_showNextImage();

    /**
     * @brief [slot] slot_toggleKeyframe() toggles keyframe-state for currently displayed image.
     */
    void slot_toggleKeyframe();

    /**
     * @brief [slot] slot_toggleKeyframesOnly(...) switches between displaying all images or keyframes only.
     * @param checked display all images if @a false, keyframes only if @a true
     */
    void slot_toggleKeyframesOnly(bool checked);

    /**
     * @brief [slot] slot_changeStepSize(...) changes stepsize for iterating the images.
     * @param stepsize new stepsize
     */
    void slot_changeStepSize(unsigned int stepsize);

    /**
     * @brief [slot] slot_changeIndex(...) displays the image from DataManager referenced by given index.
     * @param index of image to display
     */
    void slot_changeIndex(unsigned int index);

    /**
     * @brief [slot] slot_mipChanged() is called when image data changed. VideoPlayer and Timeline are updated.
     */
    void slot_mipChanged();

    /**
     * @brief [slot] slot_deleteAllKeyframes() deletes all selected keyframes.
     */
    void slot_deleteAllKeyframes();

    /**
     * @brief [slot] slot_stopPlay() stops running videoPlayer.
     */
    void slot_stopPlay();

    /**
     * @brief [slot] slot_updateBoundaries() updates the mip so that the boundaries are up to date
     */
    void slot_updateBoundaries();

    /**
     * @brief [slot] slot_imageProcessed(...) is invoked when image has been processed and provides additional images.
     * @param procesedImgs The results of image processing
     * @param id Identifier for the result
     */
    void slot_imageProcessed(cv::Mat *preview, int id);

    /**
     * @brief [slot] slot_redraw() draws selected image again.
     */
    void slot_redraw();

    /**
     * @brief [slot] slot_receiveImage draws given image with and applies a transformable plugin if neccessary
     * @param idx transformable plugin index
     * @param img image which should be displayed or transformed
     */
    void slot_receiveImage(uint idx, const cv::Mat &img);

    /**
     * @brief [slot] slot_displayImage diplays img if transformation is enabled
     * @param idx index if image
     * @param img image
     */
    void slot_displayImage(uint idx, const cv::Mat &img);

    /**
     * @brief [slot] slot_selectedITransformChanged connects new plugin to videoplayer
     * @param id plugin id
     */
    void slot_selectedITransformChanged(uint id);

    /**
     * @brief slot_transformEnabledChanged reads data with new changed enabled parameter
     * @param enabled new enabled parameter
     */
    void slot_transformEnabledChanged(bool enabled);


signals:

    /**
     * @brief [signal] sig_hasStatusMessage(...) is emitted when VideoPlayerController has a status message to display to the user.
     * @param message the QString to display.
     */
    void sig_hasStatusMessage(QString message);

    /**
     * @brief [signal] sig_imageToProcess(...) is emitted when VideoPlayer is about to display a new image.
     * @param image the image thats getting displayed
     * @param id Identifier for the result
     */
    void sig_imageToProcess(cv::Mat *image, int id);

    /**
     * @brief [signal] sig_read sends an image request to the worker thread to read the image.
     * @param idx The image index
     */
    void sig_read(uint idx);

    /**
     * @brief [signal] sig_sendToITransform sends the image for processing to the connected ITransforms.
     * @param idx The image index
     * @param img The image to process
     */
    void sig_sendToITransform(uint idx, const cv::Mat &img);

private slots:
    void slot_timerNextImage();

private:
    VideoPlayer *m_videoPlayer;
    Timeline *m_timeline;
    DataManager *m_dataManager;
    QTimer *m_timer;
    unsigned int m_imageIndex;
    unsigned int m_imageIndexOnScreen;
    unsigned int m_stepsize;
    bool m_keyframesOnly;
    bool m_playing;
    ModelInputIterator *m_iterator;
    AlgorithmController *m_algoController;
    ITransformRequestDequeue *m_transform;

    const int m_frametime = 33; //ms between frames

    void showImage();

};

#endif // VIDEOPLAYERCONTROLLER_H
