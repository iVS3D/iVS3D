#include "stationarycamera.h"

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

    // ----------- setup and creating hardware specific elements ----------------
    Factory *fac = new Factory(imageList, reader, m_downSampleFactor, useCuda);
    FlowCalculator *flowCalculator = fac->getFlowCalculator();
    ImageGatherer *imageGatherer = fac->getImageGatherer();

    std::vector<double> flowValues;
    auto fromIter = imageList.begin();
    auto toIter = std::next(imageList.begin(), 1);
    std::future<void> flowCalcHandler;
    std::future<QPair<cv::Mat, cv::Mat>> imageGatherHandler;

    // ----------- algorithms definition ------------
    // gather image pair
    std::function<QPair<cv::Mat, cv::Mat>(uint, uint)> gatherImagePairStatic = [imageGatherer](uint fromIdx, uint toIdx) {
        auto startGather = std::chrono::high_resolution_clock::now();
        QPair<cv::Mat, cv::Mat> matPair = imageGatherer->gatherImagePair(fromIdx, toIdx);
        auto endGather = std::chrono::high_resolution_clock::now();
        long gatherDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endGather - startGather).count();
        qDebug() << "gatherDuration=" << gatherDuration << "ms";
        return matPair;
    };

    // flow calculation
    std::function<void(cv::Mat, cv::Mat)> calcFlowStatic = [flowCalculator, &flowValues](cv::Mat fromMat, cv::Mat toMat) {
        auto startFlow = std::chrono::high_resolution_clock::now();
        double flowValue = flowCalculator->calculateFlow(fromMat, toMat);
        flowValues.push_back(flowValue);
        auto endFlow = std::chrono::high_resolution_clock::now();
        long flowDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endFlow - startFlow).count();
        qDebug() << "flowDuration=" << flowDuration << "ms\tvalue=" << flowValue;
    };

    // -------------- iterate through all available frame pairs ---------------
    m_logFile->startTimer("Core Computation");
    while (toIter < imageList.end()) {
        // Aborts calculations as result of user interaction
        if (*stopped) {
            qDebug() << "Execution was stopped.";
            return {};
        }

        // ----------- exectution ------------
        imageGatherHandler = std::async(std::launch::async, gatherImagePairStatic, *fromIter, *toIter);
        QPair<cv::Mat, cv::Mat> matPair = imageGatherHandler.get();
        flowCalcHandler = std::async(std::launch::async, calcFlowStatic, matPair.first, matPair.second);

        // -------- progress and debug ------------
        int progress = ((toIter - imageList.begin()) * 100) / imageList.size();
        QString currOp = "Calculating flow between frame " + QString::number(*fromIter) + " and "+ QString::number(*toIter);
        reportProgress(currOp, progress, receiver);
        // ----------------------------------
        fromIter = std::next(fromIter, 1);
        toIter = std::next(toIter, 1);
    }
    flowCalcHandler.wait();
    m_logFile->stopTimer();

    // ------------ select keyframes ----------------
    m_logFile->startTimer("Keyframe selection");
    std::vector<uint> selectedKeyframes = { imageList[0] };
    std::vector<double> copiedFlowValues = flowValues; // median is in place and reorders vector
    double medianFlow = median(copiedFlowValues);
    double allowedDiffFlow = medianFlow * m_threshold;
    for (uint flowValuesIdx = 0; flowValuesIdx < imageList.size() - 1; flowValuesIdx++) {
        // -------- reporting progress ---------
        QString currentOp = "Checking if frame " + QString::number(imageList[flowValuesIdx])+ "is a keyframe.";
        int progress = (flowValuesIdx * 100) / imageList.size();
        reportProgress(currentOp, progress, receiver);
        // -------------------------------------
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

void StationaryCamera::reportProgress(QString op, int progress, Progressable *receiver)
{
    QMetaObject::invokeMethod(
                receiver,
                "slot_makeProgress",
                Qt::DirectConnection,
                Q_ARG(int, progress),
                Q_ARG(QString, op));
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
