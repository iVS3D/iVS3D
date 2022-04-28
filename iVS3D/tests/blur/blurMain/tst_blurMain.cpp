#include <QtTest>
#include <QCoreApplication>

#include "blur.h"
#include "reader_stub.h"
#include "logfile_stub.h"
#include "resourceloader.h"

// add necessary includes here

class tst_blurMain : public QObject
{
    Q_OBJECT

public:
    tst_blurMain();
    ~tst_blurMain();

private slots:
    void initTestCase();
    void cleanupTestCase();
    void test_randomKeyframe_data();
    void test_randomKeyframe();
    void test_BlurOnAllImages_data();
    void test_BlurOnAllImages();
    void test_BlurOnKeyframes_data();
    void test_BlurOnKeyframes();

private:
    void createTestDataKeyframes();
    void createTestDataAllImages();
    std::vector<QString> m_picPaths;
    std::vector<uint> m_picOrder;
    LogFileParent* m_logFile;

};

tst_blurMain::tst_blurMain()
{

}

tst_blurMain::~tst_blurMain()
{

}



void tst_blurMain::initTestCase()
{
    qDebug() << TEST_RESOURCES;

    QString path = QString(TEST_RESOURCES) + "/BlurTest/WithoutBlur.jpg";
    requireResource(path);
    m_picPaths.push_back(path);
    path = QString(TEST_RESOURCES) + "/BlurTest/Blur5.jpg";
    requireResource(path);
    m_picPaths.push_back(path);
    path = QString(TEST_RESOURCES) + "/BlurTest/Blur25.jpg";
    requireResource(path);
    m_picPaths.push_back(path);
    path = QString(TEST_RESOURCES) + "/BlurTest/Blur50.jpg";
    requireResource(path);
    m_picPaths.push_back(path);
    path = QString(TEST_RESOURCES) + "/BlurTest/Blur100.jpg";
    requireResource(path);
    m_picPaths.push_back(path);
    //0 = noBlur, 1 = 5Blur, 2 = 25Blur, 3 = 50Blur, 4 = 100Blur
    m_logFile = new LogFile_stub("test", true);
}

void tst_blurMain::cleanupTestCase()
{

}

void tst_blurMain::test_randomKeyframe_data()
{
    QTest::addColumn<std::vector<uint>>("keyframes");
    QTest::addColumn<std::vector<uint>>("picOrder");
    QTest::addColumn<std::vector<uint>>("expectedKeyframes");
    QTest::addColumn<QString>("blurName");
    QTest::addColumn<int>("windowSize");
    QTest::addColumn<double>("localDev");

    srand(time(NULL)); //Use timebased random seed

    for (int iteration = 100; iteration <= 200; iteration = iteration + 10) {
        int length = rand() % iteration;
        int windowSize = rand() % 6 + 4;
        int keyframeStep = windowSize / 2;
        std::vector<uint> picOrder;
        std::vector<uint> keyframes;
        int keyIterator = 0;
        for (int i = 0; i < length; i++) {
            //Prevent keyframes in front and end to remove special case with cropped search windows
            if (keyIterator >= keyframeStep && i > windowSize && i + windowSize < length) {
                keyframes.push_back(i);
                keyIterator = -windowSize;
            }
            else {
                keyIterator++;
            }
            int picNo = rand() % 5;
            picOrder.push_back(picNo);
        }
        //prevent empty keframe vector or vectro with 1 element (-> if keyframe vector has size 1 its impossible to decide, which of the 2 blur variants to use)
        if (keyframes.size() == 0 || keyframes.size() == 1) {
            continue;
        }
        std::vector<uint> expectedKeyframes;
        for (uint i = 0; i < keyframes.size(); i++ ) {
            int bestIndex = keyframes.at(i);
            uint bestValue = picOrder.at(bestIndex);
            for (uint j = keyframes.at(i) - windowSize; j <= keyframes.at(i) + windowSize; j++) {
                if (picOrder.at(j) < bestValue) {
                    bestIndex = j;
                    bestValue = picOrder.at(j);
                }
            }
            expectedKeyframes.push_back(bestIndex);
        }

        expectedKeyframes.erase(std::unique(expectedKeyframes.begin(), expectedKeyframes.end()), expectedKeyframes.end()); //Remove duplicates

        QTest::addRow("Random Test Laplacian") << keyframes << picOrder << expectedKeyframes << "Laplacian Filter" << windowSize << 95.0;
        QTest::addRow("Random Test Sobel") << keyframes << picOrder << expectedKeyframes << "Sobel Filter" << windowSize << 95.0;
    }
}

void tst_blurMain::test_randomKeyframe()
{
    QFETCH(std::vector<uint>, keyframes);
    QFETCH(std::vector<uint>, picOrder);
    QFETCH(std::vector<uint>, expectedKeyframes);
    QFETCH(QString, blurName);
    QFETCH(int, windowSize);
    QFETCH(double, localDev);

    Reader_stub* stub = new Reader_stub(picOrder, m_picPaths);

    Blur* blur = new Blur();
    blur->slot_wsChanged(windowSize);
    blur->slot_ldChanged(localDev);
    volatile bool stop = false;
    std::vector<uint> newKeyframes;
    blur->initialize(stub, QMap<QString,QVariant>(), nullptr);
    newKeyframes = blur->sampleImages(keyframes, nullptr, &stop, false, m_logFile);
    if (newKeyframes != expectedKeyframes) {
        qDebug() << "expected : " << expectedKeyframes;
        qDebug() << "got : " << newKeyframes;
        qDebug() << "windowSize : " << windowSize;
        qDebug() << "localDev : " << localDev;
        qDebug() << "picOrder : " << picOrder;
        qDebug() << "keyframes : " << keyframes;
    }
    QCOMPARE(newKeyframes, expectedKeyframes);
    delete blur;
    stub = nullptr;
}

void tst_blurMain::test_BlurOnAllImages()
{
    //QSKIP("Save runtime");
    QFETCH(std::vector<uint>, picOrder);
    QFETCH(std::vector<uint>, keyframesExpected);
    QFETCH(QString, blurName);
    QFETCH(int, windowSize);
    QFETCH(double, localDev);
    Reader_stub* stub = new Reader_stub(picOrder, m_picPaths);
    Blur* blur = new Blur();
    blur->slot_wsChanged(windowSize);
    blur->slot_ldChanged(localDev);
    blur->slot_blurChanged(blurName);
    volatile bool stop = false;
    std::vector<uint> keyframe;
    std::vector<uint> imageIndex;
    for (uint i = 0; i < picOrder.size(); i++) {
      imageIndex.push_back(i);
    }
    keyframe.clear();
    blur->initialize(stub, QMap<QString,QVariant>(), nullptr);
    keyframe = blur->sampleImages(imageIndex, nullptr, &stop, false, m_logFile);

    if (keyframe != keyframesExpected) {
        qDebug() << "expected : " << keyframesExpected;
        qDebug() << "got : " << keyframe;
        qDebug() << "windowSize : " << windowSize;
        qDebug() << "localDev : " << localDev;
        qDebug() << "picOrder : " << picOrder;
        qDebug() << "keyframes : " << keyframe;
    }
    QCOMPARE(keyframe, keyframesExpected);
    delete blur;
    stub = nullptr;
    QTest::qSleep(200);
}

void tst_blurMain::test_BlurOnAllImages_data()
{
    createTestDataAllImages();
}

void tst_blurMain::test_BlurOnKeyframes()
{
    //QSKIP("Save runtime");
    QFETCH(std::vector<uint>, keyframes);
    QFETCH(std::vector<uint>, picOrder);
    QFETCH(std::vector<uint>, keyframesExpected);
    QFETCH(QString, blurName);
    QFETCH(int, windowSize);
    QFETCH(double, localDev);
    Reader_stub* stub = new Reader_stub(picOrder, m_picPaths);

    Blur* blur = new Blur();
    blur->slot_wsChanged(windowSize);
    blur->slot_ldChanged(localDev);
    blur->slot_blurChanged(blurName);
    volatile bool stop = false;
    std::vector<uint> newKeyframes;

    blur->initialize(stub, QMap<QString,QVariant>(), nullptr);
    newKeyframes = blur->sampleImages(keyframes, nullptr, &stop, false, m_logFile);
    qDebug() << newKeyframes;

    if (newKeyframes != keyframesExpected) {
        qDebug() << "expected : " << keyframesExpected;
        qDebug() << "got : " << newKeyframes;
        qDebug() << "windowSize : " << windowSize;
        qDebug() << "localDev : " << localDev;
        qDebug() << "picOrder : " << picOrder;
        qDebug() << "keyframes : " << keyframes;
    }
    QCOMPARE(newKeyframes, keyframesExpected);
    delete blur;
    stub = nullptr;
}

void tst_blurMain::test_BlurOnKeyframes_data()
{
    createTestDataKeyframes();
}

void tst_blurMain::createTestDataAllImages()
{
    QTest::addColumn<std::vector<uint>>("picOrder");
    QTest::addColumn<std::vector<uint>>("keyframesExpected");
    QTest::addColumn<QString>("blurName");
    QTest::addColumn<int>("windowSize");
    QTest::addColumn<double>("localDev");

    //0 = noBlur, 1 = 5Blur, 2 = 25Blur, 3 = 50Blur, 4 = 100Blur
    QTest::addRow("simple Input Laplacian") << std::vector<uint>({4,4,4,0,0,0}) << std::vector<uint>({3,4,5}) << "Laplacian Filter" << 10 << 95.0;
    QTest::addRow("1 sharp & 1 5blur Laplacian") << std::vector<uint>({1,0}) << std::vector<uint>({1}) << "Laplacian Filter" << 10 << 95.0;
    QTest::addRow("input only blured Laplacian") << std::vector<uint>({4,3,2,1,4,3,2,1}) << std::vector<uint>({3,7}) << "Laplacian Filter" << 10 << 95.0;
    QTest::addRow("11 blurred and 1 sharp Laplacian") << std::vector<uint>({4,4,4,4,4,4,4,4,4,4,4,0}) << std::vector<uint>({0,11}) << "Laplacian Filter" << 10 << 95.0;
    QTest::addRow("low Deviation Laplacian") << std::vector<uint>({4,3,2,1,4,3,2,1}) << std::vector<uint>({0,1,2,3,4,5,6,7}) << "Laplacian Filter" << 10 << 5.0;
    QTest::addRow("multiple sharp and 5Blur Laplacian") << std::vector<uint>({1,0,0,1,0,1,1,1,0,0}) << std::vector<uint>({1,2,4,8,9}) << "Laplacian Filter" << 10 << 99.0;
    QTest::addRow("small windowSize Laplacian") << std::vector<uint>({3,3,3,0,0,0}) << std::vector<uint>({0,3,4,5}) << "Laplacian Filter" << 2 << 95.0;

    QTest::addRow("simple Input Sobel") << std::vector<uint>({4,4,4,0,0,0}) << std::vector<uint>({3,4,5}) << "Sobel Operator" << 10 << 95.0;
    QTest::addRow("1 sharp & 1 5blur Sobel") << std::vector<uint>({1,0}) << std::vector<uint>({1}) << "Sobel Operator" << 10 << 95.0;
    QTest::addRow("input only blured Sobel") << std::vector<uint>({4,3,2,1,4,3,2,1}) << std::vector<uint>({3,7}) << "Sobel Operator" << 10 << 95.0;
    QTest::addRow("11 blurred and 1 sharp Sobel") << std::vector<uint>({4,4,4,4,4,4,4,4,4,4,4,0}) << std::vector<uint>({0,11}) << "Sobel Operator" << 10 << 95.0;
    QTest::addRow("low Deviation Sobel") << std::vector<uint>({4,3,2,1,4,3,2,1}) << std::vector<uint>({0,1,2,3,4,5,6,7}) << "Sobel Operator" << 10 << 5.0;
    QTest::addRow("multiple sharp and 5Blur Sobel") << std::vector<uint>({1,0,0,1,0,1,1,1,0,0}) << std::vector<uint>({1,2,4,8,9}) << "Sobel Operator" << 10 << 99.0;
    QTest::addRow("small windowSize Sobel") << std::vector<uint>({3,3,3,0,0,0}) << std::vector<uint>({0,3,4,5}) << "Sobel Operator" << 2 << 95.0;
}

void tst_blurMain::createTestDataKeyframes()
{
    QTest::addColumn<std::vector<uint>>("keyframes");
    QTest::addColumn<std::vector<uint>>("picOrder");
    QTest::addColumn<std::vector<uint>>("keyframesExpected");
    QTest::addColumn<QString>("blurName");
    QTest::addColumn<int>("windowSize");
    QTest::addColumn<double>("localDev");

    QTest::addRow("multiple keyframes selection Laplacian") << std::vector<uint>({2,5,7,11,15}) << std::vector<uint>({4,3,2,4,3,2,4,3,1,2,3,1,4,2,1,3,0}) << std::vector<uint>({2,5,8,11,16}) << "Laplacian Filter" << 2 << 95.0;

    QTest::addRow("multiple keyframes selection Sobel") << std::vector<uint>({2,5,7,11,15}) << std::vector<uint>({4,3,2,4,3,2,4,3,1,2,3,1,4,2,1,3,0}) << std::vector<uint>({2,5,8,11,16}) << "Sobel Filter" << 2 << 95.0;
}

QTEST_MAIN(tst_blurMain)

#include "tst_blurMain.moc"
