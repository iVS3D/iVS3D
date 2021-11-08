#ifndef RESOURCELOADER_H
#define RESOURCELOADER_H

#include <QString>
#include <QtTest>

/**
* This function checks if the resource under given path exists.
* If not the file is downloaded from ... or the test fails.
*/
void requireResource(QString res){
    if(!QFile(res).exists()){
        QProcess process;
        QString testResources = QString(TEST_RESOURCES);
        QDir testResDir = QDir(testResources);
        testResDir.cdUp();
        QString batPath = testResDir.path() + "/drf.bat";
        process.start(batPath);
        process.waitForFinished();
    }
}
#endif //RESOURCELOADER_H
