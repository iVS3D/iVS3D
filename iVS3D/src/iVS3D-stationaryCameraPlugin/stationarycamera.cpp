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

    auto start = std::chrono::high_resolution_clock::now();
    m_logFile->startTimer(LF_BUFFER);
    recreateBuffer(buffer);
    m_logFile->stopTimer();
    auto endBuffer = std::chrono::high_resolution_clock::now();

    // calculate flow values
    //      used to calculate median of flow values
    std::vector<double> computedFlowValues;
    //      used for fast selection of stationary frames
    QMap<uint, double> flowWithIndex;
    FarnebackOptFlow *farn = FarnebackOptFlowFactory::create(useCuda ? FARNEBACK_CUDA : FARNEBACK_CPU);
    farn->setup();

    auto endSetup = std::chrono::high_resolution_clock::now();

    std::cout << "\n\n<-------- COMPUTING MOVEMENT [duration in ms] ------->\n";
    std::cout << std::left << std::setw(10) << "idx curr" << std::setw(10) << "idx prev" << std::setw(10) << "load" << std::setw(10) << "farneback" << std::setw(10) << "median" << std::setw(10) << "movement" << "\n";
    long loadDurationMs = 0;

    // TODO Multithreading with qthreads
    cv::Mat lastGreyImage;
    bool lastImageValid = false; // shows that the last grey image wasnt gathered yet (cause could be that the last value was found in buffer)
    m_logFile->startTimer(LF_OPT_FLOW_TOTAL);
    for (uint imageListIdx = 1; imageListIdx < imageList.size(); imageListIdx++) {
        // send new progress update
        int progress = imageListIdx * 100 / (int)imageList.size();
        QString currentProgress = "calculate flow between the frames " + QString::number(imageList[imageListIdx - 1]) + " and " + QString::number(imageList[imageListIdx]);
        QMetaObject::invokeMethod(
                    receiver,
                    "slot_makeProgress",
                    Qt::DirectConnection,
                    Q_ARG(int, progress),
                    Q_ARG(QString, currentProgress));
        double computedFlow = 0.f;

        if (false) {
            // TODO check for buffered elements

            // ONLY for for timing
            m_durationComputationFlowMs = 0;
            m_durationFarnebackMs = 0;
        } else {
            auto startLoad =std::chrono::high_resolution_clock::now();
            if (!lastImageValid) {
                // check if a last grey image is available and if not gather it from the reader
                cv::Mat lastImage = reader->getPic(imageList[imageListIdx - 1]);
                cv::cvtColor(lastImage, lastGreyImage, cv::COLOR_BGR2GRAY);
            }
            cv::Mat currImage, currGreyImage;
            currImage = reader->getPic(imageList[imageListIdx]);
            cv::cvtColor(currImage, currGreyImage, cv::COLOR_BGR2GRAY);

            auto endLoad = std::chrono::high_resolution_clock::now();
            loadDurationMs = std::chrono::duration_cast<std::chrono::milliseconds>(endLoad - startLoad).count();
            try {
                computedFlow = computeFlow(lastGreyImage, currGreyImage, farn);
            }  catch (cv::Exception &cvExcep) {
                std::cout << cvExcep.msg << std::endl;
            }
            lastGreyImage = currGreyImage;
            lastImageValid = true;
        }
        m_logFile->addCustomEntry("ComputedFlow", computedFlow);
        computedFlowValues.push_back(computedFlow);
        flowWithIndex[imageList[imageListIdx]] = computedFlow;

        // check fo stop
        if (*stopped) {
            break;
        }

        // TODO safe flow values in buffer

        // display computation timings
        std::cout << std::left <<
                     std::setw(10) << imageList[imageListIdx - 1] <<
                     std::setw(10) << imageList[imageListIdx] <<
                     std::setw(10) << loadDurationMs <<
                     std::setw(10) << m_durationFarnebackMs <<
                     std::setw(10) << m_durationComputationFlowMs << std::setw(10) <<
                     computedFlow << std::endl;
    }
    m_logFile->stopTimer();

    auto endFlowCalc = std::chrono::high_resolution_clock::now();

    // select all keyframes and than remove all stationary frames
    m_logFile->startTimer(LF_SELECT_FRAMES);
    double medianFlow = median(computedFlowValues);
    std::vector<uint> selectedKeyframes;
    const double stationaryThreshold = medianFlow * m_threshold;
    for (uint frameIdx : imageList) {
        if (flowWithIndex[frameIdx] >= stationaryThreshold) {
            selectedKeyframes.push_back(frameIdx);
        }
    }
    m_logFile->stopTimer();

    auto endMedian = std::chrono::high_resolution_clock::now();

    // timing summary output
    auto durationBufferMs = std::chrono::duration_cast<std::chrono::microseconds>(endBuffer - start).count();
    auto durationSetupMs = std::chrono::duration_cast<std::chrono::microseconds>(endSetup - endBuffer).count();
    auto durationFlowCalcMs = std::chrono::duration_cast<std::chrono::milliseconds>(endFlowCalc - endSetup).count();
    auto durationMedianUs =std::chrono::duration_cast<std::chrono::microseconds>(endMedian - endFlowCalc).count();
    std::cout << "\n\n<-------- STATIONARY SUMMARY ----------------------->\n";
    std::cout << "buffer = " << durationBufferMs << "us" << std::endl;
    std::cout << "setup = " << durationSetupMs << "us" << std::endl;
    std::cout << "flow calcs = " << durationFlowCalcMs << "ms" << std::endl;
    std::cout << "median = " << durationMedianUs << "us" << std::endl;
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
    auto startFarneback = std::chrono::high_resolution_clock::now();

    cv::Mat flowMat(image1.size(), CV_32FC2);
    farn->calculateFlow(image1, image2, flowMat, m_downSampleFactor);

    auto endFarneback = std::chrono::high_resolution_clock::now();

    // compute flow matrix to single value (median length of all flow vectors)
    std::vector<double> flowLengths;
    for (uint x = 0; x < (uint)flowMat.rows; x++) {
        for (uint y = 0; y < (uint)flowMat.cols; y++) {
            cv::Point2f flowVector = flowMat.at<cv::Point2f>(x, y);
            const double length = cv::sqrt(flowVector.ddot(flowVector));
            flowLengths.push_back(length);
        }
    }

    auto endFlowComputation = std::chrono::high_resolution_clock::now();
    m_durationFarnebackMs = std::chrono::duration_cast<std::chrono::milliseconds>(endFarneback - startFarneback).count();
    m_durationComputationFlowMs = std::chrono::duration_cast<std::chrono::milliseconds>(endFlowComputation - endFarneback).count();

    return median(flowLengths);
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
