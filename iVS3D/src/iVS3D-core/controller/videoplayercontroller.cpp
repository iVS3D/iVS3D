#include "videoplayercontroller.h"

VideoPlayerController::VideoPlayerController(QObject* parent,
                                             VideoPlayer* player,
                                             Timeline* timeline,
                                             DataManager* dataManager,
                                             std::shared_ptr<PluginThread> pluginThread)
    : QObject(parent), m_pluginThread(std::move(pluginThread)) {
    m_videoPlayer = player;
    m_timeline = timeline;
    m_dataManager = dataManager;
    m_frametimer = new QTimer();
    m_frametimer->start(m_frametime);
    m_imageIndex = 0;
    m_imageIndexOnScreen = 1;
    m_stepsize = 1;
    m_playing = false;
    m_keyframesOnly = true;
    m_iterator = ModelInputIteratorFactory::createIterator(
        ModelInputIteratorFactory::Keyframes);

    m_boundaryMoveTimer = new QTimer(this);
    m_boundaryMoveTimer->setSingleShot(true);
    m_boundaryMoveTimer->setInterval(BOUDNARY_STATIONARY_DURATION);
    connect(m_boundaryMoveTimer, &QTimer::timeout, this,
            &VideoPlayerController::slot_boundaryStopped);

    m_videoPlayer->setEnabled(true);
    m_videoPlayer->setEnabledBackBtns(false);
    m_videoPlayer->setStepsize(m_stepsize);
    m_videoPlayer->setPlaying(m_playing);
    m_videoPlayer->setKeyframesOnly(m_keyframesOnly);

    m_timeline->setFrames(
        m_dataManager->getModelInputPictures()->getAllKeyframes(false),
        m_dataManager->getModelInputPictures()->getPicCount());
    m_timeline->setEnabled(true);
    m_timeline->selectFrame(m_imageIndex);
    QPoint boundaries = m_dataManager->getModelInputPictures()->getBoundaries();
    m_timeline->setBoundaries(boundaries);

    uint inBoundKeyframeCount =
        m_dataManager->getModelInputPictures()->getKeyframeCount(true);
    m_videoPlayer->setKeyframeCount(inBoundKeyframeCount);

    // connect videoPlayer
    connect(m_videoPlayer, &VideoPlayer::sig_play, this,
            &VideoPlayerController::slot_play);
    connect(m_videoPlayer, &VideoPlayer::sig_showFirstImage, this,
            &VideoPlayerController::slot_showFirstImage);
    connect(m_videoPlayer, &VideoPlayer::sig_showLastImage, this,
            &VideoPlayerController::slot_showLastImage);
    connect(m_videoPlayer, &VideoPlayer::sig_showNextImage, this,
            &VideoPlayerController::slot_showNextImage);
    connect(m_videoPlayer, &VideoPlayer::sig_showPreviousImage, this,
            &VideoPlayerController::slot_showPreviousImage);
    connect(m_videoPlayer, &VideoPlayer::sig_toggleKeyframes, this,
            &VideoPlayerController::slot_toggleKeyframe);
    connect(m_videoPlayer, &VideoPlayer::sig_toggleKeyframesOnly, this,
            &VideoPlayerController::slot_toggleKeyframesOnly);
    connect(m_videoPlayer, &VideoPlayer::sig_changeStepsize, this,
            &VideoPlayerController::slot_changeStepSize);

    // connect timeline
    connect(m_timeline, &Timeline::sig_selectedChanged, this,
            &VideoPlayerController::slot_changeIndex);
    connect(m_timeline, &Timeline::sig_boundariesChanged, this,
            &VideoPlayerController::slot_updateBoundaries);

    // connect dataManager->mip
    connect(m_dataManager->getModelInputPictures(),
            &ModelInputPictures::sig_mipChanged, this,
            &VideoPlayerController::slot_mipChanged);

    // connect timer
    connect(m_frametimer, &QTimer::timeout, this,
            &VideoPlayerController::slot_timerNextImage);

    auto* reader = m_dataManager->getModelInputPictures()->getReader();
    m_asyncImageLoader = std::make_unique<AsyncImageLoader>(
        [reader](const ImageRequest& req) {
            ImageResult result;
            result.idx = req.idx;
            result.img = reader->getPic(req.idx, Reader::APPLY_NONE);
            return result;
        },
        nullptr);

    connect(m_asyncImageLoader.get(), &AsyncImageLoader::finished, this,
            &VideoPlayerController::slot_receiveImage);

    connect(m_pluginThread.get(), &PluginThread::previewFinished, this,
            &VideoPlayerController::slot_receiveVisualization);

    showImage();
}

VideoPlayerController::~VideoPlayerController() {
    // connect videoPlayer
    disconnect(m_videoPlayer, &VideoPlayer::sig_play, this,
               &VideoPlayerController::slot_play);
    disconnect(m_videoPlayer, &VideoPlayer::sig_showFirstImage, this,
               &VideoPlayerController::slot_showFirstImage);
    disconnect(m_videoPlayer, &VideoPlayer::sig_showLastImage, this,
               &VideoPlayerController::slot_showLastImage);
    disconnect(m_videoPlayer, &VideoPlayer::sig_showNextImage, this,
               &VideoPlayerController::slot_showNextImage);
    disconnect(m_videoPlayer, &VideoPlayer::sig_showPreviousImage, this,
               &VideoPlayerController::slot_showPreviousImage);
    disconnect(m_videoPlayer, &VideoPlayer::sig_toggleKeyframes, this,
               &VideoPlayerController::slot_toggleKeyframe);
    disconnect(m_videoPlayer, &VideoPlayer::sig_toggleKeyframesOnly, this,
               &VideoPlayerController::slot_toggleKeyframesOnly);
    disconnect(m_videoPlayer, &VideoPlayer::sig_changeStepsize, this,
               &VideoPlayerController::slot_changeStepSize);

    // connect timeline
    disconnect(m_timeline, &Timeline::sig_selectedChanged, this,
               &VideoPlayerController::slot_changeIndex);

    // connect dataManager->mip
    disconnect(m_dataManager->getModelInputPictures(),
               &ModelInputPictures::sig_mipChanged, this,
               &VideoPlayerController::slot_mipChanged);

    // gui cleanup
    m_timeline->setEnabled(false);
    m_videoPlayer->setEnabled(false);
}

unsigned int VideoPlayerController::getImageIndexOnScreen() {
    return m_imageIndexOnScreen;
}

void VideoPlayerController::resetLayout() { 
    m_videoPlayer->clear();
    m_currentImage = cv::Mat();
}

void VideoPlayerController::setPreviewPlugin(const QString& previewPluginName) {
    m_currentPreviewPlugin = previewPluginName;
}

void VideoPlayerController::clearPreviewPlugin() {
    m_currentPreviewPlugin.reset();
}

void VideoPlayerController::slot_play() {
    // ignore play if only keyframes on 0 Keyframes is selected
    if (m_keyframesOnly &&
        m_dataManager->getModelInputPictures()->getKeyframeCount(false) == 0) {
        return;
    }
    m_playing = !m_playing;
    m_videoPlayer->setPlaying(m_playing);
}

void VideoPlayerController::slot_showFirstImage() {
    m_imageIndex = m_iterator->getFirst(m_dataManager->getModelInputPictures());
    showImage();
    m_timeline->selectFrame(m_imageIndex);
    m_pluginThread->onIndexChanged(m_imageIndex);
}

void VideoPlayerController::slot_showLastImage() {
    m_imageIndex = m_iterator->getLast(m_dataManager->getModelInputPictures());
    showImage();
    m_timeline->selectFrame(m_imageIndex);
    m_pluginThread->onIndexChanged(m_imageIndex);
}

void VideoPlayerController::slot_showPreviousImage() {
    m_imageIndex = m_iterator->getPrevious(
        m_dataManager->getModelInputPictures(), m_imageIndex, m_stepsize);
    showImage();
    m_timeline->selectFrame(m_imageIndex);
    m_pluginThread->onIndexChanged(m_imageIndex);
}

void VideoPlayerController::slot_showNextImage() {
    m_imageIndex = m_iterator->getNext(m_dataManager->getModelInputPictures(),
                                       m_imageIndex, m_stepsize);
    showImage();
    m_timeline->selectFrame(m_imageIndex);
    m_pluginThread->onIndexChanged(m_imageIndex);
}

void VideoPlayerController::slot_toggleKeyframe() {
    if (m_dataManager->getModelInputPictures()->isKeyframe(m_imageIndex)) {
        m_dataManager->getModelInputPictures()->removeKeyframe(m_imageIndex);
        emit sig_toggleKeyframe(m_imageIndex, false);
    } else {
        m_dataManager->getModelInputPictures()->addKeyframe(m_imageIndex);
        emit sig_toggleKeyframe(m_imageIndex, true);
    }
    m_dataManager->getHistory()->slot_save();
    bool isKeyframe =
        m_dataManager->getModelInputPictures()->isKeyframe(m_imageIndex);
    m_videoPlayer->setKeyframe(isKeyframe);
    m_videoPlayer->setKeyframeCount(
        m_dataManager->getModelInputPictures()->getKeyframeCount(true));
    m_timeline->updateKeyframes(
        m_dataManager->getModelInputPictures()->getAllKeyframes(false));
    m_timeline->selectFrame(m_imageIndex);

    // if we just added a new keyframe, stay there
    if (isKeyframe) {
        return;
    }
    // otherwise we just removed a keyframe, move to the next image or keyframe
    // if there is one
    if (!m_iterator->isLast(m_dataManager->getModelInputPictures(),
                            m_imageIndex)) {
        slot_showNextImage();
    }
}

void VideoPlayerController::slot_toggleKeyframesOnly(bool checked) {
    m_keyframesOnly = checked;
    // delete m_iterator;
    if (checked) {
        m_iterator = ModelInputIteratorFactory::createIterator(
            ModelInputIteratorFactory::Keyframes);
    } else {
        m_iterator = ModelInputIteratorFactory::createIterator(
            ModelInputIteratorFactory::Images);
    }

    // enable all buttons
    m_videoPlayer->setEnabledBackBtns(true);
    m_videoPlayer->setEnabledForwardBtns(true);

    // disable all buttons if no keyframes exists
    if (m_dataManager->getModelInputPictures()->getKeyframeCount(false) == 0 &&
        checked == true) {
        m_videoPlayer->setEnabledBackBtns(false);
        m_videoPlayer->setEnabledForwardBtns(false);
    }

    // disable buttons on first and last keyframe
    if (m_iterator->isFirst(m_dataManager->getModelInputPictures(),
                            m_imageIndex)) {
        m_videoPlayer->setEnabledBackBtns(false);
    }
    if (m_iterator->isLast(m_dataManager->getModelInputPictures(),
                           m_imageIndex)) {
        m_videoPlayer->setEnabledForwardBtns(false);
    }
}

void VideoPlayerController::slot_changeStepSize(unsigned int stepsize) {
    m_stepsize = stepsize;
}

void VideoPlayerController::slot_changeIndex(unsigned int index) {
    m_imageIndex = index;
    m_pluginThread->onIndexChanged(index);
    showImage();
}

void VideoPlayerController::slot_mipChanged() {
    m_timeline->updateKeyframes(
        m_dataManager->getModelInputPictures()->getAllKeyframes(false));
    m_timeline->selectFrame(m_imageIndex);
    m_timeline->setBoundaries(
        m_dataManager->getModelInputPictures()->getBoundaries());
    m_videoPlayer->setKeyframeCount(
        m_dataManager->getModelInputPictures()->getKeyframeCount(true));
    showImage();
}

void VideoPlayerController::slot_deleteKeyframes() {
    uint index = m_timeline->selectedFrame();
    m_dataManager->getModelInputPictures()->updateMIP(
        std::vector<unsigned int>());
    emit sig_deleteKeyframes();
    m_dataManager->getHistory()->slot_save();
    m_timeline->selectFrame(index);
    // force to enable correct buttons
    slot_toggleKeyframesOnly(m_keyframesOnly);
}

void VideoPlayerController::slot_deleteAllKeyframes() {
    ReallyDeleteDialog rdd(m_videoPlayer);
    if (rdd.exec() == QDialog::Accepted) {
        uint index = m_timeline->selectedFrame();
        auto bs = m_dataManager->getModelInputPictures()->getBoundaries();
        m_dataManager->getModelInputPictures()->setBoundaries(QPoint(
            0, m_dataManager->getModelInputPictures()->getPicCount() - 1));
        std::vector<uint> allFrames;
        allFrames.reserve(
            m_dataManager->getModelInputPictures()->getPicCount());
        for (uint i = 0;
             i < m_dataManager->getModelInputPictures()->getPicCount(); i++) {
            allFrames.push_back(i);
        }
        m_dataManager->getModelInputPictures()->updateMIP(allFrames);
        m_dataManager->getModelInputPictures()->setBoundaries(bs);
        emit sig_deleteAllKeyframes();
        m_dataManager->getHistory()->slot_save();
        m_timeline->selectFrame(index);
        // force to enable correct buttons
        slot_toggleKeyframesOnly(m_keyframesOnly);
    }
}

void VideoPlayerController::slot_stopPlay() {
    m_playing = false;
    m_videoPlayer->setPlaying(m_playing);
}

void VideoPlayerController::slot_updateBoundaries(QPoint boundaries) {
    int frameCount = m_dataManager->getModelInputPictures()->getPicCount();
    if (boundaries.x() < 0) boundaries.setX(0);
    if (boundaries.y() >= frameCount) boundaries.setY(frameCount - 1);

    QPoint oldBoundaries =
        m_dataManager->getModelInputPictures()->getBoundaries();
    int changedBoundIdx = 0;
    if (oldBoundaries.x() != boundaries.x())
        changedBoundIdx = boundaries.x();
    else if (oldBoundaries.y() != boundaries.y())
        changedBoundIdx = boundaries.y();
    else
        return;

    m_imageIndexOnScreen = changedBoundIdx;
    m_asyncImageLoader->request({static_cast<uint>(changedBoundIdx)});

    m_boundaryMoveTimer
        ->start();  // reset timer to indicate boundary is still moving

    m_dataManager->getModelInputPictures()->setBoundaries(boundaries);
    uint inBoundKeyframeCount =
        m_dataManager->getModelInputPictures()->getKeyframeCount(true);
    m_videoPlayer->setKeyframeCount(inBoundKeyframeCount);
}

void VideoPlayerController::slot_resetBoundaries() {
    m_timeline->resetBoundaries();
}

void VideoPlayerController::slot_redraw() {
    m_imageIndexOnScreen = UINT_MAX;
    showImage();
}

void VideoPlayerController::slot_receiveImage(const ImageRequest& request,
                                              const ImageResult& result) {
    // update status bar with info about corrupted images
    if (result.img.empty()) {
        m_foundCorruptedFrames.insert(result.idx);

        // create dynamic status bar error message
        QString errorMsg = (m_foundCorruptedFrames.size() > 1
                                ? tr("Frames %1 are corrupted. They won´t be "
                                     "considered when selecting keyframes or "
                                     "exported at the end of the process.")
                                : tr("Frame %1 is corrupted. It won´t be "
                                     "considered when selecting keyframes or "
                                     "exported at the end of the process."));
        std::stringstream ss_foundCorruptedFrames;
        int listedFramesCount = 0;
        for (int i : m_foundCorruptedFrames) {
            ss_foundCorruptedFrames << std::to_string(i);
            if (listedFramesCount < int(m_foundCorruptedFrames.size()) -
                                        1)  // prevent trailing ", "
                ss_foundCorruptedFrames << ", ";
            if (listedFramesCount >
                ERROR_MSG_APPROX_COUNT) {  // cut listing if there are too many
                ss_foundCorruptedFrames << "...";
                break;
            }
            listedFramesCount++;
        }
        emit sig_hasStatusMessage(errorMsg.arg(
            QString::fromStdString(ss_foundCorruptedFrames.str())));

        return;
    }

    std::shared_ptr<ReaderParams> params =
        m_dataManager->getModelInputPictures()->getReaderParams();

    // display the image on the video player
    if (!(Resolution(m_currentImage) == params->getWorkingResolution())) {
        resetLayout();
    }
    params->getWorkingResolution().resize(result.img, m_currentImage);
    m_videoPlayer->showImage(m_currentImage);

    // update ROI display
    QRect roi = params->getRoi().cropAsQRect(params->getWorkingResolution());
    m_videoPlayer->updateRoi(params->getUseRoi() ? roi : QRect());

    // crop image if ROI is enabled
    cv::Mat croppedImage = m_currentImage;
    if (params->getUseRoi()) {
        params->getRoi().crop(croppedImage);
    }

    // request visualization if preview plugin is available and image is still
    // the one on screen
    if (m_currentPreviewPlugin && (m_imageIndexOnScreen == result.idx)) {
        m_pluginThread->requestPreview(*m_currentPreviewPlugin,
                                      {m_imageIndexOnScreen, croppedImage});
    }
}

void VideoPlayerController::slot_receiveVisualization(
    const PreviewResult& result) {
    if (m_imageIndexOnScreen != result.idx) {
        //return;  // discard outdated visualization
    }
    if (!result.visualization) {
        QMessageBox::warning(
            m_videoPlayer, tr("Error"),
            tr("An error occurred during preview generation.\n%1")
                .arg(result.visualization.error().message));
        emit sig_disablePreview();
        return;
    }
    m_videoPlayer->showVisualization(result.visualization.value());
}

void VideoPlayerController::slot_refreshPreview(bool clearOldPreview) {
    if (m_imageIndexOnScreen == UINT_MAX || m_currentImage.empty() || !m_currentPreviewPlugin) {
        return;
    }
    if (clearOldPreview) {
        m_videoPlayer->clearVisualization();  // clear current visualization
    }
    std::shared_ptr<ReaderParams> params =
        m_dataManager->getModelInputPictures()->getReaderParams();
    if (params->getUseRoi()) {
        cv::Mat croppedImage = m_currentImage;
        params->getRoi().crop(croppedImage);
        m_pluginThread->requestPreview(*m_currentPreviewPlugin,
                                       {m_imageIndexOnScreen, croppedImage});
    } else {
        m_pluginThread->requestPreview(*m_currentPreviewPlugin,
                                       {m_imageIndexOnScreen, m_currentImage});
    }
}

void VideoPlayerController::slot_timerNextImage() {
    if (m_playing) {
        m_imageIndex = m_iterator->getNext(
            m_dataManager->getModelInputPictures(), m_imageIndex, m_stepsize);
        if (m_iterator->isLast(m_dataManager->getModelInputPictures(),
                               m_imageIndex)) {
            m_playing = false;
            m_videoPlayer->setPlaying(false);
            m_videoPlayer->setEnabledForwardBtns(false);
            showImage();
        } else {
            showImage();
        }
        m_pluginThread->onIndexChanged(m_imageIndex);
    }
}

void VideoPlayerController::slot_boundaryStopped() {
    m_imageIndexOnScreen = m_imageIndex;
    m_asyncImageLoader->request({static_cast<uint>(m_imageIndex)});
}

void VideoPlayerController::showImage() {
    // enable / disable buttons for first / last image
    m_videoPlayer->setEnabledBackBtns(true);
    m_videoPlayer->setEnabledForwardBtns(true);
    if (m_iterator->isFirst(m_dataManager->getModelInputPictures(),
                            m_imageIndex)) {
        m_videoPlayer->setEnabledBackBtns(false);
    }
    if (m_iterator->isLast(m_dataManager->getModelInputPictures(),
                           m_imageIndex)) {
        m_videoPlayer->setEnabledForwardBtns(false);
    }

    // update keyframe border and timeline
    m_videoPlayer->setKeyframe(
        m_dataManager->getModelInputPictures()->isKeyframe(m_imageIndex));
    m_timeline->selectFrame(m_imageIndex);

    m_imageIndexOnScreen = m_imageIndex;

    m_asyncImageLoader->request({m_imageIndex});
}
