#include "nouicontroller.h"



noUIController::noUIController(QString inputPath, QString settingsPath, QString outputPath, QString logPath)
{
    m_terminal = &TerminalInteraction::instance();
    m_outputPath = outputPath;
    m_autoPath = settingsPath;
    m_inputPath = inputPath;

    if (logPath != nullptr && !logPath.isEmpty()) {
        LogManager::instance().setLogDirectory(logPath);
    }
}
int noUIController::exec()
{
    //Check if input and auto fiel are provided
    if (m_autoPath.compare("") == 0 || m_inputPath.compare("") == 0 || m_outputPath.compare("") == 0) {
        m_terminal->slot_displayMessage(tr("Auto settings file (-a), input (-i) and output (-o) need to be provided."));
        QCoreApplication::quit();
        return 0;
    }

    qApp->setProperty(stringContainer::OverwriteExport, m_outputPath);

    //Setup cuda
    bool useCuda = false;
    if(ApplicationSettings::instance().getCudaAvailable()){
        useCuda = ApplicationSettings::instance().getUseCuda();
    }

    TransformManager::instance().enableCuda(useCuda);

    QElapsedTimer timer;
    timer.start();

    m_dataManager = new DataManager;
    m_terminal->slot_displayMessage(tr("Start import"));
    m_dataManager->open(m_inputPath);

    int numberImages = m_dataManager->getModelInputPictures()->getPicCount();
    //Check for valid input
    if (numberImages <= 0) {
        m_terminal->slot_displayMessage(tr("Opend no files. Please check your input path."));
        QCoreApplication::quit();
        return 0;
    }

    m_terminal->slot_displayMessage(tr("Opend ") + QString::number(numberImages) + tr(" images"));
    //Load auto settings file
    AutomaticExecSettings* autoSettings = new AutomaticExecSettings();
    autoSettings->loadPluginList(m_autoPath);
    QStringList plugins = autoSettings->getPluginNames();
    m_terminal->slot_displayMessage(tr("Loaded the following plugin settings:"));
    for (QString name : plugins) {
        m_terminal->slot_displayMessage(name);
    }
    //Check for valid settings file
    if(plugins.size() <= 0) {
        m_terminal->slot_displayMessage(tr("Imported no settings. Please check your settings file."));
        QCoreApplication::quit();
        return 0;
    }

    m_terminal->slot_displayMessage(tr("\n### Start Computation ###\n"));
    //Ignore boundaries
    QPoint boundaries = QPoint(0, numberImages - 1);
    m_dataManager->getModelInputPictures()->setBoundaries(boundaries);

    AlgorithmManager::instance().initializePlugins(m_dataManager->getModelInputPictures()->getReader(), m_dataManager->getModelAlgorithm()->getPluginBuffer());

    AutomaticExecutor* autoExec = new AutomaticExecutor(m_dataManager, autoSettings);


    autoExec->slot_startAutomaticExec();

    //Wait for execution to finish
    while (!autoExec->isFinished()) {
        QCoreApplication::processEvents( QEventLoop::AllEvents, 100 );
    }
    int keyframeCount = m_dataManager->getModelInputPictures()->getKeyframeCount(true);
    m_terminal->slot_displayMessage(tr("Sampled ") + QString::number(keyframeCount) +tr(" keyframes."));

    m_terminal->slot_displayMessage(tr("\n### Finished Computation ###\n"));
    m_terminal->slot_displayMessage(tr("Finished in ") + QString::number(timer.elapsed()/1000) + tr("s"));

    QCoreApplication::quit();
    return 0;
}


