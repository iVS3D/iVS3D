#include <QtTest>
#include <QCoreApplication>

#include "modelinputpictures.h"
#include "DataManager.h"
#include "view/outputwidget.h"
#include "controller/exportcontroller.h"
#include "resourceloader.h"

class tst_exportcontroller : public QObject
{
    Q_OBJECT

public:
    tst_exportcontroller();
    ~tst_exportcontroller();

private slots:
    void initTestCase();
    void cleanupTestCase();
    void cleanup();
    void init();
    void test_export();
    void test_exportWithMasks();

private:
    void loadTestVideo();

    QString m_testResourcePath;
    QString m_testExportPath;
    DataManager *m_testDM;
    OutputWidget *m_testOW;
    ExportController *m_testEC;

};

tst_exportcontroller::tst_exportcontroller()
{

}

tst_exportcontroller::~tst_exportcontroller()
{

}

void tst_exportcontroller::loadTestVideo()
{
    QString testVideoPath = m_testResourcePath;
    testVideoPath.append("/").append("video.mp4");
    requireResource(testVideoPath);
    qDebug() << testVideoPath;
    QVERIFY(m_testDM != nullptr);
    m_testDM->open(testVideoPath);
    QVERIFY(m_testDM != nullptr);
    QVERIFY(m_testDM->getModelInputPictures() != nullptr);
    QVERIFY(m_testDM->getModelInputPictures()->getPicCount() > 0);
}

void tst_exportcontroller::initTestCase()
{
    m_testResourcePath = QString(TEST_RESOURCES);
    m_testExportPath = m_testResourcePath + "/tst_exportcontroller";
    QDir(m_testExportPath).removeRecursively();
}

void tst_exportcontroller::cleanupTestCase()
{
    TransformManager::instance().exit();
}

void tst_exportcontroller::init()
{
    m_testDM = new DataManager();
    loadTestVideo();

    m_testOW = new OutputWidget(nullptr, "output", TransformManager::instance().getTransformList());
    QVERIFY(m_testOW != nullptr);

    m_testEC = new ExportController(m_testOW, m_testDM);
    QVERIFY(m_testEC != nullptr);

    m_testOW->setOutputPath(m_testExportPath);
}

void tst_exportcontroller::cleanup()
{
    QDir(m_testExportPath).removeRecursively();
    delete m_testEC;
    m_testEC = nullptr;
    delete m_testOW;
    m_testOW = nullptr;
    delete m_testDM;
    m_testDM = nullptr;
}

void tst_exportcontroller::test_export()
{
    // add some keayframes and set boundaries of mip
    auto *mip(m_testDM->getModelInputPictures());
    mip->setBoundaries(QPoint(0, mip->getPicCount()));
    mip->addKeyframe(1);
    mip->addKeyframe(20);
    mip->addKeyframe(80);   // index out of bounds

    // start export and wait for finished signal
    QSignalSpy spy(m_testEC, &ExportController::sig_exportFinished);
    m_testEC->slot_export();

    while(spy.count()<1){
        QTest::qWait(250);
    }

    // test if export folder, images folder, project file and images exist
    QVERIFY2(QDir(m_testExportPath).exists(), "failed to create export folder!");
    QVERIFY2(QDir(m_testExportPath).entryList().contains("tst_exportcontroller-project.json"), "failed to create project file!");
    QVERIFY2(QDir(m_testExportPath + "/images").exists(), "failed to create images folder");
    QCOMPARE((int)QDir(m_testExportPath + "/images").entryList({"*.png"}).size(), 2);
}

void tst_exportcontroller::test_exportWithMasks()
{
    QSKIP("Tests doesnt find Semantic Segmentation");
    // add some keayframes and set boundaries of mip
    auto *mip(m_testDM->getModelInputPictures());
    mip->setBoundaries(QPoint(0, mip->getPicCount()));
    mip->addKeyframe(1);
    mip->addKeyframe(8);

    std::vector<bool> itr;
    for(int i=0; i<TransformManager::instance().getTransformCount();i++){
        itr.push_back(true);
    }
    m_testOW->setSelectedITransformMasks(itr);

    // start export and wait for finished signal
    QSignalSpy spy(m_testEC, &ExportController::sig_exportFinished);
    m_testEC->slot_export();

    while(spy.count()<1){
        QTest::qWait(250);
    }
    QTest::qWait(250);
    // test if export folder, SemanticSegmentation folder, masks folder and images exist
    QVERIFY2(QDir(m_testExportPath).exists(), "failed to create export folder!");
    QVERIFY2(QDir(m_testExportPath + "/SemanticSegmentation").exists(), "failed to create semantic seg folder");
    QVERIFY2(QDir(m_testExportPath + "/SemanticSegmentation/masks").exists(), "failed to create masks folder");
    QCOMPARE((int)QDir(m_testExportPath + "/SemanticSegmentation/masks").entryList({"*.png"}).size(), 2);
}

QTEST_MAIN(tst_exportcontroller)

#include "tst_exportcontroller.moc"
