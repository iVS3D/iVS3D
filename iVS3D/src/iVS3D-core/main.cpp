#include "view/mainwindow.h"
#include "controller.h"
#include "nouicontroller.h"
#include "stringcontainer.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QVBoxLayout>
#include "view/darkstyle/DarkStyle.h"
#include "cvmat_qmetadata.h"

#include <QFlags>
#include <QTranslator>

#include <stdio.h>
#include <iostream>

#if defined(Q_OS_WIN)
    #include <Windows.h>
#endif

#ifndef IVS3D_VER
#define IVS3D_VER unknown
#endif

#ifndef IVS3D_DAT
#define IVS3D_DAT unknown
#endif

#ifndef QUOTE
#define QUOTE(str) _QUOTE(str)
#define _QUOTE(str) #str
#endif

#if defined(Q_OS_LINUX)
    #include "translations.h"
#endif

void ignoreMessages(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    (void) type;
    (void) context;
    (void) msg;
}

int main(int argc, char *argv[])
{

    qRegisterMetaType<cv::Mat>("cvMat");
    qRegisterMetaType<ImageList>("ImageList");
    qRegisterMetaType<QStringList>("QStringList");
    qRegisterMetaType<QColorList>("QColorList");
    qRegisterMetaType<QBoolList>("QBoolList");
    qRegisterMetaType<std::vector<uint>>("vectorUint");

    QCommandLineParser parser;
    QCommandLineOption noGUI("nogui", "Executes in terminal mode without the GUI. In this case auto settings file (-a), input (-i) and output (-o) need to be provided.");
    QCommandLineOption inputPath(QStringList() << "i" << "in", "Load input from <path>.", "path");
    QCommandLineOption autoPath(QStringList() << "a" << "auto", "Load settings from <path>.", "path");
    QCommandLineOption outputPath(QStringList() << "o" << "out", "Save result to <path>.", "path");

    parser.setApplicationDescription("intelligent video sampler 3d is designed to process image sequences and videos for 3d reconstruction. "
                                     "enhance reconstruction quality by filtering out blurry images and masking dynamic objects such as people or cars. "
                                     "speedup the reconstruction process by ignoring similar images and using only frames with significant camera movement in between. "
                                     "use colmap on the local system or a remote server to reconstruct a 3d model from your input sequene. \n\n"
                                     "Build date: " + QString(QUOTE(IVS3D_DAT)));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption(autoPath);
    parser.addOption(inputPath);
    parser.addOption(outputPath);
    parser.addOption(noGUI);

    QStringList arguments;
    for(int i = 0; i < argc; ++i){
        arguments << argv[i];
    }

    QTranslator* translator = new QTranslator();
    translator->load(QLocale::system(), "core", "_", ":/translations", ".qm");

    if(!arguments.contains("--nogui")){

        #if defined(Q_OS_WIN)
            FreeConsole();
        #endif

        QApplication a( argc, argv );
        a.setApplicationName("iVS3D");
        a.setApplicationVersion(QString(QUOTE(IVS3D_VER)));
        parser.process(a);
        a.installTranslator(translator);

#if defined(Q_OS_WIN)
        QDir appDir(QApplication::applicationDirPath());
        QApplication::addLibraryPath(appDir.absolutePath());

        QStringList dirs = {"bin", "qml", "plugins" };
        for(auto dir : dirs){
            QDir d(QApplication::applicationDirPath());
            if(d.cd(dir)) QApplication::addLibraryPath(d.absolutePath());
            else qDebug() << "Missing library directory: " << dir;
        }
        qDebug() << "Library search paths: " << QApplication::libraryPaths();
#endif
        Controller *mainController = new Controller(parser.value(inputPath), parser.value(autoPath), parser.value(outputPath));
        qApp->setProperty(stringContainer::UIIdentifier, true);



        auto res = a.exec();
        delete mainController;
        return res;
    }
    else {
        //Disable all Messages comming from Qt
        qInstallMessageHandler(ignoreMessages);
        QCoreApplication a( argc, argv );
        a.setApplicationName("iVS3D");
        a.setApplicationVersion(QString(QUOTE(IVS3D_VER)));
        parser.process(a);
        a.installTranslator(translator);

#if defined(Q_OS_WIN)
        QDir appDir(QCoreApplication::applicationDirPath());
        QCoreApplication::addLibraryPath(appDir.absolutePath());

        QStringList dirs = {"bin", "qml", "plugins" };
        for(auto dir : dirs){
            QDir d(QCoreApplication::applicationDirPath());
            if(d.cd(dir)) QCoreApplication::addLibraryPath(d.absolutePath());
            else qDebug() << "Missing library directory: " << dir;
        }
        qDebug() << "Library search paths: " << QCoreApplication::libraryPaths();
#endif

        qApp->setProperty(stringContainer::UIIdentifier, false);
        noUIController* noUI = new noUIController(parser.value(inputPath), parser.value(autoPath), parser.value(outputPath));
        QTimer::singleShot(0, noUI, SLOT(exec()));
        return a.exec();
    }

}
