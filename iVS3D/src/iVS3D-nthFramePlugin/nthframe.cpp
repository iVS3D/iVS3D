#include "nthframe.h"

NthFrame::NthFrame()
{
    QLocale locale = qApp->property("translation").toLocale();
    QTranslator* translator = new QTranslator();
    translator->load(locale, "nth", "_", ":/translations", ".qm");
    qApp->installTranslator(translator);
    m_N = 1;    // N initialized to stepwidth 1
    m_numFrames = 0;
    m_settingsWidget = nullptr;
    m_keepLonely = true;
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

std::vector<uint> NthFrame::sampleImages(const std::vector<unsigned int> &imageList, Progressable *receiver, volatile bool *stopped, bool, LogFileParent *logFile)
{
    logFile->startTimer(LF_TOTAL);
    int expectedKfDistance = roundf((float)m_N / ((float)imageList.size() / m_numFrames));
    qDebug() << "Expected Distance: " << expectedKfDistance;
    qDebug() << "N: " << m_N;

    // allocate memory for keyframes
    std::vector<unsigned int> keyframes;
    keyframes.reserve(1 + imageList.size() / m_N);

    uint lastKfIndex = imageList[0];
    uint ctr = UINT_MAX-2;

    for(int i = 0; i < (int)imageList.size(); i++){
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
                    Q_ARG(QString, tr("getting every n-th frame")));

        ctr++;

        if(ctr >= m_N || (m_keepLonely && (imageList[i] - lastKfIndex > expectedKfDistance))){
            keyframes.push_back(imageList[i]);    // add keyframe
            lastKfIndex = imageList[i];
            ctr = 0;
        }

    }

    logFile->stopTimer();

    // return the result
    keyframes.shrink_to_fit();
    QMetaObject::invokeMethod(receiver,
                              "slot_makeProgress",
                              Qt::DirectConnection,
                              Q_ARG(int, 100),
                              Q_ARG(QString, tr("Nth-Frame progress")));
    return keyframes;
}

QString NthFrame::getName() const
{
    return tr("NthFrame");
}

void NthFrame::initialize(Reader *reader, QMap<QString, QVariant>, signalObject *so)
{
    m_reader = reader;
    m_numFrames = reader->getPicCount();
    if (reader->getFPS() == -1) {
        m_fps = 30;
    }
    else {
        m_fps = round(reader->getFPS());
    }

    m_N = m_fps;
    if(m_settingsWidget)
        m_spinBox->setValue(m_N);
}

void NthFrame::setSettings(QMap<QString, QVariant> settings)
{
    QMap<QString, QVariant>::iterator iterator = settings.find(NAME_N);
    if (iterator != settings.end()) {
        m_N = iterator.value().toInt();
        if(m_settingsWidget)
            m_spinBox->setValue(m_N);
    }
}

QMap<QString, QVariant> NthFrame::generateSettings(Progressable *receiver, bool useCuda, volatile bool* stopped)
{
    (void) receiver;
    (void) useCuda;
    (void) stopped;
    m_N = m_fps / 5;
    if(m_settingsWidget)
        m_spinBox->setValue(m_N);
    return QMap<QString, QVariant>();
}

QMap<QString, QVariant> NthFrame::getSettings()
{
   QString valueN = QString::number(m_N);
   QMap<QString, QVariant> settings;
   settings.insert(NAME_N, valueN);
   return settings;
}

void NthFrame::slot_nChanged(int n)
{
    m_N = (unsigned int)n;
}

void NthFrame::slot_checkboxToggled(bool checked)
{
    m_keepLonely = checked;
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
    w->layout()->addWidget(new QLabel(tr("Select N "),parent));

    m_spinBox = new QSpinBox(parent);
    m_spinBox->setMinimum(1);
    m_spinBox->setMaximum(9999);
    m_spinBox->setValue(m_fps);
    m_spinBox->setAlignment(Qt::AlignRight);
    w->layout()->addWidget(m_spinBox);
    QObject::connect(m_spinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &NthFrame::slot_nChanged);

    m_settingsWidget->layout()->addWidget(w);

    QLabel *txt = new QLabel(tr("Every Nth frame is selected as keyframe."));
    txt->setStyleSheet(DESCRIPTION_STYLE);
    txt->setWordWrap(true);
    m_settingsWidget->layout()->addWidget(txt);

    m_checkBox = new QCheckBox(tr("keep lonely keyframes"), parent);
    m_checkBox->setChecked(m_keepLonely);
    connect(m_checkBox, &QCheckBox::toggled, this, &NthFrame::slot_checkboxToggled);
    m_settingsWidget->layout()->addWidget(m_checkBox);

    QLabel *txt2 = new QLabel(tr("When selecting strictly every Nth frame, lonely keyframes or small batches are unlikely to get selected for larger N. Select additional keyframes in thinly populated areas."));
    txt2->setStyleSheet(DESCRIPTION_STYLE);
    txt2->setWordWrap(true);
    m_settingsWidget->layout()->addWidget(txt2);

    m_settingsWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    m_settingsWidget->adjustSize();
}
