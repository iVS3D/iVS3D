#include "view/mainwindow.h"
#include "controller.h"
#include "nouicontroller.h"
#include "stringcontainer.h"

#include <QApplication>
#include "view/darkstyle/DarkStyle.h"
#include "cvmat_qmetadata.h"

#include <QFlags>

#include <stdio.h>
#include <iostream>

#if defined(Q_OS_WIN)
    #include <Windows.h>
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

    parser.addOption(autoPath);
    parser.addOption(inputPath);
    parser.addOption(outputPath);
    parser.addOption(noGUI);

    parser.addHelpOption();

    QStringList arguments;
    for(int i = 0; i < argc; ++i){
        arguments << argv[i];
    }

    if(!arguments.contains("--nogui")){

        #if defined(Q_OS_WIN)
            FreeConsole();
        #endif

        QApplication a( argc, argv );
        parser.parse(arguments);
        Controller mainController(parser.value(inputPath), parser.value(autoPath), parser.value(outputPath));
        qApp->setProperty(stringContainer::UIIdentifier, true);

        return a.exec();
    }
    else {
        //Disable all Messages comming from Qt
        qInstallMessageHandler(ignoreMessages);
        QCoreApplication a( argc, argv );
        parser.parse(arguments);
        qApp->setProperty(stringContainer::UIIdentifier, false);
        noUIController* noUI = new noUIController(parser.value(inputPath), parser.value(autoPath), parser.value(outputPath));
        QTimer::singleShot(0, noUI, SLOT(exec()));
        return a.exec();
    }

}
