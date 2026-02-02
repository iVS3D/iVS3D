#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QWidget>
#include <QLabel>
#include <QTextEdit>
#include <QTime>
#include <QDebug>
#include <QCommandLineParser>
#include <QFileInfo>

#include <ModelSettingsWidget.h>
#include <ModelManager.h>

class ExampleMainWindow : public QMainWindow {
    Q_OBJECT

public:
    ExampleMainWindow(const QString& modelDirectory, const QString& filter) 
        : m_modelManager(modelDirectory) {
        setWindowTitle("iVS3D Detection Plugin GUI Example");
        resize(800, 600);
        m_modelManager.setNameFilter(filter);
        // Refresh to load models
        m_modelManager.refresh();

        // Create central widget with layout
        auto* centralWidget = new QWidget(this);
        auto* layout = new QVBoxLayout(centralWidget);

        // Add title
        auto* titleLabel = new QLabel("<h2>ObjectDetection Settings Widget Example</h2>");
        layout->addWidget(titleLabel);

        // Create the settings widget
        m_settingsWidget = new ModelSettingsWidget(m_modelManager, this);
        layout->addWidget(m_settingsWidget);

        // Create log output area
        auto* logLabel = new QLabel("<b>Event Log:</b>");
        layout->addWidget(logLabel);
        
        m_logOutput = new QTextEdit();
        m_logOutput->setReadOnly(true);
        m_logOutput->setMaximumHeight(200);
        layout->addWidget(m_logOutput);

        setCentralWidget(centralWidget);

        // Connect signals
        connect(m_settingsWidget, &ModelSettingsWidget::modelChanged,
                this, &ExampleMainWindow::onModelChanged);
        
        connect(m_settingsWidget, &ModelSettingsWidget::classSelectionChanged,
                this, &ExampleMainWindow::onClassSelectionChanged);

        // Log initial state
        logMessage("Application started");
        logMessage(QString("Model directory: %1").arg(modelDirectory));
        logMessage(QString("Found %1 models").arg(m_modelManager.models().size()));
    }

private slots:
    void onModelChanged(const QString& modelName, bool isValid) {
        if (isValid) {
            logMessage(QString("✓ Model changed to: <b>%1</b> (valid)").arg(modelName));
            
            // Try to activate the model
            auto result = m_modelManager.activateModel(modelName);
            if (result.has_value()) {
                auto model = result.value();
                const auto& classes = model.config->getClasses();
                logMessage(QString("  → Model loaded successfully with %1 classes").arg(classes.size()));
            } else {
                logMessage(QString("  ⚠ Failed to activate model"));
            }
        } else {
            logMessage(QString("⚠ Model changed to: <b>%1</b> (invalid)").arg(modelName));
            logMessage(QString("  → Error: %1").arg(m_modelManager.modelError(modelName)));
        }
    }

    void onClassSelectionChanged(const QVector<uint>& selectedClassIds) {
        logMessage(QString("Class selection changed: %1 classes selected").arg(selectedClassIds.size()));
        
        if (!selectedClassIds.isEmpty()) {
            QStringList idList;
            for (uint id : selectedClassIds) {
                idList.append(QString::number(id));
            }
            logMessage(QString("  → Selected class IDs: [%1]").arg(idList.join(", ")));
        }
    }

private:
    void logMessage(const QString& message) {
        QString timestamp = QTime::currentTime().toString("hh:mm:ss");
        m_logOutput->append(QString("[%1] %2").arg(timestamp, message));
    }

    ModelManager m_modelManager;
    ModelSettingsWidget* m_settingsWidget = nullptr;
    QTextEdit* m_logOutput = nullptr;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    // Setup command line parser
    QCommandLineParser parser;
    parser.setApplicationDescription("Model Settings Widget Example");
    parser.addHelpOption();
    
    parser.addPositionalArgument("model-dir", 
                                 "Directory containing model configuration files (optional)");

    parser.addOption(QCommandLineOption("filter", 
                                        "Filter models by name (wildcard supported).", 
                                        "regex"));
    
    parser.process(app);
    
    // Determine model directory
    QString modelDirectory;
    QStringList args = parser.positionalArguments();
    
    if (!args.isEmpty()) {
        modelDirectory = args.first();
        qDebug() << "Using model directory from command line:" << modelDirectory;
    } else {
        // Default to testdata directory
        modelDirectory = QFileInfo(__FILE__).dir().absolutePath() + "/../tests/testdata";
        qDebug() << "No model directory specified, using default testdata:" << modelDirectory;
    }

    QString nameFilter = "";
    if (parser.isSet("filter")) {
        nameFilter = parser.value("filter");
    }


    ExampleMainWindow mainWindow(modelDirectory, nameFilter);
    mainWindow.show();

    return app.exec();
}

#include "gui_example.moc"