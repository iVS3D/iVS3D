#include "blur.h"



Blur::Blur()
{
    m_settingsWidget = nullptr;
    m_blurAlgorithms.push_back(new BlurLaplacian());
    m_blurAlgorithms.push_back(new BlurSobel());
    m_usedBlur = m_blurAlgorithms[0];
    m_localDeviation = 95;
    m_windowSize = 10;
}

QWidget* Blur::getSettingsWidget(QWidget *parent)
{
    if(!m_settingsWidget){
        createSettingsWidget(parent);
    }
    return m_settingsWidget;
}

std::vector<uint> Blur::sampleImages(const std::vector<unsigned int> &imageList, Progressable *receiver, volatile bool *stopped, bool useCuda, LogFileParent *logFile)
{
    (void) useCuda;
    m_logFile = logFile;
    m_logFile->startTimer("complete");
    m_blurValues.clear();
    //Qvariant is not empty
    if (m_buffer.size() != 0) {
        //Get the QMap from Variant
        QMapIterator<QString, QVariant> mapIt(m_buffer);
        //Check if usedBlur is in the buffer
        while (mapIt.hasNext()) {
            mapIt.next();
            if (mapIt.key().compare(m_usedBlur->getName()) == 0) {
               m_blurValues = splitDoubleString(mapIt.value().toString());
               break;
            }
        }

    }
    //If no buffer ist found init new blur vector and set first value to 0 to detect new blur vector
    if (m_blurValues.empty()) {
        m_blurValues = std::vector<double>(m_reader->getPicCount());
        m_blurValues[0] = 0;
    }
    //Decide if blur detection for all images or only for keyframes is needed
    std::vector<uint> sampledImages;
    if (imageList.size() == imageList.back() - imageList[0] + 1) {
        //All images have to be sampled
        if (m_blurValues[0] == 0) {
            m_blurValues = m_usedBlur->calcFullBluriness(m_reader, receiver, stopped, imageList[0], imageList.back(), m_blurValues);
        }


        sampledImages = sampleAllImages(m_reader, receiver, stopped, imageList[0], imageList.back());
    }
    else {
        sampledImages = sampleKeyframes(m_reader, receiver, stopped, imageList);
    }

    m_logFile->stopTimer();
    computeBuffer();
    return sampledImages;
}

QString Blur::getName() const
{
    return PLUGIN_NAME;
}


void Blur::computeBuffer()
{
    std::stringstream bufferStream;
    for (uint i = 0; i < m_blurValues.size(); i++) {
        if (i != 0) {
           bufferStream << ",";
        }
        bufferStream << m_blurValues[i];
    }
    std::string buffer = bufferStream.str();
    QVariant blurValues(QString::fromStdString(buffer));
    m_buffer.insert(m_usedBlur->getName(), blurValues);
    emit updateBuffer(m_buffer);
}

void Blur::initialize(Reader *reader, QMap<QString, QVariant> buffer, signalObject* sig_obj)
{
    m_reader = reader;
    m_buffer = buffer;
    m_sigObj = sig_obj;
    connect(m_sigObj, SIGNAL(sig_selectedImageIndex(uint)), this, SLOT(slot_selectedImageIndex(uint)));
}

void Blur::setSettings(QMap<QString, QVariant> settings)
{
    m_windowSize = settings.find(WINDOW_SIZE).value().toInt();
    m_localDeviation = settings.find(LOCAL_DEVIATION).value().toDouble();
    QString usedAlgo = settings.find(USED_BLUR).value().toString();
    int blurIndex = 0;
    for (BlurAlgorithm* algo : m_blurAlgorithms) {
        if (usedAlgo.compare(algo->getName()) == 0) {
            m_usedBlur = algo;
            break;
        }
        blurIndex++;
    }
    if(m_settingsWidget) {
        m_spinBoxLD->setValue(m_localDeviation);
        m_spinBoxWS->setValue(m_windowSize);
        m_comboBoxBlur->setCurrentIndex(blurIndex);
    }



}

QMap<QString, QVariant> Blur::generateSettings(Progressable *receiver, bool useCuda, volatile bool* stopped)
{
    (void) receiver;
    (void) useCuda;
    (void) stopped;
    return getSettings();
}

QMap<QString, QVariant> Blur::getSettings()
{
    QMap<QString, QVariant> settings;
    settings.insert(USED_BLUR, m_usedBlur->getName());
    settings.insert(LOCAL_DEVIATION, m_localDeviation);
    settings.insert(WINDOW_SIZE, m_windowSize);
    return settings;
}

void Blur::slot_blurChanged(const QString & name)
{
    for (BlurAlgorithm* b : m_blurAlgorithms) {
        if(b->getName().compare(name) == 0) {
            m_usedBlur = b;
            break;
        }
    }
}

void Blur::slot_wsChanged(int ws)
{
    m_windowSize = ws;
}

void Blur::slot_ldChanged(int ld)
{
    m_localDeviation = ld;
}

void Blur::slot_selectedImageIndex(uint index)
{
    if (m_buffer.contains(m_usedBlur->getName())) {
        QVariant currentBuffer = m_buffer[m_usedBlur->getName()];
        double currentBlurValue = splitDoubleString(currentBuffer.toString())[index];
        QString info;
        info = currentBlurValue == 0 ? "not calculated" : QString::number(currentBlurValue);
        m_infoLabel->setText("Blur value for the current image is " + info);
    }
}


void Blur::createSettingsWidget(QWidget *parent)
{
    m_settingsWidget = new QWidget(parent);
    m_settingsWidget->setLayout(new QVBoxLayout());
    m_settingsWidget->layout()->setSpacing(0);
    m_settingsWidget->layout()->setMargin(0);

    QWidget *w = new QWidget(parent);
    w->setLayout(new QHBoxLayout(parent));
    w->layout()->setSpacing(0);
    w->layout()->setMargin(0);
    w->layout()->addWidget(new QLabel("Select blur ",parent));

    m_comboBoxBlur = new QComboBox(parent);
    for (BlurAlgorithm* b : m_blurAlgorithms) {
        m_comboBoxBlur->addItem(b->getName());
    }
    w->layout()->addWidget(m_comboBoxBlur);
    QObject::connect(m_comboBoxBlur, QOverload<const QString &>::of(&QComboBox::currentTextChanged), this, &Blur::slot_blurChanged);

    m_settingsWidget->layout()->addWidget(w);

    QLabel *LabelBlur = new QLabel("Blur algorithm to be used");
    LabelBlur->setStyleSheet(DESCRIPTION_STYLE);
    LabelBlur->setWordWrap(true);
    m_settingsWidget->layout()->addWidget(LabelBlur);

    QWidget *ws = new QWidget(parent);
    ws->setLayout(new QHBoxLayout(parent));
    ws->layout()->setSpacing(0);
    ws->layout()->setMargin(0);
    ws->layout()->addWidget(new QLabel("Set window size",parent));

    m_spinBoxWS = new QSpinBox(parent);
    m_spinBoxWS->setMinimum(1);
    m_spinBoxWS->setMaximum(9999);
    m_spinBoxWS->setValue(m_windowSize);
    m_spinBoxWS->setAlignment(Qt::AlignRight);
    ws->layout()->addWidget(m_spinBoxWS);
    QObject::connect(m_spinBoxWS, QOverload<int>::of(&QSpinBox::valueChanged), this, &Blur::slot_wsChanged);

    m_settingsWidget->layout()->addWidget(ws);


    QLabel *windowSize = new QLabel("Number of images around the frame");
    windowSize->setStyleSheet(DESCRIPTION_STYLE);
    windowSize->setWordWrap(true);
    m_settingsWidget->layout()->addWidget(windowSize);


    QWidget *ld = new QWidget(parent);
    ld->setLayout(new QHBoxLayout(parent));
    ld->layout()->setSpacing(0);
    ld->layout()->setMargin(0);
    ld->layout()->addWidget(new QLabel("Set local deviation",parent));

    m_spinBoxLD = new QSpinBox(parent);
    m_spinBoxLD->setMinimum(1);
    m_spinBoxLD->setMaximum(9999);
    m_spinBoxLD->setValue(m_localDeviation);
    m_spinBoxLD->setAlignment(Qt::AlignRight);
    ld->layout()->addWidget(m_spinBoxLD);
    QObject::connect(m_spinBoxLD, QOverload<int>::of(&QSpinBox::valueChanged), this, &Blur::slot_ldChanged);

    m_settingsWidget->layout()->addWidget(ld);



    QLabel *localDeviation = new QLabel("Deviation from the image to the window average blur");
    localDeviation->setStyleSheet(DESCRIPTION_STYLE);
    localDeviation->setWordWrap(true);
    m_settingsWidget->layout()->addWidget(localDeviation);

    m_infoLabel = new QLabel("Blur value for the current image is not calculated");
    m_infoLabel->setWordWrap(true);
    m_settingsWidget->layout()->addWidget(m_infoLabel);

    m_settingsWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    m_settingsWidget->adjustSize();


}

std::vector<double> Blur::splitDoubleString(QString string) {
    std::vector<double> returnVector;
    QStringList values = string.split(",");
    for (const QString &val : qAsConst(values)) {
        if (!val.isEmpty()) {
          returnVector.push_back(val.toDouble());
        }
    }
    return returnVector;
}

std::vector<uint> Blur::sampleAllImages(Reader *reader,  Progressable *receiver, volatile bool *stopped, int start, int end) {
    std::vector<unsigned int> sharpImages;
    int picCount = end - start + 1;

    int windowStart = start;
    int windowEnd;
    //Left border can't be larger then picCount
    if (m_windowSize < picCount - 1) {
        windowEnd = start + m_windowSize;
    }
    else {
        windowEnd = start + picCount - 1;
    }

    for (int i = start; i <= end; i++) {
        if (*stopped) {
            return std::vector<unsigned int>();
        }

        if (receiver != nullptr) {
            int progress = ((i - start) * 100 / picCount);
            QString currentProgress = "Calculate blur of frame number " + QString::number(i - start) + " of " + QString::number(picCount) + " total frames";
            QMetaObject::invokeMethod(receiver, "slot_makeProgress", Qt::DirectConnection, Q_ARG(int, progress), Q_ARG(QString, currentProgress));
        }

        double windowTotal = 0;

        for (int window = windowStart; window <= windowEnd; window++) {
            //Check if current blur value exists & calculate them if they don't
            if (m_blurValues[window] == 0) {
                m_blurValues[window] = m_usedBlur->calcOneBluriness(reader, window);
            }
            windowTotal += m_blurValues[window];
        }
        //+ 1 because its 0 indexed
        double windowAvg = windowTotal / (windowEnd - windowStart + 1);
        double currentBlurValue = m_blurValues[i];

        if ((currentBlurValue/windowAvg) >= (m_localDeviation / 100)) {
            sharpImages.push_back(i);
        }



        //If there are less images then 2x windowSize no border has to be moved, if left is at start and right at picCount - 1
        if (windowEnd >= start + picCount - 1 && windowStart == start) {
            continue;
        }
        //left border won't be moved until i >= windowSize
        if (windowStart == start) {
            if (i < start + m_windowSize) {
                windowEnd++;
            }
            //first time left border is at the start index and i >= windowSize
            else {
                windowStart++;
                windowEnd++;
            }
        }
        //if the right border reaches the end, only the left one is increased
        else if (windowEnd == start + picCount - 1) {
            windowStart++;
        }
        //in the middle both borders increase
        else {
            windowStart++;
            windowEnd++;
        }
    }
    return sharpImages;
}


std::vector<uint> Blur::sampleKeyframes(Reader *reader,  Progressable *receiver, volatile bool *stopped, std::vector<uint> sharpImages) {

    std::vector<uint> sampledImages;
    int picCount = static_cast<int>(sharpImages.size());
    for (uint i = 0; i < sharpImages.size(); i++) {
        if (*stopped) {
            return std::vector<unsigned int>();
        }

        //Get start of the current window
        int windowStart;
        int calcStart = sharpImages[i] - m_windowSize;
        if (calcStart < 0) {
            windowStart = 0;
        }
        else {
            windowStart = sharpImages[i] - m_windowSize;
        }


        //Get End of the current window
        int windowEnd;
        uint calcEnd = sharpImages[i] + m_windowSize;
        if (calcEnd >= reader->getPicCount()) {
            windowEnd = reader->getPicCount() - 1;
        }
        else {
            windowEnd = sharpImages[i] + m_windowSize;
        }

        if (receiver != nullptr) {
            int progress = (i  * 100 / picCount);
            QString currentProgress = "Calculate blur of Keyframe number " + QString::number(i) + " of " + QString::number(sharpImages.size()) + " total keyframes";
            //receiver->slot_makeProgress(progress, currentProgress);
            QMetaObject::invokeMethod(
                        receiver,
                        "slot_makeProgress",
                        Qt::DirectConnection,
                        Q_ARG(int, progress),
                        Q_ARG(QString, currentProgress));
        }

        for (int window = windowStart; window <= windowEnd; window++) {
            //Check if current blur value exists & calculate them if they don't
            if (m_blurValues[window] == 0 || m_blurValues[window] == -1) {
                m_blurValues[window] = m_usedBlur->calcOneBluriness(reader, window);
            }
        }

        uint currentBestIndex = sharpImages[i];
        double currentBestBlur = m_blurValues[currentBestIndex];

        for (int window = windowStart; window <= windowEnd; window++) {
           if (m_blurValues[window] > currentBestBlur) {
               currentBestIndex = window;
               currentBestBlur = m_blurValues[window];
           }
        }

        //Check if selected keyframe is already in the result and add it otherwise
        if(std::find(sampledImages.begin(), sampledImages.end(), currentBestIndex) == sampledImages.end()) {
            sampledImages.push_back(currentBestIndex);
        }

    }

    QMetaObject::invokeMethod(
                receiver,
                "slot_makeProgress",
                Qt::DirectConnection,
                Q_ARG(int, 100),
                Q_ARG(QString, "Blur progress"));
    return sampledImages;
}


