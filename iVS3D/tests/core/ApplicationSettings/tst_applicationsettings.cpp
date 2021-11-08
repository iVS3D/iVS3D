#include <QtTest>
#include <QCoreApplication>

#include "applicationsettings.h"

// add necessary includes here

class tst_ApplicationSettings : public QObject
{
    Q_OBJECT

public:
    tst_ApplicationSettings();
    ~tst_ApplicationSettings();

private slots:
    void test_saveSettings();

};

tst_ApplicationSettings::tst_ApplicationSettings()
{

}

tst_ApplicationSettings::~tst_ApplicationSettings()
{

}

void tst_ApplicationSettings::test_saveSettings()
{
    qDebug() << TEST_RESOURCES;
    QString standardInputOld = ApplicationSettings::instance().getStandardInputPath();

    int random = rand();
    QString newStandardInput = QString::number(random);
    ApplicationSettings::instance().setStandardInputPath(newStandardInput);

    QString standardInputNew = ApplicationSettings::instance().getStandardInputPath();

    QCOMPARE(standardInputNew, newStandardInput);

    ApplicationSettings::instance().setStandardInputPath(standardInputOld);

    QCOMPARE(standardInputOld, ApplicationSettings::instance().getStandardInputPath());
}

QTEST_MAIN(tst_ApplicationSettings)

#include "tst_applicationsettings.moc"
