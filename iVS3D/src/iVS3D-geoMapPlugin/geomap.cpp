#include "geomap.h"

// Qt
#include <QQuickView>
#include <QTranslator>

// iVS3D-core
#include "../iVS3D-core/model/metaData/metadata.h"

//==================================================================================================
GeoMap::GeoMap()
    : mpSigObj(nullptr)
    , mpReader(nullptr)
    , mpSettingsWidget(nullptr)
    , mpQuickView(nullptr)
    , mpQuickViewContainerWidget(nullptr)
    , mpMapHandler(nullptr)
    , mpMap(nullptr)
    , mBuffer({})
    , mMetaData({})
    , mGpsData({})
    , mIsQmlMapInitialized(false)
    , mIsGpsAvailable(false)
    , mpShowGpsButton(nullptr)
    , mpUseAltitudeCheckBox(nullptr)
    , mpDeviationSpinBox(nullptr)
{
    //--- load and install translations
    QTranslator* translator = new QTranslator();
    translator->load(QLocale::system(), "geomap", "_", ":/translations", ".qm");
    qApp->installTranslator(translator);
}

//==================================================================================================
GeoMap::~GeoMap()
{
    if (mpQuickViewContainerWidget)
        mpQuickViewContainerWidget->deleteLater();
    if (mpQuickView)
        mpQuickView->deleteLater();
}

//==================================================================================================
QWidget* GeoMap::getSettingsWidget(QWidget* parent)
{
    //--- if settings widget is not yet created, do so
    if (!mpSettingsWidget)
        createSettingsWidget(parent);

    if (mpUseAltitudeCheckBox)
        mpUseAltitudeCheckBox->setVisible(mIsAltitudeAvailable);

    return mpSettingsWidget;
}

//==================================================================================================
QString GeoMap::getName() const
{
    return "GeoMap";
}

//==================================================================================================
std::vector<uint> GeoMap::sampleImages(const std::vector<uint>& imageList,
                                       Progressable* receiver, volatile bool* stopped,
                                       bool useCuda, LogFileParent* logFile)
{
    return imageList;

#ifdef OLD_CODE
    //--- if gps data does not exist, return unchanged input image list
    if (!mIsGpsAvailable)
        return imageList;

    /// IDs / Indices of sampled images that are to be returned
    std::vector<uint> sampledImgIds;

    //--- initialize return value, first image will always be a keyframe
    int prevIndex = 0;
    mGpsData.replace(0, QPair<QPointF, bool>(mGpsData.at(0).first, true));
    sampledImgIds.push_back(imageList[0]);

    double currentDistance = 0;
    for (int i = 1; i < imageList.size(); i++)
    {
        QPointF current  = mGpsData.at(imageList[i]).first;
        int currentIndex = imageList[i];
        if (imageList[i] != i)
        {
            for (int k = imageList[i - 1] + 1; k < imageList[i]; k++)
            {
                // mGpsData has to be filled with all gps values. Values that aren't in the imageList are added here as false -> unused values
                mGpsData.replace(k, QPair<QPointF, bool>(mGpsData.at(k).first, false));
            }
        }
        currentDistance += distanceBetweenPoints(prevIndex, currentIndex);

        prevIndex = currentIndex;
        // if total distance between images is lower then the selected deviation it won't be selected
        if (currentDistance < mDeviation)
        {
            mGpsData.replace(i, QPair<QPointF, bool>(current, false));
            continue;
        }

        mGpsData.replace(i, QPair<QPointF, bool>(current, true));
        currentDistance = 0;
        sampledImgIds.push_back(imageList[i]);
    }
    if (imageList.back() != mpReader->getPicCount())
    {
        for (int i = imageList.back(); i < mpReader->getPicCount(); i++)
        {
            // mGpsData has to be filled with all gps values. Values that aren't in the imageList are added here
            mGpsData.append(QPair<QPointF, bool>(mGpsData.at(i).first, false));
        }
    }

    return sampledImgIds;
#endif
}

//==================================================================================================
void GeoMap::initialize(Reader* reader, QMap<QString, QVariant> buffer, signalObject* sigObj)
{
    //--- assign member variables
    mpReader = reader;
    mBuffer  = buffer;
    mpSigObj = sigObj;

    //--- clear existing data
    mGpsData.clear();
    mIsGpsAvailable = false;

    //--- connect to signals from iVS3D-core
    connect(mpSigObj, SIGNAL(sig_newMetaData()), this, SLOT(onNewMetaData()));
    connect(mpSigObj, SIGNAL(sig_keyframesChanged(std::vector<uint>)),
            this, SLOT(onKeyframesChanged(std::vector<uint>)));

    //--- Get reader of metadata. if reader is not available, return
    MetaData* metaDataReader = mpReader->getMetaData();
    if (metaDataReader == nullptr)
        return;

    //--- get meta data from reader, if Empty return
    QStringList metaData = metaDataReader->availableMetaData();
    if (metaData.size() == 0)
        return;

    readMetaData();
    // if (mpMapHandler)
    //     mpMapHandler->addPoints(mGpsData);
}

//==================================================================================================
void GeoMap::setSettings(QMap<QString, QVariant> settings)
{
    QMap<QString, QVariant>::iterator iterator = settings.find(NAME_DEVIATION);
    if (iterator != settings.end())
        mDeviation = iterator.value().toDouble();
}

//==================================================================================================
QMap<QString, QVariant> GeoMap::getSettings()
{
    QString valueDev = QString::number(mDeviation);
    QMap<QString, QVariant> settings;
    settings.insert(NAME_DEVIATION, valueDev);

    return settings;
}

//==================================================================================================
QMap<QString, QVariant> GeoMap::generateSettings(Progressable* receiver, bool useCuda,
                                                 volatile bool* stopped)
{
    return QMap<QString, QVariant>();
}

//==================================================================================================
void GeoMap::onGpsClicked(QPointF gpsPoint, bool used)
{
    qDebug() << __PRETTY_FUNCTION__;

    QPair<QPointF, bool> current(gpsPoint, !used);
    int currentIndex = mGpsData.indexOf(current);
    while (currentIndex != -1)
    {
        mGpsData[currentIndex] = QPair<QPointF, bool>(current.first, used);
        currentIndex           = mGpsData.indexOf(current, currentIndex);
    }
    emit updateKeyframes(getKeyframesFromGps());
}

#ifdef OLD_CODE
void gps::showMapWithPoints()
{
    mpMap = new map(mGpsData, this);
    mpMap->addPoints();
    mpShowGpsButton->setEnabled(false);
}

void gps::slot_devChanged(double n)
{
    mDeviation = n;
}
#endif

//==================================================================================================
void GeoMap::onNewMetaData()
{
    qDebug() << __PRETTY_FUNCTION__;

    //--- if gps data does not yet exist, clear and read meta data
    // if (!mIsGpsAvailable)
    // {
    //--- clear all items that might have already been drawn on map
    mGpsData.clear();
    readMetaData();

    if (mpQuickView && mpQuickViewContainerWidget)
        reinitializeQmlMap();

    // if (mpMapHandler)
    //     mpMapHandler->addPoints(mGpsData);
    // }
}

//==================================================================================================
void GeoMap::onKeyframesChanged(std::vector<uint> keyframes)
{
    //--- if no gps data exist, disable button
    if (mGpsData.length() <= 0)
    {
        if (mpShowGpsButton != nullptr)
            mpShowGpsButton->setEnabled(false);

        return;
    }

    //--- Check if its the current keyframe list
    if (keyframes == getKeyframesFromGps())
    {
        return;
    }

    for (int gpsIndex = 0; gpsIndex < mGpsData.length(); gpsIndex++)
    {
        mGpsData[gpsIndex].second = false;
    }

    if (keyframes.size() == 0)
    {
        return;
    }

    for (int index : keyframes)
    {
        mGpsData[index].second = true;
    }
}

//==================================================================================================
void GeoMap::createSettingsWidget(QWidget* parent)
{
#if 0
    mpSettingsWidget = new QWidget(parent);
    mpSettingsWidget->setLayout(new QVBoxLayout());
    mpSettingsWidget->layout()->setSpacing(10);
    mpSettingsWidget->layout()->setMargin(0);

    QWidget* buttonWidget = new QWidget(parent);
    buttonWidget->setLayout(new QHBoxLayout(parent));
    buttonWidget->layout()->setSpacing(0);
    buttonWidget->layout()->setMargin(0);

    mpShowGpsButton = new QPushButton();
    mpShowGpsButton->setText(tr("Show on map"));
    mpShowGpsButton->setEnabled(mIsGpsAvailable);
    // QObject::connect(mpShowGpsButton, QOverload<bool>::of(&QPushButton::clicked),
    //                  this, &GeoMap::showMapWithPoints);
    buttonWidget->layout()->addWidget(mpShowGpsButton);

    mpSettingsWidget->layout()->addWidget(buttonWidget);

    QWidget* spinBoxWidget = new QWidget(parent);
    spinBoxWidget->setLayout(new QHBoxLayout(parent));
    spinBoxWidget->layout()->setSpacing(0);
    spinBoxWidget->layout()->setMargin(0);
    spinBoxWidget->layout()->addWidget(new QLabel(tr("Select distance in meter"), parent));

    mpDeviationSpinBox = new QDoubleSpinBox(parent);
    mpDeviationSpinBox->setMinimum(0);
    mpDeviationSpinBox->setMaximum(1000);
    mpDeviationSpinBox->setDecimals(2);
    mpDeviationSpinBox->setValue(mDeviation);
    mpDeviationSpinBox->setSingleStep(0.5);
    mpDeviationSpinBox->setAlignment(Qt::AlignRight);
    spinBoxWidget->layout()->addWidget(mpDeviationSpinBox);
    // QObject::connect(mpDeviationSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
    //                  this, &GeoMap::slot_devChanged);
    mpSettingsWidget->layout()->addWidget(spinBoxWidget);

    QLabel* LabelDistance = new QLabel(tr("Minimum distance there has to be between two keyframes"));
    LabelDistance->setStyleSheet(DESCRIPTION_STYLE);
    LabelDistance->setWordWrap(true);
    mpSettingsWidget->layout()->addWidget(LabelDistance);

    mpUseAltitudeCheckBox = new QCheckBox(parent);
    mpUseAltitudeCheckBox->setText("Include altitude in calculation");
    // QObject::connect(mpUseAltitudeCheckBox, &QCheckBox::clicked,
    // this, &GeoMap::slot_altitudeCheckChanged);
    mpSettingsWidget->layout()->addWidget(mpUseAltitudeCheckBox);

    mpSettingsWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    mpSettingsWidget->adjustSize();
#endif
    qDebug() << __PRETTY_FUNCTION__;

    mpSettingsWidget = new QWidget(parent);
    mpSettingsWidget->setLayout(new QVBoxLayout());
    mpSettingsWidget->layout()->setSpacing(10);
    mpSettingsWidget->layout()->setMargin(0);

    //--- initialize qml map
    initializeQmlMap();
    // QQuickView* pQuickView = new QQuickView();
    // QWidget* pQuickViewContainerWidget =
    //   QWidget::createWindowContainer(pQuickView, mpSettingsWidget);
    // mpMapHandler.reset(new MapHandler());
    // pQuickView->rootContext()->setContextProperty("handler", mpMapHandler.get());
    // pQuickView->setSource(QUrl("qrc:/map.qml"));
    // mpSettingsWidget->layout()->addWidget(pQuickViewContainerWidget);

    // if (!pQuickView->errors().isEmpty())
    // {
    //     qCritical() << pQuickView->errors();
    // }
    // else
    // {
    //     //--- make connections to qml map

    //     QObject* qmlRoot = pQuickView->rootObject();
    //     QObject::connect(qmlRoot, SIGNAL(gpsClicked(QString)),
    //                      mpMapHandler.get(), SLOT(onQmlGpsClicked(QString)));
    //     QObject::connect(qmlRoot, SIGNAL(qmlClosed()),
    //                      mpMapHandler.get(), SLOT(onQmlMapClosed()));
    //     QObject::connect(qmlRoot, SIGNAL(mapClicked(QString)),
    //                      mpMapHandler.get(), SLOT(onQmlMapClicked(QString)));
    //     QObject::connect(qmlRoot, SIGNAL(mapItems(QVariant)),
    //                      mpMapHandler.get(), SLOT(onQmlMapItems(QVariant)));

    //     QObject::connect(qmlRoot, SIGNAL(deleteSelection()),
    //                      mpMapHandler.get(), SLOT(onQmlDeleteSelection()));
    //     QObject::connect(qmlRoot, SIGNAL(selectionBack()),
    //                      mpMapHandler.get(), SLOT(onQmlSelectionBack()));
    //     QObject::connect(qmlRoot, SIGNAL(selectionForward()),
    //                      mpMapHandler.get(), SLOT(onQmlSelectionForward()));

    //     QObject::connect(mpMapHandler.get(), &MapHandler::gpsClicked,
    //                      this, &GeoMap::onGpsClicked);
    //     QObject::connect(mpMapHandler.get(), &MapHandler::gpsSelected,
    //                      this, &GeoMap::onGpsSelected);

    //     //--- set default map center to IOSB
    //     mpMapHandler->emitAdjustMapCenter(QGeoCoordinate(49.01554184059616, 8.425800420583966));

    //     //--- draw available data onto map
    //     mpMapHandler->drawGpsDataOnMap();
    // }
}

//==================================================================================================
void GeoMap::onGpsSelected(QPolygonF polyF)
{
    // If polyF has length 0 delete all keyframes
    if (polyF.length() == 0)
    {
        for (int i = 0; i < mGpsData.size(); i++)
        {
            mGpsData[i].second = false;
        }
        updateKeyframes(getKeyframesFromGps());
        return;
    }
    // polyF is the perimeter of the polygon drwan on the map
    for (int i = 0; i < mGpsData.size(); i++)
    {
        // Check for every gps point, if it's inside the polygon and set it as keyframe accordingly
        QPointF point = mGpsData[i].first;
        if (polyF.containsPoint(point, Qt::OddEvenFill))
        {
            mGpsData[i].second = true;
        }
        else
        {
            mGpsData[i].second = false;
        }
    }
    updateKeyframes(getKeyframesFromGps());
}

#ifdef OLD_CODE
void gps::slot_altitudeCheckChanged(bool check)
{
    mUseAltitude = check;
}
#endif

//==================================================================================================
void GeoMap::readMetaData()
{
    MetaData* metaData     = mpReader->getMetaData();
    QStringList metaReader = metaData->availableMetaData();
    for (QString metaName : metaReader)
    {
        if (metaName.startsWith("GPS"))
        {
            mMetaData = metaData->loadMetaData(metaName)->getAllMetaData();

            QHash<QString, QVariant> metaHash             = mMetaData[0].toHash();
            QHash<QString, QVariant>::const_iterator iter = metaHash.find("GPSAltitude");
            if (iter == metaHash.end())
            {
                mIsAltitudeAvailable = false;
            }
            else
            {
                mIsAltitudeAvailable = true;
            }

            // Initial all frames are keyframes
            for (QVariant var : mMetaData)
            {
                mGpsData.append(QPair<QPointF, bool>(gpsHashToLatLong(var), true));
            }
            mIsGpsAvailable = true;
            // Enable button if existing
            if (mpShowGpsButton != nullptr)
            {
                mpShowGpsButton->setEnabled(true);
            }
            return;
        }
    }
}

//==================================================================================================
double GeoMap::calculateGreatCircleDistance(QPointF first, QPointF second)
{
    // calculate distance between to points using the haversine formula
    // Earth radius in meters
    const int r = 6371008;
    // Convert latitude and longitude to radiant
    double lat1     = first.x() * (M_PI / 180);
    double lat2     = second.x() * (M_PI / 180);
    double latDiff  = (second.x() - first.x()) * (M_PI / 180);
    double longDiff = (second.y() - first.y()) * (M_PI / 180);
    // Calculate haversine
    double a        = pow(sin(latDiff / 2), 2) + cos(lat1) * cos(lat2) * pow(sin(longDiff / 2), 2);
    double distance = 2 * r * asin(sqrt(a));
    return distance;
}

//==================================================================================================
double GeoMap::distanceBetweenPoints(int first, int second)
{
    QGeoCoordinate firstGPS    = gpsHashtoGeoCo(mMetaData.at(first));
    QGeoCoordinate secondGPS   = gpsHashtoGeoCo(mMetaData.at(second));
    QPointF firstLatLong       = QPointF(firstGPS.latitude(), firstGPS.longitude());
    QPointF secondLatLong      = QPointF(secondGPS.latitude(), secondGPS.longitude());
    double greatCircleDistance = calculateGreatCircleDistance(firstLatLong, secondLatLong);
    if (!mUseAltitude)
    {
        return greatCircleDistance;
    }
    double euclidDistance = qSqrt(qPow(greatCircleDistance, 2) +
                                  qPow(firstGPS.altitude() - secondGPS.altitude(), 2));
    return euclidDistance;
}

//==================================================================================================
std::vector<unsigned int> GeoMap::getKeyframesFromGps()
{
    std::vector<unsigned int> keyframes;
    for (int i = 0; i < mGpsData.size(); i++)
    {
        if (mGpsData[i].second)
        {
            keyframes.push_back(i);
        }
    }
    return keyframes;
}

//==================================================================================================
QPointF GeoMap::gpsHashToLatLong(QVariant hash)
{
    QHash<QString, QVariant> gpsHash = hash.toHash();
    double latitude_abs              = gpsHash.find("GPSLatitude").value().toDouble();
    double longitude_abs             = gpsHash.find("GPSLongitude").value().toDouble();
    double latitude                  = (gpsHash.find("GPSLatitudeRef").value().toString() == "N")
                                         ? latitude_abs
                                         : latitude_abs * -1;
    double longitude                 = (gpsHash.find("GPSLongitudeRef").value().toString() == "E")
                                         ? longitude_abs
                                         : longitude_abs * -1;
    return QPointF(latitude, longitude);
}

//==================================================================================================
QGeoCoordinate GeoMap::gpsHashtoGeoCo(QVariant hash)
{
    QHash<QString, QVariant> gpsHash = hash.toHash();
    double latitude_abs              = gpsHash.find("GPSLatitude").value().toDouble();
    double longitude_abs             = gpsHash.find("GPSLongitude").value().toDouble();
    double latitude                  = (gpsHash.find("GPSLatitudeRef").value().toString() == "N")
                                         ? latitude_abs
                                         : latitude_abs * -1;
    double longitude                 = (gpsHash.find("GPSLongitudeRef").value().toString() == "E")
                                         ? longitude_abs
                                         : longitude_abs * -1;
    if (!mIsAltitudeAvailable)
    {
        return QGeoCoordinate(latitude, longitude);
    }
    double altitude_abs = gpsHash.find("GPSAltitude").value().toDouble();
    double altitude     = (gpsHash.find("GPSAltitudeRef").value().toString() == "0")
                            ? altitude_abs
                            : altitude_abs * -1;

    return QGeoCoordinate(latitude, longitude, altitude);
}

//==================================================================================================
void GeoMap::initializeQmlMap()
{
    //--- initialize qml map
    mpQuickView                = new QQuickView();
    mpQuickViewContainerWidget = QWidget::createWindowContainer(mpQuickView, mpSettingsWidget);
    mpMapHandler.reset(new MapHandler());
    mpQuickView->engine()->clearComponentCache();
    mpQuickView->rootContext()->setContextProperty("handler", mpMapHandler.get());
    mpQuickView->setSource(QUrl("qrc:/map.qml"));
    mpSettingsWidget->layout()->addWidget(mpQuickViewContainerWidget);

    if (!mpQuickView->errors().isEmpty())
    {
        qCritical() << mpQuickView->errors();
    }
    else
    {
        //--- make connections to qml map

        QObject* qmlRoot = mpQuickView->rootObject();
        QObject::connect(qmlRoot, SIGNAL(gpsClicked(QString)),
                         mpMapHandler.get(), SLOT(onQmlGpsClicked(QString)));
        QObject::connect(qmlRoot, SIGNAL(qmlClosed()),
                         mpMapHandler.get(), SLOT(onQmlMapClosed()));
        QObject::connect(qmlRoot, SIGNAL(mapClicked(QString)),
                         mpMapHandler.get(), SLOT(onQmlMapClicked(QString)));
        QObject::connect(qmlRoot, SIGNAL(mapItems(QVariant)),
                         mpMapHandler.get(), SLOT(onQmlMapItems(QVariant)));

        QObject::connect(qmlRoot, SIGNAL(deleteSelection()),
                         mpMapHandler.get(), SLOT(onQmlDeleteSelection()));
        QObject::connect(qmlRoot, SIGNAL(selectionBack()),
                         mpMapHandler.get(), SLOT(onQmlSelectionBack()));
        QObject::connect(qmlRoot, SIGNAL(selectionForward()),
                         mpMapHandler.get(), SLOT(onQmlSelectionForward()));

        QObject::connect(mpMapHandler.get(), &MapHandler::gpsClicked,
                         this, &GeoMap::onGpsClicked);
        QObject::connect(mpMapHandler.get(), &MapHandler::gpsSelected,
                         this, &GeoMap::onGpsSelected);

        //--- set default map center to IOSB
        mpMapHandler->emitAdjustMapCenter(QGeoCoordinate(49.01554184059616, 8.425800420583966));

        //--- draw available data onto map
        mpMapHandler->addPoints(mGpsData);
    }
}

//==================================================================================================
void GeoMap::reinitializeQmlMap()
{
    //--- remove old map widget
    mpSettingsWidget->layout()->removeWidget(mpQuickViewContainerWidget);
    if (mpQuickViewContainerWidget)
        delete mpQuickViewContainerWidget;

    //--- initialize map
    initializeQmlMap();
}