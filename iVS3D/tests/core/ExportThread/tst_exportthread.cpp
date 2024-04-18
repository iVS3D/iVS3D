#include <QtTest>
#include <QCoreApplication>
#include <opencv2/core.hpp>

#include "modelinputpictures.h"
#include "exportthread.h"
#include "logfile.h"
#include "itransform_stub.h"
#include "resourceloader.h"
#include "progressable.h"


class tst_exportThread : public QObject
{
    Q_OBJECT

public:
    tst_exportThread();
    ~tst_exportThread();

private slots:
    void init();
    void cleanup();
    void initTestCase();
    void cleanupTestCase();
    void test_exportVideo();
    void test_abortExportImmediately();
    void test_exportCorrectImages();
    void test_exportResolutionAndCrop();
    void test_exportResolutionAndCrop_data();

private:
    QString m_path;
    QString m_exportWimages;
    QStringList m_exportWtransforms;
    QString m_exportPath;
    QString m_testresourcePath;
    QString m_testVideoPath;
    ModelInputPictures *m_mip;
    ITransform_stub *m_iTransformStub;

    bool createDummyImages();
    bool compareImages();
    bool deleteImages(QString path);
    std::vector<uint> genRNum(uint maxNum, uint count);

};

tst_exportThread::tst_exportThread()
{

}

tst_exportThread::~tst_exportThread()
{

}

void tst_exportThread::init() {
    // setup mip
    m_mip = new ModelInputPictures(m_testVideoPath);
    if (m_mip != nullptr) {
        QVERIFY(m_mip->getPicCount() != 0);
    }
    else {
        qDebug() << "m_mip is nullptr";
    }

    m_iTransformStub = new ITransform_stub();
}

void tst_exportThread::cleanup() {
    if (m_mip != nullptr) {
        delete m_mip;
    }
    m_mip = nullptr;

    if (m_iTransformStub != nullptr) {
        delete m_iTransformStub;
    }
    m_iTransformStub = nullptr;

    deleteImages(m_exportWimages);
    for(auto p : m_exportWtransforms){
        deleteImages(p);
    }
}

void tst_exportThread::initTestCase()
{
    srand (time(0));
    m_exportPath = QString(TEST_RESOURCES) + "/testexport";
    m_exportWimages = m_exportPath + "/images";
    if (!QDir(m_exportWimages).exists()) {
        qDebug() << "m_exportWimages: " << m_exportWimages;
        if (!QDir().mkpath(m_exportWimages)) {
            //couldn't create testexport path
            qDebug() << "couldn't create testexport path";
            return;
        }
    }

    for(int i = 0; i<ITransform_stub().getOutputNames().size(); i++){
        m_exportWtransforms.push_back(m_exportPath + "/"+ ITransform_stub().getName() + "/" + ITransform_stub().getOutputNames()[i]);
        if (!QDir(m_exportWtransforms[i]).exists()) {
            if (!QDir().mkpath(m_exportWtransforms[i])) {
                //couldn't create testexport path
                qDebug() << "couldn't create testexport path";
                return;
            }
        }
    }
    deleteImages(m_exportWimages);
    for(auto p : m_exportWtransforms){
        deleteImages(p);
    }
    m_testresourcePath = QString(TEST_RESOURCES);
    m_testVideoPath = m_testresourcePath + "/video.mp4";
    requireResource(m_testVideoPath);
}

void tst_exportThread::cleanupTestCase()
{
    deleteImages(m_exportWimages);
    QVERIFY(QDir(m_exportWimages).isEmpty());
    QDir(m_exportPath).removeRecursively();
}


void tst_exportThread::test_exportVideo()
{
    for(uint i = 0; i<m_mip->getPicCount() && i<=50; i+=5){
        m_mip->removeKeyframe(i);
    }
    qDebug() << QString::number(m_mip->getKeyframeCount(false));
    m_mip->setBoundaries(QPoint(0,50));
    qDebug() << m_mip->getBoundaries();

    volatile bool stopped = false;
    std::vector<ITransform*> transforms;
    auto prog = new Progressable();
    ExportThread t(prog, m_mip, QPoint(200,400), m_exportPath, "MyTestExport", &stopped, QRect(0,0,0,0), transforms, new LogFile("test", false));
    t.start();
    while(!t.isFinished()){
        QTest::qWait(250);
    }

    QVERIFY(t.getResult() == 0);
    QVERIFY(t.isFinished());
    QStringList imgs = QDir(m_exportWimages).entryList(QStringList("*.png"));
    QCOMPARE(imgs.length(), 40);
    delete prog;

}

void tst_exportThread::test_abortExportImmediately()
{
    for(uint i = 0; i < m_mip->getPicCount() && i < 50; i += 5) {
        m_mip->addKeyframe(i);
    }
    m_mip->setBoundaries(QPoint(0,m_mip->getPicCount() - 1));

    volatile bool stopped = false;
    std::vector<ITransform*> transforms;
    auto prog = new Progressable();
    ExportThread exportThread(prog, m_mip, QPoint(200,400), m_exportPath, "MyTestExport", &stopped, QRect(0,0,0,0),transforms, new LogFile("test", false));
    exportThread.start();
    stopped = true;
    while (!exportThread.isFinished()) {
        QTest::qWait(250);
    }

    QVERIFY(exportThread.getResult() == 1);
    QVERIFY(exportThread.isFinished());
    delete prog;
}

void tst_exportThread::test_exportCorrectImages()
{
    std::vector<uint> randomIdx = genRNum(m_mip->getPicCount(), 20);

    for (int i = 0; i < randomIdx.size(); ++i) {
        m_mip->addKeyframe(randomIdx[i]);
    }
    m_mip->setBoundaries(QPoint(0,m_mip->getPicCount() - 1));

    volatile bool stopped = false;

    //prepare iTransformStub and create folders
    std::vector<ITransform*> transforms;
    transforms.push_back(new ITransform_stub());
    QString iTransformPath = m_exportPath;
    iTransformPath.append("/").append(transforms[0]->getName());
    QStringList iTransformOutputNames = transforms[0]->getOutputNames();
    QStringList iTransformSubDirs;
    for (int i = 0; i < iTransformOutputNames.length(); ++i) {
        QString iTransformSubDir = iTransformPath;
        iTransformSubDir.append("/").append(iTransformOutputNames[i]);
        if (!QDir(iTransformSubDir).exists()) {
            bool dirsuccess = QDir().mkpath(iTransformSubDir);
            QVERIFY2(dirsuccess, "dir not created");
        }
        iTransformSubDirs.push_back(iTransformSubDir);
    }
    auto prog = new Progressable();
    ExportThread exportThread(prog, m_mip, m_mip->getInputResolution(), m_exportPath, "MyTestExport", &stopped, QRect(0,0,0,0),transforms, new LogFile("test", false));
    exportThread.start();
    qDebug () << "started exportthread, this needs some time.";
    while (!exportThread.isFinished()) {
        QTest::qWait(250);
    }
    qDebug () << "exportthread finished";

    QVERIFY(exportThread.getResult() == 0);
    QVERIFY(exportThread.isFinished());

    //verifying correctness

    QStringList allSubDirs = iTransformSubDirs;
    allSubDirs.push_back(m_exportWimages);
    for (int i = 0; i < allSubDirs.length(); ++i) {
        //directories hold right number of files
        QVERIFY(QDir(allSubDirs[i]).entryList().length() -2 == m_mip->getKeyframeCount(false));
    }
    std::vector<uint> keyframelist = m_mip->getAllKeyframes(false);
    for (uint i = 0; i < m_mip->getKeyframeCount(false); ++i) {
        cv::Mat mipPic = *m_mip->getPic(keyframelist[i]);
        ImageList comparePic = m_iTransformStub->transform(0, mipPic);
        QVERIFY(comparePic.length() == iTransformOutputNames.length());
        for (int j = 0; j < iTransformOutputNames.length(); ++j) {
            //aquire images
            QString imgPath = iTransformSubDirs[j];
            imgPath.append("/").append(QString::number(keyframelist[i], 10).rightJustified(8, '0').append(".png"));
            cv::Mat exportImg = cv::imread(imgPath.toStdString());
            cv::cvtColor(exportImg, exportImg, cv::COLOR_BGR2GRAY);
            cv::Mat dummyImg = comparePic[j];

            QVERIFY(exportImg.rows == dummyImg.rows);
            QVERIFY(exportImg.cols == dummyImg.cols);
            QVERIFY(exportImg.type() == dummyImg.type());

            //compare cv:mats
            bool eq = false;
            cv::Mat diff;
            diff = exportImg != dummyImg;
            eq = cv::countNonZero(diff) == 0;
            QVERIFY(eq);
        }
    }

    //end verifying correctness


    for (int i = 0; i < iTransformSubDirs.length(); ++i) {
        deleteImages(iTransformSubDirs[i]);
    }
    delete prog;
}
void tst_exportThread::test_exportResolutionAndCrop()
{
    QFETCH(QPoint, in_resolution);
    QFETCH(QRect, in_roi);
    QFETCH(bool, in_withTransfrom);
    QFETCH(QPoint, out_resolution);

    auto kfs = genRNum(m_mip->getPicCount(),20);
    std::sort(kfs.begin(), kfs.end());
    m_mip->setBoundaries(QPoint(0,m_mip->getPicCount()-1));
    m_mip->updateMIP(kfs);
    volatile bool stopped = false;
    std::vector<ITransform*> transforms;
    if(in_withTransfrom){
        transforms.push_back(new ITransform_stub);
    }
    auto prog = new Progressable();
    ExportThread t(prog, m_mip, in_resolution, m_exportPath, "MyTestExport", &stopped, in_roi, transforms, new LogFile("test", false));

    t.start();
    while(!t.isFinished()){
        QTest::qWait(250);
    }

    QVERIFY(t.getResult() == 0);
    QVERIFY(t.isFinished());

    auto compareOutput = [=](QString path){
        QStringList imgs = QDir(path).entryList(QStringList("*.png"));
        for(auto img : imgs){
            QString pathToImg = path + "/" + img;
            auto mat = cv::imread(pathToImg.toStdString());
            QPoint exported_resolution(mat.cols, mat.rows);
            QCOMPARE(exported_resolution, out_resolution);
        }
    };

    compareOutput(m_exportWimages);

    if(in_withTransfrom){
        for(auto exportP : m_exportWtransforms){
            compareOutput(exportP);
        }
    }
    delete prog;
}

void tst_exportThread::test_exportResolutionAndCrop_data()
{
    QTest::addColumn<QPoint>("in_resolution");
    QTest::addColumn<QRect>("in_roi");
    QTest::addColumn<bool>("in_withTransfrom");
    QTest::addColumn<QPoint>("out_resolution");

    // test id                  |   in_resolution       |   in_roi              | in_withTr | out_resolution
    // -------------------------+-----------------------+-----------------------+-----------+-------
    QTest::addRow("default")     << QPoint(1080,1920)   << QRect(0,0,0,0)       << false    << QPoint(1080,1920);
    QTest::addRow("default+roi") << QPoint(1080,1920)   << QRect(20,20,300,300) << false    << QPoint(300,300);
    QTest::addRow("res")         << QPoint(455,1000)    << QRect(0,0,0,0)       << false    << QPoint(455,1000);
    QTest::addRow("res+roi")     << QPoint(540,960)     << QRect(250,250,540,960)<< false   << QPoint(270,480);
    QTest::addRow("iTr_default")     << QPoint(1080,1920)   << QRect(0,0,0,0)       << true    << QPoint(1080,1920);
    QTest::addRow("iTr_default+roi") << QPoint(1080,1920)   << QRect(20,20,300,300) << true    << QPoint(300,300);
    QTest::addRow("iTr_res")         << QPoint(455,1000)    << QRect(0,0,0,0)       << true    << QPoint(455,1000);
    QTest::addRow("iTr_res+roi")     << QPoint(540,960)     << QRect(250,250,540,960)<< true   << QPoint(270,480);
}

bool tst_exportThread::deleteImages(QString path)
{
    QStringList imagesList = QDir(path).entryList();
    for (int i = 0; i < imagesList.length(); ++i) {
        if (imagesList[i].endsWith(".png")) {
            QDir(path).remove(imagesList[i]);
        }
    }
    return true;
}

std::vector<uint> tst_exportThread::genRNum(uint maxNum, uint count)
{
    std::vector<uint> randomIdx;
    if (count > m_mip->getPicCount()) {
        count = m_mip->getPicCount();
    }
    if (maxNum > m_mip->getPicCount()) {
        maxNum = m_mip->getPicCount();
    }
    for (uint i = 0; i < count; ++i) {
        uint randomNum = std::rand();
        randomNum = randomNum % maxNum;
        if (randomNum >= m_mip->getPicCount()) {
            return randomIdx;
        }
        randomIdx.push_back(randomNum);
    }
    return randomIdx;
}

QTEST_MAIN(tst_exportThread)

#include "tst_exportthread.moc"
