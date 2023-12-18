#include "applicationsettings.h"


ApplicationSettings::ApplicationSettings()
{
    //Set default values
    m_disableChecks = false;
    m_createLogs = true;
    m_useCuda = true;
    m_colorTheme = LIGHT;
    loadSettings();
}

bool ApplicationSettings::getInterpolateMetaData()
{
    return m_interpolateMetaData;
}

void ApplicationSettings::setInterpolateMetaData(bool interpolateMetaData)
{
    m_interpolateMetaData = interpolateMetaData;
    saveSettings();
}

void ApplicationSettings::loadSettings()
{
    QSettings settings(stringContainer::settingsCompany, stringContainer::settingsProgramm);
    qDebug() << "Loading configuration from " << settings.fileName();
    m_standardInputPath = settings.value(stringContainer::standardInputPathIdentifier).value<QString>();
    m_disableChecks = settings.value(stringContainer::disableChecksIdentifier).value<bool>();
    m_colorTheme = settings.value(stringContainer::colorThemeIdentifier).value<ColorTheme>();
    if(settings.contains(stringContainer::useCudaIdentifier))
        m_useCuda = settings.value(stringContainer::useCudaIdentifier).value<bool>();
    QVariantMap reconstructMap = settings.value(stringContainer::reconstructSoftwareIdentifier).value<QVariantMap>();
    QMapIterator<QString, QVariant> mapIt(reconstructMap);
    m_reconstructPath.clear();
    while (mapIt.hasNext()) {
        mapIt.next();
        m_reconstructPath.insert(mapIt.key(), mapIt.value().toString());
    }
    m_createLogs = settings.value(stringContainer::createLogsIdentifier).value<bool>();
    m_interpolateMetaData = settings.value(stringContainer::interpolateIdentifier).value<bool>();
}

void ApplicationSettings::saveSettings()
{
    QSettings settings(stringContainer::settingsCompany, stringContainer::settingsProgramm);
    settings.setValue(stringContainer::standardInputPathIdentifier, m_standardInputPath);
    settings.setValue(stringContainer::disableChecksIdentifier, m_disableChecks);
    settings.setValue(stringContainer::colorThemeIdentifier, m_colorTheme);
    settings.setValue(stringContainer::useCudaIdentifier, m_useCuda);
    settings.setValue(stringContainer::interpolateIdentifier, m_interpolateMetaData);
    QVariantMap reconstructMap;
    QMapIterator<QString, QString> mapIt(m_reconstructPath);
    while (mapIt.hasNext()) {
        mapIt.next();
        reconstructMap.insert(mapIt.key(), mapIt.value());
    }
    settings.setValue(stringContainer::reconstructSoftwareIdentifier, reconstructMap);
    settings.setValue(stringContainer::createLogsIdentifier, m_createLogs);
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

void ApplicationSettings::setDisableChecks(bool disableChecks)
{
    m_disableChecks = disableChecks;
    saveSettings();
}

void ApplicationSettings::setColorTheme(ColorTheme theme)
{
    m_colorTheme = theme;
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

bool ApplicationSettings::getDisableChecks()
{
    return m_disableChecks;
}

ColorTheme ApplicationSettings::getColorTheme()
{
    return m_colorTheme;
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
                line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](int c) {return !std::isspace(c);}));

                // Trim from right.
                line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](int c) {return !std::isspace(c);}));

                // Convert to lowercase may not be necessary.
                std::transform(line.begin(), line.end(), line.begin(), ::tolower);
                if (line.find("yes") != std::string::npos) {
                    //std::cout << disp << std::endl;
                    _cudaSupported = true;
                    break;
                }
            }
        }
        try {
            if (_cudaSupported) {
                if (cv::cuda::getCudaEnabledDeviceCount() < 1) {
                    _cudaSupported = false;
                }
            }

            // Check if installed GPU is compatible Cuda Version
            if (_cudaSupported) {
                cv::cuda::DeviceInfo di;
                _cudaSupported = di.isCompatible();
            }
        } catch (cv::Exception e) {
            _cudaSupported = false;
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
