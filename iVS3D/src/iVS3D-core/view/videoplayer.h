#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <QWidget>
#include <QShortcut>
#include <QKeySequence>

#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"
#include "ui_videoplayer.h"


namespace Ui {
class VideoPlayer;
}

/**
 * @class VideoPlayer
 *
 * @ingroup View
 *
 * @brief The VideoPlayer class provides a view to display images and holds buttons to interact with image sequences. \n
 * These buttons are:
 * <table>
 * <tr><th>Button       <th>Function                        <th> Emitted signal
 * <tr><td>@a |<<       <td>display first image             <td> VideoPlayer::sig_showFirstImage()
 * <tr><td>@a >>|       <td>display last image              <td> VideoPlayer::sig_showLastImage()
 * <tr><td>@a |<        <td>display previous image          <td> VideoPlayer::sig_showPreviousImage()
 * <tr><td>@a >|        <td>display next image              <td> VideoPlayer::sig_showNextImage()
 * <tr><td>@a > / @a || <td>play / pause image sequence     <td> VideoPlayer::sig_play()
 * </table>
 * Depending on the selected image, these buttons can be disabled. The state of the play / pause button can be changed using
 * VideoPlayer::setPlaying(). The stepsize for playing the image sequence can be changed as well.
 *
 * Images are displayed using VideoPlayer::showPixmap() and can be highlighted using VideoPlayer::setKeyframe(). Keyframes can be added
 * and removed one by one or all at once. The VideoPlayer handles resizing of the images.
 *
 * @author Dominik Wüst
 *
 * @date 2021/03/02
 */
class VideoPlayer : public QWidget
{
    Q_OBJECT

public:

    /**
     * @brief Create a VideoPlayer and set the Gui to dark mode, if dark is @a true.
     * @param parent Parent for the QWidget
     * @param dark Gui is dark if @a true, light otherwise
     */
    explicit VideoPlayer(QWidget *parent = nullptr, bool dark = false);

    ~VideoPlayer();

    /**
     * @brief showImages displays the given images.
     * @param images The images as cv::Mat to display
     */
    void showImages(std::vector<cv::Mat*> images);

    /**
     * @brief showImage displays the given image.
     * @param image The image as cv:Mat
     */
    void showImage(cv::Mat *image);

    /**
     * @brief setKeyframe highlights the displayed image if isKeyframe is @a true.
     * @param isKeyframe Highlights the image if @a true
     */
    void setKeyframe(bool isKeyframe);

    /**
     * @brief setKeyframeCount displays the given number under keyframe count.
     * @param keyframeCount The number of keyframes
     */
    void setKeyframeCount(unsigned int keyframeCount);

    /**
     * @brief setEnabledBackBtns enables or disables the @a |<< and @a |< buttons.
     * @param enabled Enable the buttons if @a true, disable otherwise
     */
    void setEnabledBackBtns(bool enabled);

    /**
     * @brief setEnabledForwardBtns enables or disables the @a >| and @a >>| buttons.
     * @param enabled Enable the buttons if @a true, disable otherwise
     */
    void setEnabledForwardBtns(bool enabled);

    /**
     * @brief setPlaying changes the play/pause button between @a > and @a ||.
     * @param playing Show @a || if @a true, @a > otherwise
     */
    void setPlaying(bool playing);

    /**
     * @brief setStepsize changes the value of the stepsize box.
     * @param stepsize The new stepsize value
     */
    void setStepsize(unsigned int stepsize);

    /**
     * @brief addWidgetToLayout adds the given QWidget to the VideoPlayer between the displayed image and the interaction buttons.
     * @param widget The QWidget to add
     */
    void addWidgetToLayout(QWidget *widget);

    /**
     * @brief removeWidgetFromLayout removes the given QWidget from the VideoPlayer.
     * @param widget The QWidget to remove
     */
    void removeWidgetFromLayout(QWidget *widget);

signals:

    /**
     * @brief [signal] sig_play() is emitted on play / pause button press.
     */
    void sig_play();

    /**
     * @brief [signal] sig_showNextImage() is emitted on show next button press.
     */
    void sig_showNextImage();

    /**
     * @brief [signal] sig_showPreviousImage() is emitted on show previous button press.
     */
    void sig_showPreviousImage();

    /**
     * @brief [signal] sig_showFirstImage() is emitted on show first button press.
     */
    void sig_showFirstImage();

    /**
     * @brief [signal] sig_showLastImage() is emitted on show last button press.
     */
    void sig_showLastImage();

    /**
     * @brief [signal] sig_toggleKeyframes() is emitted on set keyframe / remove keyframe button press.
     */
    void sig_toggleKeyframes();

    /**
     * @brief [signal] sig_toggleKeyframesOnly(...) is emitted if the keyframes only checkbox is checked/unchecked.
     * @param checked Is @a true if the scheckbox is checked, @a false otherwise
     */
    void sig_toggleKeyframesOnly(bool checked);

    /**
     * @brief [signal] sig_changeStepsize(...) is emitted if stepsize changed.
     * @param stepsize The new stepsize
     */
    void sig_changeStepsize(unsigned int stepsize);

    /**
     * @brief [signal] sig_deleteAllKeyframes() is emitted on delete all button press.
     */
    void sig_deleteAllKeyframes();

protected:
    void resizeEvent(QResizeEvent *e); // used to resize displayed image

private slots:
    void on_pushButton_firstPic_clicked();
    void on_pushButton_prevPic_clicked();
    void on_pushButton_playPause_clicked();
    void on_pushButton_nextPic_clicked();
    void on_pushButton_lastPic_clicked();
    void on_checkBox_onlyKeyframes_stateChanged(int arg1);
    void on_pushButton_setKeyframe_clicked();
    void on_spinBox_stepsize_valueChanged(int arg1);
    void on_pushButton_clicked();

private:
    Ui::VideoPlayer *ui;
    bool m_dark;
    QShortcut *m_prevSC;
    QShortcut *m_nextSC;

    QImage qImageFromCvMat(cv::Mat* input, bool bgr = true);
    void alphaBlend(cv::Mat *foreground, cv::Mat *background, float alpha, cv::Mat &output);
};

#endif // VIDEOPLAYER_H
