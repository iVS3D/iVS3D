#include <QtTest>

#include "logfile.h"

#define STAMP_COUNT 15
#define PROCESS_PREFIX "InternalProcess"
#define MAX_TIME 1000
#define MAX_FRAMES 100
#define MAX_KEYFRAMES 50
#define TIMING_TOLERANCE 50
#define SETTING_COUNT 15
#define MAX_SETTING_VALUE 1000
#define SETTING_PREFIX "Setting"
#define TYPE_CUSTOM "test type"

class tst_logfile : public QObject
{
    Q_OBJECT

public:
    tst_logfile();
    ~tst_logfile();

private slots:
    void init();
    void test_algoMockup();
    void cleanup();

private:
    void algoMockup(LogFile *lf);
    std::vector<qint64> m_timeStamps = {};
    LogFile* m_lf;
    std::vector<uint> m_keyframes;
    std::vector<uint> m_inputFrames;
    QMap<QString, QVariant> m_settings;

};

tst_logfile::tst_logfile() {

}

tst_logfile::~tst_logfile() {

}

void tst_logfile::algoMockup(LogFile *lf)
{
    // log input information
    lf->setInputInfo(m_inputFrames);

    // log settings
    lf->setSettings(m_settings);

    // simulate internal processes of diffrent length
    for (uint id = 0; id < m_timeStamps.size(); id++) {
        lf->startTimer(PROCESS_PREFIX + QString::number(id));
        QTest::qSleep(m_timeStamps[id]);
        lf->stopTimer();
        lf->addCustomEntry("Custom log entry " + QString::number(id), id, TYPE_CUSTOM);
    }

    // save results
    lf->setResultsInfo(m_keyframes);
}

void tst_logfile::init()
{
    // setup how long the internal processes of algoMockup will take
    for (uint i = 0; i < STAMP_COUNT; i++) {
        m_timeStamps.push_back(std::rand() % MAX_TIME);
    }

    // preset input
    const uint inputFrameCount = rand() % MAX_FRAMES;
    for (uint i = 0; i < inputFrameCount; i++) {
        uint randFrame = rand() % MAX_FRAMES;
        //if (std::find(m_inputFrames.begin(), m_inputFrames.end(), randFrame) == m_inputFrames.end())
            m_inputFrames.push_back(randFrame);
    }
    std::sort(m_inputFrames.begin(), m_inputFrames.end());
    // select random keyframes from inputFrames
    const uint keyframeCount = rand() % MAX_KEYFRAMES;
    for (uint i = 0; i < keyframeCount; i++) {
        uint randIdx = rand() % inputFrameCount;
        m_keyframes.push_back(m_inputFrames[randIdx]);
    }
    std::sort(m_keyframes.begin(), m_keyframes.end());
    // preset settings
    for (uint i = 0; i < SETTING_COUNT; i++) {
        m_settings.insert(SETTING_PREFIX + QString::number(i), QVariant(std::rand() % MAX_SETTING_VALUE));
    }
}

void tst_logfile::test_algoMockup()
{
    const QString processName = "Algo Mockup";

    m_lf = new LogFile(processName, true);
    algoMockup(m_lf);

    //      check log file
    QJsonObject jsonLf = m_lf->toQJSON();
    // name
    QJsonValueRef tmpName = jsonLf.find(stringContainer::logNameIdentifier).value();
    QVERIFY2(tmpName.isString(), "Name should be a string");
    QCOMPARE(tmpName.toString(), processName);
    // input
    QJsonObject jsonInput = jsonLf.find(stringContainer::logInputIdentifier).value().toObject();
    int frameCount = jsonInput.find(stringContainer::logKeyframeCountIdentifier)->toInt();
    QCOMPARE(frameCount, (int)m_inputFrames.size());
    // settings
    QJsonValueRef tmpSettings = jsonLf.find(stringContainer::logSettingsIdentifier).value();
    QMap<QString, QVariant> tmpSettingsMap = tmpSettings.toVariant().toMap();
    QVERIFY2(tmpSettingsMap.size() == m_settings.size(), "Number of stored settings is wrong");
    for (int i = 0; i < tmpSettingsMap.size(); i++) {
        QString identifier = SETTING_PREFIX + QString::number(i);
        int logValue = tmpSettingsMap.find(identifier)->toInt();
        int expectedValue = m_settings.find(identifier)->toInt();
        QVERIFY(logValue == expectedValue);
        if (logValue != expectedValue) {
            qDebug() << "logValue=" << logValue << " Expected=" << expectedValue;
        }
    }
    // internal processes
    QJsonArray jsonArray = jsonLf.find(stringContainer::logProcedureIdentifier).value().toArray();
    for (int i = 1; i < jsonArray.size(); i++) {
        QJsonObject jsonProcedure = jsonArray[i].toObject();
        qint64 elapsedTime = jsonProcedure.find(stringContainer::logElapsedTimeIdentifier)->toInt();
        // i - 1 because the first entry of the jsonArray are the gloabl information about the internal procedures
        qint64 expectedValue = m_timeStamps[i - 1];
        bool toleranceCheck = expectedValue - TIMING_TOLERANCE < elapsedTime || elapsedTime < expectedValue + TIMING_TOLERANCE;
        QVERIFY(toleranceCheck);
        if (!toleranceCheck) {
            qDebug() << "logValue=" << elapsedTime << " Expected=" << expectedValue;
        }
    }
    // result
    QJsonObject jsonResult = jsonLf.find(stringContainer::logResultsIdentifier).value().toObject();
    int keyframeCount = jsonResult.find(stringContainer::logKeyframeCountIdentifier)->toInt();
    QCOMPARE(keyframeCount, (int)m_keyframes.size());

}

void tst_logfile::cleanup()
{
    delete m_lf;

    // reset parameters
    m_timeStamps.clear();
    m_inputFrames = {};
    m_keyframes = {};
    m_settings = {};
}

QTEST_APPLESS_MAIN(tst_logfile)

#include "tst_logfile.moc"
