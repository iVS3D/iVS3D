#include "optflowcontroller.h"

OptFlowController::OptFlowController()
{
    QTranslator* translator = new QTranslator();
    translator->load(QLocale::system(), "stationary", "_", ":/translations", ".qm");
    qApp->installTranslator(translator);
}

QWidget *OptFlowController::getSettingsWidget(QWidget *parent)
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

std::vector<uint> OptFlowController::sampleImages(const std::vector<uint> &imageList, Progressable *receiver, volatile bool *stopped, bool useCuda, LogFileParent *logFile)
{
    // ----------- setup and creating hardware specific elements ----------------
    QString selectorName = m_selectorDropDown->currentText();
    KeyframeSelector::Settings selectorSettings = m_selectorSettingsMap.value(selectorName);

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
            selectorName,
            selectorSettings);
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
    updateBufferInfo(m_bufferMat.hdr->nodeCount);
    emit updateBuffer(sendBuffer());
    logFile->stopTimer();

    return keyframes;
}

QString OptFlowController::getName() const
{
    return PLUGIN_NAME;
}

QMap<QString, QVariant> OptFlowController::sendBuffer()
{
    QVariant bufferVariant = bufferMatToVariant(m_bufferMat);
    QMap<QString, QVariant> bufferMap;
    bufferMap.insert(BUFFER_NAME, bufferVariant);
    return bufferMap;
}

void OptFlowController::initialize(Reader *reader, QMap<QString, QVariant> buffer, signalObject *sigObj)
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
    if (m_resetBufferBt) {
        updateBufferInfo(m_bufferMat.hdr->nodeCount);
    }
    bool isChecked = downInputResToCheck(m_inputResolution);
    sampleCheckChanged(isChecked);
}

void OptFlowController::setSettings(QMap<QString, QVariant> settings)
{
    bool sampleResActive = settings.find(SETTINGS_SAMPLE_RESOLUTION).value().toBool();
    if (m_downSampleCheck) {
        // UI mode
        m_downSampleCheck->setChecked(sampleResActive);
    } else {
        // Headless modell
        sampleCheckChanged(sampleResActive);
    }

    // deconstruct QVariant to KeyframeSelector::Settings
    QString selectorName = settings.find(SETTINGS_SELECTOR_NAME).value().toString();
    QList<QVariant> selectorSettingsVar = settings.find(SETTINGS_SELECTOR_SETTINGS).value().toList();
        // recreate settings as KeyframeSelector::Settings
        KeyframeSelector::Settings selectorSetting;
        // iterate over parameters in this setting (settings for one selector)
        for (const QVariant &varParam : qAsConst(selectorSettingsVar)) {
            KeyframeSelector::Parameter param = KeyframeSelector::Parameter(varParam);
            selectorSetting.append(varParam);
            emit changeUIParameter(param.value, param.name, selectorName);
        }
        m_selectorSettingsMap.insert(selectorName, selectorSetting);

    m_selectorDropDown->setCurrentText(selectorName);
}

QMap<QString, QVariant> OptFlowController::generateSettings(Progressable *receiver, bool useCuda, volatile bool *stopped)
{
    (void) receiver;
    (void) useCuda;
    (void) stopped;
    m_downSampleFactor = downInputResToCheck(m_inputResolution);
    return getSettings();
}

QMap<QString, QVariant> OptFlowController::getSettings()
{
    QMap<QString, QVariant> settings;
    bool samplingResActive = downFactorToCheck(m_downSampleFactor);
    settings.insert(SETTINGS_SAMPLE_RESOLUTION, samplingResActive);

    // construct QVariant from KeyfraneSelector::Settings
    QString selectorName = m_selectorDropDown->currentText();
    settings.insert(SETTINGS_SELECTOR_NAME, selectorName);

    KeyframeSelector::Settings selectorSettings = m_selectorSettingsMap.find(selectorName).value();
    // convert settings to QVariant
    QList<QVariant> selectorSettingsVar;
    for (KeyframeSelector::Parameter param : selectorSettings) {
        selectorSettingsVar.append(param.toQVariant());
    }
    settings.insert(SETTINGS_SELECTOR_SETTINGS, QVariant(selectorSettingsVar));
    return settings;
}

bool OptFlowController::downInputResToCheck(QPointF inputRes)
{
    return !(inputRes.x() <= 720 || inputRes.y() <= 720);
}

bool OptFlowController::downFactorToCheck(double downFactor)
{
    return downFactor > 1.0;
}

double OptFlowController::downCheckToFactor(bool boxChecked, QPointF inputRes)
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

void OptFlowController::reportProgress(QString op, int progress, Progressable *receiver)
{
    QMetaObject::invokeMethod(
                receiver,
                "slot_makeProgress",
                Qt::DirectConnection,
                Q_ARG(int, progress),
                Q_ARG(QString, op));
}

void OptFlowController::createSettingsWidget(QWidget *parent)
{
    // keyframeSelector layout
    //      setup drop down for KeyframeSelectors
    m_selectorDropDown = new QComboBox(parent);
    QWidget *selectorDropDownLayout = new QWidget(parent);
    selectorDropDownLayout->setLayout(new QHBoxLayout(parent));
    selectorDropDownLayout->layout()->addWidget(new QLabel(SELECTOR_DROPDOWN));
    selectorDropDownLayout->layout()->addWidget(m_selectorDropDown);
    m_selectorSettingsMap = Factory::instance().getAllSelectors();
    //      additems and create QWidgets
    QStringList selectorNames = m_selectorSettingsMap.keys();
    for (QString selectorName : qAsConst(selectorNames)) {
        m_selectorDropDown->addItem(selectorName);
        QWidget *settingsWidget = selectorSettingsToWidget(parent, selectorName);
        m_selectorWidgetMap.insert(selectorName, settingsWidget);
    }
    //      set first QWidget
    if (m_selectorWidgetMap.size() > 0) {
        m_currentSelectorWidget = m_selectorWidgetMap.values().first();
    }
    QObject::connect(m_selectorDropDown, &QComboBox::currentTextChanged, this, &OptFlowController::selectorChanged);

    // downSample layout
    QWidget *downSampleLayout = new QWidget(parent);
    downSampleLayout->setLayout(new QHBoxLayout(parent));
    downSampleLayout->layout()->addWidget(new QLabel(DOWNSAMPLE_LABEL_TEXT));
    downSampleLayout->layout()->setMargin(0);
    downSampleLayout->layout()->setSpacing(0);

    // downSample checkBox
    m_downSampleCheck = new QCheckBox(parent);
    m_downSampleCheck->setText(DOWNSAMPLE_CHECKBOX_TEXT);
    QObject::connect(m_downSampleCheck, &QCheckBox::clicked, this, &OptFlowController::sampleCheckChanged);
    downSampleLayout->layout()->addItem(new QSpacerItem(0, 0,QSizePolicy::Expanding));
    downSampleLayout->layout()->addWidget(m_downSampleCheck);

    // downSample description
    QLabel *downSampleLable = new QLabel(DESCRIPTION_DOWNSAMPLE);
    downSampleLable->setStyleSheet(DESCRIPTION_STYLE);
    downSampleLable->setWordWrap(true);

    // buffer reset button
    m_resetBufferBt = new QPushButton();
    m_resetBufferBt->setText(RESET_BT_TEXT);
    QObject::connect(m_resetBufferBt, &QPushButton::pressed, this, &OptFlowController::resetBuffer);

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
    m_settingsWidget->layout()->addWidget(selectorDropDownLayout);
    m_settingsWidget->layout()->addWidget(m_currentSelectorWidget);
    m_settingsWidget->layout()->addWidget(downSampleLayout);
    m_settingsWidget->layout()->addWidget(downSampleLable);
    m_settingsWidget->layout()->addWidget(m_resetBufferBt);
    m_settingsWidget->layout()->addWidget(m_resetBufferLabel);

    m_settingsWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    m_settingsWidget->adjustSize();
}

void OptFlowController::resetBuffer()
{
    m_bufferMat.clear();
    emit updateBuffer(sendBuffer());
    if (m_resetBufferBt) {
        updateBufferInfo(m_bufferMat.hdr->nodeCount);
    }
}

void OptFlowController::sampleCheckChanged(bool isChecked)
{
    m_downSampleFactor = downCheckToFactor(isChecked, m_inputResolution);
}

void OptFlowController::selectorChanged(QString selectorName)
{
    QWidget *nSelectorWidget = m_selectorWidgetMap.value(selectorName);
    m_settingsWidget->layout()->replaceWidget(m_currentSelectorWidget, nSelectorWidget, Qt::FindChildrenRecursively);
    m_currentSelectorWidget->setVisible(false);
    nSelectorWidget->setVisible(true);
    m_currentSelectorWidget = nSelectorWidget;
}

void OptFlowController::updateSettingsMap(QVariant nValue, KeyframeSelector::Parameter param, QString selectorName)
{
    KeyframeSelector::Settings oldSettings = m_selectorSettingsMap.value(selectorName);
    // Search for parameter
    QList<KeyframeSelector::Parameter>::iterator it;
    for (it = oldSettings.begin(); it != oldSettings.end(); ++it) {
        KeyframeSelector::Parameter oldParam = *it;
        if (oldParam.name == param.name && oldParam.value.type() == param.value.type()) {
            // modifie old param entry with new value
            oldSettings.erase(it);
            KeyframeSelector::Parameter nParam = oldParam;
            nParam.value = nValue;
            oldSettings.append(nParam);
            // repalce modified param in old settings wiht new param and add modified settings to settings map
            m_selectorSettingsMap.insert(selectorName, oldSettings);
            break;
        }
    }
}

void OptFlowController::updateBufferInfo(long bufferedValueCount)
{
    QString txt = RESET_TEXT_PRE + QString::number(bufferedValueCount) + RESET_TEXT_SUF;
    m_resetBufferLabel->setText(txt);
}

void OptFlowController::recreateBufferMatrix(QMap<QString, QVariant> buffer)
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

void OptFlowController::stringToBufferMat(QString string)
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

QVariant OptFlowController::bufferMatToVariant(cv::SparseMat bufferMat)
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

QWidget *OptFlowController::selectorSettingsToWidget(QWidget *parent, QString selectorName)
{
    KeyframeSelector::Settings settings = m_selectorSettingsMap.value(selectorName);
    QWidget *selectorWidget = new QWidget(parent);
    selectorWidget->setLayout(new QVBoxLayout(parent));
    selectorWidget->layout()->setMargin(0);
    // add elements to widgets based on data type of the parameter
    for (KeyframeSelector::Parameter param : qAsConst(settings)) {
        QWidget *paramLayout = new QWidget(parent);
        paramLayout->setLayout(new QHBoxLayout(parent));
        // name
        QLabel *label = new QLabel(param.name);
        label->setWordWrap(true);
        paramLayout->layout()->addWidget(label);
        // input
        if (param.value.isNull()) {
            // skip parameter if it has no value
            delete paramLayout;
            continue;
        }
        //      create fitting input widget based on the data type of the value
        switch (param.value.type()) {
        case QVariant::Int: {
            QSpinBox *input = new QSpinBox(parent);
            input->setValue(param.value.toInt());
            input->setMinimum(INT_MIN);
            input->setMaximum(INT_MAX);
            input->setAlignment(Qt::AlignRight);
            paramLayout->layout()->addItem(new QSpacerItem(0, 0));
            paramLayout->layout()->addWidget(input);
            // updates member settings map using the ui signal
            QObject::connect(input, QOverload<int>::of(&QSpinBox::valueChanged), this, [param, this, selectorName](int nValue){
                updateSettingsMap(nValue, param, selectorName);
            });
            // updates ui using the changeUIParameter signal
            QObject::connect(this, &OptFlowController::changeUIParameter, this, [input, param, selectorName] (QVariant nValue, QString sigParamName, QString sigSelectorName) {
                // guards to only change specified ui element (neede because every ui element will receive that signal)
                if (QString::compare(selectorName, sigSelectorName, Qt::CaseSensitive) != 0)
                    return;
                if (param.name != sigParamName)
                    return;
                if (param.value.type() != nValue.type())
                    return;
                // update value
                input->setValue(nValue.toInt());
            });
            break;
        }
        case QVariant::Double: {
            QDoubleSpinBox *input = new QDoubleSpinBox(parent);
            input->setValue(param.value.toDouble());
            input->setAlignment(Qt::AlignRight);
            input->setSingleStep(0.1);
            input->setMinimum(-1000);
            input->setMaximum(1000);
            paramLayout->layout()->addItem(new QSpacerItem(0, 0));
            paramLayout->layout()->addWidget(input);
            // updates member settings map using the ui signal
            QObject::connect(input, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [param, this, selectorName](double nValue){
                updateSettingsMap(nValue, param, selectorName);
            });
            // updates ui using the changeUIParameter signal
            QObject::connect(this, &OptFlowController::changeUIParameter, this, [input, param, selectorName] (QVariant nValue, QString sigParamName, QString sigSelectorName) {
                // guards to only change specified ui element (neede because every ui element will receive that signal)
                if (QString::compare(selectorName, sigSelectorName, Qt::CaseSensitive) != 0)
                    return;
                if (param.name != sigParamName)
                    return;
                if (param.value.type() != nValue.type())
                    return;
                // update value
                input->setValue(nValue.toDouble());
            });
            break;
        }
        case QVariant::String: {
            QLineEdit *input = new QLineEdit(parent);
            input->setText(param.value.toString());
            input->setAlignment(Qt::AlignRight);
            paramLayout->layout()->addItem(new QSpacerItem(0, 0));
            paramLayout->layout()->addWidget(input);
            QObject::connect(input, &QLineEdit::textChanged, this, [param, this, selectorName](QString nValue){
                updateSettingsMap(nValue, param, selectorName);
            });
            // updates ui using the changeUIParameter signal
            QObject::connect(this, &OptFlowController::changeUIParameter, this, [input, param, selectorName] (QVariant nValue, QString sigParamName, QString sigSelectorName) {
                // guards to only change specified ui element (neede because every ui element will receive that signal)
                if (QString::compare(selectorName, sigSelectorName, Qt::CaseSensitive) != 0)
                    return;
                if (param.name != sigParamName)
                    return;
                if (param.value.type() != nValue.type())
                    return;
                // update value
                input->setText(nValue.toString());
            });
            break;
        }
        case QVariant::Bool: {
            QCheckBox *input = new QCheckBox(parent);
            input->setChecked(param.value.toBool());
            paramLayout->layout()->addItem(new QSpacerItem(0, 0));
            paramLayout->layout()->addWidget(input);
            // updates member settings map using the ui signal
            QObject::connect(input, &QCheckBox::toggled, this, [param, this, selectorName](bool nValue){
                updateSettingsMap(nValue, param, selectorName);
            });
            // updates ui using the changeUIParameter signal
            QObject::connect(this, &OptFlowController::changeUIParameter, this, [input, param, selectorName] (QVariant nValue, QString sigParamName, QString sigSelectorName) {
                // guards to only change specified ui element (neede because every ui element will receive that signal)
                if (QString::compare(selectorName, sigSelectorName, Qt::CaseSensitive) != 0)
                    return;
                if (param.name != sigParamName)
                    return;
                if (param.value.type() != nValue.type())
                    return;
                // update value
                input->setChecked(nValue.toBool());
            });
            break;
        }

        default:
            // skip parameter if it is not parseable
            delete paramLayout;
            continue;
            break;
        }
        // add parameterWidget and infoLable to selectorWidget (vertical)
        selectorWidget->layout()->addWidget(paramLayout);
        QLabel *paramInfoLable = new QLabel(param.info);
        paramInfoLable->setStyleSheet(DESCRIPTION_STYLE);
        paramInfoLable->setWordWrap(true);
        selectorWidget->layout()->addWidget(paramInfoLable);
    }

    return selectorWidget;
}
