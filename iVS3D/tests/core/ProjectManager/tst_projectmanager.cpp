#include <QtTest>

#include "projectmanager.h"
#include "modelalgorithm.h"
#include "modelinputpictures.h"
#include "resourceloader.h"

class tst_projectmanager : public QObject
{
    Q_OBJECT

public:
    tst_projectmanager();
    ~tst_projectmanager();

private slots:
    void initTestCase();
    void cleanupTestCase();
    void cleanup();
    void init();
    void test_saveProject();
    void test_createProject();
    void test_loadProject();
    void test_loadFalsePath();
    void test_saveNullModels();
    void test_saveFalsePath();

private:
    bool saveTestProject(QString projectPath, QString projectName);
    void loadTestVideo();

    ProjectManager *m_testPM;
    ModelInputPictures *m_testMIP;
    ModelAlgorithm *m_testMA;
    QString m_testResourcePath;
    QString m_projectPath;
    QString m_projectName;
    QStringList m_filesToDelete;

};

tst_projectmanager::tst_projectmanager()
{

}

tst_projectmanager::~tst_projectmanager()
{

}

bool tst_projectmanager::saveTestProject(QString projectPath, QString projectName)
{
    return m_testPM->saveProjectAs(m_testMIP, m_testMA, projectPath, projectName);
}

void tst_projectmanager::loadTestVideo()
{
    QString testVideoPath = m_testResourcePath;
    testVideoPath.append("/").append("video.mp4");
    requireResource(testVideoPath);
    m_testMIP = new ModelInputPictures(testVideoPath);
    QVERIFY(m_testMIP != nullptr);
    QCOMPARE(m_testMIP->getPicCount(), (uint)61);
}

void tst_projectmanager::initTestCase()
{
    m_testResourcePath = QString(TEST_RESOURCES);

    m_testMA = new ModelAlgorithm();
    m_testMA->addPluginBuffer("testPluginName", "testBufferName", QVariant("testValue"));
    QVERIFY(m_testMA != nullptr);

    loadTestVideo();
}

void tst_projectmanager::cleanupTestCase()
{
    for (QString path : m_filesToDelete) {
        QFile::remove(path);
    }
}

void tst_projectmanager::init()
{
    m_testPM = new ProjectManager();
    QVERIFY(m_testPM != nullptr);
}

void tst_projectmanager::cleanup()
{
    m_filesToDelete.append(m_projectPath);
    delete m_testPM;
    m_testPM = nullptr;
}

void tst_projectmanager::test_saveProject()
{
    m_projectPath = m_testResourcePath;
    m_projectPath.append("/").append("test_saveProject.json");
    if (QFile(m_projectPath).exists()) {
        QFile(m_projectPath).remove();
    }
    m_projectName = "test_saveProject";

    QVERIFY(m_testPM != nullptr);
    QVERIFY(!QFile(m_projectPath).exists());
    QVERIFY(!m_testPM->isProjectLoaded());

    QVERIFY(saveTestProject(m_projectPath, m_projectName));
    QVERIFY(QFile(m_projectPath).exists());
    QVERIFY(m_testPM->isProjectLoaded());
    QVERIFY(0 == QString::compare(m_testPM->getProjectName(), m_projectName, Qt::CaseSensitive));
    QVERIFY(0 == QString::compare(m_testPM->getProjectPath(), m_projectPath, Qt::CaseSensitive));

    QFile projectFile(m_projectPath);
    projectFile.open(QIODevice::ReadOnly | QIODevice::Text);
    QString data = projectFile.readAll();
    projectFile.close();
    QStringList projectLines = data.split("\n");

    QVERIFY(0 == QString::compare(projectLines[3], "            \"testBufferName\": \"testValue\"", Qt::CaseSensitive));
    QVERIFY(0 == QString::compare(projectLines[14], "    \"Project name\": \"test_saveProject\"", Qt::CaseSensitive));
}

void tst_projectmanager::test_createProject()
{
    m_projectPath = m_testResourcePath;
    m_projectPath.append("/").append("test_saveProject.json");
    if (QFile(m_projectPath).exists()) {
        QFile(m_projectPath).remove();
    }
    m_projectName = "test_saveProject";

    QVERIFY(m_testPM != nullptr);
    QVERIFY(!QFile(m_projectPath).exists());
    QVERIFY(!m_testPM->isProjectLoaded());

    QVERIFY(saveTestProject(m_projectPath, m_projectName));
    QVERIFY(QFile(m_projectPath).exists());
    QVERIFY(m_testPM->isProjectLoaded());

    QString oldName = m_testPM->getProjectName();
    QString oldPath = m_testPM->getProjectPath();

    m_projectPath = m_testResourcePath;
    m_projectPath.append("/").append("test_createProject.json");
    if (QFile(m_projectPath).exists()) {
        QFile(m_projectPath).remove();
    }
    m_projectName = "test_createProject";

    QVERIFY(m_testPM->createProject(m_testMIP, m_testMA, m_projectPath, m_projectName));
    QVERIFY(QFile(m_projectPath).exists());
    QVERIFY(QFile(oldPath).exists());
    QVERIFY(0 == QString::compare(m_testPM->getProjectName(), oldName, Qt::CaseSensitive));
    QVERIFY(0 == QString::compare(m_testPM->getProjectPath(), oldPath, Qt::CaseSensitive));
}

void tst_projectmanager::test_loadProject()
{
    QVERIFY(m_testPM != nullptr);
    QVERIFY(!m_testPM->isProjectLoaded());
    QVERIFY(0 == QString::compare(m_testPM->getProjectName(), "", Qt::CaseSensitive));
    QVERIFY(0 == QString::compare(m_testPM->getProjectPath(), "", Qt::CaseSensitive));

    m_projectPath = m_testResourcePath;
    m_projectPath.append("/").append("test_openProject.json");
    if (QFile(m_projectPath).exists()) {
        QFile(m_projectPath).remove();
    }
    m_projectName = "test_openProject";

    QVERIFY(saveTestProject(m_projectPath, m_projectName));
    QVERIFY(QFile(m_projectPath).exists());
    QVERIFY(m_testPM->isProjectLoaded());

    cleanup();
    init();
    delete m_testMIP;
    m_testMIP = nullptr;
    QVERIFY(m_testMIP == nullptr);
    m_testMIP = new ModelInputPictures();
    QVERIFY(m_testMIP != nullptr);

    QVERIFY(m_testPM->loadProject(m_testMIP, m_testMA, m_projectPath));
    QVERIFY(m_testPM->isProjectLoaded());
    QVERIFY(0 == QString::compare(m_testPM->getProjectName(), m_projectName, Qt::CaseSensitive));
    QVERIFY(0 == QString::compare(m_testPM->getProjectPath(), m_projectPath, Qt::CaseSensitive));

    QVERIFY(m_testMIP->getPicCount() == 61);
    QVERIFY(m_testMIP->getKeyframeCount(false) == 61);
    QVERIFY(m_testMIP->getInputResolution() == QPoint(1080, 1920));

    delete m_testMIP;
    m_testMIP = nullptr;
    QVERIFY(m_testMIP == nullptr);
    loadTestVideo();
}

void tst_projectmanager::test_loadFalsePath()
{
    delete m_testMIP;
    m_testMIP = nullptr;
    QVERIFY(m_testMIP == nullptr);
    m_testMIP = new ModelInputPictures();
    QVERIFY(m_testMIP != nullptr);
    QVERIFY(!m_testPM->loadProject(m_testMIP, m_testMA, ""));

    delete m_testMIP;
    m_testMIP = nullptr;
    QVERIFY(m_testMIP == nullptr);
    loadTestVideo();
}

void tst_projectmanager::test_saveNullModels()
{
    m_projectPath = m_testResourcePath;
    m_projectPath.append("/").append("test_saveProject.json");
    if (QFile(m_projectPath).exists()) {
        QFile(m_projectPath).remove();
    }
    m_projectName = "test_saveProject";

    QVERIFY(m_testPM != nullptr);
    QVERIFY(!QFile(m_projectPath).exists());
    QVERIFY(!m_testPM->isProjectLoaded());

    delete m_testMIP;
    m_testMIP = nullptr;
    QVERIFY(m_testMIP == nullptr);

    QVERIFY(!saveTestProject(m_projectPath, m_projectName));

    loadTestVideo();

    delete m_testMA;
    m_testMA = nullptr;
    QVERIFY(m_testMA == nullptr);

    QVERIFY(!saveTestProject(m_projectPath, m_projectName));

    m_testMA = new ModelAlgorithm();
    m_testMA->addPluginBuffer("testPluginName", "testBufferName", QVariant("testValue"));
    QVERIFY(m_testMA != nullptr);
}

void tst_projectmanager::test_saveFalsePath()
{
    test_saveProject();
    QVERIFY(m_testPM->isProjectLoaded());
    QVERIFY(0 == QString::compare(m_testPM->getProjectName(), m_projectName, Qt::CaseSensitive));
    QVERIFY(0 == QString::compare(m_testPM->getProjectPath(), m_projectPath, Qt::CaseSensitive));

    QVERIFY(!saveTestProject("DumbPath", "DumbName"));
    QVERIFY(m_testPM->isProjectLoaded());
    QVERIFY(0 == QString::compare(m_testPM->getProjectName(), m_projectName, Qt::CaseSensitive));
    QVERIFY(0 == QString::compare(m_testPM->getProjectPath(), m_projectPath, Qt::CaseSensitive));

}

QTEST_APPLESS_MAIN(tst_projectmanager)

#include "tst_projectmanager.moc"
