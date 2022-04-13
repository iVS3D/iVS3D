#include "nthframe.h"

NthFrame::NthFrame()
{
    m_N = 1;    // N initialized to stepwidth 1
    m_numFrames = 0;
    m_settingsWidget = nullptr;
}

NthFrame::~NthFrame()
{

}

QWidget* NthFrame::getSettingsWidget(QWidget *parent)
{
    if(!m_settingsWidget){
        createSettingsWidget(parent);
    }
    m_spinBox->setValue(m_N);
    return m_settingsWidget;
}

std::vector<uint> NthFrame::sampleImages(Reader *, const std::vector<unsigned int> &imageList, Progressable *receiver, volatile bool *stopped, QMap<QString, QVariant>, bool, LogFileParent *logFile)
{
    logFile->startTimer(LF_TOTAL);

    // allocate memory for keyframes
    std::vector<unsigned int> keyframes;
    keyframes.reserve(1 + imageList.size() / m_N);

    for(int i = 0; i < (int)imageList.size(); i += m_N){
        // iterate every N-th sharp image
        if(*stopped){
            // user stopped the algorithm -> return
            return std::vector<unsigned int>();
        }
        // show progress to user
        int progress = (i * 100 / imageList.size()) ;
        QMetaObject::invokeMethod(
                    receiver,
                    "slot_makeProgress",
                    Qt::DirectConnection,
                    Q_ARG(int, progress),
                    Q_ARG(QString, "getting every n-th frame"));
        keyframes.push_back(imageList[i]);    // add keyframe
    }

    logFile->stopTimer();

    // return the result
    keyframes.shrink_to_fit();
    QMetaObject::invokeMethod(receiver,
                              "slot_makeProgress",
                              Qt::DirectConnection,
                              Q_ARG(int, 100),
                              Q_ARG(QString, "Nth-Frame progress"));
    return keyframes;
}

QString NthFrame::getName() const
{
    return "NthFrame";
}

QVariant NthFrame::getBuffer()
{
    return 0;
}

QString NthFrame::getBufferName()
{
    return "Nth Frame Algorithm";
}

void NthFrame::initialize(Reader *reader)
{
    m_numFrames = reader->getPicCount();
    if (reader->getFPS() == -1) {
        m_fps = 30;
    }
    else {
        m_fps = round(reader->getFPS());
    }

    m_N = m_fps;
}

void NthFrame::setSettings(QMap<QString, QVariant> settings)
{
    QMap<QString, QVariant>::iterator iterator = settings.find(NAME_N);
    if (iterator != settings.end()) {
        m_N = iterator.value().toInt();
    }
}

QMap<QString, QVariant> NthFrame::generateSettings(Progressable *receiver, QMap<QString, QVariant> buffer, bool useCuda, volatile bool* stopped)
{
    (void) receiver;
    (void) buffer;
    (void) useCuda;
    (void) stopped;
    m_N = m_fps / 5;
    return QMap<QString, QVariant>();
}

QMap<QString, QVariant> NthFrame::getSettings()
{
   QString valueN = QString::number(m_N);
   QMap<QString, QVariant> settings;
   settings.insert(NAME_N, valueN);
   return settings;
}

void NthFrame::setSignalObject(signalObject *sigObj)
{
    m_sigObj = sigObj;
    QObject::connect(m_sigObj, SIGNAL(sig_newMetaData()), this, SLOT(slot_newMetaData()));
}

void NthFrame::slot_nChanged(int n)
{
    m_N = (unsigned int)n;
    // allocate memory for keyframes
    std::vector<uint> keyframes;
    keyframes.reserve(1 + m_numFrames / m_N);

    for(uint i = 0; i < m_numFrames; i += m_N){
        // iterate every N-th sharp image

        keyframes.push_back(i);    // add keyframe
    }
    keyframes.shrink_to_fit();
    emit updateKeyframes(keyframes);
}

void NthFrame::slot_newMetaData()
{
    if (m_spinBox) {
        m_spinBox->setValue(999);
    }

}

void NthFrame::createSettingsWidget(QWidget *parent)
{
    m_settingsWidget = new QWidget(parent);
    m_settingsWidget->setLayout(new QVBoxLayout());
    m_settingsWidget->layout()->setSpacing(0);
    m_settingsWidget->layout()->setMargin(0);

    QWidget *w = new QWidget(parent);
    w->setLayout(new QHBoxLayout(parent));
    w->layout()->setSpacing(0);
    w->layout()->setMargin(0);
    w->layout()->addWidget(new QLabel("Select N ",parent));

    m_spinBox = new QSpinBox(parent);
    m_spinBox->setMinimum(1);
    m_spinBox->setMaximum(9999);
    m_spinBox->setValue(m_fps);
    m_spinBox->setAlignment(Qt::AlignRight);
    w->layout()->addWidget(m_spinBox);
    QObject::connect(m_spinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &NthFrame::slot_nChanged);

    m_settingsWidget->layout()->addWidget(w);

    QLabel *txt = new QLabel(DESCRIPTION_TEXT);
    txt->setStyleSheet(DESCRIPTION_STYLE);
    txt->setWordWrap(true);
    m_settingsWidget->layout()->addWidget(txt);

    m_settingsWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    m_settingsWidget->adjustSize();
}
