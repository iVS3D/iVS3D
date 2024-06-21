#include "optflowcontroller.h"

SmoothController::SmoothController()
{
    QLocale locale = qApp->property("translation").toLocale();
    QTranslator* translator = new QTranslator();
    translator->load(locale, "stationary", "_", ":/translations", ".qm");
    qApp->installTranslator(translator);
}

QWidget *SmoothController::getSettingsWidget(QWidget *parent)
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

std::vector<uint> SmoothController::sampleImages(const std::vector<uint> &imageList, Progressable *receiver, volatile bool *stopped, bool useCuda, LogFileParent *logFile)
{
    if (imageList.size() == 1) {
        return imageList;
    }

    // ---------- create all neccessary components ------------
    reportProgress(tr("Excluding buffered values from computation list"), 0, receiver);
    // create a vector that containst only indices that need to be gathered (futureFrames + indices with buffered values = imageList)
    std::vector<uint> futureFrames;
    for (uint imageListIdx = 0; imageListIdx < imageList.size() - 1; imageListIdx++) {
        uint fromIdx = imageList[imageListIdx];
        uint toIdx = imageList[imageListIdx + 1];
        double bufferedMovement = m_bufferMat.value<double>(fromIdx, toIdx);
        if (bufferedMovement <= 0.0) {
            futureFrames.push_back(fromIdx);
            futureFrames.push_back(toIdx);
        }
    }
    futureFrames.erase(std::unique(futureFrames.begin(), futureFrames.end()), futureFrames.end()); // remove duplicates

    reportProgress(tr("Creating calculation units"), 0, receiver);
    std::tuple<ImageGatherer*, FlowCalculator*, KeyframeSelector*> components = Factory::instance().createComponents(
            futureFrames,
            m_reader,
            m_downSampleFactor,
            useCuda,
            m_selectorThreshold);
    ImageGatherer *imageGatherer = std::get<0>(components);
    FlowCalculator *flowCalculator = std::get<1>(components);
    KeyframeSelector *keyframeSelector = std::get<2>(components);

    std::vector<double> flowValues = {};
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
            bufferedMovement = m_bufferMat.value<double>(*fromIter, *toIter);
        }
        if (bufferedMovement <= 0.0) {
            imageGatherHandler = std::async(std::launch::async, gatherImagePairStatic, *fromIter, *toIter);
            QPair<cv::Mat, cv::Mat> matPair = imageGatherHandler.get();
            flowCalcHandler = std::async(std::launch::async, calcFlowStatic, matPair.first, matPair.second);
        } else {
            // use buffered flow value
            flowValues.push_back(bufferedMovement);
            usedBufferedValues++;
        }
        // -------- progress and debug ------------
        int progress = ((toIter - imageList.begin()) * 100) / (int)imageList.size();
        QString currOp = tr("Calculating flow between frame ") + QString::number(*fromIter) + tr(" and ") + QString::number(*toIter);
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
    logFile->startTimer(LF_SELECT_FRAMES);
    if (flowValues.size() == imageList.size() + 1) {
        return {};
    }
    std::vector<uint> keyframes = keyframeSelector->select(imageList, flowValues, stopped);
    logFile->stopTimer();

    // -------- update buffer --------------
    logFile->startTimer(LF_TIMER_BUFFER);
    for (uint flowValuesIdx = 0; flowValuesIdx < flowValues.size() - 1; flowValuesIdx++) {
        int progress = (100.0f * flowValues.size()) / (flowValuesIdx + 1);
        reportProgress(tr("Buffering values"), progress, receiver);
        if (m_bufferMat.ref<double>(imageList[flowValuesIdx], imageList[flowValuesIdx + 1]) <= 0.0)
            m_bufferMat.ref<double>(imageList[flowValuesIdx], imageList[flowValuesIdx + 1]) = flowValues[flowValuesIdx];
        // DEBUG write flow values in logFile
//        logFile->addCustomEntry(LF_CE_NAME_FLOWVALUE, flowValues[flowValuesIdx], LF_CE_TYPE_DEBUG);
    }

    // Display Buffer Info
    QString txt = updateBufferInfo(m_bufferMat.hdr->nodeCount);
    if (m_resetBufferLabel) {
        m_resetBufferLabel->setText(txt);
    } else {
        displayMessage(txt, receiver);
    }

    emit updateBuffer(sendBuffer());
    logFile->stopTimer();

    return keyframes;
}

QString SmoothController::getName() const
{
    return PLUGIN_NAME;
}

QMap<QString, QVariant> SmoothController::sendBuffer()
{
    QVariant bufferVariant = bufferMatToVariant(m_bufferMat);
    QMap<QString, QVariant> bufferMap;
    bufferMap.insert(BUFFER_NAME, bufferVariant);
    return bufferMap;
}

void SmoothController::initialize(Reader *reader, QMap<QString, QVariant> buffer, signalObject *sigObj)
{
    if (m_settingsWidget) {
        m_settingsWidget->deleteLater();
        m_settingsWidget = nullptr;
        m_downSampleCheck = nullptr;
    }

    m_sigObj = sigObj;

    m_reader = reader;
    cv::Mat testPic = reader->getPic(0);
    m_inputResolution.setX(testPic.cols);
    m_inputResolution.setY(testPic.rows);

    int picCount = reader->getPicCount();
    int size[2] = {picCount, picCount};
    m_bufferMat = cv::SparseMat(2, size, CV_32F);
    recreateBufferMatrix(buffer);
    if (m_resetBufferLabel) {
        updateBufferInfo(m_bufferMat.hdr->nodeCount);
    }
    bool isChecked = downInputResToCheck(m_inputResolution);
    sampleCheckChanged(isChecked);
}

void SmoothController::setSettings(QMap<QString, QVariant> settings)
{
    bool sampleResActive = settings.find(SETTINGS_SAMPLE_RESOLUTION).value().toBool();
    m_selectorThreshold = settings.find(SETTINGS_SELECTOR_THRESHOLD).value().toDouble();
    if (m_downSampleCheck) {
        // UI mode
        m_downSampleCheck->setChecked(sampleResActive);
        m_selectorThresholdSpinBox->setValue(m_selectorThreshold*100.0);
    } else {
        // Headless modell
        sampleCheckChanged(sampleResActive);
    }
}

QMap<QString, QVariant> SmoothController::generateSettings(Progressable *receiver, bool useCuda, volatile bool *stopped)
{
    (void) receiver;
    (void) useCuda;
    (void) stopped;
    m_downSampleFactor = downInputResToCheck(m_inputResolution);
    return getSettings();
}

QMap<QString, QVariant> SmoothController::getSettings()
{
    QMap<QString, QVariant> settings;
    bool samplingResActive = downFactorToCheck(m_downSampleFactor);
    settings.insert(SETTINGS_SAMPLE_RESOLUTION, samplingResActive);
    settings.insert(SETTINGS_SELECTOR_THRESHOLD, m_selectorThreshold);
    return settings;
}

bool SmoothController::downInputResToCheck(QPointF inputRes)
{
    return !(inputRes.x() <= 720 || inputRes.y() <= 720);
}

bool SmoothController::downFactorToCheck(double downFactor)
{
    return downFactor > 1.0;
}

double SmoothController::downCheckToFactor(bool boxChecked, QPointF inputRes)
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

void SmoothController::reportProgress(QString op, int progress, Progressable *receiver)
{
    QMetaObject::invokeMethod(
                receiver,
                "slot_makeProgress",
                Qt::DirectConnection,
                Q_ARG(int, progress),
                Q_ARG(QString, op));
}

void SmoothController::displayMessage(QString txt, Progressable *receiver)
{
    QMetaObject::invokeMethod(
                receiver,
                "slot_displayMessage",
                Qt::DirectConnection,
                Q_ARG(QString, txt));
}

void SmoothController::createSettingsWidget(QWidget *parent)
{

    // selector layout
    QWidget *selectorLayout = new QWidget(parent);
    selectorLayout->setLayout(new QHBoxLayout(parent));
    selectorLayout->layout()->addWidget(new QLabel(SELECTOR_LABEL_TEXT));
    selectorLayout->layout()->setMargin(0);
    selectorLayout->layout()->setSpacing(0);
    // selector spinBox
    m_selectorThresholdSpinBox = new QDoubleSpinBox(parent);
    m_selectorThresholdSpinBox->setValue(m_selectorThreshold*100.0);
    m_selectorThresholdSpinBox->setDecimals(2);
    m_selectorThresholdSpinBox->setMinimum(0.0);
    m_selectorThresholdSpinBox->setMaximum(200.0);
    m_selectorThresholdSpinBox->setSuffix("%");
    m_selectorThresholdSpinBox->setSingleStep(1.0);
    m_selectorThresholdSpinBox->setAlignment(Qt::AlignRight);
    QObject::connect(m_selectorThresholdSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
                     [this](double v){ m_selectorThreshold = v/100.0; });
    selectorLayout->layout()->addWidget(m_selectorThresholdSpinBox);
    // selector description
    QLabel* selectorLabel = new QLabel(SELECTOR_DESCRIPTION);
    selectorLabel->setStyleSheet(DESCRIPTION_STYLE);
    selectorLabel->setWordWrap(true);

    // downSample layout
    QWidget *downSampleLayout = new QWidget(parent);
    downSampleLayout->setLayout(new QHBoxLayout(parent));
    downSampleLayout->layout()->addWidget(new QLabel(DOWNSAMPLE_LABEL_TEXT));
    downSampleLayout->layout()->setMargin(0);
    downSampleLayout->layout()->setSpacing(0);

    // downSample checkBox
    m_downSampleCheck = new QCheckBox(parent);
    m_downSampleCheck->setText(DOWNSAMPLE_CHECKBOX_TEXT);
    QObject::connect(m_downSampleCheck, &QCheckBox::clicked, this, &SmoothController::sampleCheckChanged);
    downSampleLayout->layout()->addItem(new QSpacerItem(0, 0,QSizePolicy::Expanding));
    downSampleLayout->layout()->addWidget(m_downSampleCheck);

    // downSample description
    QLabel *downSampleLable = new QLabel(DESCRIPTION_DOWNSAMPLE);
    downSampleLable->setStyleSheet(DESCRIPTION_STYLE);
    downSampleLable->setWordWrap(true);

    // buffer reset label
    m_resetBufferLabel = new QLabel();
    m_resetBufferLabel->setStyleSheet(INFO_STYLE);
    m_resetBufferLabel->setWordWrap(true);
    updateBufferInfo(m_bufferMat.hdr->nodeCount);

    // create main widget
    m_settingsWidget = new QWidget(parent);
    m_settingsWidget->setLayout(new QVBoxLayout(parent));
    m_settingsWidget->layout()->setSpacing(0);
    m_settingsWidget->layout()->setMargin(0);
    // add elements
    m_settingsWidget->layout()->addWidget(selectorLayout);
    m_settingsWidget->layout()->addWidget(selectorLabel);
    m_settingsWidget->layout()->addWidget(downSampleLayout);
    m_settingsWidget->layout()->addWidget(downSampleLable);
    m_settingsWidget->layout()->addWidget(m_resetBufferLabel);

    m_settingsWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    m_settingsWidget->adjustSize();
}

void SmoothController::sampleCheckChanged(bool isChecked)
{
    m_downSampleFactor = downCheckToFactor(isChecked, m_inputResolution);
}

QString SmoothController::updateBufferInfo(long bufferedValueCount)
{
    return RESET_TEXT_PRE + QString::number(bufferedValueCount) + RESET_TEXT_SUF;
}

void SmoothController::recreateBufferMatrix(QMap<QString, QVariant> buffer)
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

void SmoothController::stringToBufferMat(QString string)
{
    QStringList entryStrList = string.split(DELIMITER_ENTITY);

    for (const QString nzEntity : entryStrList) {
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

QVariant SmoothController::bufferMatToVariant(cv::SparseMat bufferMat)
{
    std::stringstream matStream;
    const int *size = bufferMat.size();

    for (cv::SparseMatConstIterator it = bufferMat.begin(); it != bufferMat.end(); it++) {
        const cv::SparseMat::Node *node = it.node();
        uint x = node->idx[0];
        uint y = node->idx[1];
        double value = bufferMat.value<double>(x,y);
        if (value >= 0) {
            matStream << x << DELIMITER_COORDINATE << y << DELIMITER_COORDINATE << value << ((x + 1 < (uint)*size) ? DELIMITER_ENTITY : "");
        }
    }

    std::string matString = matStream.str();
    return QVariant(QString::fromStdString(matString));
}
