#include "pythonsandbox.h" 

PythonSandbox::PythonSandbox() 
{ 
    QTranslator* translator = new QTranslator(); 
    translator->load(QLocale::system(), "pythonsandbox", "_", ":/translations", ".qm"); 
    qApp->installTranslator(translator); 
    m_survivalProbability = 1.0;
    m_settingsWidget = nullptr; 
} 

PythonSandbox::~PythonSandbox() 
{ 

} 

QWidget* PythonSandbox::getSettingsWidget(QWidget *parent) 
{ 
    if(!m_settingsWidget){ 
        createSettingsWidget(parent); 
    } 
    m_spinBox->setValue(m_survivalProbability);
    return m_settingsWidget; 
} 

std::vector<uint> PythonSandbox::sampleImages(const std::vector<unsigned int> &imageList, Progressable *receiver, volatile bool *stopped, bool, LogFileParent *logFile) 
{ 
    // make an log file entry to measure performance (computation time in this routine)
    logFile->startTimer(LF_TOTAL); 

    // allocate memory for keyframes 
    uint numExpectedKeyframes = ceil(imageList.size() * m_survivalProbability);
    std::vector<unsigned int> keyframes; 
    keyframes.reserve(numExpectedKeyframes);

    // iterate the image list
    for(int i = 0; i < (int)imageList.size(); i++){

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
                    Q_ARG(QString, tr("processing..."))); 

        // decide if we keep the image as keyframe, based on a random number
        // and our survival probability
        if (QRandomGenerator::global()->bounded(1.0) < m_survivalProbability){
            keyframes.push_back(imageList[i]);    // add keyframe
        }

    } 

    // stop the time capture
    logFile->stopTimer(); 

    // return the result 
    keyframes.shrink_to_fit(); 
    QMetaObject::invokeMethod(receiver, 
                              "slot_makeProgress", 
                              Qt::DirectConnection, 
                              Q_ARG(int, 100), 
                              Q_ARG(QString, tr("Done!"))); 
    return keyframes; 
} 

QString PythonSandbox::getName() const 
{ 
    return tr("PythonSandbox"); 
} 

void PythonSandbox::initialize(Reader *reader, QMap<QString, QVariant>, signalObject *so) 
{ 
    // this routine runs if new images have been loaded, it should be performant
    // as it is executed on image load for every plugin

    m_signalObject = so; // this is a link to the core which provides updates
    m_reader = reader; // this is our reader for the images
    m_survivalProbability = 1.0; // we could set the initial survival probability based on the images
    if(m_settingsWidget) 
        m_spinBox->setValue(m_survivalProbability);
} 

void PythonSandbox::setSettings(QMap<QString, QVariant> settings) 
{ 
    QMap<QString, QVariant>::iterator iterator = settings.find(NAME_SURVIVAL_PROBABILITY);
    if (iterator != settings.end()) { 
        m_survivalProbability = iterator.value().toDouble();
        if(m_settingsWidget) 
            m_spinBox->setValue(m_survivalProbability);
    } 
} 

QMap<QString, QVariant> PythonSandbox::generateSettings(Progressable *receiver, bool useCuda, volatile bool* stopped) 
{ 
    // this routine runs if the user activly wants to generate optimal settings for the given images. It can be
    // slow or heavy computation here, the user is prompted with an progress bar.
    (void) receiver; 
    (void) useCuda; 
    (void) stopped; 
    m_survivalProbability = 1.0;
    if(m_settingsWidget) 
        m_spinBox->setValue(m_survivalProbability);

    // return the settings we computed to store them persistently in the core and project file.
    QString value = QString::number(m_survivalProbability);
    QMap<QString, QVariant> settings;
    settings.insert(NAME_SURVIVAL_PROBABILITY, value);
    return settings;
} 

QMap<QString, QVariant> PythonSandbox::getSettings() 
{ 
   QString value = QString::number(m_survivalProbability);
   QMap<QString, QVariant> settings; 
   settings.insert(NAME_SURVIVAL_PROBABILITY, value);
   return settings; 
} 

void PythonSandbox::slot_survivalProbChanged(double probability)
{ 
    m_survivalProbability = probability;
} 

void PythonSandbox::createSettingsWidget(QWidget *parent) 
{ 
    m_settingsWidget = new QWidget(parent); 
    m_settingsWidget->setLayout(new QVBoxLayout()); 
    m_settingsWidget->layout()->setSpacing(0); 
    m_settingsWidget->layout()->setMargin(0); 

    QWidget *w = new QWidget(parent); 
    w->setLayout(new QHBoxLayout(parent)); 
    w->layout()->setSpacing(0); 
    w->layout()->setMargin(0); 
    w->layout()->addWidget(new QLabel(tr("Select survival probability"),parent));

    m_spinBox = new QDoubleSpinBox(parent);
    m_spinBox->setMinimum(0.0);
    m_spinBox->setMaximum(1.0);
    m_spinBox->setSingleStep(0.01);
    m_spinBox->setValue(m_survivalProbability);
    m_spinBox->setAlignment(Qt::AlignRight); 
    w->layout()->addWidget(m_spinBox); 
    QObject::connect(m_spinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &PythonSandbox::slot_survivalProbChanged);

    m_settingsWidget->layout()->addWidget(w); 

    QLabel *txt = new QLabel(tr("The probability for each keyframe to be selected as keyframe again."));
    txt->setStyleSheet(DESCRIPTION_STYLE); 
    txt->setWordWrap(true); 
    m_settingsWidget->layout()->addWidget(txt); 

    m_settingsWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum); 
    m_settingsWidget->adjustSize(); 
} 