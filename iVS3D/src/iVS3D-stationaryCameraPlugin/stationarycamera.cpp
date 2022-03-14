#include "stationarycamera.h"

// Temporary
//#include <opencv2/cudaoptflow.hpp>
//#include <opencv2/cudaarithm.hpp>
//#include <opencv2/cudawarping.hpp>
//#include <opencv2/cudaimgproc.hpp>
#include <QThreadPool>
//

StationaryCamera::StationaryCamera()
{
}

QWidget *StationaryCamera::getSettingsWidget(QWidget *parent)
{
    if (!m_settingsWidget) {
        createSettingsWidget(parent);
    }
    return m_settingsWidget;
}

std::vector<uint> StationaryCamera::sampleImages(Reader *reader, const std::vector<unsigned int> &imageList, Progressable *receiver, volatile bool *stopped, QMap<QString, QVariant> buffer, bool useCuda, LogFileParent *logFile)
{
    m_logFile = logFile;

    m_logFile->startTimer(LF_BUFFER);
    recreateBuffer(buffer);
    m_logFile->stopTimer();

    double reciprocalFactor = 1.0 / m_downSampleFactor;
    // cpu read version
    std::function<cv::Mat(const uint)> func_gatherPics_Cpu =
            [reader, reciprocalFactor, receiver](uint idx) {
        cv::Mat readMat, downMat, greyMat;
        while (readMat.empty()) {
            readMat = reader->getPic(idx, true);
        }
        cv::resize(readMat, downMat, cv::Size(), reciprocalFactor, reciprocalFactor);
        cv::cvtColor(downMat, greyMat, cv::COLOR_BGR2GRAY);
        // notify for progress update
        int progress = 0;
        QString currentOperation = "gathering, resizing and coloring image " + QString::number(idx);
        QMetaObject::invokeMethod(
                    receiver,
                    "slot_makeProgress",
                    Qt::DirectConnection,
                    Q_ARG(int, progress),
                    Q_ARG(QString, currentOperation));
        return greyMat;
    };
    // cuda read version
    std::function<cv::Mat(const uint)> func_gatherPics_Cuda;
#ifdef WITH_CUDA
    func_gatherPics_Cuda =
            [reader, reciprocalFactor, receiver](uint idx) {
        cv::Mat readMat;
        while (readMat.empty()) {
            readMat = reader->getPic(idx, true);
        }
        cv::cuda::GpuMat gpu_downMat, gpu_greyMat, gpu_readMat(readMat);
        cv::cuda::resize(gpu_readMat, gpu_downMat, cv::Size(), reciprocalFactor, reciprocalFactor);
        cv::cuda::cvtColor(gpu_downMat, gpu_greyMat, cv::COLOR_BGR2GRAY);
        cv::Mat outMat;
        gpu_greyMat.download(outMat);
        gpu_downMat.release();
        gpu_greyMat.release();
        gpu_greyMat.release();
        // notify for progress update
        int progress = 0;
        QString currentOperation = "gathering, resizing and coloring image " + QString::number(idx);
        QMetaObject::invokeMethod(
                    receiver,
                    "slot_makeProgress",
                    Qt::DirectConnection,
                    Q_ARG(int, progress),
                    Q_ARG(QString, currentOperation));
        return outMat;
    };
#endif
    std::function<cv::Mat(const uint)> func_gatherPics = func_gatherPics_Cpu;
    if (useCuda && func_gatherPics_Cuda) {
        func_gatherPics = func_gatherPics_Cuda;
    } else if (func_gatherPics_Cpu) {
        func_gatherPics = func_gatherPics_Cpu;
    } else {
        return {};
    }
    struct FLOW_PAIR {
        uint fromIndex;
        uint toIndex;
        cv::Mat fromMat;
        cv::Mat toMat;
    };

    FarnebackOptFlow *farn = FarnebackOptFlowFactory::create(useCuda ? FARNEBACK_CUDA : FARNEBACK_CPU);
    farn->setup();
    std::function<double(const FLOW_PAIR)> func_farn =
            [this, farn, receiver](FLOW_PAIR fp) {
        QString currentOperation = "computing flow between " + QString::number(fp.fromIndex) + " and " + QString::number(fp.toIndex);
        int progress = 0;
        QMetaObject::invokeMethod(
                    receiver,
                    "slot_makeProgress",
                    Qt::DirectConnection,
                    Q_ARG(int, progress),
                    Q_ARG(QString, currentOperation));
        double flowValue = computeFlow(fp.fromMat, fp.toMat, farn);
        std::cout << std::setw(10) << fp.fromIndex << std::setw(10) << fp.toIndex << std::setw(10) << flowValue << std::endl;
        return flowValue;
    };
    // ------------------------
    uint chunkCount = 10;
    uint chunkSize = imageList.size() / chunkCount;
    std::vector<double> flowValues;
    for (uint chunkIndex = 0; chunkIndex < chunkCount; chunkIndex++) {

        // gather all images of the chunk
        m_logFile->startTimer("Gather images of chunk " + QString::number(chunkIndex));
        auto sequenceBegin = imageList.begin();
        std::advance(sequenceBegin, chunkSize * chunkIndex);
        auto sequenceEnd = imageList.begin();
        if (chunkSize * (chunkIndex + 1) < (uint)imageList.size())
            std::advance(sequenceEnd, chunkSize * (chunkIndex + 1));
        else
            sequenceEnd = imageList.end();
        std::vector<uint> chunkIndicies(sequenceBegin, sequenceEnd);
        reader->initMultipleAccess(chunkIndicies);
        std::vector<cv::Mat> imageMats = QtConcurrent::blockingMapped<std::vector<cv::Mat>>(sequenceBegin, sequenceEnd, func_gatherPics);
        m_logFile->stopTimer();

        // create FLOW_PAIRs for flow calculation of the current chunk
        m_logFile->startTimer("Create flow pair of chunk " + QString::number(chunkIndex));
        std::vector<FLOW_PAIR> flowPair;
        uint chunkOffset = chunkIndex * chunkSize;
        for (uint i = 0; i < chunkSize - 1; i++) {
            FLOW_PAIR nFlowPair;
            nFlowPair.fromIndex = imageList[chunkOffset + i];
            nFlowPair.toIndex = imageList[chunkOffset + i + 1];
            nFlowPair.fromMat = imageMats[i];
            nFlowPair.toMat = imageMats[i + 1];
            flowPair.push_back(nFlowPair);
        }
        m_logFile->stopTimer();

        QThreadPool::globalInstance()->setMaxThreadCount(1);
        // start flow calculation for chunk SEQUENTIAL
        m_logFile->startTimer("Calculate flow values of chunk " + QString::number(chunkIndex));
        std::vector<double> chunkFlowValues = QtConcurrent::blockingMapped<std::vector<double>>(flowPair, func_farn);
        flowValues.insert(flowValues.end(), chunkFlowValues.begin(), chunkFlowValues.end());
        m_logFile->stopTimer();
        QThreadPool::globalInstance()->setMaxThreadCount(16);
    };

    // select keyframes
    m_logFile->startTimer("Keyframe selection");
    std::vector<uint> selectedKeyframes = { imageList[0] };
    std::vector<double> copiedFlowValues = flowValues; // median is in place and reorders vector
    double medianFlow = median(copiedFlowValues);
    double allowedDiffFlow = medianFlow * m_threshold;
    for (uint flowValuesIdx = 0; flowValuesIdx < imageList.size() - 1; flowValuesIdx++) {
        if (flowValues[flowValuesIdx] > allowedDiffFlow) {
            selectedKeyframes.push_back(imageList[flowValuesIdx + 1]); // flow value represents flow for the next frame (if camera moved enough until next frame)
        }
    }
    m_logFile->stopTimer();

    return selectedKeyframes;
}

QString StationaryCamera::getName() const
{
    return PLUGIN_NAME;
}

QVariant StationaryCamera::getBuffer()
{
    // TODO
    return "buffer placeholder";
}

QString StationaryCamera::getBufferName()
{
    return BUFFER_NAME;
}

void StationaryCamera::initialize(Reader *reader)
{
    m_reader = reader;
    m_threshold = 0.1;
    m_downSampleFactor = 1.0;
    cv::Mat testPic = reader->getPic(0);
    m_inputResolution.setX(testPic.rows);
    m_inputResolution.setY(testPic.cols);
}

void StationaryCamera::setSettings(QMap<QString, QVariant> settings)
{
    m_threshold = settings.find(SETTINGS_THRESHOLD).value().toDouble();
    if (m_settingsWidget) {
        m_thresholdSpinBox->setValue(m_threshold * 100.0f);
    }
}

QMap<QString, QVariant> StationaryCamera::generateSettings(Progressable *receiver, QMap<QString, QVariant> buffer, bool useCuda, volatile bool *stopped)
{
    // TODO
    (void) receiver;
    (void) buffer;
    (void) useCuda;
    (void) stopped;
    return getSettings();
}

QMap<QString, QVariant> StationaryCamera::getSettings()
{
    QMap<QString, QVariant> settings;
    settings.insert(SETTINGS_THRESHOLD, m_threshold);
    settings.insert(SETTINGS_DOWNSAMPLE, m_downSampleFactor);
    return settings;
}

void StationaryCamera::createSettingsWidget(QWidget *parent)
{
    // create threshold layout with spinBox and label
    QWidget *thresholdLayout = new QWidget(parent);
    thresholdLayout->setLayout(new QHBoxLayout(parent));
    thresholdLayout->layout()->addWidget(new QLabel(THRESHOLD_LABEL_TEXT));
    thresholdLayout->layout()->setMargin(0);
    thresholdLayout->layout()->setSpacing(0);
    if (m_thresholdSpinBox) {
        delete m_thresholdSpinBox;
    }
    m_thresholdSpinBox = new QDoubleSpinBox(parent);
    m_thresholdSpinBox->setMinimum(0.0f);
    m_thresholdSpinBox->setMaximum(100.0f);
    m_thresholdSpinBox->setValue(m_threshold * 100);
    m_thresholdSpinBox->setAlignment(Qt::AlignRight);
    m_thresholdSpinBox->setSuffix("%");
    QObject::connect(m_thresholdSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
                     [=](double v) {
                        m_threshold = v / 100.f;
                     });
    thresholdLayout->layout()->addWidget(m_thresholdSpinBox);

    // threshold description
    QLabel *thresholdLable = new QLabel(DESCRIPTION_THRESHOLD);
    thresholdLable->setStyleSheet(DESCRIPTION_STYLE);
    thresholdLable->setWordWrap(true);

    // create downSample layout with spinBox and lable
    QWidget *downSampleLayout = new QWidget(parent);
    downSampleLayout->setLayout(new QHBoxLayout(parent));
    downSampleLayout->layout()->addWidget(new QLabel(DOWNSAMPLE_LABEL_TEXT));
    downSampleLayout->layout()->setMargin(0);
    downSampleLayout->layout()->setSpacing(0);
    if (m_downSampleSpinBox) {
        delete m_downSampleSpinBox;
    }
    m_downSampleSpinBox = new QDoubleSpinBox(parent);
    m_downSampleSpinBox->setMinimum(1.0);
    m_downSampleSpinBox->setMaximum(100.0);
    m_downSampleSpinBox->setSingleStep(0.1);
    m_downSampleSpinBox->setValue(m_downSampleFactor);
    m_downSampleSpinBox->setAlignment(Qt::AlignRight);
    QObject::connect(m_downSampleSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
                     [=](double v) {
                        m_downSampleFactor = v;
                        updateInfoLabel();
                     });
    downSampleLayout->layout()->addWidget(m_downSampleSpinBox);

    // downSample description
    QLabel *downSampleLable = new QLabel(DESCRIPTION_DOWNSAMPLE);
    downSampleLable->setStyleSheet(DESCRIPTION_STYLE);
    downSampleLable->setWordWrap(true);

    // info label
    m_infoLabel = new QLabel();
    m_infoLabel->setStyleSheet(INFO_STYLE);
    m_infoLabel->setWordWrap(true);
    updateInfoLabel();

    // create main widget
    m_settingsWidget = new QWidget(parent);
    m_settingsWidget->setLayout(new QVBoxLayout(parent));
    m_settingsWidget->layout()->setSpacing(0);
    m_settingsWidget->layout()->setMargin(0);
    // add elements
    m_settingsWidget->layout()->addWidget(thresholdLayout);
    m_settingsWidget->layout()->addWidget(thresholdLable);
    m_settingsWidget->layout()->addWidget(downSampleLayout);
    m_settingsWidget->layout()->addWidget(downSampleLable);
    m_settingsWidget->layout()->addWidget(m_infoLabel);

    m_settingsWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    m_settingsWidget->adjustSize();
}

void StationaryCamera::updateInfoLabel()
{
    double reciprocalFactor = 1.0 / m_downSampleFactor;
    int resizedWidth = round((float)m_inputResolution.x() * reciprocalFactor);
    int resizedHeight = round((float)m_inputResolution.y() * reciprocalFactor);
    QString nText = INFO_PREFIX + QString::number(resizedHeight) + " x " + QString::number(resizedWidth) + INFO_SUFFIX;
    m_infoLabel->setText(nText);
}

double StationaryCamera::computeFlow(cv::Mat image1, cv::Mat image2, FarnebackOptFlow *farn)
{
    cv::Mat flowMat(image1.size(), CV_32FC2);
    if(!farn->calculateFlow(image1, image2, flowMat, m_downSampleFactor)) {
        return -1;
    }

    // compute flow matrix to single value (median length of all flow vectors)
    std::vector<double> flowLengths;
    for (uint x = 0; x < (uint)flowMat.rows; x++) {
        for (uint y = 0; y < (uint)flowMat.cols; y++) {
            cv::Point2f flowVector = flowMat.at<cv::Point2f>(x, y);
            const double length = cv::norm(flowVector);
            flowLengths.push_back(length);
        }
    }
    double m = median(flowLengths);
    return m;
}

void StationaryCamera::recreateBuffer(QMap<QString, QVariant> buffer)
{
    (void) buffer;
    // TODO
}

double StationaryCamera::median(std::vector<double> &vec)
{
    std::vector<double>::iterator median = vec.begin() + vec.size() / 2;
    std::nth_element(vec.begin(), median, vec.end());
    return vec[vec.size() / 2];
}
