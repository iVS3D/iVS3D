#pragma once

#include <memory>

#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QStringList>

#include "DataManager.h"
#include "exportexecutor.h"
#include "maskstack.h"
#include "pluginthread.h"

class HeadlessController : public QObject {
    Q_OBJECT

public:
    HeadlessController(QString inputPath, QString settingsPath,
                       QString outputPath, QString logPath,
                       QStringList metadataPaths = {},
                       QObject* parent = nullptr);

private slots:
    void onSelectionFinished(const SelectionResultData& result);
    void onExportFinished(ExportResult result);

private:
    enum class StepType { Selection, Mask, Export };

    struct Step {
        StepType type;
        QString pluginName;
        QMap<QString, QVariant> settings;
        QJsonObject exportSettings;
    };

    void start();
    bool loadSettings(QString* error);
    bool loadExplicitMetadata(QString* error);
    bool parseStep(const QJsonObject& object, Step* step, QString* error) const;
    void runNextStep();
    void runSelectionStep(const Step& step);
    void runMaskStep(const Step& step);
    void runExportStep(const Step& step);
    void exitWithFail(const QString& message);
    void finish(int exitCode);
    QString requirePath(const QString& path, const QString& name) const;
    QMap<QString, QVariant> objectToVariantMap(const QJsonObject& object) const;

    QString m_inputPath;
    QString m_settingsPath;
    QString m_outputPath;
    QString m_logPath;
    QStringList m_metadataPaths;

    DataManager m_dataManager;
    std::shared_ptr<PluginThread> m_pluginThread;
    std::shared_ptr<MaskStack> m_maskStack;
    ExportExecutor* m_exportExecutor = nullptr;
    std::shared_ptr<LogFile> m_exportLog;

    QVector<Step> m_steps;
    int m_stepIndex = 0;
    QString m_currentSelectionPlugin;
};
