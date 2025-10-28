#include <QCoreApplication>
#include <QtTest>
#include <opencv2/core.hpp>

#include "exportthread.h"
#include "itransform_stub.h"
#include "logfile.h"
#include "modelinputpictures.h"
#include "progressable.h"
#include "resourceloader.h"

class tst_exportThread : public QObject {
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

tst_exportThread::tst_exportThread() {}

tst_exportThread::~tst_exportThread() {}

void tst_exportThread::init() {
    // setup mip
    m_mip = new ModelInputPictures(m_testVideoPath);
    if (m_mip != nullptr) {
        QVERIFY(m_mip->getPicCount() != 0);
    } else {
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
    deleteImages(m_exportPath + "/masks");
}

void tst_exportThread::initTestCase() {
    srand(time(0));
    m_exportPath = QString(TEST_RESOURCES) + "/testexport";
    m_exportWimages = m_exportPath + "/images";
    if (!QDir(m_exportWimages).exists()) {
        qDebug() << "m_exportWimages: " << m_exportWimages;
        if (!QDir().mkpath(m_exportWimages)) {
            // couldn't create testexport path
            qDebug() << "couldn't create testexport path";
            return;
        }
    }

    deleteImages(m_exportWimages);
    deleteImages(m_exportPath + "/masks");

    m_testresourcePath = QString(TEST_RESOURCES);
    m_testVideoPath = m_testresourcePath + "/video.mp4";
    requireResource(m_testVideoPath);

    // m_mip->getReaderParams()->setWorkingResolution(m_mip->getReaderParams()->getOriginalResolution());
    // m_mip->getReaderParams()->setRoi(ROI());
    // m_mip->getReaderParams()->setUseRoi(false);
}

void tst_exportThread::cleanupTestCase() {
    deleteImages(m_exportWimages);
    QVERIFY(QDir(m_exportWimages).isEmpty());
    QDir(m_exportPath).removeRecursively();
}

void tst_exportThread::test_exportVideo() {
    for (uint i = 0; i < m_mip->getPicCount() && i <= 50; i += 5) {
        m_mip->removeKeyframe(i);
    }
    qDebug() << QString::number(m_mip->getKeyframeCount(false));
    m_mip->setBoundaries(QPoint(0, 50));
    qDebug() << m_mip->getBoundaries();

    volatile bool stopped = false;
    std::vector<ITransform *> transforms;
    auto prog = new Progressable();
    ExportConfig cfg;
    cfg.destination = m_exportPath;
    cfg.format = "png";
    cfg.name = "MyTestExport";
    cfg.original_resolution = m_mip->getReaderParams()->getOriginalResolution();
    cfg.working_resolution = m_mip->getReaderParams()->getWorkingResolution();
    cfg.export_resolution = m_mip->getReaderParams()->getWorkingResolution();
    cfg.roi = std::nullopt;
    cfg.transformations = transforms;
    ExportThread t(prog, m_mip, cfg, &stopped, new LogFile("test", false));
    t.start();
    while (!t.isFinished()) {
        QTest::qWait(250);
    }

    QVERIFY(t.getResult().type == ExportResultType::Success);
    QVERIFY(t.isFinished());
    QStringList imgs = QDir(m_exportWimages).entryList(QStringList("*.png"));
    QCOMPARE(imgs.length(), 40);
    delete prog;
}

void tst_exportThread::test_abortExportImmediately() {
    for (uint i = 0; i < m_mip->getPicCount() && i < 50; i += 5) {
        m_mip->addKeyframe(i);
    }
    m_mip->setBoundaries(QPoint(0, m_mip->getPicCount() - 1));

    volatile bool stopped = false;
    std::vector<ITransform *> transforms;
    auto prog = new Progressable();
    ExportConfig cfg;
    cfg.destination = m_exportPath;
    cfg.format = "png";
    cfg.name = "MyTestExport";
    cfg.original_resolution = m_mip->getReaderParams()->getOriginalResolution();
    cfg.working_resolution = m_mip->getReaderParams()->getWorkingResolution();
    cfg.export_resolution = m_mip->getReaderParams()->getWorkingResolution();
    cfg.roi = std::nullopt;
    cfg.transformations = transforms;
    ExportThread exportThread(prog, m_mip, cfg, &stopped, new LogFile("test", false));
    exportThread.start();
    stopped = true;
    while (!exportThread.isFinished()) {
        QTest::qWait(250);
    }

    QVERIFY(exportThread.getResult().type == ExportResultType::Aborted);
    QVERIFY(exportThread.isFinished());
    delete prog;
}

void tst_exportThread::test_exportCorrectImages() {
    std::vector<uint> randomIdx = genRNum(m_mip->getPicCount(), 20);

    for (int i = 0; i < randomIdx.size(); ++i) {
        m_mip->addKeyframe(randomIdx[i]);
    }
    m_mip->setBoundaries(QPoint(0, m_mip->getPicCount() - 1));

    volatile bool stopped = false;

    // prepare iTransformStub and create folders
    std::vector<ITransform *> transforms;
    transforms.push_back(new ITransform_stub());
    QString iTransformPath = m_exportPath + "/masks";

    auto prog = new Progressable();
    ExportConfig cfg;
    cfg.destination = m_exportPath;
    cfg.format = "png";
    cfg.name = "MyTestExport";
    cfg.original_resolution = m_mip->getReaderParams()->getOriginalResolution();
    cfg.working_resolution = m_mip->getReaderParams()->getWorkingResolution();
    cfg.export_resolution = m_mip->getReaderParams()->getWorkingResolution();
    cfg.roi = m_mip->getReaderParams()->getUseRoi() ? std::optional<ROI>(m_mip->getReaderParams()->getRoi()) : std::nullopt;
    cfg.transformations = transforms;
    ExportThread exportThread(prog, m_mip, cfg, &stopped, new LogFile("test", false));
    exportThread.start();
    qDebug() << "started exportthread, this needs some time.";
    while (!exportThread.isFinished()) {
        QTest::qWait(250);
    }
    qDebug() << "exportthread finished";

    QVERIFY(exportThread.getResult().type == ExportResultType::Success);
    QVERIFY(exportThread.isFinished());

    // verifying correctness

    QStringList allSubDirs = {iTransformPath, m_exportWimages};

    QCOMPARE(allSubDirs.length(), 2);
    for (int i = 0; i < allSubDirs.length(); ++i) {
        // directories hold right number of files
        qDebug() << "[Export " << i << "] at " << allSubDirs[i];
        QCOMPARE(QDir(allSubDirs[i]).entryList().length() - 2,
                 m_mip->getKeyframeCount(false));
    }
    std::vector<uint> keyframelist = m_mip->getAllKeyframes(false);
    for (uint i = 0; i < m_mip->getKeyframeCount(false); ++i) {
        cv::Mat mipPic = *m_mip->getPic(keyframelist[i]);
        cv::Mat dummyImg =
            m_iTransformStub
                ->transform(0, mipPic,
                            m_mip->getReaderParams()->getWorkingResolution(),
                            m_mip->getReaderParams()->getRoi())
                .value();

        // aquire image
        QString imgPath = iTransformPath;
        imgPath.append("/").append(QString::number(keyframelist[i], 10)
                                       .rightJustified(8, '0')
                                       .append(".png"));
        cv::Mat exportImg = cv::imread(imgPath.toStdString());
        cv::cvtColor(exportImg, exportImg, cv::COLOR_BGR2GRAY);

        QVERIFY(exportImg.rows == dummyImg.rows);
        QVERIFY(exportImg.cols == dummyImg.cols);
        QVERIFY(exportImg.type() == dummyImg.type());

        // compare cv:mats
        bool eq = false;
        cv::Mat diff;
        diff = exportImg != dummyImg;
        eq = cv::countNonZero(diff) == 0;
        QVERIFY(eq);
    }

    // end verifying correctness
    deleteImages(iTransformPath);
    delete prog;
}
void tst_exportThread::test_exportResolutionAndCrop() {
    QFETCH(QPoint, in_resolution);
    QFETCH(QRect, in_roi);
    QFETCH(bool, in_useRoi);
    QFETCH(bool, in_withTransfrom);
    QFETCH(QPoint, out_resolution);

    auto kfs = genRNum(m_mip->getPicCount(), 20);
    std::sort(kfs.begin(), kfs.end());
    m_mip->setBoundaries(QPoint(0, m_mip->getPicCount() - 1));
    m_mip->updateMIP(kfs);

    m_mip->getReaderParams()->setWorkingResolution(Resolution(in_resolution));
    m_mip->getReaderParams()->setRoi(
        ROI(in_roi, m_mip->getReaderParams()->getOriginalResolution()));
    m_mip->getReaderParams()->setUseRoi(in_useRoi);

    qDebug() << "m_mip setup done: " << m_mip->getReaderParams()->toText();
    volatile bool stopped = false;
    std::vector<ITransform *> transforms;
    if (in_withTransfrom) {
        transforms.push_back(new ITransform_stub);
    }
    auto prog = new Progressable();

    ExportConfig cfg;
    cfg.destination = m_exportPath;
    cfg.format = "png";
    cfg.name = "MyTestExport";
    cfg.original_resolution = m_mip->getReaderParams()->getOriginalResolution();
    cfg.working_resolution = m_mip->getReaderParams()->getWorkingResolution();
    cfg.export_resolution = m_mip->getReaderParams()->getWorkingResolution();
    cfg.roi = m_mip->getReaderParams()->getUseRoi() ? std::optional<ROI>(m_mip->getReaderParams()->getRoi()) : std::nullopt;
    cfg.transformations = transforms;

    ExportThread t(prog, m_mip, cfg, &stopped, new LogFile("test", false));
    qDebug() << "thread created";
    t.start();
    while (!t.isFinished()) {
        QTest::qWait(250);
    }

    QVERIFY(t.getResult().type == ExportResultType::Success);
    QVERIFY(t.isFinished());
    qDebug() << "Thread finished";

    auto compareOutput = [=](QString path) {
        QStringList imgs = QDir(path).entryList(QStringList("*.png"));
        for (auto img : imgs) {
            QString pathToImg = path + "/" + img;
            auto mat = cv::imread(pathToImg.toStdString());
            QPoint exported_resolution(mat.cols, mat.rows);
            QCOMPARE(exported_resolution, out_resolution);
        }
    };

    compareOutput(m_exportWimages);

    if (in_withTransfrom) {
        compareOutput(m_exportPath + "/masks");
    }
    delete prog;
}

void tst_exportThread::test_exportResolutionAndCrop_data() {
    QTest::addColumn<QPoint>("in_resolution");
    QTest::addColumn<QRect>("in_roi");
    QTest::addColumn<bool>("in_useRoi");
    QTest::addColumn<bool>("in_withTransfrom");
    QTest::addColumn<QPoint>("out_resolution");

    // test id                       |   in_resolution       |   in_roi |
    // in_useRoi  | in_withTr | out_resolution
    // ------------------------------+-----------------------+-----------------------+------------+-----------+----------------
    QTest::addRow("default") << QPoint(1080, 1920) << QRect(0, 0, 0, 0) << false
                             << false << QPoint(1080, 1920);
    QTest::addRow("default+roi")
        << QPoint(1080, 1920) << QRect(20, 20, 300, 300) << true << false
        << QPoint(300, 300);
    QTest::addRow("res") << QPoint(455, 1000) << QRect(0, 0, 0, 0) << false
                         << false << QPoint(455, 1000);
    QTest::addRow("res+roi") << QPoint(540, 960) << QRect(250, 250, 540, 960)
                             << true << false << QPoint(270, 480);
    QTest::addRow("iTr_default") << QPoint(1080, 1920) << QRect(0, 0, 0, 0)
                                 << false << true << QPoint(1080, 1920);
    QTest::addRow("iTr_default+roi")
        << QPoint(1080, 1920) << QRect(20, 20, 300, 300) << true << true
        << QPoint(300, 300);
    QTest::addRow("iTr_res") << QPoint(455, 1000) << QRect(0, 0, 0, 0) << false
                             << true << QPoint(455, 1000);
    QTest::addRow("iTr_res+roi")
        << QPoint(540, 960) << QRect(250, 250, 540, 960) << true << true
        << QPoint(270, 480);
}

bool tst_exportThread::deleteImages(QString path) {
    QStringList imagesList = QDir(path).entryList();
    for (int i = 0; i < imagesList.length(); ++i) {
        if (imagesList[i].endsWith(".png")) {
            QDir(path).remove(imagesList[i]);
        }
    }
    return true;
}

std::vector<uint> tst_exportThread::genRNum(uint maxNum, uint count) {
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
