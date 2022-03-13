#include "applicationsettings.h"


ApplicationSettings::ApplicationSettings()
{
    //Set default values
    m_createLogs = true;
    m_useCuda = true;
    loadSettings();
    m_activeStyle = m_darkStyle;
}

void ApplicationSettings::loadSettings()
{
    QSettings settings("Fraunhofer", "iVS3D");
    m_standardInputPath = settings.value(jsonEnum::standardInputPathIdentifier).value<QString>();
    m_darkStyle = settings.value(jsonEnum::darkStyleIdentifier).value<bool>();
    m_useCuda = settings.value(jsonEnum::useCudaIdentifier).value<bool>();
    QVariantMap reconstructMap = settings.value(jsonEnum::reconstructSoftwareIdentifier).value<QVariantMap>();
    QMapIterator<QString, QVariant> mapIt(reconstructMap);
    m_reconstructPath.clear();
    while (mapIt.hasNext()) {
        mapIt.next();
        m_reconstructPath.insert(mapIt.key(), mapIt.value().toString());
    }
    m_createLogs = settings.value(jsonEnum::createLogsIdentifier).value<bool>();
}

void ApplicationSettings::saveSettings()
{
    QSettings settings("Fraunhofer", "iVS3D");
    settings.setValue(jsonEnum::standardInputPathIdentifier, m_standardInputPath);
    settings.setValue(jsonEnum::darkStyleIdentifier, m_darkStyle);
    settings.setValue(jsonEnum::useCudaIdentifier, m_useCuda);
    QVariantMap reconstructMap;
    QMapIterator<QString, QString> mapIt(m_reconstructPath);
    while (mapIt.hasNext()) {
        mapIt.next();
        reconstructMap.insert(mapIt.key(), mapIt.value());
    }
    settings.setValue(jsonEnum::reconstructSoftwareIdentifier, reconstructMap);
    settings.setValue(jsonEnum::createLogsIdentifier, m_createLogs);
}


ApplicationSettings &ApplicationSettings::instance()
{
    static ApplicationSettings INSTANCE;
    return INSTANCE;
}

void ApplicationSettings::addReconstructPath(QString name, QString path)
{
    m_reconstructPath.insert(name, path);
    saveSettings();
}

bool ApplicationSettings::removeReconstructPath(QString name)
{
    return m_reconstructPath.remove(name);
}



void ApplicationSettings::setStandardInputPath(const QString &standardInput) {
    this->m_standardInputPath = standardInput;
    saveSettings();
    return;
}


void ApplicationSettings::setDarkStyle(bool dark)
{
    m_darkStyle = dark;
    saveSettings();
}


void ApplicationSettings::setUseCuda(bool useCuda)
{
    m_useCuda = useCuda;
    saveSettings();
}

void ApplicationSettings::setCreateLogs(bool createLogs)
{
    m_createLogs = createLogs;
    saveSettings();
}

QMap<QString, QString> ApplicationSettings::getReconstructPath(){
    return this->m_reconstructPath;
}

QString ApplicationSettings::getStandardInputPath(){
    return this->m_standardInputPath;
}


bool ApplicationSettings::getDarkStyle()
{
    return m_darkStyle;
}

bool ApplicationSettings::getActiveStyle()
{
    return m_activeStyle;
}

bool ApplicationSettings::getUseCuda()
{
    return m_useCuda && getCudaAvailable();
}

bool ApplicationSettings::getCudaAvailable() const
{
#ifdef WITH_CUDA
    bool _cudaSupported  = false;

        // Obtain information from the OpenCV compilation
        // Here is a lot of information.
        const cv::String str = cv::getBuildInformation();

        // Parse looking for "CUDA" or the option you are looking for.
        std::istringstream strStream(str);

        std::string line;
        while (std::getline(strStream, line)) {
            // Enable this to see all the options. (Remember to remove the break)
            //std::cout << line << std::endl;
            //std::string disp(line);

            if (line.find("CUDA") != std::string::npos) {
                // Trim from left.
                line.erase(line.begin(), std::find_if(line.begin(), line.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));

                // Trim from right.
                line.erase(line.begin(), std::find_if(line.begin(), line.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));

                // Convert to lowercase may not be necessary.
                std::transform(line.begin(), line.end(), line.begin(), ::tolower);
                if (line.find("yes") != std::string::npos) {
                    //std::cout << disp << std::endl;
                    _cudaSupported = true;
                    break;
                }
            }
        }

        if (_cudaSupported) {
            if (cv::cuda::getCudaEnabledDeviceCount() < 1) {
                _cudaSupported = false;
            }
        }

        return _cudaSupported;
#else
    return false;
#endif // WITH_CUDA
}

bool ApplicationSettings::getCreateLogs()
{
    return m_createLogs;
}
