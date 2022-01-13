#ifndef RESOURCELOADER_H
#define RESOURCELOADER_H

#include <QString>
#include <QtTest>
#include <opencv2/videoio.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>


void expandVideo(){
    QString videoPath(QString(TEST_RESOURCES) + "/video.mp4");
    if(QFile(videoPath).exists() && !QDir(QString(TEST_RESOURCES) + "/images").exists()){
        cv::VideoCapture cap(videoPath.toStdString());
        cv::Mat frame;
        QString imgPath(QString(TEST_RESOURCES) + "/images");
        QDir(QString(TEST_RESOURCES)).mkdir("images");
        int i = 1;
        while (1) {
            cap >> frame;
            if(!frame.data) break;
            QString img(imgPath + "/image (" + QString::number(i) + ").png");
            cv::imwrite(img.toStdString(),frame);
            i++;
        }
    }
}

/**
* This function checks if the resource under given path exists.
* If not the file is downloaded from ... or the test fails.
*/
void requireResource(QString res){
    qDebug() << "looking for test-resource: " << res;
    if(!QFile(res).exists()){
        qDebug() << "RESOURCE MISSING! Downloading...";
        QProcess process;
        QString testResources = QString(TEST_RESOURCES);
        QDir testResDir = QDir(testResources);
        testResDir.cdUp();
        QString batPath = testResDir.path() + "/drf.bat";
        qDebug() << batPath;
        process.start(batPath);
        process.waitForFinished();
        qDebug() << "Finished downloading.";
        expandVideo();
    }
}


#endif //RESOURCELOADER_H
