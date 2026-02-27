#ifndef VIDEOPLAYERCONTROLLER_H
#define VIDEOPLAYERCONTROLLER_H

#include <QObject>  // used for signals and slots
#include <QTimer>   // used for periodic timer events to update displayed image
#include <QMessageBox>  // used to display error messages

#include <set>
#include <memory>

#include "DataManager.h"  // used to access image data for displaying and manipulation

#include "view/videoplayer.h"   // used to display images
#include "view/timeline.h"      // used to visualize keyframe distribution

#include "controller/ModelInputIterator.h"  // used to iterate over given image data

#include "controller/imageiterator.h"
#include "controller/modelinputiteratorfactory.h"
#include "view/reallydeletedialog.h"

#include "model/asyncimageloader.h"
#include "plugin/pluginthread.h"

#include "ipreview.h"

#include "cvmat_qmetadata.h"

#define ERROR_MSG_APPROX_COUNT 5
#define BOUDNARY_STATIONARY_DURATION 1000 // in ms

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
    explicit VideoPlayerController(QObject *parent = nullptr, VideoPlayer *player = nullptr, Timeline *timeline = nullptr, DataManager *dataManager = nullptr, std::shared_ptr<PluginThread> pluginThread = nullptr);

    /**
     * @brief disconnects signals, deletes timer and VideoPlayerController instance.
     */
    ~VideoPlayerController();

    /**
     * @brief getImageIndexOnScreen getter for m_imageIndexOnScreen (only used in tests for now)
     * @return index of image currently displayed
     */
    unsigned int getImageIndexOnScreen();

    void resetLayout();

    void setPreviewPlugin(const QString& previewPluginName);

    void clearPreviewPlugin();

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
     * @brief [slot] slot_deleteKeyframes() deletes all selected keyframes.
     */
    void slot_deleteKeyframes();

    /**
     * @brief [slot] slot_deleteAllKeyframes() deletes all keyframes.
     */
    void slot_deleteAllKeyframes();

    /**
     * @brief [slot] slot_stopPlay() stops running videoPlayer.
     */
    void slot_stopPlay();

    /**
     * @brief [slot] slot_updateBoundaries() updates the mip so that the boundaries are up to date
     */
    void slot_updateBoundaries(QPoint boundaries);

    /**
     * @brief [slot] slot_resetBoundaries() updates the mip so that the boundaries are back to default
     */
    void slot_resetBoundaries();

    /**
     * @brief [slot] slot_redraw() draws selected image again.
     */
    void slot_redraw();

    /**
     * @brief [slot] slot_refreshPreview() updates the visualization for the currently displayed image by requesting a new preview from the preview plugin.
     */
    void slot_refreshPreview(bool clearOldPreview);

signals:

    /**
     * @brief [signal] sig_hasStatusMessage(...) is emitted when VideoPlayerController has a status message to display to the user.
     * @param message the QString to display.
     */
    void sig_hasStatusMessage(QString message);

    /**
     * @brief [signal] sig_toggleKeyframe notifies about a manual keyframe change by clicking Add keyframe/Remove keyframe
     * @param idx Index of the changed frame
     * @param isNowKeyframe @a true if the frame wasn't a keyframe and became one by the user, @a false otherwise
     */
    void sig_toggleKeyframe(uint idx, bool isNowKeyframe);

    /**
     * @brief [signal] sig_deleteKeyframes is emitted when all keyframes are deleted
     */
    void sig_deleteKeyframes();

    /**
     * @brief [signal] sig_deleteAllKeyframes is emitted when frames are reseted to be keyframes
     */
    void sig_deleteAllKeyframes();

    void sig_disablePreview();

private slots:
    void slot_timerNextImage();
    void slot_boundaryStopped();
    void slot_receiveImage(const ImageRequest &request, const ImageResult &result);
    void slot_receiveVisualization(const PreviewResult& result);

private:
    VideoPlayer *m_videoPlayer;
    Timeline *m_timeline;
    DataManager *m_dataManager;
    QTimer *m_frametimer;
    QTimer *m_boundaryMoveTimer;

    // current image data
    cv::Mat m_currentImage;
    QRect m_roi;
    QRect m_sceneBoundaries;

    unsigned int m_imageIndex;
    unsigned int m_imageIndexOnScreen;
    unsigned int m_stepsize;
    bool m_keyframesOnly;
    bool m_playing;
    ModelInputIterator *m_iterator;

    const int m_frametime = 33; //ms between frames

    std::set<int> m_foundCorruptedFrames = {};

    void showImage();

    std::unique_ptr<AsyncImageLoader> m_asyncImageLoader;
    std::shared_ptr<PluginThread> m_pluginThread;
    std::optional<QString> m_currentPreviewPlugin;

};

#endif // VIDEOPLAYERCONTROLLER_H
