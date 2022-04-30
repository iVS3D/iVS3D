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

    m_logFile->startTimer(LF_TIMER_BUFFER);
    recreateBufferMatrix(buffer);
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
    const double downSampleFactorConst = m_downSampleFactor;
    std::function<void(cv::Mat, cv::Mat)> calcFlowStatic = [flowCalculator, &flowValues, downSampleFactorConst](cv::Mat fromMat, cv::Mat toMat) {
        auto startFlow = std::chrono::high_resolution_clock::now();
        // muliplication with down sample factor corrects the reduced resolution
        double flowValue = flowCalculator->calculateFlow(fromMat, toMat) * downSampleFactorConst;
        flowValues.push_back(flowValue);
        auto endFlow = std::chrono::high_resolution_clock::now();
        long flowDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endFlow - startFlow).count();
        qDebug() << "flowDuration=" << flowDuration << "ms\tvalue=" << flowValue;
    };

    // -------------- iterate through all available frame pairs ---------------
    uint usedBufferedValues = 0;
    m_logFile->startTimer(LF_TIMER_CORE);
    while (toIter < imageList.end()) {
        // Aborts calculations as result of user interaction
        if (*stopped) {
            qDebug() << "Execution was stopped.";
            return {};
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
    m_logFile->stopTimer();
    m_logFile->addCustomEntry(LF_CE_VALUE_USED_BUFFERED, usedBufferedValues, LF_CE_TYPE_ADDITIONAL_INFO);

    // ------------ select keyframes ----------------
    if (flowValues.size() != imageList.size() - 1) {
        return {};
    }
    m_logFile->startTimer(LF_TIMER_SELECTION);
    std::vector<uint> selectedKeyframes = { imageList[0] };
    std::vector<double> copiedFlowValues = flowValues; // median is in place and reorders vector
    double medianFlow = median(copiedFlowValues);
    double allowedDiffFlow = medianFlow * m_threshold;
    for (uint flowValuesIdx = 0; flowValuesIdx < imageList.size() - 1; flowValuesIdx++) {
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
    m_logFile->stopTimer();
    QPoint samplingResolution = m_inputResolution / m_downSampleFactor;
    QString strSampleResolution = QString::number(samplingResolution.x()) + "x" + QString::number(samplingResolution.y());
    m_logFile->addCustomEntry(LF_CE_NAME_SAMPLERES, samplingResolution, LF_CE_TYPE_ADDITIONAL_INFO);

    updateBufferBtText(m_bufferedValueCount);
    return selectedKeyframes;
}

QString StationaryCamera::getName() const
{
    return PLUGIN_NAME;
}

QVariant StationaryCamera::getBuffer()
{
    QVariant bufferVariant = bufferMatToVariant(m_bufferMat);
    // ------------- IAlgorithm CHANGES -----------
//    QMap<QString, QVariant> bufferMap;
//    bufferMap.insert(BUFFER_NAME, bufferVariant);
//    return bufferMap;
    // --------------------------------------------
    return bufferVariant;
}

QString StationaryCamera::getBufferName()
{
    return BUFFER_NAME;
}

void StationaryCamera::initialize(Reader *reader)
{
    m_reader = reader;
    cv::Mat testPic = reader->getPic(0);
    m_inputResolution.setX(testPic.cols);
    m_inputResolution.setY(testPic.rows);

    int picCount = reader->getPicCount();
    int size[2] = {picCount, picCount};
    m_bufferMat = cv::SparseMat(2, size, CV_32F);
    resetBuffer();
    if (m_resetBufferBt) {
        updateBufferBtText(m_bufferedValueCount);
    }
}

void StationaryCamera::setSettings(QMap<QString, QVariant> settings)
{
    m_threshold = settings.find(SETTINGS_THRESHOLD).value().toDouble();
    m_downSampleFactor = settings.find(SETTINGS_DOWNSAMPLE).value().toDouble();

    if (m_settingsWidget) {
        m_thresholdSpinBox->setValue(m_threshold * 100.0f);
        auto ptrToFactor = std::find(std::begin(m_downSampleFactorArray), std::end(m_downSampleFactorArray), m_downSampleFactor);
        int idx = ptrToFactor - std::begin(m_downSampleFactorArray);
        m_downSampleDropDown->setCurrentIndex(idx);
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

    QWidget *downSampleLayout = new QWidget(parent);
    downSampleLayout->setLayout(new QHBoxLayout(parent));
    downSampleLayout->layout()->addWidget(new QLabel(DOWNSAMPLE_LABEL_TEXT));
    downSampleLayout->layout()->setMargin(0);
    downSampleLayout->layout()->setSpacing(0);
    m_downSampleDropDown = new QComboBox(parent);
    for (double entryFactor : m_downSampleFactorArray) {
        // create an item in the comboBox for every down sample factor
        QPoint sampleResolution = m_inputResolution / entryFactor;
        QString txt = QString::number(sampleResolution.x()) + " x " + QString::number(sampleResolution.y());
        if (entryFactor == 1.0) {
            txt += " (input resolution)";
        }
        m_downSampleDropDown->addItem(txt, entryFactor);
    }
    //
    QObject::connect(m_downSampleDropDown, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                     [=](int idx) {
                        m_downSampleFactor = m_downSampleDropDown->currentData().toDouble();
                        qDebug() << "Sample Factor changed to " << m_downSampleFactor;
                     });
    //
    downSampleLayout->layout()->addWidget(m_downSampleDropDown);

    // downSample description
    QLabel *downSampleLable = new QLabel(DESCRIPTION_DOWNSAMPLE);
    downSampleLable->setStyleSheet(DESCRIPTION_STYLE);
    downSampleLable->setWordWrap(true);

    // buffer reset button
    m_resetBufferBt = new QPushButton();
    m_resetBufferBt->setText(RESET_BT_TEXT);
    QObject::connect(m_resetBufferBt, &QPushButton::pressed, this, &StationaryCamera::resetBuffer);

    // buffer reset label
    m_resetBufferLabel = new QLabel();
    m_resetBufferLabel->setStyleSheet(INFO_STYLE);
    m_resetBufferLabel->setWordWrap(true);
    updateBufferBtText(m_bufferedValueCount);

    // create main widget
    m_settingsWidget = new QWidget(parent);
    m_settingsWidget->setLayout(new QVBoxLayout(parent));
    m_settingsWidget->layout()->setSpacing(0);
    m_settingsWidget->layout()->setMargin(0);
    // add elements
    m_settingsWidget->layout()->addWidget(m_resetBufferBt);
    m_settingsWidget->layout()->addWidget(m_resetBufferLabel);
    m_settingsWidget->layout()->addWidget(thresholdLayout);
    m_settingsWidget->layout()->addWidget(thresholdLable);
    m_settingsWidget->layout()->addWidget(downSampleLayout);
    m_settingsWidget->layout()->addWidget(downSampleLable);

    m_settingsWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    m_settingsWidget->adjustSize();
}

void StationaryCamera::resetBuffer()
{
    m_bufferMat.clear();
    m_bufferedValueCount = 0;
    // TODO send signal that buffer changed
    if (m_resetBufferBt) {
        updateBufferBtText(m_bufferedValueCount);
    }
}

void StationaryCamera::updateBufferBtText(long bufferedValueCount)
{
    QString txt = RESET_TEXT_PRE + QString::number(bufferedValueCount) + RESET_TEXT_SUF;
    m_resetBufferLabel->setText(txt);
}

void StationaryCamera::recreateBufferMatrix(QMap<QString, QVariant> buffer)
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

void StationaryCamera::stringToBufferMat(QString string)
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

QVariant StationaryCamera::bufferMatToVariant(cv::SparseMat bufferMat)
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

double StationaryCamera::median(std::vector<double> &vec)
{
    std::vector<double>::iterator median = vec.begin() + vec.size() / 2;
    std::nth_element(vec.begin(), median, vec.end());
    return vec[vec.size() / 2];
}
