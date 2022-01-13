#include <QtTest>
#include <resourceloader.h>
// add necessary includes here
#include <DataManager.h>
#include <modelinputpictures.h>

class tst_datamanager_and_mip : public QObject
{
    Q_OBJECT

public:
    tst_datamanager_and_mip();
    ~tst_datamanager_and_mip();

private slots:
    void initTestCase();
    void testForZeroKeyframes();
    void testResolution();
    void testEmptyFolder();
    void testFolderWithoutImages();
    void testUnvalidPath();
    void testEmptyPath();
    void testWrongDataType();
    void testSortedKeyframes();
    void testKeyframesOutOfRange();
    void testRemoveInvalidKeyframe();
    void cleanupTestCase();
private:
    DataManager* dm;
    ModelInputPictures* mip;
    QString testPath;

};

tst_datamanager_and_mip::tst_datamanager_and_mip()
{

}

tst_datamanager_and_mip::~tst_datamanager_and_mip()
{

}

void tst_datamanager_and_mip::testForZeroKeyframes()
{
    for (uint i = 0; i < mip->getPicCount(); i++) {
        QVERIFY(!(mip->isKeyframe(i)));
    }
}

void tst_datamanager_and_mip::testResolution()
{
    QPoint res = mip->getInputResolution();
    QCOMPARE(QPoint(res), QPoint(1080,1920));
}

void tst_datamanager_and_mip::testEmptyFolder()
{
    DataManager* empty = new DataManager;
    QDir* currentDir = new QDir(testPath);
    currentDir->mkdir("emptyFolder");
    QString path = testPath + "/emptyFolder";
    int numPics = empty->open(path);
    QCOMPARE(numPics, 0);  //Verify that no images were loaded
    delete empty;
}

void tst_datamanager_and_mip::initTestCase()
{
    dm = new DataManager;
    testPath = TEST_RESOURCES;
    QString path = testPath + "/video.mp4";
    requireResource(path);
    int numPics = dm->open(path);
    mip = dm->getModelInputPictures();
    QCOMPARE(numPics, 61);  //Verify correct number of Frames imported in DataManger
}


void tst_datamanager_and_mip::testFolderWithoutImages()
{
    DataManager* empty = new DataManager;
    QString path = testPath + "/folderWithoutPictures";
    requireResource(path);
    int numPics = empty->open(path);
    QCOMPARE(numPics, 0);  //Verify that no images were loaded
    delete empty;
}

void tst_datamanager_and_mip::testUnvalidPath()
{
    QSKIP("DataManager can't import non existing paths");
    int random=rand();
    QString path = testPath + "/"  + QString::number(random);
    DataManager* empty = new DataManager;
    int numPics = empty->open(path);
    QCOMPARE(numPics, 0);  //Verify that no images were loaded
    delete empty;
}

void tst_datamanager_and_mip::testEmptyPath()
{
    QSKIP("DataManager can't handle empty paths");
    DataManager* empty = new DataManager;
    int numPics = empty->open("");
    QCOMPARE(numPics, 0);  //Verify that no images were loaded
    delete empty;
}

void tst_datamanager_and_mip::testWrongDataType()
{
    QSKIP("DataManager (-> MIP) doesn't check paths");
    DataManager* empty = new DataManager;

    QString path = testPath + "/wrongDataType.exe";
    requireResource(path);
    qDebug() << path;
    int numPics = empty->open("");
    QCOMPARE(numPics, 0);  //Verify that no images were loaded
    delete empty;
}

void tst_datamanager_and_mip::testSortedKeyframes()
{
    int numPics = mip->getPicCount();
    std::vector<uint> keyframes;
    for (int i = 0; i < numPics / 4; i++) { //Generate random Keyframes and insert them to mip
       int random = rand() % numPics;
       keyframes.push_back(random);
       mip->addKeyframe(random);
    }

    std::sort(keyframes.begin(), keyframes.end()); //sort the keyframes
    keyframes.erase(std::unique(keyframes.begin(), keyframes.end()), keyframes.end()); //Remove duplicates

    QVector<uint> orignalKeyframes = QVector<uint>::fromStdVector(keyframes);
    QVector<uint> mipKeyframes = QVector<uint>::fromStdVector(mip->getAllKeyframes());
    QCOMPARE(orignalKeyframes, mipKeyframes); //Check if the keyframe vectors are equal

    for(uint value  : keyframes) { //Remove all keyframes
        mip->removeKeyframe(value);
    }
    QVERIFY(mip->getAllKeyframes().size() == 0); //Check if keyframe vector is empty
}

void tst_datamanager_and_mip::testKeyframesOutOfRange()
{
    QSKIP("MIP doesn't check for wrong keyframe values");
    int numPics = mip->getPicCount();
    mip->addKeyframe(numPics + 100);
    QVERIFY(mip->getAllKeyframes().size() == 0); //Check if keyframe vector is still empty
}

void tst_datamanager_and_mip::testRemoveInvalidKeyframe()
{
    mip->removeKeyframe(-1);
    QVERIFY(mip->getAllKeyframes().size() == 0); //Check if keyframe vector is still empty
    int numPics = mip->getPicCount();
    mip->removeKeyframe(numPics + 100);
    QVERIFY(mip->getAllKeyframes().size() == 0); //Check if keyframe vector is still empty
}

void tst_datamanager_and_mip::cleanupTestCase()
{
    QDir* currentDir = new QDir(testPath);
    currentDir->rmdir("emptyFolder");
    delete dm;
}


QTEST_MAIN(tst_datamanager_and_mip)

#include "tst_datamanager_and_mip.moc"
