#include <QtTest>
#include "reader.h"
#include "readerfactory.h"
#include "openexecutor.h"
#include "DataManager.h"
#include "resourceloader.h"

// add necessary includes here

class tst_reader : public QObject
{
    Q_OBJECT

public:
    tst_reader();
    ~tst_reader();

private slots:
    void initTestCase();
    void cleanupTestCase();
    void test_DeleteVideo();
    void test_DeleteImages();
    void test_EmptyFile();
    void test_ReadOneToMany();
    void test_ReadWayToMany();
    void test_ReadNegativeIndex();
    void test_CopyImageReader();
    void test_CopyVideoReader();

private:
    void compare(Reader* a, Reader* b);
    QString m_resources = QString(TEST_RESOURCES);
    QStringList m_cleanUp = {};

};

tst_reader::tst_reader()
{

}

tst_reader::~tst_reader()
{

}

void tst_reader::initTestCase()
{
    requireResource(m_resources + "/video.mp4");
    requireResource(m_resources + "/image.png");
    //for(int i = 1; i<=217; i++){
    //    requireResource(m_resources + "/images/image (" + QString::number(i) + ").png");
    //}
}

void tst_reader::cleanupTestCase()
{
    while(!m_cleanUp.empty()){
        QFile* file = new QFile(m_resources + "/" + m_cleanUp.takeFirst());
        file->remove();
    }
    QDir(m_resources + "/v").removeRecursively();
}

void tst_reader::test_DeleteVideo()
{
    QDir* currentDir = new QDir(m_resources);
    currentDir->mkdir("v");
    QFile* video = new QFile((m_resources + "/video.mp4"));
    video->copy(m_resources + "/v/video.mp4");

    Reader* v = ReaderFactory::instance().createReader(m_resources + "/v/video.mp4");
    cv::Mat before = v->getPic(0);
    currentDir->cd("v");

    bool deleted = currentDir->removeRecursively();
    cv::Mat after = v->getPic(0);

    if(after.empty() && deleted){
        // displaing black pics after input is deleted is acceptable behaviour
        // app did not crash
        QVERIFY(true);
    } else {
        // delete not successfull
        cv::Mat equal;
        cv::compare(before, after, equal, CV_HAL_CMP_EQ);
        QVERIFY(equal.data[0] == 255);
        // data is save
        m_cleanUp.push_back("v");
    }
    delete v;
}

void tst_reader::test_DeleteImages()
{
    QDir* currentDir = new QDir(m_resources);
    currentDir->mkdir("i");
    QString image = m_resources + "/image.png";
    QFile::copy(image, m_resources + "/i/image.png");

    Reader* i = ReaderFactory::instance().createReader(m_resources + "/i");
    cv::Mat before = i->getPic(0);
    currentDir->cd("i");

    bool deleted = currentDir->removeRecursively();
    cv::Mat after = i->getPic(0);

    if(after.empty() && deleted){
        // displaing black pics after input is deleted is acceptable behaviour
        // app did not crash
        QVERIFY(true);
    } else {
        // delete not successfull
        cv::Mat equal;
        cv::compare(before, after, equal, CV_HAL_CMP_EQ);
        QVERIFY(equal.data[0] == 255);
        // data is save
        m_cleanUp.push_back("i");
    }
}

void tst_reader::test_EmptyFile()
{
    Reader* r = ReaderFactory::instance().createReader(m_resources + "/emptyFolder");
    QCOMPARE(r, nullptr);
}

void tst_reader::test_ReadOneToMany()
{
    Reader* v = ReaderFactory::instance().createReader(m_resources + "/video.mp4");
    int toMuch = v->getPicCount() + 1;
    cv::Mat vIShouldBeEmpty = v->getPic(toMuch);

    cv::Mat empty;
    cv::Mat equal;
    cv::compare(vIShouldBeEmpty, empty, equal, CV_HAL_CMP_EQ);
    QVERIFY(equal.empty());

    Reader* i = ReaderFactory::instance().createReader(m_resources + "/BlurTest");
    toMuch = i->getPicCount() + 1;
    cv::Mat iIShouldBeEmpty = i->getPic(toMuch);

    cv::Mat newequal;
    cv::compare(iIShouldBeEmpty, empty, newequal, CV_HAL_CMP_EQ);
    QVERIFY(equal.empty());
    delete v;
}

void tst_reader::test_ReadWayToMany()
{
    Reader* v = ReaderFactory::instance().createReader(m_resources + "/video.mp4");
    int toMuch = v->getPicCount() * 2;
    cv::Mat vIShouldBeEmpty = v->getPic(toMuch);

    cv::Mat empty;
    cv::Mat equal;
    cv::compare(vIShouldBeEmpty, empty, equal, CV_HAL_CMP_EQ);
    QVERIFY(equal.empty());

    Reader* i = ReaderFactory::instance().createReader(m_resources + "/BlurTest");
    toMuch = i->getPicCount() * 2;
    cv::Mat iIShouldBeEmpty = i->getPic(toMuch);

    cv::Mat newequal;
    cv::compare(iIShouldBeEmpty, empty, newequal, CV_HAL_CMP_EQ);
    QVERIFY(equal.empty());
    delete v;
}

void tst_reader::test_ReadNegativeIndex()
{
    Reader* v = ReaderFactory::instance().createReader(m_resources + "/video.mp4");
    cv::Mat vIShouldBeEmpty = v->getPic(-1);

    cv::Mat empty;
    cv::Mat equal;
    cv::compare(vIShouldBeEmpty, empty, equal, CV_HAL_CMP_EQ);
    QVERIFY(equal.empty());

    Reader* i = ReaderFactory::instance().createReader(m_resources + "/BlurTest");
    cv::Mat iIShouldBeEmpty = i->getPic(-1);

    cv::Mat newequal;
    cv::compare(iIShouldBeEmpty, empty, newequal, CV_HAL_CMP_EQ);
    QVERIFY(equal.empty());
    delete v;
}

void tst_reader::test_CopyImageReader()
{
    Reader* i = ReaderFactory::instance().createReader(m_resources + "/BlurTest");

    Reader* copy = i->copy();

    compare(i, copy);
}

void tst_reader::test_CopyVideoReader()
{
    Reader* v = ReaderFactory::instance().createReader(m_resources + "/video.mp4");

    Reader* copy = v->copy();

    compare(v, copy);
    delete v;
}

void tst_reader::compare(Reader *a, Reader *b)
{
    QCOMPARE(a->getFPS(), b->getFPS());

    QCOMPARE(a->getFileVector(), b->getFileVector());

    QCOMPARE(a->getInputPath(), b->getInputPath());

    QCOMPARE(a->getPicCount(), b->getPicCount());

    QCOMPARE(a->getVideoDuration(), b->getVideoDuration());

    QCOMPARE(a->isDir(), b->isDir());
}

QTEST_APPLESS_MAIN(tst_reader)

#include "tst_reader.moc"
