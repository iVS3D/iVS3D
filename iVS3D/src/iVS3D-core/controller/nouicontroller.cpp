#include "nouicontroller.h"



noUIController::noUIController(QStringList arguments)
{
    m_terminal = &TerminalInteraction::instance();
    QCommandLineParser parser;
    parser.addHelpOption();
    QCommandLineOption inputPath(QStringList() << "i" << "in", "Load input from <path>.", "path");
    QCommandLineOption autoPath(QStringList() << "a" << "auto", "Load settings from <path>.", "path");
    QCommandLineOption outputPath(QStringList() << "o" << "out", "Save result to <path>  -  OPTIONAL (will overwrite path in the auto settings file).", "path");
    parser.addOption(autoPath);
    parser.addOption(inputPath);
    parser.addOption(outputPath);
    parser.process(arguments);

    m_outputPath = parser.value(outputPath);
    m_autoPath = parser.value(autoPath);
    m_inputPath = parser.value(inputPath);

}
int noUIController::exec()
{
    //Check if input and auto fiel are provided
    if (m_autoPath.compare("") == 0 || m_inputPath.compare("") == 0 ) {
        m_terminal->slot_displayMessage("Auto settings file (-a) AND input (-i) need to be provided.");
        QCoreApplication::quit();
        return 0;
    }
    //Check if export will be overwritten
    bool overwriteExport = m_autoPath.compare("") != 0;

    if (overwriteExport) {
        qApp->setProperty(stringContainer::OverwriteExport, m_outputPath);
    }

    //Setup cuda
    bool useCuda = false;
    if(ApplicationSettings::instance().getCudaAvailable()){
        useCuda = ApplicationSettings::instance().getUseCuda();
    }

    TransformManager::instance().enableCuda(useCuda);

    QElapsedTimer timer;
    timer.start();

    m_dataManager = new DataManager;
    m_terminal->slot_displayMessage("Start import");
    m_dataManager->open(m_inputPath);

    int numberImages = m_dataManager->getModelInputPictures()->getPicCount();
    //Check for valid input
    if (numberImages <= 0) {
        m_terminal->slot_displayMessage("Opend no files. Please check your input path.");
        QCoreApplication::quit();
        return 0;
    }

    m_terminal->slot_displayMessage("Opend " + QString::number(numberImages) + " images");
    //Load auto settings file
    AutomaticExecSettings* autoSettings = new AutomaticExecSettings(nullptr, nullptr, nullptr);
    autoSettings->loadPluginList(m_autoPath);
    QStringList plugins = autoSettings->getPluginNames();
    m_terminal->slot_displayMessage("Loaded the following plugin settings:");
    for (QString name : plugins) {
        m_terminal->slot_displayMessage(name);
        //Print when export path will be overwritten
        if (name.left(stringContainer::Export.size()).compare(stringContainer::Export) == 0 && overwriteExport) {
            m_terminal->slot_displayMessage("\nEXPORT PATH WILL BE OVERWRITTEN WITH " + m_outputPath);
        }
    }
    //Check for valid settings file
    if(plugins.size() <= 0) {
        m_terminal->slot_displayMessage("Imported no settings. Please check your settings file.");
        QCoreApplication::quit();
        return 0;
    }

    m_terminal->slot_displayMessage("\n### Start Computation ###\n");
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
    m_terminal->slot_displayMessage("Sampled " + QString::number(keyframeCount) + " keyframes.");

    m_terminal->slot_displayMessage("\n### Finished Computation ###\n");
    m_terminal->slot_displayMessage("Finished in " + QString::number(timer.elapsed()/1000) + "s");

    QCoreApplication::quit();
    return 0;
}


