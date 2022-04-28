#include <QtTest>
#include <QCoreApplication>

// add necessary includes here
#include "resourceloader.h"
#include "controller/modelinputiteratorfactory.h"
#include "controller/ModelInputIterator.h"
#include "model/modelinputpictures.h"
#include "controller/imageiterator.h"
#include "controller/keyframeiterator.h"

class tst_modelinputiterator : public QObject
{
    Q_OBJECT

public:
    tst_modelinputiterator();
    ~tst_modelinputiterator();

private:
    QString m_testresourcePath;
    QString m_testImagesPath;
    ModelInputPictures *m_mip;

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void test_createImageItr();
    void test_createKeyframeItr();
    void test_iterateImages();
    void test_iterateImages_data();
    void test_iterateKeyframes();
    void test_iterateKeyframes_data();
};

tst_modelinputiterator::tst_modelinputiterator() {}
tst_modelinputiterator::~tst_modelinputiterator() {}

void tst_modelinputiterator::initTestCase()
{
    m_testImagesPath = QString(TEST_RESOURCES) + "/BlurTest";

    requireResource(m_testImagesPath + "/Blur5.jpg");
    requireResource(m_testImagesPath + "/Blur25.jpg");
    requireResource(m_testImagesPath + "/Blur50.jpg");
    requireResource(m_testImagesPath + "/WithoutBlur.jpg");
}

void tst_modelinputiterator::init()
{
    m_mip = new ModelInputPictures(m_testImagesPath);
    QVERIFY2(m_mip->getPicCount()>0, "failed to load test images!");

    m_mip->setBoundaries(QPoint(0,m_mip->getPicCount()));
}

void tst_modelinputiterator::cleanup()
{
    delete m_mip;
    m_mip = nullptr;
}

void tst_modelinputiterator::test_createImageItr()
{
    ModelInputIterator *mii = ModelInputIteratorFactory::createIterator(ModelInputIteratorFactory::IteratorType::Images);
    ImageIterator *ii = dynamic_cast<ImageIterator*>(mii);
    QVERIFY2(ii, "failed to create image iterator");
}

void tst_modelinputiterator::test_createKeyframeItr()
{
    ModelInputIterator *mii = ModelInputIteratorFactory::createIterator(ModelInputIteratorFactory::IteratorType::Keyframes);
    KeyframeIterator *ki = dynamic_cast<KeyframeIterator*>(mii);
    QVERIFY2(ki, "failed to create keyframe iterator");
}

void tst_modelinputiterator::test_iterateImages()
{
    QFETCH(std::vector<uint>, in_keyframes);
    QFETCH(uint, in_idx);
    QFETCH(uint, in_stepsize);
    QFETCH(bool, out_isFirst);
    QFETCH(bool, out_isLast);
    QFETCH(uint, out_first);
    QFETCH(uint, out_last);
    QFETCH(uint, out_next);
    QFETCH(uint, out_previous);

    m_mip->updateMIP(in_keyframes);

    ImageIterator itr;

    QCOMPARE(itr.isFirst(m_mip, in_idx), out_isFirst);
    QCOMPARE(itr.isLast(m_mip, in_idx),  out_isLast);

    QCOMPARE(itr.getFirst(m_mip), out_first);
    QCOMPARE(itr.getLast(m_mip), out_last);

    QCOMPARE(itr.getNext(m_mip, in_idx, in_stepsize), out_next);
    QCOMPARE(itr.getPrevious(m_mip, in_idx, in_stepsize), out_previous);
}

void tst_modelinputiterator::test_iterateImages_data()
{
    QTest::addColumn<std::vector<uint>>("in_keyframes");
    QTest::addColumn<uint>("in_idx");
    QTest::addColumn<uint>("in_stepsize");
    QTest::addColumn<bool>("out_isFirst");
    QTest::addColumn<bool>("out_isLast");
    QTest::addColumn<uint>("out_first");
    QTest::addColumn<uint>("out_last");
    QTest::addColumn<uint>("out_next");
    QTest::addColumn<uint>("out_previous");

    //      test id         | in_keyframes                      | in_idx    | in_step   | out_isF | out_isL | out_first | out_last  | out_next  | out_prev
    // ---------------------+-----------------------------------+-----------+-----------+---------+---------+-----------+-----------+-----------+----------
    QTest::addRow("first")  << std::vector<uint>({0,1,2,3,4})   << (uint)0  <<  (uint)1 << true   << false  << (uint)0  << (uint)4  << (uint)1  << (uint)0;
    QTest::addRow("second") << std::vector<uint>({0,1,2,3,4})   << (uint)1  <<  (uint)1 << false  << false  << (uint)0  << (uint)4  << (uint)2  << (uint)0;
    QTest::addRow("third")  << std::vector<uint>({0,1,2,3,4})   << (uint)2  <<  (uint)1 << false  << false  << (uint)0  << (uint)4  << (uint)3  << (uint)1;
    QTest::addRow("last")   << std::vector<uint>({0,1,2,3,4})   << (uint)4  <<  (uint)1 << false  << true   << (uint)0  << (uint)4  << (uint)4  << (uint)3;
    QTest::addRow("first2") << std::vector<uint>({0,1,2,3,4})   << (uint)0  <<  (uint)2 << true   << false  << (uint)0  << (uint)4  << (uint)2  << (uint)0;
    QTest::addRow("second2")<< std::vector<uint>({0,1,2,3,4})   << (uint)1  <<  (uint)2 << false  << false  << (uint)0  << (uint)4  << (uint)3  << (uint)0;
    QTest::addRow("third2") << std::vector<uint>({0,1,2,3,4})   << (uint)2  <<  (uint)2 << false  << false  << (uint)0  << (uint)4  << (uint)4  << (uint)0;
    QTest::addRow("last2")  << std::vector<uint>({0,1,2,3,4})   << (uint)4  <<  (uint)2 << false  << true   << (uint)0  << (uint)4  << (uint)4  << (uint)2;
    QTest::addRow("_first")  << std::vector<uint>({1,2})        << (uint)0  <<  (uint)1 << true   << false  << (uint)0  << (uint)4  << (uint)1  << (uint)0;
    QTest::addRow("_second") << std::vector<uint>({1,2})        << (uint)1  <<  (uint)1 << false  << false  << (uint)0  << (uint)4  << (uint)2  << (uint)0;
    QTest::addRow("_third")  << std::vector<uint>({1,2})        << (uint)2  <<  (uint)1 << false  << false  << (uint)0  << (uint)4  << (uint)3  << (uint)1;
    QTest::addRow("_last")   << std::vector<uint>({1,2})        << (uint)4  <<  (uint)1 << false  << true   << (uint)0  << (uint)4  << (uint)4  << (uint)3;
    QTest::addRow("_first2") << std::vector<uint>({1,2})        << (uint)0  <<  (uint)2 << true   << false  << (uint)0  << (uint)4  << (uint)2  << (uint)0;
    QTest::addRow("_second2")<< std::vector<uint>({1,2})        << (uint)1  <<  (uint)2 << false  << false  << (uint)0  << (uint)4  << (uint)3  << (uint)0;
    QTest::addRow("_third2") << std::vector<uint>({1,2})        << (uint)2  <<  (uint)2 << false  << false  << (uint)0  << (uint)4  << (uint)4  << (uint)0;
    QTest::addRow("_last2")  << std::vector<uint>({1,2})        << (uint)4  <<  (uint)2 << false  << true   << (uint)0  << (uint)4  << (uint)4  << (uint)2;
}

void tst_modelinputiterator::test_iterateKeyframes()
{
    QFETCH(std::vector<uint>, in_keyframes);
    QFETCH(uint, in_idx);
    QFETCH(uint, in_stepsize);
    QFETCH(bool, out_isFirst);
    QFETCH(bool, out_isLast);
    QFETCH(uint, out_first);
    QFETCH(uint, out_last);
    QFETCH(uint, out_next);
    QFETCH(uint, out_previous);

    m_mip->updateMIP(in_keyframes);

    KeyframeIterator itr;

    QCOMPARE(itr.isFirst(m_mip, in_idx), out_isFirst);
    QCOMPARE(itr.isLast(m_mip, in_idx),  out_isLast);

    QCOMPARE(itr.getFirst(m_mip), out_first);
    QCOMPARE(itr.getLast(m_mip), out_last);

    QCOMPARE(itr.getNext(m_mip, in_idx, in_stepsize), out_next);
    QCOMPARE(itr.getPrevious(m_mip, in_idx, in_stepsize), out_previous);
}

void tst_modelinputiterator::test_iterateKeyframes_data()
{
    QTest::addColumn<std::vector<uint>>("in_keyframes");
    QTest::addColumn<uint>("in_idx");
    QTest::addColumn<uint>("in_stepsize");
    QTest::addColumn<bool>("out_isFirst");
    QTest::addColumn<bool>("out_isLast");
    QTest::addColumn<uint>("out_first");
    QTest::addColumn<uint>("out_last");
    QTest::addColumn<uint>("out_next");
    QTest::addColumn<uint>("out_previous");

    //      test id         | in_keyframes                      | in_idx    | in_step   | out_isF | out_isL | out_first | out_last  | out_next  | out_prev
    // ---------------------+-----------------------------------+-----------+-----------+---------+---------+-----------+-----------+-----------+----------
    QTest::addRow("first")  << std::vector<uint>({0,1,2,3})     << (uint)0  <<  (uint)1 << true   << false  << (uint)0  << (uint)3  << (uint)1  << (uint)0;
    QTest::addRow("second") << std::vector<uint>({0,1,2,3})     << (uint)1  <<  (uint)1 << false  << false  << (uint)0  << (uint)3  << (uint)2  << (uint)0;
    QTest::addRow("third")  << std::vector<uint>({0,1,2,3})     << (uint)2  <<  (uint)1 << false  << false  << (uint)0  << (uint)3  << (uint)3  << (uint)1;
    QTest::addRow("last")   << std::vector<uint>({0,1,2,3})     << (uint)4  <<  (uint)1 << false  << true   << (uint)0  << (uint)3  << (uint)3  << (uint)3;
    QTest::addRow("first2") << std::vector<uint>({0,1,2,3})     << (uint)0  <<  (uint)2 << true   << false  << (uint)0  << (uint)3  << (uint)2  << (uint)0;
    QTest::addRow("second2")<< std::vector<uint>({0,1,2,3})     << (uint)1  <<  (uint)2 << false  << false  << (uint)0  << (uint)3  << (uint)3  << (uint)0;
    QTest::addRow("third2") << std::vector<uint>({0,1,2,3})     << (uint)2  <<  (uint)2 << false  << false  << (uint)0  << (uint)3  << (uint)3  << (uint)0;
    QTest::addRow("last2")  << std::vector<uint>({0,1,2,3})     << (uint)4  <<  (uint)2 << false  << true   << (uint)0  << (uint)3  << (uint)3  << (uint)2;
    QTest::addRow("_first")  << std::vector<uint>({1,3,4})      << (uint)0  <<  (uint)1 << true   << false  << (uint)1  << (uint)4  << (uint)1  << (uint)1;
    QTest::addRow("_second") << std::vector<uint>({1,3,4})      << (uint)1  <<  (uint)1 << true   << false  << (uint)1  << (uint)4  << (uint)3  << (uint)1;
    QTest::addRow("_third")  << std::vector<uint>({1,3,4})      << (uint)2  <<  (uint)1 << false  << false  << (uint)1  << (uint)4  << (uint)3  << (uint)1;
    QTest::addRow("_last")   << std::vector<uint>({1,3,4})      << (uint)4  <<  (uint)1 << false  << true   << (uint)1  << (uint)4  << (uint)4  << (uint)3;
    QTest::addRow("_first2") << std::vector<uint>({1,3,4})      << (uint)0  <<  (uint)2 << true   << false  << (uint)1  << (uint)4  << (uint)3  << (uint)1;
    QTest::addRow("_second2")<< std::vector<uint>({1,3,4})      << (uint)1  <<  (uint)2 << true   << false  << (uint)1  << (uint)4  << (uint)4  << (uint)1;
    QTest::addRow("_third2") << std::vector<uint>({1,3,4})      << (uint)2  <<  (uint)2 << false  << false  << (uint)1  << (uint)4  << (uint)4  << (uint)1;
    QTest::addRow("_last2")  << std::vector<uint>({1,3,4})      << (uint)4  <<  (uint)2 << false  << true   << (uint)1  << (uint)4  << (uint)4  << (uint)1;
}

QTEST_MAIN(tst_modelinputiterator)

#include "tst_modelinputiterator.moc"
