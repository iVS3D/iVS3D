#include "nouicontroller.h"



noUIController::noUIController(QStringList arguments)
{
    m_terminal = &TerminalInteraction::instance();

    for (int i = 0; i < arguments.size(); i++) {
        QString argument = arguments[i];
        if (argument.startsWith("-auto")) {
            i++;
            m_autoPath = arguments[i];
            continue;
        }
        if (argument.startsWith("-in")) {
            i++;
            m_inputPath = arguments[i];
            continue;
        }
    }
}
int noUIController::exec()
{
    //Setup cuda
    bool useCuda = false;
    if(ApplicationSettings::instance().getCudaAvailable()){
        useCuda = ApplicationSettings::instance().getUseCuda();
    }

    TransformManager::instance().enableCuda(useCuda);

    m_dataManager = new DataManager;
    m_terminal->slot_displayMessage("Start import");
    m_dataManager->open(m_inputPath);

    int numberImages = m_dataManager->getModelInputPictures()->getPicCount();
    //Check for valid input
    if (numberImages <= 0) {
        m_terminal->slot_displayMessage("Opend no files. Please check your input path");
        QCoreApplication::quit();
        return 0;
    }

    m_terminal->slot_displayMessage("Opend " + QString::number(numberImages) + " pics");

    AutomaticExecSettings* autoSettings = new AutomaticExecSettings(nullptr, nullptr, nullptr);
    autoSettings->loadPluginList(m_autoPath);
    QStringList plugins = autoSettings->getPluginNames();
    m_terminal->slot_displayMessage("Loaded the following plugin settings");
    for (QString name : plugins) {
        m_terminal->slot_displayMessage(name);
    }
    //Check for valid settings file
    if(plugins.size() <= 0) {
        m_terminal->slot_displayMessage("Imported no settings. Please check your settings file");
        QCoreApplication::quit();
        return 0;
    }

    //Ignore boundaries
    QPoint boundaries = QPoint(0, numberImages - 1);
    m_dataManager->getModelInputPictures()->setBoundaries(boundaries);

    AlgorithmManager::instance().initializePlugins(m_dataManager->getModelInputPictures()->getReader());

    AutomaticExecutor* autoExec = new AutomaticExecutor(m_dataManager, nullptr, autoSettings, nullptr);
    autoExec->slot_startAutomaticExec();

    //Wait for execution to finish
    while (!autoExec->isFinished()) {
        QCoreApplication::processEvents( QEventLoop::AllEvents, 100 );
    }
    int keyframeCount = m_dataManager->getModelInputPictures()->getKeyframeCount();
    m_terminal->slot_displayMessage("Sampled " + QString::number(keyframeCount) + " keyframes");

    QCoreApplication::quit();
    return 0;

}


