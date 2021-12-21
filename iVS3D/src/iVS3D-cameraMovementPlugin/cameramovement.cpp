#include "cameramovement.h"

CameraMovement::CameraMovement()
{
    m_settingsWidget = nullptr;
    m_movementSpacer = nullptr;
    m_doubleVal = nullptr;
    m_movementLineEdit = nullptr;
    m_resetSpinBox = nullptr;
    m_resetDelta = 5;
    m_cameraThreshold = 2.0;
    m_infoLabel = nullptr;

}

QWidget* CameraMovement::getSettingsWidget(QWidget *parent)
{
    if(!m_settingsWidget){
        createSettingsWidget(parent);
    }
    m_movementLineEdit->setText(QString::number(m_cameraThreshold));
    m_resetSpinBox->setValue(m_resetDelta);
    return m_settingsWidget;
}

std::vector<uint> CameraMovement::sampleImages(const std::vector<unsigned int> &imageList, Progressable *receiver, volatile bool *stopped, bool useCuda, LogFileParent *logFile)
{
    m_logFile = logFile;
    m_logFile->startTimer(LF_BUFFER);
    m_logFile->stopTimer();

    m_cuda = useCuda;
    std::vector<uint> keyframes;
    m_logFile->startTimer(LF_OPT_FLOW_TOTAL);
    try {
        calcOptFlowSingle(keyframes,m_reader,imageList,receiver,stopped);
    }  catch (cv::Exception &cvExcep) {
        std::cout << cvExcep.msg << std::endl;
    }
    m_logFile->stopTimer();
    QMetaObject::invokeMethod(
                receiver,
                "slot_makeProgress",
                Qt::DirectConnection,
                Q_ARG(int, 100),
                Q_ARG(QString, "Camera movement progress"));

    sendBuffer();
    return keyframes;
}

QString CameraMovement::getName() const
{
    return PLUGIN_NAME;
}

void CameraMovement::sendBuffer()
{
    std::stringstream matStream;
    const int *size = m_bufferMat.size();
    for (int x = 0; x < *size; x++) {
        for (int y = 0; y < *size; y++) {
            double value = m_bufferMat.ref<double>(x, y);
            if (value != 0) {
                matStream << x << DELIMITER_COORDINATE << y << DELIMITER_COORDINATE << value << ((x + 1 < *size) ? DELIMITER_ENTITY : "");
            }
        }
    }

    std::string matString = matStream.str();
    auto val = QVariant(QString::fromStdString(matString));
    QMap<QString, QVariant> buf;
    buf.insert(BUFFER_NAME,val);
    emit updateBuffer(buf);
}


double CameraMovement::averageFlow(cv::Mat flow, uint stepSize)
{
    cv::Point2f averageLine = {0, 0};
    for (uint x = 0; x < (uint)flow.rows; x += stepSize) {
        for (uint y = 0; y < (uint)flow.cols; y += stepSize) {
            // calculates and stores the length of each flow line every stepSize (lenght != 0)
            cv::Point2f line = flow.at<cv::Point2f>(x, y);
            if (line.x == 0 || line.y == 0) {
                continue;
            }
            averageLine += line;
        }
    }

    uint size = pow(flow.rows, 2);
    averageLine.x /= size;
    averageLine.y /= size;
    return sqrt(pow(averageLine.x, 2) + pow(averageLine.y, 2));
}

void CameraMovement::calcOptFlowSingle(std::vector<uint> &keyframes, Reader *reader, std::vector<unsigned int> sharpImages, Progressable *receiver, volatile bool *stopped)
{
    cv::Mat currGrey, prevGrey;
    uint prevKeyframeIndex = 0;
    cv::cvtColor(reader->getPic(0), prevGrey, cv::COLOR_BGR2GRAY);
    keyframes.push_back(sharpImages[0]);
    // for calculating averageMovement (infoLabel)
    double averageMovement = 0;
    uint movementCalcs = 0;
    // setting up farneback
    auto *farn = FarnebackOptFlowFactory::create(m_cuda ? FARNEBACK_CUDA : FARNEBACK_CPU);
    farn->setup(1, 0.5, false, 13, 1, 5, 1.1, 0);

    std::cout << "\n\n<-------- COMPUTING MOVEMENT [duration in ms] ------->\n";
    std::cout << std::left << std::setw(10) << "idx curr" << std::setw(10) << "idx prev" << std::setw(10) << "load" << std::setw(10) << "farneback" << std::setw(10) << "average" << std::setw(10) << "movement" << std::setw(10) << "newMovement" << "\n";

    double noKeyframeMovement = 0;

    for (uint i = 0; i < sharpImages.size(); i++) {
        QElapsedTimer timer;
        timer.start();

        // check if the current camera movement between the selected images was already calculated
        double bufferMovement = 0.0;
        if (m_bufferMat.size() != 0)
            bufferMovement = m_bufferMat.ref<double>(sharpImages[prevKeyframeIndex], sharpImages[i]);
        double newMovement;
        long long afterLoad, afterFarneback;
        if (bufferMovement <= 0.0) {
            // load next image
            cv::cvtColor(reader->getPic(sharpImages[i]), currGrey, cv::COLOR_BGR2GRAY);
            afterLoad = timer.elapsed();

            // calculate optical flow
            cv::Mat flow(prevGrey.size(),CV_32FC2);
            farn->calculateFlow(prevGrey, currGrey, flow);
            afterFarneback = timer.elapsed();

            // calculate movement between sharpImages[prevKeyframeIndex] and sharpImages[i]
            newMovement = averageFlow(flow, 1);

            // save movement in buffer
            m_bufferMat.ref<double>(sharpImages[prevKeyframeIndex], sharpImages[i]) = newMovement;
        } else {
            // use previous calculated movement
            newMovement = bufferMovement;
        }

        // combine movement between two frames with old movement after reset
        double movement = newMovement + noKeyframeMovement;
        averageMovement += newMovement;
        movementCalcs++;
        long long afterAvrg = timer.elapsed();

        auto durationLoadMs = afterLoad;
        auto durationFarnebackMs = afterFarneback - afterLoad;
        auto durationAvrgMs = afterAvrg - afterFarneback;
        std::cout << std::left << std::setw(10) << sharpImages[i] << std::setw(10) << sharpImages[prevKeyframeIndex] << std::setw(10) << durationLoadMs << std::setw(10) << durationFarnebackMs << std::setw(10) << durationAvrgMs << std::setw(10) << movement << std::setw(10)<< newMovement << "\n";

        // send new progress update
        int progress = i * 100 / (int)sharpImages.size();
        QString currentProgress = "Calculate movement between the frames " + QString::number(sharpImages[prevKeyframeIndex]) + " and " + QString::number(sharpImages[i]);
        QMetaObject::invokeMethod(
                    receiver,
                    "slot_makeProgress",
                    Qt::DirectConnection,
                    Q_ARG(int, progress),
                    Q_ARG(QString, currentProgress));

        // reset
        if (prevKeyframeIndex < i - m_resetDelta) {
            cv::cvtColor(reader->getPic(sharpImages[i]), prevGrey, cv::COLOR_BGR2GRAY);
            prevKeyframeIndex = i;
            noKeyframeMovement = movement;
            movement = 0;
        }

        // set new reference picture and reset temporary variables
        if (movement > m_cameraThreshold) {
            keyframes.push_back(sharpImages[i]);
            cv::cvtColor(reader->getPic(sharpImages[i]), prevGrey, cv::COLOR_BGR2GRAY);
            prevKeyframeIndex = i;
            noKeyframeMovement = 0;
        }

        // check for stop
        if (*stopped) {
            break;
        }
    }

    delete farn;

    // update info label and calculate averageMovement
    averageMovement /= movementCalcs;
    updateInfoLabel(averageMovement);
}

void CameraMovement::recreateBufferMatrix(QMap<QString, QVariant> nBuffer)
{
    // recreate bufferMatrix if the matrix is empty
    m_bufferMat.clear();
    if (nBuffer.size() != 0) {
        //Get the QMap from Variant
        QMapIterator<QString, QVariant> mapIt(nBuffer);
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

void CameraMovement::stringToBufferMat(QString string)
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

void CameraMovement::initialize(Reader *reader, QMap<QString, QVariant> buffer, signalObject *)
{
    // setup buffer matrix size
    int picCount = reader->getPicCount();
    int size[2] = {picCount, picCount};
    m_bufferMat = cv::SparseMat(2, size, CV_32F);
    m_reader = reader;
    recreateBufferMatrix(buffer);

    return;
}

void CameraMovement::setMovementThreshold(double movementThreshold)
{
    m_cameraThreshold = movementThreshold;
}

void CameraMovement::setResetDelta(int resetDelta)
{
    m_resetDelta = resetDelta;
}

void CameraMovement::setSettings(QMap<QString, QVariant> settings)
{
    m_resetDelta = settings.find(RESET_DELTA).value().toInt();
    m_cameraThreshold = settings.find(CAMERA_MOVEMENT_THRESHOLD).value().toDouble();
    if (m_settingsWidget) {
        m_resetSpinBox->setValue(m_resetDelta);
        m_movementLineEdit->setText(QString::number(m_cameraThreshold));
    }

}

QMap<QString, QVariant> CameraMovement::generateSettings(Progressable *receiver, bool useCuda, volatile bool* stopped)
{
    // WARNING: All this estimations rely on the fact that the plugin is operation on the all frames not just the keyframes.

    //       select correct farneback
    auto *farn = FarnebackOptFlowFactory::create(useCuda ? FARNEBACK_CUDA : FARNEBACK_CPU);
    farn->setup(1, 0.5, false, 13, 1, 5, 1.1, 0);

    //       calibrate default thresehold value
    double calibratedDefaultThreshold = 0;
    //
    uint calibratePicCount = m_reader->getPicCount() / 50;
    if (calibratePicCount < 10) {
        calibratePicCount = m_reader->getPicCount() > 10 ? 10 : 1;
    }
    std::vector<uint> calibratedPictures;
    for (uint i = 0; i < calibratePicCount; i++) {
        if (*stopped) {
            delete farn;
            return getSettings();
        }
        int progress = ((i + 1) * 100) / calibratePicCount;
        QMetaObject::invokeMethod(
                    receiver,
                    "slot_makeProgress",
                    Qt::DirectConnection,
                    Q_ARG(int, progress),
                    Q_ARG(QString, "Generating settings for CameraMovement"));
        // generate vector of pictures which are used for calibration
        uint nPic = rand() % m_reader->getPicCount();
        if (std::find(calibratedPictures.begin(), calibratedPictures.end(), nPic) != calibratedPictures.end()) {
            // picture was already used for calibration
            i--;
            continue;
        }

        // get grey images
        cv::Mat currGrey, prevGrey;
        cv::cvtColor(m_reader->getPic(nPic), currGrey, cv::COLOR_BGR2GRAY);
        cv::cvtColor(m_reader->getPic(nPic + 1), prevGrey, cv::COLOR_BGR2GRAY);
        // calculate optical flow
        cv::Mat flow(prevGrey.size(), CV_32FC2);
        farn->calculateFlow(prevGrey, currGrey, flow);
        // calculate movement
        calibratedDefaultThreshold += averageFlow(flow, 1);

        // mark picture as calibrated
        calibratedPictures.push_back(nPic);
    }
    m_cameraThreshold = calibratedDefaultThreshold / calibratedPictures.size();

    //       estimate resetDelta
    /*
     * The fps count, as well as the total amount of images can be an identification
     * for how fast the recorded scene ist changing.
     * If the camera movement would be uniform a higher fps would result in a faster change in scence.
     */
    double fps = m_reader->getFPS() > 0 ? m_reader->getFPS() : 30;
    /* 0.15 factor to approximate 15|2 and 30|5 (fps|resetDelta)
     * There is no point in further optimizing this because the resetDelta depends on a lot more factors
     * that cant all be known estiminated. Its still a good approximation to an optimum.*/
    m_resetDelta = fps * 0.15f;

    sendBuffer();
    return getSettings();
}

QMap<QString, QVariant> CameraMovement::getSettings()
{
    QMap<QString, QVariant> settings;
    settings.insert(RESET_DELTA, m_resetDelta);
    settings.insert(CAMERA_MOVEMENT_THRESHOLD, m_cameraThreshold);
    return settings;
}

void CameraMovement::movementThresholdChanged(QString sThreshold)
{
    // checks validation through QDoubleValidator
    if(!m_movementLineEdit->hasAcceptableInput()) {
        return;
    }

    // transforms and checks validation for the second time
    QLocale qlocal(QLocale::C);
    bool validInput = false;
    m_cameraThreshold = qlocal.toDouble(sThreshold, &validInput);
    // changes color to display invalid input
    QColor textColor = validInput ? m_defaultTextColor : Qt::red;
    QPalette lineEditPal = m_movementLineEdit->palette();
    QColor currTextColor = lineEditPal.text().color();    // save old color if it was the default text color
    if (currTextColor != Qt::red) {
        m_defaultTextColor = currTextColor;
    }
    lineEditPal.setColor(QPalette::Text, textColor);
    m_movementLineEdit->setPalette(lineEditPal);
}

void CameraMovement::createSettingsWidget(QWidget *parent)
{
    // create movement threshold layout with spinBox and label
    QWidget *movementLayout = new QWidget(parent);
    movementLayout->setLayout(new QHBoxLayout(parent));
    movementLayout->layout()->addWidget(new QLabel(CAMERA_MOVEMENT_THRESHOLD));
    movementLayout->layout()->setMargin(0);
    movementLayout->layout()->setSpacing(0);
    m_movementLineEdit = new QLineEdit(parent);
    m_doubleVal = new QDoubleValidator();
    m_doubleVal->setBottom(0);
    m_doubleVal->setNotation(QDoubleValidator::StandardNotation);
    m_movementLineEdit->setValidator(m_doubleVal);
    m_movementLineEdit->setAlignment(Qt::AlignRight);
    m_movementLineEdit->setMaximumWidth(200);
    QObject::connect(m_movementLineEdit, &QLineEdit::textChanged, this, &CameraMovement::movementThresholdChanged);
    movementLayout->layout()->addWidget(m_movementLineEdit);

    // create reset delta layout with spinBox and label
    QWidget *resetLayout = new QWidget(parent);
    resetLayout->setLayout(new QHBoxLayout(parent));
    resetLayout->layout()->addWidget(new QLabel(RESET_DELTA));
    resetLayout->layout()->setMargin(0);
    resetLayout->layout()->setSpacing(0);
    m_resetSpinBox = new QSpinBox(parent);
    m_resetSpinBox->setMinimum(1);
    m_resetSpinBox->setMaximum(999999);
    m_resetSpinBox->setValue(m_resetDelta);
    m_resetSpinBox->setAlignment(Qt::AlignRight);
    QObject::connect(m_resetSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [=](int v) {m_resetDelta = v;});
    resetLayout->layout()->addWidget(m_resetSpinBox);

    // movement threshold description
    QLabel *movementThresholdDes = new QLabel(MOVEMENTTHRESHOLD_DESCRIPTION);
    movementThresholdDes->setStyleSheet(DESCRIPTION_STYLE);
    movementThresholdDes->setWordWrap(true);

    // reset delta description
    QLabel *resetDeltaDes = new QLabel(RESETDELTA_DESCRIPTION);
    resetDeltaDes->setStyleSheet(DESCRIPTION_STYLE);
    resetDeltaDes->setWordWrap(true);

    // info label
    m_infoLabel = new QLabel(INFO_DEFAULT);
    m_infoLabel->setStyleSheet(INFO_STYLE);
    m_infoLabel->setWordWrap(true);

    // creaet widget
    m_settingsWidget = new QWidget(parent);
    m_settingsWidget->setLayout(new QVBoxLayout(parent));
    m_settingsWidget->layout()->setSpacing(0);
    m_settingsWidget->layout()->setMargin(0);
    // add elements
    m_settingsWidget->layout()->addWidget(movementLayout);
    m_settingsWidget->layout()->addWidget(movementThresholdDes);
    m_settingsWidget->layout()->addWidget(resetLayout);
    m_settingsWidget->layout()->addWidget(resetDeltaDes);
    m_settingsWidget->layout()->addWidget(m_infoLabel);

    m_settingsWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

    m_settingsWidget->adjustSize();
}

void CameraMovement::updateInfoLabel(double averageMovement)
{
    if(m_infoLabel){
        m_infoLabel->setText("The last dataset had an average camera movement of " + QString::number(averageMovement) + " between each analyzed frame.");
    }
}
