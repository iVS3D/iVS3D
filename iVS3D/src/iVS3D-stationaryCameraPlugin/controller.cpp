#include "controller.h"

Controller::Controller()
{
}

QWidget *Controller::getSettingsWidget(QWidget *parent)
{
    if (!m_settingsWidget) {
        createSettingsWidget(parent);
        // update widget items
        bool boxIsChecked = downInputResToCheck(m_inputResolution);
        m_downSampleCheck->setChecked(boxIsChecked);
        m_downSampleCheck->setEnabled(boxIsChecked);
    }
    return m_settingsWidget;
}

std::vector<uint> Controller::sampleImages(const std::vector<uint> &imageList, Progressable *receiver, volatile bool *stopped, bool useCuda, LogFileParent *logFile)
{
    qDebug() << "Down sampling factor: " << m_downSampleFactor;
    // ----------- setup and creating hardware specific elements ----------------
    Factory *fac = new Factory(imageList, m_reader, m_downSampleFactor, useCuda);
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
        QElapsedTimer timer;
        timer.start();
        QPair<cv::Mat, cv::Mat> matPair = imageGatherer->gatherImagePair(fromIdx, toIdx);
        qDebug() << "gatherDuration=" << timer.elapsed() << "ms";
        return matPair;
    };

    // flow calculation
    const double downSampleFactorConst = m_downSampleFactor;
    std::function<void(cv::Mat, cv::Mat)> calcFlowStatic = [flowCalculator, &flowValues, downSampleFactorConst](cv::Mat fromMat, cv::Mat toMat) {
        QElapsedTimer timer;
        timer.start();
        // muliplication with down sample factor corrects the reduced resolution
        double flowValue = flowCalculator->calculateFlow(fromMat, toMat) * downSampleFactorConst;
        flowValues.push_back(flowValue);
        qDebug() << "flowDuration=" << timer.elapsed() << "ms\tvalue=" << flowValue;
    };

    // -------------- iterate through all available frame pairs ---------------
    uint usedBufferedValues = 0;
    logFile->startTimer(LF_TIMER_CORE);
    while (toIter < imageList.end()) {
        // Aborts calculations as result of user interaction
        if (*stopped) {
            qDebug() << "Execution was stopped.";
            break;
        }

        // ----------- exectution ------------
        double bufferedMovement = 0.0;
        if (m_bufferMat.size() != 0) {
            bufferedMovement = m_bufferMat.ref<double>(*fromIter, *toIter);
        }
        if (bufferedMovement <= 0.0) {
            imageGatherHandler = std::async(std::launch::async, gatherImagePairStatic, *fromIter, *toIter);
            QPair<cv::Mat, cv::Mat> matPair = imageGatherHandler.get();
            flowCalcHandler = std::async(std::launch::async, calcFlowStatic, matPair.first, matPair.second);
            m_bufferedValueCount++;
        } else {
            // use buffered flow value
            flowValues.push_back(bufferedMovement);
            usedBufferedValues++;
        }
        // -------- progress and debug ------------
        int progress = ((toIter - imageList.begin()) * 100) / (int)imageList.size();
        QString currOp = "Calculating flow between frame " + QString::number(*fromIter) + " and "+ QString::number(*toIter);
        reportProgress(currOp, progress, receiver);
        // ----------------------------------------
        fromIter = std::next(fromIter, 1);
        toIter = std::next(toIter, 1);
    }
    if (flowCalcHandler.valid())
        flowCalcHandler.wait();
    logFile->stopTimer();
    logFile->addCustomEntry(LF_CE_VALUE_USED_BUFFERED, usedBufferedValues, LF_CE_TYPE_ADDITIONAL_INFO);

    // ------------ select keyframes ----------------
    if (flowValues.size() > imageList.size() - 1) {
        return {};
    }
    logFile->startTimer(LF_TIMER_SELECTION);
    std::vector<uint> selectedKeyframes = { imageList[0] };
    std::vector<double> copiedFlowValues = flowValues; // median is in place and reorders vector
    double medianFlow = median(copiedFlowValues);
    double allowedDiffFlow = medianFlow * m_threshold;
    for (uint flowValuesIdx = 0; flowValuesIdx < flowValues.size() - 1; flowValuesIdx++) {
        // ----------- selection --------------
        if (flowValues[flowValuesIdx] > allowedDiffFlow) {
            selectedKeyframes.push_back(imageList[flowValuesIdx + 1]); // flow value represents flow for the next frame (if camera moved enough until next frame)
        }
        // -------- reporting progress ---------
        QString currentOp = "Checking if " + QString::number(imageList[flowValuesIdx])+ " is a keyframe.";
        int progress = (flowValuesIdx * 100) / (int)imageList.size();
        reportProgress(currentOp, progress, receiver);
        // -------- update buffer --------------
        if (m_bufferMat.ref<double>(imageList[flowValuesIdx], imageList[flowValuesIdx + 1]) <= 0.0)
            m_bufferMat.ref<double>(imageList[flowValuesIdx], imageList[flowValuesIdx + 1]) = flowValues[flowValuesIdx];
        // DEBUG write flow values in logFile
//        m_logFile->addCustomEntry(LF_CE_NAME_FLOWVALUE, flowValues[flowValuesIdx], LF_CE_TYPE_DEBUG);
    }
    logFile->stopTimer();

    updateBufferInfo(m_bufferedValueCount);
    emit updateBuffer(sendBuffer());

    if (*stopped) {
        // clear keyframes if algorithm was aborted
        selectedKeyframes = {};
    }
    return selectedKeyframes;
}

QString Controller::getName() const
{
    return PLUGIN_NAME;
}

QMap<QString, QVariant> Controller::sendBuffer()
{
    QVariant bufferVariant = bufferMatToVariant(m_bufferMat);
    QMap<QString, QVariant> bufferMap;
    bufferMap.insert(BUFFER_NAME, bufferVariant);
    return bufferMap;
}

void Controller::initialize(Reader *reader, QMap<QString, QVariant> buffer, signalObject *sigObj)
{
    if (m_settingsWidget) {
        m_settingsWidget->deleteLater();
        m_settingsWidget = nullptr;
        m_thresholdSpinBox = nullptr;
        m_downSampleCheck = nullptr;
    }

    recreateBufferMatrix(buffer);
    m_sigObj = sigObj;

    m_reader = reader;
    cv::Mat testPic = reader->getPic(0);
    m_inputResolution.setX(testPic.cols);
    m_inputResolution.setY(testPic.rows);

    int picCount = reader->getPicCount();
    int size[2] = {picCount, picCount};
    m_bufferMat = cv::SparseMat(2, size, CV_32F);
    if (m_resetBufferBt) {
        updateBufferInfo(m_bufferedValueCount);
    }
    bool isChecked = downInputResToCheck(m_inputResolution);
    sampleCheckChanged(isChecked);
}

void Controller::setSettings(QMap<QString, QVariant> settings)
{
    m_threshold = settings.find(SETTINGS_THRESHOLD).value().toDouble();
    bool sampleResActive = settings.find(SETTINGS_SAMPLE_RESOLUTION).value().toBool();
    if (m_downSampleCheck) {
        // UI mode
        m_downSampleCheck->setChecked(sampleResActive);
    } else {
        // Headless modell
        sampleCheckChanged(sampleResActive);
    }
}

QMap<QString, QVariant> Controller::generateSettings(Progressable *receiver, bool useCuda, volatile bool *stopped)
{
    (void) receiver;
    (void) useCuda;
    (void) stopped;
    m_downSampleFactor = downInputResToCheck(m_inputResolution);
    m_threshold = 0.3;
    return getSettings();
}

QMap<QString, QVariant> Controller::getSettings()
{
    QMap<QString, QVariant> settings;
    settings.insert(SETTINGS_THRESHOLD, m_threshold);
    bool samplingResActive = downFactorToCheck(m_downSampleFactor);
    settings.insert(SETTINGS_SAMPLE_RESOLUTION, samplingResActive);
    return settings;
}

bool Controller::downInputResToCheck(QPointF inputRes)
{
    return !(inputRes.x() <= 720 || inputRes.y() <= 720);
}

bool Controller::downFactorToCheck(double downFactor)
{
    return downFactor > 1.0;
}

double Controller::downCheckToFactor(bool boxChecked, QPointF inputRes)
{
    double downFactor = 1.0; // deactivated => default resolution
    if (boxChecked) {
        //      activated => downsampled resolution
        // calc optimal downSampling factor
        QPointF approxFactors = (QPointF)inputRes / 720.0;
        downFactor = approxFactors.x() < approxFactors.y() ? approxFactors.x() : approxFactors.y();

        // round to half or full values
        downFactor = round(downFactor * 2) / 2;

        // prevent up sampling
        downFactor = downFactor < 1.0 ? 1.0 : downFactor;
    }
    return downFactor;
}

void Controller::reportProgress(QString op, int progress, Progressable *receiver)
{
    QMetaObject::invokeMethod(
                receiver,
                "slot_makeProgress",
                Qt::DirectConnection,
                Q_ARG(int, progress),
                Q_ARG(QString, op));
}

void Controller::createSettingsWidget(QWidget *parent)
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
    m_thresholdSpinBox->setMinimum(0.0);
    m_thresholdSpinBox->setMaximum(100.0);
    m_thresholdSpinBox->setSingleStep(1.0);
    m_thresholdSpinBox->setValue(m_threshold * 100.0);
    m_thresholdSpinBox->setAlignment(Qt::AlignRight);
    m_thresholdSpinBox->setSuffix("%");
    QObject::connect(m_thresholdSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
                     [=](double v) {
                        m_threshold = v / 100.0;
                     });
    thresholdLayout->layout()->addWidget(m_thresholdSpinBox);

    // threshold description
    QLabel *thresholdLable = new QLabel(DESCRIPTION_THRESHOLD);
    thresholdLable->setStyleSheet(DESCRIPTION_STYLE);
    thresholdLable->setWordWrap(true);

    // downSample layout
    QWidget *downSampleLayout = new QWidget(parent);
    downSampleLayout->setLayout(new QHBoxLayout(parent));
    downSampleLayout->layout()->addWidget(new QLabel(DOWNSAMPLE_LABEL_TEXT));
    downSampleLayout->layout()->setMargin(0);
    downSampleLayout->layout()->setSpacing(0);

    // downSample checkBox
    m_downSampleCheck = new QCheckBox(parent);
    m_downSampleCheck->setText(DOWNSAMPLE_CHECKBOX_TEXT);
    QObject::connect(m_downSampleCheck, &QCheckBox::clicked, this, &Controller::sampleCheckChanged);
    downSampleLayout->layout()->addItem(new QSpacerItem(0, 0,QSizePolicy::Expanding));
    downSampleLayout->layout()->addWidget(m_downSampleCheck);

    // downSample description
    QLabel *downSampleLable = new QLabel(DESCRIPTION_DOWNSAMPLE);
    downSampleLable->setStyleSheet(DESCRIPTION_STYLE);
    downSampleLable->setWordWrap(true);

    // buffer reset button
    m_resetBufferBt = new QPushButton();
    m_resetBufferBt->setText(RESET_BT_TEXT);
    QObject::connect(m_resetBufferBt, &QPushButton::pressed, this, &Controller::resetBuffer);

    // buffer reset label
    m_resetBufferLabel = new QLabel();
    m_resetBufferLabel->setStyleSheet(INFO_STYLE);
    m_resetBufferLabel->setWordWrap(true);
    updateBufferInfo(m_bufferedValueCount);

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
    m_settingsWidget->layout()->addWidget(m_resetBufferBt);
    m_settingsWidget->layout()->addWidget(m_resetBufferLabel);

    m_settingsWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    m_settingsWidget->adjustSize();
}

void Controller::resetBuffer()
{
    m_bufferMat.clear();
    m_bufferedValueCount = 0;
    emit updateBuffer(sendBuffer());
    if (m_resetBufferBt) {
        updateBufferInfo(m_bufferedValueCount);
    }
}

void Controller::sampleCheckChanged(bool isChecked)
{
    m_downSampleFactor = downCheckToFactor(isChecked, m_inputResolution);
}

void Controller::updateBufferInfo(long bufferedValueCount)
{
    QString txt = RESET_TEXT_PRE + QString::number(bufferedValueCount) + RESET_TEXT_SUF;
    m_resetBufferLabel->setText(txt);
}

void Controller::recreateBufferMatrix(QMap<QString, QVariant> buffer)
{
    // recreate bufferMatrix if the matrix is empty
    m_bufferMat.clear();
    if (buffer.size() != 0) {
        //Get the QMap from Variant
        QMapIterator<QString, QVariant> mapIt(buffer);
        //Find movementBased buffer in the buffer
        while (mapIt.hasNext()) {
            mapIt.next();
            if (mapIt.key().compare(BUFFER_NAME) == 0) {
                stringToBufferMat(mapIt.value().toString());
                break;
            }
        }
    }
}

void Controller::stringToBufferMat(QString string)
{
    QStringList entryStrList = string.split(DELIMITER_ENTITY);

    for (QString nzEntity : entryStrList) {
        QStringList coorStr = nzEntity.split(DELIMITER_COORDINATE);
        // check format "x|y|value"
        if (coorStr.size() != 3) {
            continue;
        }
        bool convertionCheck;
        int x = coorStr[0].toInt(&convertionCheck);
        if (!convertionCheck) {
            continue;
        }
        int y = coorStr[1].toInt(&convertionCheck);
        if (!convertionCheck) {
            continue;
        }
        double value = coorStr[2].toDouble(&convertionCheck);
        if (!convertionCheck) {
            continue;
        }

        // set entry in recreated buffer matrix
        m_bufferMat.ref<double>(x, y) = value;
    }
}

QVariant Controller::bufferMatToVariant(cv::SparseMat bufferMat)
{
    std::stringstream matStream;
    const int *size = bufferMat.size();
    for (int x = 0; x < *size; x++) {
        for (int y = 0; y < *size; y++) {
            double value = bufferMat.ref<double>(x, y);
            if (value != 0) {
                matStream << x << DELIMITER_COORDINATE << y << DELIMITER_COORDINATE << value << ((x + 1 < *size) ? DELIMITER_ENTITY : "");
            }
        }
    }

    std::string matString = matStream.str();
    return QVariant(QString::fromStdString(matString));
}

double Controller::median(std::vector<double> &vec)
{
    std::vector<double>::iterator median = vec.begin() + vec.size() / 2;
    std::nth_element(vec.begin(), median, vec.end());
    return vec[vec.size() / 2];
}
