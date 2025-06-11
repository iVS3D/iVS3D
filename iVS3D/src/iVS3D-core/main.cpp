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
#include <NeuralUtil.h>

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

int nn_playground() {
    // ############# SOME TESTS FOR iVS3D-NeuralNet #########################

    auto netResult = NN::NeuralNetFactory::create("/home/dom/Documents/IOSB/intelligent-video-sampler-3d/iVS3D/iVS3D/src/iVS3D-models/neural_network_models/Segmentation_ConvNeXt-tiny_Aerial_512x512.onnx", false);
    if(!netResult) {
        std::cout << "Failed to load the ONNX model: " << netResult.error();
        return 1;
    }
    NN::NeuralNetPtr net = *netResult;
    SHAPE_DEBUG_PRINT(net->inputShape())
    SHAPE_DEBUG_PRINT(net->outputShape())


    cv::Mat img = cv::imread("/home/dom/Documents/IOSB/cuda-nn/cityscapes.jpeg");
    //cv::Mat input = cv::dnn::blobFromImage(img, 1.0f, cv::Size(512, 512), cv::Scalar(0, 0, 0), true, false);
    //std::cout << "Blob shape: " << input.size << std::endl;

    cv::Mat blob;
    {
        double scaleFactor = 1.0/255;
        cv::Scalar mean = cv::Scalar(0.485, 0.456, 0.406);
        bool swapRB = true;
        bool crop = false;
        //cv::dnn::blobFromImage(img, blob, scaleFactor, cv::Size(1024,net->inputShape()[3]), mean, swapRB, crop, CV_32F);
        cv::dnn::blobFromImage(img, blob);
        std::cout << "Blob shape: " << blob.size << std::endl;
    }

    cv::Mat square;
    //cv::resize(img, square, cv::Size(net->inputShape()[2],net->inputShape()[3]));


    
    //square = img-cv::Scalar(0.485, 0.456, 0.406); // Subtract mean values
    //cv::cvtColor(square, square, cv::COLOR_BGR2RGB); // Convert BGR to RGB if needed

    img.convertTo(square, CV_32FC3);
    
    auto in_tensor = NN::Tensor::fromCvMat(square);
    if(!in_tensor) {
        std::cout <<"Failed to create input " << in_tensor.error();
        return 1;
    }
    std::cout << "Input tensor: ";
    TENSOR_DEBUG_PRINT(*in_tensor)

    uint64_t blob_size = blob.size[0] * blob.size[1] * blob.size[2] * blob.size[3];
    if(in_tensor->numElements() != blob_size) {
        std::cout << "Input tensor size does not match blob size: " << in_tensor->numElements() << " vs " << blob_size << std::endl;
        return 1;
    }
    std::vector<float> in_data = in_tensor->toVector<float>().value();
    
    std::vector<float> blob_data(blob.begin<float>(), blob.end<float>());
    if(in_data.size() != blob_data.size()) {
        std::cout << "Input tensor data size does not match blob data size: " << in_data.size() << " vs " << blob_data.size() << std::endl;
        return 1;
    }
    for(size_t i = 0; i < in_data.size(); ++i) {
        if(std::abs(in_data[i] - blob_data[i]) > 1e-6) {
            std::cout << "Input tensor data does not match blob data at index " << i << ": " << in_data[i] << " vs " << blob_data[i] << std::endl;
            return 1;
        }
    }
    std::cout << "Input tensor data matches blob data." << std::endl;
    



    std::cout << "Running inference..." << std::endl;
    auto out_tensor = net->infer(*in_tensor);
    if(!out_tensor) {
        std::cout << "Failed at inference " << out_tensor.error();
        return 1;
    }
    std::cout << "Output tensor: ";
    TENSOR_DEBUG_PRINT(*out_tensor)

    auto out_mat = out_tensor->toCvMat();
    if(!out_mat) {
        std::cout << "Failed to convert output tensor to cv::Mat: " << out_mat.error();
        return 1;
    }

    std::string typestr;
    switch (out_mat.value().depth())
    {
    case CV_32F:
        typestr = "CV_32F";
        break;
    case CV_8U:
        typestr = "CV_8U";
        break;
    
    default:
        typestr = "Unknown type (" + std::to_string(out_mat.value().depth()) + ")";
        break;
    }
    std::cout << "Output cv::Mat(size=" << out_mat.value().size() << ", channels=" << out_mat.value().channels() << ", type=" << typestr << ")" << std::endl;

    auto min_tensor = out_tensor.value().reduce(NN::ReduceMin{}, 1);
    if(!min_tensor) {
        std::cout << "Failed to reduce tensor: " << min_tensor.error();
        return 1;
    }
    std::cout << "Min tensor: ";
    TENSOR_DEBUG_PRINT(*min_tensor)

    auto argmax_tensor = out_tensor.value().reduceWithIndex(NN::ReduceArgMax{}, 1);
    if(!argmax_tensor) {
        std::cout << "Failed to reduce tensor: " << argmax_tensor.error();
        return 1;
    }
    std::cout << "Argmax tensor: ";
    TENSOR_DEBUG_PRINT(*argmax_tensor)

    RETURN_ON_ERROR( argmax_tensor.value().squeeze(), 1 )
    std::cout << "Argmax tensor after squeeze: ";
    TENSOR_DEBUG_PRINT(*argmax_tensor)

    auto colorized_tensor_r = argmax_tensor.value().map([](const int64_t& value) -> float {
        // Simple colorization: map values to grayscale
        return static_cast<float>(value);
    });
    if(!colorized_tensor_r) {
        std::cout << "Failed to colorize tensor: " << colorized_tensor_r.error();
        return 1;
    }
    std::cout << "Colorized tensor (red_channel): ";
    TENSOR_DEBUG_PRINT(*colorized_tensor_r)

    auto colorized_tensor = argmax_tensor.value().map([](const int64_t& value) -> std::array<uint8_t,3> {
        // Simple colorization: map values to grayscale
        return {0, static_cast<uint8_t>(value), 0};
    }, 2);
    if(!colorized_tensor) {
        std::cout << "Failed to colorize tensor: " << colorized_tensor.error();
        return 1;
    }
    std::cout << "Colorized tensor: ";
    TENSOR_DEBUG_PRINT(*colorized_tensor)

    {
        std::cout << std::endl << std::endl << "Tensor::map example"<< std::endl;
        NN::Tensor classes = std::move(argmax_tensor.value());

        std::cout << classes.toString() << std::endl;

        std::vector<std::array<uint8_t, 3>> colors = {
            {255,0,0}, {0,255,0}, {0,0,255}, {127,127,0}, {127,0,127}, {0,127,127}
        };
        auto result = classes.map([colors](int64_t index){
            return colors[index];
        }, 0);

        if(!result) {
            std::cerr << "ERROR: " << result.error() << std::endl;
            return -1;
        }
        NN::Tensor colorized = std::move(result.value());
        std::cout << colorized.toString() << std::endl;
    }

    {
        std::cout << std::endl << std::endl << "Tensor::reduceWithIndex example"<< std::endl;
        NN::Tensor scores = std::move(out_tensor.value());

        std::cout << scores.toString() << std::endl;
        
        auto result = scores.reduceWithIndex(NN::ReduceArgMax{}, 1);
        if(!result) {
            std::cerr << "ERROR: " << result.error() << std::endl;
            return -1;
        }
        NN::Tensor reduced = std::move(result.value());
        std::cout << reduced.toString() << std::endl;
    }
    std::cout << std::endl << std::endl;

    // #################### ROUND 2 of testing ##################################

    std::vector<float> data3d = {0.0, 1.0, 2.0, 3.0, 4.0, 5.0,
                                 6.0, 7.0, 8.0, 9.0, 10.0, 11.0};
    NN::Shape shape3d = {2, 3, 2}; // 2x3x2 tensor (CHW format)
    auto tensor3d = NN::Tensor::fromData(std::move(data3d), shape3d);
    if(!tensor3d) {
        std::cout << "Failed to create 3D tensor: " << tensor3d.error();
        return 1;
    }
    std::cout << "3D Tensor: ";
    TENSOR_DEBUG_PRINT(*tensor3d)

    auto reduced_tensor3d = tensor3d.value().reduceWithIndex(NN::ReduceArgMax{}, 1);
    if(!reduced_tensor3d) {
        std::cout << "Failed to reduce 3D tensor: " << reduced_tensor3d.error();
        return 1;
    }
    std::cout << "Reduced 3D tensor: ";
    TENSOR_DEBUG_PRINT(*reduced_tensor3d)

    auto squeeze_result = reduced_tensor3d.value().squeeze();
    if(!squeeze_result) {
        std::cout << "Failed to squeeze 3D tensor: " << squeeze_result.error();
        return 1;
    }
    std::cout << "Reduced 3D tensor after squeeze: ";
    TENSOR_DEBUG_PRINT(*reduced_tensor3d)

    auto values = reduced_tensor3d.value().toVector<int64_t>();
    if(!values) {
        std::cout << "Failed to convert 2D tensor to vector: " << values.error();
        return 1;
    }
    std::cout << "2D tensor values: ";
    for(const auto& v : values.value()) {
        std::cout << v << " ";
    }
    std::cout << std::endl;

    auto color_tensor = reduced_tensor3d.value().map([](const int64_t& value) -> std::array<uint8_t, 3> {
        // Simple colorization: map values to grayscale
        return std::array<uint8_t, 3>{static_cast<uint8_t>(value), static_cast<uint8_t>(value * 2), static_cast<uint8_t>(value * 3)};
    }, 0);

    if(!color_tensor) {
        std::cout << "Failed to colorize 3D tensor: " << color_tensor.error();
        return 1;
    }
    std::cout << "Colorized 3D tensor: ";
    TENSOR_DEBUG_PRINT(*color_tensor)

    auto colors = color_tensor.value().toVector<uint8_t>();
    if(!colors) {
        std::cout << "Failed to convert 2D tensor to vector: " << colors.error();
        return 1;
    }
    std::cout << colors.value().size() << " color values in 3D tensor." << std::endl;
    std::cout << "3D color tensor values: ";
    for(const auto& v : colors.value()) {
        std::cout << +v << " ";
    }
    std::cout << std::endl;

    auto resultStack = NN::Tensor::fromCvMat(square)
        .and_then(NN::Util::bind_inference(net))
        .and_then(NN::Util::bind_reduce(NN::ReduceSum{}, 1))
        .and_then(NN::Util::bind_squeeze())
        .and_then(NN::Util::bind_toCvMat());

    if(!resultStack) {
        std::cout << "Failed to process tensor: " << resultStack.error();
        return 1;
    }
    std::cout << "Resulting cv::Mat from tensor stack: " << resultStack.value().size() << std::endl;
    return 0;
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

    // TODO: Remove this after testing
    nn_playground();
    
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
