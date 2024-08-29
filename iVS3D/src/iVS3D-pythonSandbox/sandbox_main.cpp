#include "sandbox_main.h"

SandboxMain::SandboxMain() {
    QLocale locale = qApp->property("translation").toLocale();
    QTranslator* translator = new QTranslator();
    translator->load(locale, "nth", "_", ":/translations", ".qm");
    qApp->installTranslator(translator);
}

SandboxMain::~SandboxMain() {

}

QWidget *SandboxMain::getSettingsWidget(QWidget *parent) {
    return new QWidget();
}

std::vector<uint> SandboxMain::sampleImages(const std::vector<unsigned int> &imageList, Progressable *receiver, volatile bool *stopped, bool useCuda, LogFileParent *logFile) {
    QProcess process;
    process.start("xterm", QStringList() << "-e" << "python3");
    process.waitForStarted();
    std::cout << "started python" << std::endl;
    process.waitForFinished(-1);
    std::cout << "finished python" << std::endl;
    return {};
}

QString SandboxMain::getName() const {
    return "Python-Sandbox";
}

void SandboxMain::initialize(Reader *reader, QMap<QString, QVariant> buffer, signalObject *sigObj) {

}

void SandboxMain::setSettings(QMap<QString, QVariant> settings) {

}

QMap<QString, QVariant> SandboxMain::generateSettings(Progressable *receiver, bool useCuda, volatile bool *stopped) {
    (void) receiver;
    (void) useCuda;
    (void) stopped;
    return QMap<QString, QVariant>();
}

QMap<QString, QVariant> SandboxMain::getSettings() {
    return QMap<QString, QVariant>();
}
