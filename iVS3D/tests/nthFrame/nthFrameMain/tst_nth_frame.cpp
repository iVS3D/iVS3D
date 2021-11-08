#include <QtTest>

// add necessary includes here
#include <QObject>
#include "reader_stub.h"
#include "nthframe.h"
#include "logfile_stub.h"

class tst_nth_frame : public QObject
{
    Q_OBJECT

public:
    tst_nth_frame();
    ~tst_nth_frame();

private slots:
    void test_initNthFrame();
    void test_nthframeAllImgs_data();
    void test_nthframeAllImgs();
    void test_nthframeLimits();
    void initTestCase();
private:
    LogFileParent* m_logFile;

};

tst_nth_frame::tst_nth_frame()
{

}

tst_nth_frame::~tst_nth_frame()
{

}

void tst_nth_frame::test_initNthFrame()
{
    Reader_stub r(10,3.0);
    volatile bool stopped = false;
    NthFrame algo;
    algo.initialize(&r);
    auto res = algo.sampleImages(&r,std::vector<uint>({0,1,2,3,4}),nullptr, &stopped, QMap<QString,QVariant>(), false, m_logFile);
    QCOMPARE((int)res.size(),2);
    QCOMPARE(res[0],(uint)0);
    QCOMPARE(res[1],(uint)3);
}

void tst_nth_frame::test_nthframeAllImgs_data()
{
    QTest::addColumn<uint>("N");
    QTest::addColumn<uint>("firstKF");
    QTest::addColumn<uint>("lastKF");
    QTest::addColumn<uint>("KFcount");
    QTest::addColumn<uint>("totalImgs");

    QTest::addRow("N=1, 20Pics") << (uint)1 << (uint)0 << (uint)19 << (uint)20 << (uint)20;
    QTest::addRow("N=2, 20Pics") << (uint)2 << (uint)0 << (uint)18 << (uint)10 << (uint)20;
    QTest::addRow("N=3, 20Pics") << (uint)3 << (uint)0 << (uint)18 << (uint)7  << (uint)20;
}

void tst_nth_frame::test_nthframeAllImgs()
{
    QFETCH(uint,N);
    QFETCH(uint,firstKF);
    QFETCH(uint,lastKF);
    QFETCH(uint,KFcount);
    QFETCH(uint,totalImgs);

    Reader_stub r(totalImgs,1.0);
    bool stopped = false;
    std::vector<uint> vec;
    for(uint i = 0; i<totalImgs; i++){
        vec.push_back(i);
    }
    NthFrame algo;
    algo.slot_nChanged(N);
    auto res = algo.sampleImages(&r,vec,nullptr, &stopped, QMap<QString,QVariant>(), false, m_logFile);
    QCOMPARE((uint)res.size(),KFcount);
    QCOMPARE(res.front(),firstKF);
    QCOMPARE(res.back(),lastKF);
}

void tst_nth_frame::test_nthframeLimits()
{
    Reader_stub r(20,33.33);
    bool stopped = false;
    NthFrame algo;
    auto res = algo.sampleImages(&r,std::vector<uint>({2,3,4,7,8}),nullptr, &stopped, QMap<QString,QVariant>(), false, m_logFile);
    QCOMPARE((int)res.size(),5);
    QCOMPARE(res[0], (uint)2);
    QCOMPARE(res[4], (uint)8);
}

void tst_nth_frame::initTestCase()
{
    m_logFile = new LogFile_stub("test", true);

}


QTEST_APPLESS_MAIN(tst_nth_frame)

#include "tst_nth_frame.moc"
