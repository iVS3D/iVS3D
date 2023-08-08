#include <QtTest>
#include <QCoreApplication>

#include "DataManager.h"
#include "controller/videoplayercontroller.h"
#include "view/timeline.h"
#include "view/videoplayer.h"
#include "resourceloader.h"



class tst_videoplayercontroller : public QObject
{
    Q_OBJECT

public:
    tst_videoplayercontroller();
    ~tst_videoplayercontroller();

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void test_displayRandomImgs();
    void test_displayBorderLineCases();
    void test_playpause();
    void test_iterateKeyFrames();
    void test_SetRemoveKeyframes();
private:
    VideoPlayerController *m_testVPC;
    VideoPlayer *m_testVP;
    Timeline *m_testTL;
    DataManager *m_testDM;
    QString m_testresourcePath;
    QString m_testVideoPath;


    std::vector<uint> genRNum(uint maxNum, uint count);
};

tst_videoplayercontroller::tst_videoplayercontroller() {

}

tst_videoplayercontroller::~tst_videoplayercontroller() {

}

void tst_videoplayercontroller::initTestCase()
{
    srand (time(0));

    m_testVPC = nullptr;
    m_testDM = nullptr;
    m_testTL = nullptr;
    m_testVP = nullptr;

    m_testresourcePath = QString(TEST_RESOURCES);
    m_testVideoPath = m_testresourcePath + "/video.mp4";
    requireResource(m_testVideoPath);
}

void tst_videoplayercontroller::cleanupTestCase()
{

}

void tst_videoplayercontroller::init()
{
    m_testVP = new VideoPlayer();
    m_testDM = new DataManager();
    m_testDM->open(m_testVideoPath);
    QVERIFY(m_testDM->getModelInputPictures()->getPicCount() != 0);
    m_testTL = new Timeline();
    m_testVPC = new VideoPlayerController(this, m_testVP, m_testTL, m_testDM, nullptr);
    m_testVPC->slot_mipChanged();
}

void tst_videoplayercontroller::cleanup()
{
    qDebug () << "start cleanup";
    if (m_testVPC != nullptr) {
        delete m_testVPC;
    }
    m_testVPC = nullptr;

    if (m_testVP != nullptr) {
        delete m_testVP;
    }
    m_testVP = nullptr;

    if (m_testDM != nullptr) {
        delete m_testDM;
    }
    m_testDM = nullptr;

    if (m_testTL != nullptr) {
        delete m_testTL;
    }
    m_testTL = nullptr;

    qDebug () << "cleanup done";
}

void tst_videoplayercontroller::test_displayRandomImgs()
{
    std::vector<uint> randomIdx = genRNum(m_testDM->getModelInputPictures()->getPicCount(), 20);

    for (uint i = 0; i < randomIdx.size(); ++i) {
        m_testVPC->slot_changeIndex(randomIdx[i]);
        QVERIFY2(m_testVPC->getImageIndexOnScreen() == randomIdx[i], "displaying wrong picture");
    }
}

void tst_videoplayercontroller::test_displayBorderLineCases()
{
    m_testVPC->slot_showLastImage();
    QVERIFY2(m_testVPC->getImageIndexOnScreen() == m_testDM->getModelInputPictures()->getPicCount()-1, "not displaying last picture");

    m_testVPC->slot_showFirstImage();
    QVERIFY2(m_testVPC->getImageIndexOnScreen() == 0, "not displaying first picture");


    std::vector<uint> randomIdx = genRNum(m_testDM->getModelInputPictures()->getPicCount() - 1, 2);
    QVERIFY(randomIdx.size() == 2);
    uint next = randomIdx[0];
    uint prev = randomIdx[1];
    if (randomIdx[0] < randomIdx[1]) {
        next = randomIdx[1];
        prev = randomIdx[0];
    }
    QVERIFY (next > prev);
    qDebug () << "next " << next << " prev " << prev;
    for(uint i = 0; i < next; ++i) {
        m_testVPC->slot_showNextImage();
        QVERIFY(m_testVPC->getImageIndexOnScreen() == (i+1));
    }
    for(uint i = 0; i < prev; ++i) {
        m_testVPC->slot_showPreviousImage();
        QVERIFY(m_testVPC->getImageIndexOnScreen() == next-i-1);
    }


}

void tst_videoplayercontroller::test_playpause()
{
    std::vector<uint> randomIdx = genRNum(50, 1);
    QVERIFY(randomIdx.size() == 1);
    m_testVPC->slot_play();
    std::vector<uint> displayedImages;
    for (uint i = 0; i < randomIdx[0]; ++i) {
        displayedImages.push_back(m_testVPC->getImageIndexOnScreen());
        QTest::qWait(50);
    }
    m_testVPC->slot_stopPlay();
    std::vector<uint> stoppedImages;
    for (uint i = 0; i < randomIdx[0]; ++i) {
        stoppedImages.push_back(m_testVPC->getImageIndexOnScreen());
        QTest::qWait(50);
    }
    QVERIFY(displayedImages.size() == stoppedImages.size());
    for (uint i = 1; i < displayedImages.size(); ++i) {
        QVERIFY2(displayedImages[i-1] <= displayedImages[i], "jumped a frame back");
        QVERIFY2(stoppedImages[i-1] == stoppedImages[i], "stopped play but image changed");
    }
}

void tst_videoplayercontroller::test_iterateKeyFrames()
{
    ModelInputPictures *mip = m_testDM->getModelInputPictures();
    std::vector<uint> randomIdx = genRNum(mip->getPicCount() - 1, 50);
    QVERIFY(randomIdx.size() == 50);
    for (int i = 0; i < 50; ++i) {
        mip->addKeyframe(randomIdx[i]);
    }
    std::vector<uint> keyframes = mip->getAllKeyframes(false);
    m_testVPC->slot_toggleKeyframesOnly(true);
    m_testVPC->slot_showFirstImage();
    for (int i = 0; i < (int)keyframes.size(); ++i) {
        QVERIFY(keyframes[i] == m_testVPC->getImageIndexOnScreen());
        m_testVPC->slot_showNextImage();
    }
}

void tst_videoplayercontroller::test_SetRemoveKeyframes()
{
    ModelInputPictures *mip = m_testDM->getModelInputPictures();
    std::vector<uint> randomIdx = {uint(0)};
    int maxNum = (mip->getPicCount() - 1) / 10;
    for (int i = 0; i < 50; ++i) {
        uint randomNum = std::rand();
        randomNum = randomNum % maxNum;
        uint picIdx = randomIdx[randomIdx.size()-1] + randomNum;
        if (randomNum != 0 && picIdx < mip->getPicCount()) {
            randomIdx.push_back(picIdx);
        }
    }
    QVERIFY(mip->getKeyframeCount(false) == 0);

    for(int i = 0; i < (int)randomIdx.size(); ++i) {
        uint keyframes = mip->getKeyframeCount(false);
        m_testVPC->slot_changeIndex(randomIdx[i]);
        m_testVPC->slot_toggleKeyframe();
        QTest::qWait(50);
        QVERIFY2(keyframes < mip->getKeyframeCount(false), "didn't add new keyframe");
    }
    QVERIFY2(randomIdx.size() == mip->getKeyframeCount(false), "keyframecount doesn't match randomIdx size");
    std::vector<uint> mipKF = mip->getAllKeyframes(false);
    for(int i = 0; i < (int)randomIdx.size(); ++i) {
        QVERIFY(randomIdx[i] == mipKF[i]);
    }

    for(int i = 0; i < (int)randomIdx.size(); ++i) {
        uint keyframes = mip->getKeyframeCount(false);
        m_testVPC->slot_changeIndex(randomIdx[i]);
        m_testVPC->slot_toggleKeyframe();
        QTest::qWait(50);
        QVERIFY2(keyframes > mip->getKeyframeCount(false), "didn't remove keyframe");
    }
    QVERIFY(mip->getKeyframeCount(false) == 0);
}

std::vector<uint> tst_videoplayercontroller::genRNum(uint maxNum, uint count)
{
    ModelInputPictures *mip = m_testDM->getModelInputPictures();
    std::vector<uint> randomIdx;
    if (count > mip->getPicCount()) {
        count = mip->getPicCount();
    }
    if (maxNum > mip->getPicCount()) {
        maxNum = mip->getPicCount();
    }
    for (uint i = 0; i < count; ++i) {
        uint randomNum = std::rand();
        randomNum = randomNum % maxNum;
        if (randomNum >= mip->getPicCount()) {
            return randomIdx;
        }
        randomIdx.push_back(randomNum);
    }
    return randomIdx;
}

QTEST_MAIN(tst_videoplayercontroller)

#include "tst_videoplayercontroller.moc"
