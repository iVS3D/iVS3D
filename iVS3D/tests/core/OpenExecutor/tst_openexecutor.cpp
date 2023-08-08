#include <QtTest>
#include <QCoreApplication>
#include "DataManager.h"
#include "openexecutor.h"
#include "resourceloader.h"

class tst_openExecutor : public QObject
{
    Q_OBJECT

public:
    tst_openExecutor();
    ~tst_openExecutor();

private slots:
    void init();
    void cleanup();
    void initTestCase();
    void cleanupTestCase();
    void test_openVideo();
    void test_openFolder();
    void test_projectFile();

private:
    DataManager *m_dataManager;
    QString m_project_videoPath;
    ModelInputPictures *m_project_mip;
    ModelAlgorithm *m_project_ma;
    void createJSON();
    void removeJSON();
    QString m_projectPath = QString(TEST_RESOURCES) + "/openExecuterProject.json";
    QString m_projectName = "openExecuterProject";

};

tst_openExecutor::tst_openExecutor()
{

}

tst_openExecutor::~tst_openExecutor()
{

}

void tst_openExecutor::init() {
    m_dataManager = new DataManager();
}

void tst_openExecutor::cleanup() {
    m_dataManager = new DataManager();
    delete m_dataManager;
}

void tst_openExecutor::initTestCase()
{

}


void tst_openExecutor::createJSON()
{
    // generate data
    m_project_videoPath = QString(TEST_RESOURCES) + "/video.mp4";
    requireResource(m_project_videoPath);
    m_project_mip = new ModelInputPictures(m_project_videoPath);
    m_project_mip->addKeyframe(0);
    m_project_ma = new ModelAlgorithm();
    m_project_ma->addPluginBuffer("FictionalPlugin", "FicitionalBuffer", QVariant(42));
    ProjectManager *pm = new ProjectManager();
    QVERIFY2(pm->saveProjectAs(m_project_mip, m_project_ma, m_projectPath, m_projectName), "Couldn't create test project file.");

}

void tst_openExecutor::removeJSON()
{
    QFile file(m_projectPath);
    if (file.exists()) {
        QVERIFY2(file.remove(), "Project file couldn't be removed.");
    }
}

void tst_openExecutor::cleanupTestCase()
{
    removeJSON();
}

void tst_openExecutor::test_openVideo()
{
    QString videoPath = QString(TEST_RESOURCES) + "/video.mp4";
    requireResource(videoPath);

    OpenExecutor *oex = new OpenExecutor(videoPath, m_dataManager);
    QSignalSpy spy(oex, &OpenExecutor::sig_finished);
    oex->open();

    while (spy.count() < 1) {
        QTest::qWait(250);
    }

    QVERIFY2(m_dataManager->getModelInputPictures() != nullptr, "no ModelInputPictures was created");
    ModelInputPictures *mip = m_dataManager->getModelInputPictures();

    QCOMPARE(mip->getPicCount(), (uint)61);
    QCOMPARE(mip->getInputResolution(), QPoint(1080, 1920));
}

void tst_openExecutor::test_openFolder()
{
    QString folderPath = QString(TEST_RESOURCES) + "/BlurTest";
    requireResource(folderPath + "/Blur5.jpg");
    requireResource(folderPath + "/Blur25.jpg");
    requireResource(folderPath + "/Blur50.jpg");
    requireResource(folderPath + "/Blur100.jpg");
    requireResource(folderPath + "/WithoutBlur.jpg");

    OpenExecutor *oex = new OpenExecutor(folderPath, m_dataManager);
    QSignalSpy spy(oex, &OpenExecutor::sig_finished);
    oex->open();

    while (spy.count() < 1) {
        QTest::qWait(250);
    }

    QVERIFY2(m_dataManager->getModelInputPictures() != nullptr, "no ModelInputPictures was created");
    ModelInputPictures *mip = m_dataManager->getModelInputPictures();

    QCOMPARE(mip->getPicCount(), (uint)5);
    QCOMPARE(mip->getInputResolution(), QPoint(640, 428));
}

void tst_openExecutor::test_projectFile()
{
    QString projectTotalPath = m_projectPath;
    createJSON();

    OpenExecutor *oex = new OpenExecutor(projectTotalPath, m_dataManager);
    QSignalSpy spy(oex, &OpenExecutor::sig_finished);

    oex->open();

    while (spy.count() < 1) {
        QTest::qWait(250);
    }

    QVERIFY2(m_dataManager->getModelInputPictures() != nullptr, "no ModelInputPictures was created");
    // check mip
    ModelInputPictures *mip = m_dataManager->getModelInputPictures();
    QCOMPARE(mip->getPicCount(), m_project_mip->getPicCount());
    QCOMPARE(mip->getInputResolution(), m_project_mip->getInputResolution());
    QCOMPARE(mip->getAllKeyframes(false), mip->getAllKeyframes(false));
    // check ma
    QVERIFY2(m_dataManager->getModelAlgorithm() != nullptr, "no ModelAlgorithm was created");
    ModelAlgorithm *ma = m_dataManager->getModelAlgorithm();
    QCOMPARE(ma->getPluginBuffer(), m_project_ma->getPluginBuffer());


    // cleanup the created JSON
    removeJSON();
}

QTEST_MAIN(tst_openExecutor)

#include "tst_openexecutor.moc"
