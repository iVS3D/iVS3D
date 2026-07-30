#include "headlesscontroller.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QMetaObject>
#include <QTextStream>
#include <iostream>
#include <utility>

#include "applicationsettings.h"
#include "logmanager.h"
#include "pluginmanager.h"
#include "stringcontainer.h"

HeadlessController::HeadlessController(QString inputPath, QString settingsPath,
                                       QString outputPath, QString logPath,
                                       QStringList metadataPaths,
                                       QObject* parent)
    : QObject(parent),
      m_inputPath(std::move(inputPath)),
      m_settingsPath(std::move(settingsPath)),
      m_outputPath(std::move(outputPath)),
      m_logPath(std::move(logPath)),
      m_metadataPaths(std::move(metadataPaths)) {
    start();
}

void HeadlessController::start() {
    const QString inputError = requirePath(m_inputPath, "input");
    if (!inputError.isEmpty()) {
        exitWithFail(inputError);
        return;
    }

    const QString settingsError = requirePath(m_settingsPath, "settings");
    if (!settingsError.isEmpty()) {
        exitWithFail(settingsError);
        return;
    }

    for (const QString& metadataPath : m_metadataPaths) {
        const QString metadataError = requirePath(metadataPath, "metadata");
        if (!metadataError.isEmpty()) {
            exitWithFail(metadataError);
            return;
        }
    }

    if (!m_logPath.isEmpty()) {
        LogManager::instance().setLogDirectory(m_logPath);
    }

    QString error;
    if (!loadSettings(&error)) {
        exitWithFail(error);
        return;
    }

    if (m_outputPath.isEmpty() && !m_outputPathInSteps) {
        exitWithFail("Missing output path. Use -o/--out in headless mode.");
        return;
    }

    PluginManager::instance().enableCuda(
        ApplicationSettings::instance().getUseCuda());
    m_pluginThread = PluginManager::instance().getPluginThread();
    m_maskStack = std::make_shared<MaskStack>();

    const int imageCount = m_dataManager.open(m_inputPath);
    if (imageCount <= 0 || !m_dataManager.getModelInputPictures() ||
        !m_dataManager.getModelInputPictures()->getReader()) {
        exitWithFail(QString("Failed to open input: %1").arg(m_inputPath));
        return;
    }

    auto* reader = m_dataManager.getModelInputPictures()->getReader();
    m_pluginThread->onInputLoaded(reader);

    if (!loadExplicitMetadata(&error)) {
        exitWithFail(error);
        return;
    }

    if (auto* md = reader->getMetaData()) {
        m_pluginThread->onMetaDataLoaded(PLUG::InputMetaData{md});
    }
    m_pluginThread->onSelectedImagesChanged(
        m_dataManager.getModelInputPictures()->getAllKeyframes(true));

    runNextStep();
}

bool HeadlessController::loadSettings(QString* error) {
    QFile file(m_settingsPath);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        *error =
            QString("Failed to open settings file: %1").arg(m_settingsPath);
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument doc =
        QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        *error =
            QString("Invalid settings JSON: %1").arg(parseError.errorString());
        return false;
    }

    const QJsonValue stepsValue = doc.object().value("steps");
    if (!stepsValue.isArray()) {
        *error = "Headless settings must contain a 'steps' array.";
        return false;
    }

    const QJsonArray steps = stepsValue.toArray();
    if (steps.isEmpty()) {
        *error = "Headless settings must contain at least one step.";
        return false;
    }

    for (const QJsonValue& value : steps) {
        if (!value.isObject()) {
            *error = "Every headless settings step must be an object.";
            return false;
        }

        Step step;
        if (!parseStep(value.toObject(), &step, error)) {
            return false;
        }
        m_steps.append(step);
    }

    return true;
}

bool HeadlessController::loadExplicitMetadata(QString* error) {
    if (m_metadataPaths.isEmpty()) {
        return true;
    }

    const int metadataCount =
        m_dataManager.getModelInputPictures()->loadMetaData(m_metadataPaths);
    if (metadataCount <= 0) {
        *error =
            "No metadata features were detected in the provided metadata "
            "files.";
        return false;
    }

    std::cout << "Loaded " << metadataCount << " metadata feature"
              << (metadataCount == 1 ? "" : "s") << "." << std::endl;
    return true;
}

bool HeadlessController::parseStep(const QJsonObject& object, Step* step,
                                   QString* error) {
    const QString type = object.value("type").toString().toLower();
    if (type == "selection") {
        step->type = StepType::Selection;
        step->pluginName = object.value("plugin").toString();
        if (step->pluginName.isEmpty()) {
            *error = "Selection step requires a plugin name.";
            return false;
        }
        step->settings =
            objectToVariantMap(object.value("settings").toObject());
        return true;
    }

    if (type == "mask") {
        step->type = StepType::Mask;
        step->pluginName = object.value("plugin").toString();
        if (step->pluginName.isEmpty()) {
            *error = "Mask step requires a plugin name.";
            return false;
        }
        step->settings =
            objectToVariantMap(object.value("settings").toObject());
        return true;
    }

    if (type == "export") {
        step->type = StepType::Export;
        step->exportSettings = object.value("settings").toObject();
        m_outputPathInSteps = step->exportSettings.contains("path");
        return true;
    }

    *error = QString("Unsupported headless step type: %1").arg(type);
    return false;
}

void HeadlessController::runNextStep() {
    if (m_stepIndex >= m_steps.size()) {
        finish(0);
        return;
    }

    const Step step = m_steps.at(m_stepIndex++);
    switch (step.type) {
        case StepType::Selection:
            runSelectionStep(step);
            break;
        case StepType::Mask:
            runMaskStep(step);
            break;
        case StepType::Export:
            runExportStep(step);
            break;
    }
}

void HeadlessController::runSelectionStep(const Step& step) {
    if (!PluginManager::instance().hasSelectionPlugin(step.pluginName)) {
        QString msg = QString(
                          "Plugin does not support selection: %1 \n Choose one "
                          "of the following:")
                          .arg(step.pluginName);
        for (QString name : PluginManager::instance().getPluginNames()) {
            if (!PluginManager::instance().hasSelectionPlugin(name)) continue;
            msg += QString("\n- %1").arg(name);
        }

        exitWithFail(msg);
        return;
    }

    const auto applyResult = PluginManager::instance().applyPluginSettings(
        step.pluginName, step.settings);
    if (!applyResult) {
        exitWithFail(QString("Failed to apply settings for %1: %2")
                         .arg(step.pluginName, applyResult.error().message));
        return;
    }

    PLUG::SelectionData data;
    auto* mip = m_dataManager.getModelInputPictures();
    data.selectedIndices = mip->getAllKeyframes(true);
    data.roi = mip->getReaderParams()->getRoi();
    data.workingResolution = mip->getReaderParams()->getWorkingResolution();
    data.reader = mip->getReader();

    auto selectionLog =
        LogManager::instance().createLogFile(step.pluginName, true);
    if (selectionLog) {
        selectionLog->setSettings(
            PluginManager::instance().getPluginSettings(step.pluginName));
        selectionLog->setInputInfo(data.selectedIndices);
        data.logFile = selectionLog;
    }

    m_currentSelectionPlugin = step.pluginName;
    connect(m_pluginThread.get(), &PluginThread::selectionFinished, this,
            &HeadlessController::onSelectionFinished, Qt::UniqueConnection);
    m_pluginThread->requestSelection(step.pluginName, data);
}

void HeadlessController::onSelectionFinished(
    const SelectionResultData& result) {
    if (result.pluginName != m_currentSelectionPlugin) {
        return;
    }

    disconnect(m_pluginThread.get(), &PluginThread::selectionFinished, this,
               &HeadlessController::onSelectionFinished);

    if (result.cancelled) {
        exitWithFail(QString("Selection cancelled: %1").arg(result.pluginName));
        return;
    }

    if (!result.success) {
        exitWithFail(QString("Selection failed for %1: %2")
                         .arg(result.pluginName, result.errorMessage));
        return;
    }

    m_dataManager.getModelInputPictures()->updateMIP(result.selectedIndices);
    runNextStep();
}

void HeadlessController::runMaskStep(const Step& step) {
    if (!PluginManager::instance().hasMaskPlugin(step.pluginName)) {
        exitWithFail(QString("Plugin does not support mask generation: %1")
                         .arg(step.pluginName));
        return;
    }

    const auto applyResult = PluginManager::instance().applyPluginSettings(
        step.pluginName, step.settings);
    if (!applyResult) {
        exitWithFail(QString("Failed to apply settings for %1: %2")
                         .arg(step.pluginName, applyResult.error().message));
        return;
    }

    auto* mip = m_dataManager.getModelInputPictures();
    MaskRecord record;
    record.pluginName = step.pluginName;
    record.pluginSettings =
        PluginManager::instance().getPluginSettings(step.pluginName);
    record.pluginSettingsString =
        PluginManager::instance().getPluginSettingsString(step.pluginName);
    record.workingResolution = mip->getReaderParams()->getWorkingResolution();
    record.roi = mip->getReaderParams()->getUseRoi()
                     ? mip->getReaderParams()->getRoi()
                     : ROI();
    m_maskStack->addRecord(record);
    runNextStep();
}

void HeadlessController::runExportStep(const Step& step) {
    QDir exportDir;
    QString path = m_outputPath;
    if (step.exportSettings.contains("path")) {
        path = step.exportSettings.value("path").toString(path);
    }
    path.replace("\\", "/");
    if (path.endsWith("/")) {
        path.chop(1);
    }
    if (!exportDir.mkpath(path)) {
        exitWithFail(
            QString("Failed to create output directory: %1").arg(path));
        return;
    }

    QString outputName = QFileInfo(path).fileName();
    QString imagePath = path;
    if (!imagePath.endsWith("/images")) {
        imagePath += "/images";
    } else {
        QDir parent(path);
        parent.cdUp();
        outputName = parent.dirName();
        path = parent.absolutePath();
        imagePath = path + "/images";
    }
    QDir().mkpath(imagePath);

    auto* mip = m_dataManager.getModelInputPictures();
    auto readerParams = mip->getReaderParams();
    Resolution exportResolution = readerParams->getOriginalResolution();
    const QString resolutionString =
        step.exportSettings.value("resolution").toString();
    if (!resolutionString.isEmpty()) {
        Resolution requested;
        if (!requested.fromString(resolutionString)) {
            exitWithFail(
                QString("Invalid export resolution: %1").arg(resolutionString));
            return;
        }
        exportResolution = requested;
    }

    ExportConfig config;
    config.name = outputName;
    config.destination = imagePath;
    config.format = step.exportSettings.value("format").toString("png");
    config.original_resolution = readerParams->getOriginalResolution();
    config.working_resolution = readerParams->getWorkingResolution();
    config.export_resolution = exportResolution;
    config.copy_images = false;
    config.maskStack = m_maskStack.get();
    if (readerParams->getUseRoi() && !readerParams->getRoi().isDefault()) {
        config.roi = readerParams->getRoi();
    }

    m_dataManager.createProject(outputName,
                                path + "/" + outputName + "-project.json");

    m_exportLog = LogManager::instance().createLogFile("Export", false);
    if (m_exportLog) {
        QMap<QString, QVariant> settings;
        settings.insert(stringContainer::OutputPath, path);
        settings.insert(stringContainer::Resolution,
                        exportResolution.toString());
        settings.insert(stringContainer::OutputFormat, config.format);
        settings.insert(stringContainer::UseROI, config.roi.has_value());
        m_exportLog->setSettings(settings);
    }

    m_exportExecutor = new ExportExecutor(this, &m_dataManager, m_pluginThread);
    connect(m_exportExecutor, &ExportExecutor::sig_exportFinished, this,
            &HeadlessController::onExportFinished);
    connect(m_exportExecutor, &ExportExecutor::sig_message, this,
            [](const QString& message) {
                std::cout << message.toStdString() << std::endl;
            });
    connect(m_exportExecutor, &ExportExecutor::sig_warning, this,
            [](const QString& warning) {
                std::cerr << warning.toStdString() << std::endl;
            });
    m_exportExecutor->startExport(config, m_exportLog);
}

void HeadlessController::onExportFinished(ExportResult result) {
    if (result.type == ExportResultType::Failed) {
        exitWithFail(QString("Export failed: %1").arg(result.errorMessage));
        return;
    }

    if (result.type == ExportResultType::Aborted) {
        exitWithFail("Export aborted.");
        return;
    }

    if (m_exportExecutor) {
        m_exportExecutor->deleteLater();
        m_exportExecutor = nullptr;
    }
    runNextStep();
}

void HeadlessController::exitWithFail(const QString& message) {
    std::cerr << "[ERROR] " << message.toStdString() << std::endl;
    finish(1);
}

void HeadlessController::finish(int exitCode) {
    QMetaObject::invokeMethod(
        QCoreApplication::instance(),
        [exitCode]() { QCoreApplication::exit(exitCode); },
        Qt::QueuedConnection);
}

QString HeadlessController::requirePath(const QString& path,
                                        const QString& name) const {
    if (path.isEmpty()) {
        return QString("Missing %1 path.").arg(name);
    }
    if (!QFileInfo::exists(path)) {
        return QString("%1 path does not exist: %2").arg(name, path);
    }
    return {};
}

QMap<QString, QVariant> HeadlessController::objectToVariantMap(
    const QJsonObject& object) const {
    return object.toVariantMap();
}
