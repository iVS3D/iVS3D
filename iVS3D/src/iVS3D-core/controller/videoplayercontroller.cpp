#include "videoplayercontroller.h"


VideoPlayerController::VideoPlayerController(QObject *parent, VideoPlayer *player, Timeline *timeline, DataManager *dataManager, AlgorithmController *algoController)
    : QObject(parent)
{
    m_videoPlayer = player;
    m_timeline = timeline;
    m_dataManager = dataManager;
    m_timer = new QTimer();
    m_timer->start(m_frametime);
    m_imageIndex = 0;
    m_imageIndexOnScreen = 1;
    m_stepsize = 1;
    m_playing = false;
    m_keyframesOnly = false;
    m_iterator = ModelInputIteratorFactory::createIterator(ModelInputIteratorFactory::Images);
    m_algoController = algoController;


    m_videoPlayer->setEnabled(true);
    m_videoPlayer->setEnabledBackBtns(false);
    m_videoPlayer->setKeyframeCount(m_dataManager->getModelInputPictures()->getKeyframeCount());


    m_timeline->setFrames(m_dataManager->getModelInputPictures()->getAllKeyframes(), m_dataManager->getModelInputPictures()->getPicCount());
    m_timeline->setEnabled(true);
    m_timeline->selectFrame(m_imageIndex);


    // connect videoPlayer
    connect(m_videoPlayer, &VideoPlayer::sig_play, this, &VideoPlayerController::slot_play);
    connect(m_videoPlayer, &VideoPlayer::sig_showFirstImage, this, &VideoPlayerController::slot_showFirstImage);
    connect(m_videoPlayer, &VideoPlayer::sig_showLastImage, this, &VideoPlayerController::slot_showLastImage);
    connect(m_videoPlayer, &VideoPlayer::sig_showNextImage, this, &VideoPlayerController::slot_showNextImage);
    connect(m_videoPlayer, &VideoPlayer::sig_showPreviousImage, this, &VideoPlayerController::slot_showPreviousImage);
    connect(m_videoPlayer, &VideoPlayer::sig_toggleKeyframes, this, &VideoPlayerController::slot_toggleKeyframe);
    connect(m_videoPlayer, &VideoPlayer::sig_toggleKeyframesOnly, this, &VideoPlayerController::slot_toggleKeyframesOnly);
    connect(m_videoPlayer, &VideoPlayer::sig_changeStepsize, this, &VideoPlayerController::slot_changeStepSize);
    //connect(m_videoPlayer, &VideoPlayer::sig_deleteAllKeyframes, this, &VideoPlayerController::slot_deleteAllKeyframes);

    // connect timeline
    connect(m_timeline, &Timeline::sig_selectedChanged, this, &VideoPlayerController::slot_changeIndex);
    connect(m_timeline, &Timeline::sig_boundariesChanged, this, &VideoPlayerController::slot_updateBoundaries);

    // connect dataManager->mip
    connect(m_dataManager->getModelInputPictures(), &ModelInputPictures::sig_mipChanged, this, &VideoPlayerController::slot_mipChanged);

    // connect timer
    connect(m_timer, &QTimer::timeout, this, &VideoPlayerController::slot_timerNextImage);

    // connect algoController
    //connect(algoController, &AlgorithmController::sig_updateView, this, &VideoPlayerController::slot_redraw);
    //connect(algoController, QOverload<cv::Mat*,int>::of(&AlgorithmController::sig_transformFinished), this, &VideoPlayerController::slot_imageProcessed);

    ConcurrentReader *r = m_dataManager->getModelInputPictures()->createConcurrentReader();
    r->moveToThread(&workerThread);
    connect(&workerThread, &QThread::finished, r, &ConcurrentReader::deleteLater);
    connect(this, &VideoPlayerController::sig_read, r, &ConcurrentReader::slot_read);
    connect(r, &ConcurrentReader::sig_imageReady, this, &VideoPlayerController::slot_receiveImage);
    workerThread.start();

    m_transform = nullptr;
    TransformManager *tm = &TransformManager::instance();
    connect(tm, &TransformManager::sig_selectedTransformChanged, this, &VideoPlayerController::slot_selectedITransformChanged);
    connect(tm, &TransformManager::sig_transformEnabledChanged, this, &VideoPlayerController::slot_transformEnabledChanged);

    showImage();
}

VideoPlayerController::~VideoPlayerController()
{
    // connect videoPlayer
    disconnect(m_videoPlayer, &VideoPlayer::sig_play, this, &VideoPlayerController::slot_play);
    disconnect(m_videoPlayer, &VideoPlayer::sig_showFirstImage, this, &VideoPlayerController::slot_showFirstImage);
    disconnect(m_videoPlayer, &VideoPlayer::sig_showLastImage, this, &VideoPlayerController::slot_showLastImage);
    disconnect(m_videoPlayer, &VideoPlayer::sig_showNextImage, this, &VideoPlayerController::slot_showNextImage);
    disconnect(m_videoPlayer, &VideoPlayer::sig_showPreviousImage, this, &VideoPlayerController::slot_showPreviousImage);
    disconnect(m_videoPlayer, &VideoPlayer::sig_toggleKeyframes, this, &VideoPlayerController::slot_toggleKeyframe);
    disconnect(m_videoPlayer, &VideoPlayer::sig_toggleKeyframesOnly, this, &VideoPlayerController::slot_toggleKeyframesOnly);
    disconnect(m_videoPlayer, &VideoPlayer::sig_changeStepsize, this, &VideoPlayerController::slot_changeStepSize);

    // connect timeline
    disconnect(m_timeline, &Timeline::sig_selectedChanged, this, &VideoPlayerController::slot_changeIndex);

    // connect dataManager->mip
    disconnect(m_dataManager->getModelInputPictures(), &ModelInputPictures::sig_mipChanged, this, &VideoPlayerController::slot_mipChanged);

    // gui cleanup
    m_timeline->setEnabled(false);
    m_videoPlayer->setEnabled(false);

    cv::Mat mat = cv::imread(":/icons/ivs3dIcon.png");
    m_videoPlayer->showImage(&mat);
    m_videoPlayer->setKeyframe(false);
    m_videoPlayer->setKeyframeCount(0);

    workerThread.quit();
    workerThread.wait();
}

unsigned int VideoPlayerController::getImageIndexOnScreen()
{
    return m_imageIndexOnScreen;
}

void VideoPlayerController::slot_play()
{
    //ignore play if only keyframes on 0 Keyframes is selected
    if(m_keyframesOnly && m_dataManager->getModelInputPictures()->getKeyframeCount() == 0) {
        return;
    }
    m_playing = !m_playing;
    m_videoPlayer->setPlaying(m_playing);
}

void VideoPlayerController::slot_showFirstImage()
{
    m_imageIndex = m_iterator->getFirst(m_dataManager->getModelInputPictures());
    showImage();
    m_timeline->selectFrame(m_imageIndex);
    AlgorithmManager::instance().notifySelectedImageIndex(m_imageIndex);
}

void VideoPlayerController::slot_showLastImage()
{
    m_imageIndex = m_iterator->getLast(m_dataManager->getModelInputPictures());
    showImage();
    m_timeline->selectFrame(m_imageIndex);
    AlgorithmManager::instance().notifySelectedImageIndex(m_imageIndex);
}

void VideoPlayerController::slot_showPreviousImage()
{
    m_imageIndex = m_iterator->getPrevious(m_dataManager->getModelInputPictures(), m_imageIndex, m_stepsize);
    showImage();
    m_timeline->selectFrame(m_imageIndex);
    AlgorithmManager::instance().notifySelectedImageIndex(m_imageIndex);
}

void VideoPlayerController::slot_showNextImage()
{
    m_imageIndex = m_iterator->getNext(m_dataManager->getModelInputPictures(),m_imageIndex, m_stepsize);
    showImage();
    m_timeline->selectFrame(m_imageIndex);
    AlgorithmManager::instance().notifySelectedImageIndex(m_imageIndex);
}

void VideoPlayerController::slot_toggleKeyframe()
{
    if(m_dataManager->getModelInputPictures()->isKeyframe(m_imageIndex)){
        m_dataManager->getModelInputPictures()->removeKeyframe(m_imageIndex);
    } else {
        m_dataManager->getModelInputPictures()->addKeyframe(m_imageIndex);
    }
    m_dataManager->getHistory()->slot_save();
    m_videoPlayer->setKeyframe(m_dataManager->getModelInputPictures()->isKeyframe(m_imageIndex));
    m_videoPlayer->setKeyframeCount(m_dataManager->getModelInputPictures()->getKeyframeCount());
    m_timeline->updateKeyframes(m_dataManager->getModelInputPictures()->getAllKeyframes());
    m_timeline->selectFrame(m_imageIndex);
}

void VideoPlayerController::slot_toggleKeyframesOnly(bool checked)
{
    m_keyframesOnly = checked;
    //delete m_iterator;
    if(checked){
        m_iterator = ModelInputIteratorFactory::createIterator(ModelInputIteratorFactory::Keyframes);
    } else {
        m_iterator = ModelInputIteratorFactory::createIterator(ModelInputIteratorFactory::Images);
    }

    //enable all buttons
    m_videoPlayer->setEnabledBackBtns(true);
    m_videoPlayer->setEnabledForwardBtns(true);

    //disable all buttons if no keyframes exists
    if (m_dataManager->getModelInputPictures()->getKeyframeCount() == 0 && checked == true) {
        m_videoPlayer->setEnabledBackBtns(false);
        m_videoPlayer->setEnabledForwardBtns(false);
    }

    //disable buttons on first and last keyframe
    if(m_iterator->isFirst(m_dataManager->getModelInputPictures(), m_imageIndex)){
        m_videoPlayer->setEnabledBackBtns(false);
    }
    if(m_iterator->isLast(m_dataManager->getModelInputPictures(), m_imageIndex)){
        m_videoPlayer->setEnabledForwardBtns(false);
    }
}

void VideoPlayerController::slot_changeStepSize(unsigned int stepsize)
{
    m_stepsize = stepsize;
}

void VideoPlayerController::slot_changeIndex(unsigned int index)
{
    m_imageIndex = index;
    AlgorithmManager::instance().notifySelectedImageIndex(index);
    showImage();
}

void VideoPlayerController::slot_mipChanged()
{
    m_timeline->updateKeyframes(m_dataManager->getModelInputPictures()->getAllKeyframes());
    m_timeline->selectFrame(m_imageIndex);
    m_videoPlayer->setKeyframeCount(m_dataManager->getModelInputPictures()->getKeyframeCount());
    showImage();
}

void VideoPlayerController::slot_deleteKeyframes()
{
    uint index = m_timeline->selectedFrame();
    m_dataManager->getModelInputPictures()->updateMIP(std::vector<unsigned int>());
    m_dataManager->getHistory()->slot_save();
    m_timeline->selectFrame(index);
    //force to enable correct buttons
    slot_toggleKeyframesOnly(m_keyframesOnly);
}

void VideoPlayerController::slot_deleteAllKeyframes()
{
    ReallyDeleteDialog rdd(m_videoPlayer);
    if(rdd.exec() == QDialog::Accepted){
        uint index = m_timeline->selectedFrame();
        auto bs = m_dataManager->getModelInputPictures()->getBoundaries();
        m_dataManager->getModelInputPictures()->setBoundaries(QPoint(0, m_dataManager->getModelInputPictures()->getPicCount()-1));
        m_dataManager->getModelInputPictures()->updateMIP(std::vector<unsigned int>());
        m_dataManager->getModelInputPictures()->setBoundaries(bs);
        m_dataManager->getHistory()->slot_save();
        m_timeline->selectFrame(index);
        //force to enable correct buttons
        slot_toggleKeyframesOnly(m_keyframesOnly);
    }
}

void VideoPlayerController::slot_stopPlay()
{
    m_playing = false;
    m_videoPlayer->setPlaying(m_playing);
}

void VideoPlayerController::slot_updateBoundaries(QPoint boundaries)
{
    m_dataManager->getModelInputPictures()->setBoundaries(boundaries);
}

void VideoPlayerController::slot_resetBoundaries()
{
    m_timeline->resetBoundaries();
}

void VideoPlayerController::slot_imageProcessed(cv::Mat *preview, int id)
{
    if((uint)id == m_imageIndex){
        m_videoPlayer->showImage(preview);
    }
}

void VideoPlayerController::slot_redraw()
{
    m_imageIndexOnScreen = UINT_MAX;
}

void VideoPlayerController::slot_receiveImage(uint idx, const cv::Mat &img)
{
    if(TransformManager::instance().isTransformEnabled()){
        emit sig_sendToITransform(idx,img);
    } else {
        cv::Mat image = img;
        m_videoPlayer->showImage(&image);
    }
}

void VideoPlayerController::slot_displayImage(uint idx, const cv::Mat &img)
{
    (void) idx;
    if(TransformManager::instance().isTransformEnabled()){
        cv::Mat image = img;
        m_videoPlayer->showImage(&image);
    }
}

void VideoPlayerController::slot_selectedITransformChanged(uint id)
{
    if(m_transform){
        disconnect(m_transform, &ITransform::sendToGui, this, &VideoPlayerController::slot_displayImage);
        disconnect(this, &VideoPlayerController::sig_sendToITransform, m_transform, &ITransformRequestDequeue::slot_transform);
    }
    m_transform = TransformManager::instance().getTransform(id);
    if(m_transform){
        connect(m_transform, &ITransform::sendToGui, this, &VideoPlayerController::slot_displayImage);
        connect(this, &VideoPlayerController::sig_sendToITransform, m_transform, &ITransformRequestDequeue::slot_transform);
    }
    emit sig_read(m_imageIndex);
}

void VideoPlayerController::slot_transformEnabledChanged(bool)
{
    emit sig_read(m_imageIndex);
}

void VideoPlayerController::slot_timerNextImage()
{
    if(m_playing){
        m_imageIndex = m_iterator->getNext(m_dataManager->getModelInputPictures(), m_imageIndex, m_stepsize);
        if(m_iterator->isLast(m_dataManager->getModelInputPictures(), m_imageIndex)){
            m_playing = false;
            m_videoPlayer->setPlaying(false);
            m_videoPlayer->setEnabledForwardBtns(false);
            showImage();
            AlgorithmManager::instance().notifySelectedImageIndex(m_imageIndex);
        } else {
            showImage();
            AlgorithmManager::instance().notifySelectedImageIndex(m_imageIndex);
        }
    }

}

void VideoPlayerController::showImage()
{

    // enable / disable buttons for first / last image
    m_videoPlayer->setEnabledBackBtns(true);
    m_videoPlayer->setEnabledForwardBtns(true);
    if(m_iterator->isFirst(m_dataManager->getModelInputPictures(), m_imageIndex)){
        m_videoPlayer->setEnabledBackBtns(false);
    }
    if(m_iterator->isLast(m_dataManager->getModelInputPictures(), m_imageIndex)){
        m_videoPlayer->setEnabledForwardBtns(false);
    }

    // update keyframe border and timeline
    m_videoPlayer->setKeyframe(m_dataManager->getModelInputPictures()->isKeyframe(m_imageIndex));
    m_timeline->selectFrame(m_imageIndex);

    m_imageIndexOnScreen = m_imageIndex;

    emit sig_read(m_imageIndex);
}
