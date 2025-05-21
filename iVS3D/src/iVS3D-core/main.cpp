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

#include <NeuralNetFactory.h>
#include <opencv2/dnn.hpp>

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
    qRegisterMetaType<Resolution>("Resolution");
    qRegisterMetaType<ROI>("ROI");


    // ############# SOME TESTS FOR iVS3D-NeuralNet #########################

    auto netResult = NN::NeuralNetFactory::create("/path/to/neural_network_models/Segmentation_ConvNeXt-tiny_Aerial_512x512.onnx", true);
    if(!netResult) {
        std::cout << "Failed to load the ONNX model: " << netResult.error();
        return 1;
    }
    NN::NeuralNetPtr net = *netResult;
    SHAPE_DEBUG_PRINT(net->inputShape())
    SHAPE_DEBUG_PRINT(net->outputShape())


    cv::Mat img = cv::imread("/home/dom/Videos/export158/images/00000000.png");
    //cv::Mat input = cv::dnn::blobFromImage(img, 1.0f, cv::Size(512, 512), cv::Scalar(0, 0, 0), true, false);
    //std::cout << "Blob shape: " << input.size << std::endl;

    cv::Mat square;
    cv::resize(img, square, cv::Size(net->inputShape()[2],net->inputShape()[3]));

    square.convertTo(square, CV_32FC3);
    auto in_tensor = NN::Tensor::fromCvMat(square);
    if(!in_tensor) {
        std::cout <<"Failed to create input " << in_tensor.error();
        return 1;
    }
    std::cout << "Created input tensor: ";
    TENSOR_DEBUG_PRINT(*in_tensor)

    std::cout << "Running inference..." << std::endl;
    auto out_tensor = net->infer(*in_tensor);
    if(!out_tensor) {
        std::cout << "Failed at inference " << out_tensor.error();
        return 1;
    }
    TENSOR_DEBUG_PRINT(*out_tensor)

    // #################### END of testing ##################################

    QCommandLineParser parser;
    QCommandLineOption noGUI("nogui", "Executes in terminal mode without the GUI. In this case auto settings file (-a), input (-i) and output (-o) need to be provided.");
    QCommandLineOption inputPath(QStringList() << "i" << "in", "Load input from <path>.", "path");
    QCommandLineOption autoPath(QStringList() << "a" << "auto", "Load settings from <path>.", "path");
    QCommandLineOption outputPath(QStringList() << "o" << "out", "Save result to <path>.", "path");
    QCommandLineOption logPath(QStringList() << "l" << "log", "Log resulsts and process information to <path>.", "path");

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
    parser.addOption(logPath);

    QStringList arguments;
    for(int i = 0; i < argc; ++i){
        arguments << argv[i];
    }

    if(!arguments.contains("--nogui")){

        #if defined(Q_OS_WIN)
            FreeConsole();
        #endif

        QApplication a( argc, argv );
        a.setApplicationName("iVS3D");
        a.setApplicationVersion(QString(QUOTE(IVS3D_VER)));
        parser.process(a);
        qApp->setProperty(stringContainer::UIIdentifier, true);
        qApp->setProperty("translation", ApplicationSettings::instance().getLocale());
        qDebug() << "Locale: " << qApp->property("translation").toLocale();
        QTranslator* translator = new QTranslator();
        translator->load(qApp->property("translation").toLocale(), "core", "_", ":/translations", ".qm");
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
        Controller *mainController = new Controller(parser.value(inputPath), parser.value(autoPath), parser.value(outputPath), parser.value(logPath));

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
        qApp->setProperty(stringContainer::UIIdentifier, false);
        qApp->setProperty("translation", ApplicationSettings::instance().getLocale());
        qDebug() << "Locale: " << qApp->property("translation").toLocale();
        QTranslator* translator = new QTranslator();
        translator->load(qApp->property("translation").toLocale(), "core", "_", ":/translations", ".qm");
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

        noUIController* noUI = new noUIController(parser.value(inputPath), parser.value(autoPath), parser.value(outputPath), parser.value(logPath));
        QTimer::singleShot(0, noUI, SLOT(exec()));
        return a.exec();
    }

}
