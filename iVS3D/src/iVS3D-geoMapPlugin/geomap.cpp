#include "geomap.h"

// Qt
#include <QMessageBox>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickView>
#include <QTranslator>
#include <QtMath>

// iVS3D-core
#include "../iVS3D-core/model/metaData/metadata.h"


//==================================================================================================
GeoMap::GeoMap()
    : mpSigObj(nullptr)
    , mpReader(nullptr)
    , mpSettingsWidget(nullptr)
    , mpQuickViewContainerWidget(nullptr)
    , mpMapHandler(nullptr)
    , mBuffer({})
    , mMetaData({})
    , mGpsData({})
    , mIsQmlMapInitialized(false)
    , mIsGpsAvailable(false)
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
}

//==================================================================================================
QWidget* GeoMap::getSettingsWidget(QWidget* parent)
{
    //--- if settings widget is not yet created, do so
    if (!mpSettingsWidget)
        createSettingsWidget(parent);

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
    std::vector<uint> ret;
    if (!mIsGpsAvailable) {
        return imageList;
    }

    int prevIndex = 0;
    mGpsData.replace(0, QPair<QPointF, bool> (mGpsData.at(0).first, true));
    ret.push_back(imageList[0]);

    double currentDistance = 0;
    // first image will always be a keyframe
    for (int i = 1; i < imageList.size(); i++) {
        QPointF current = mGpsData.at(imageList[i]).first;
        int currentIndex = imageList[i];
        if (imageList[i] != i) {
            for (int k = imageList[i - 1] + 1; k < imageList[i]; k++) {
                // mGpsData has to be filled with all gps values. Values that aren't in the imageList are added here as false -> unused values
                mGpsData.replace(k, QPair<QPointF, bool> (mGpsData.at(k).first, false));
            }
        }
        currentDistance += distanceBetweenPoints(prevIndex, currentIndex);

        prevIndex = currentIndex;
        // if total distance between images is lower then the selected deviation it won't be selected
        if (currentDistance < mDistance) {
            mGpsData.replace(i, QPair<QPointF, bool>(current, false));
            continue;
        }

        mGpsData.replace(i, QPair<QPointF, bool>(current, true));
        currentDistance = 0;
        ret.push_back(imageList[i]);
    }
    if (imageList.back() != mpReader->getPicCount()) {
        for (int i = imageList.back(); i < mpReader->getPicCount(); i++) {
            // mGpsData has to be filled with all gps values. Values that aren't in the imageList are added here
            mGpsData.append(QPair<QPointF, bool> (mGpsData.at(i).first, false));
        }
    }

    return ret;

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
}

//==================================================================================================
void GeoMap::setSettings(QMap<QString, QVariant> settings)
{
    QMap<QString, QVariant>::iterator iterator = settings.find(NAME_Distance);
    if (iterator != settings.end()) {
        mDistance = iterator.value().toDouble();
        mpSpinBoxDist->setValue(mDistance);
    }

    iterator = settings.find(NAME_Altitude);
    if (iterator != settings.end()) {
        mUseAltitude = iterator.value().toBool();
        mpAltitudeCheckBox->setChecked(mUseAltitude);
    }

}

//==================================================================================================
QMap<QString, QVariant> GeoMap::getSettings()
{
    QString valueDev = QString::number(mDistance);
    QMap<QString, QVariant> settings;
    settings.insert(NAME_Distance, valueDev);
    settings.insert(NAME_Altitude, mUseAltitude);
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
    //--- if no gps is available, return
    if (!mIsGpsAvailable)
        return;

    QPair<QPointF, bool> current(gpsPoint, !used);
    int currentIndex = mGpsData.indexOf(current);
    while (currentIndex != -1)
    {
        mGpsData[currentIndex] = QPair<QPointF, bool>(current.first, used);
        currentIndex           = mGpsData.indexOf(current, currentIndex);
    }

    emit updateKeyframes(getKeyframesFromGps());
}

//==================================================================================================
void GeoMap::onNewMetaData()
{
    //--- clear all items that might have already been drawn on map
    mGpsData.clear();
    readMetaData();

    if (mpQuickViewContainerWidget)
        reinitializeQmlMap();
}

//==================================================================================================
void GeoMap::onKeyframesChanged(std::vector<uint> keyframes)
{
    //--- Check if its the current keyframe list
    if (keyframes == getKeyframesFromGps())
    {
        return;
    }

    for (int gpsIndex = 0; gpsIndex < mGpsData.length(); gpsIndex++)
    {
        mGpsData[gpsIndex].second = false;
    }

    if (keyframes.size() == 0 || !mIsGpsAvailable)
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
    mpSettingsWidget = new QTabWidget(parent);
    mpMapWidget = new QWidget(mpSettingsWidget);
    mpMapWidget->setLayout(new QVBoxLayout());
    mpMapWidget->layout()->setSpacing(3);
    mpMapWidget->layout()->setMargin(3);

    //--- initialize qml map
    initializeQmlMap();

    //--- add Help Button
    QHBoxLayout* pHLayout    = new QHBoxLayout();
    QPushButton* pHelpButton = new QPushButton(QObject::tr("Help"), mpMapWidget);
    pHLayout->layout()->setSpacing(3);
    pHLayout->layout()->setMargin(3);
    pHLayout->addSpacerItem(new QSpacerItem(20, 20, QSizePolicy::MinimumExpanding, QSizePolicy::Minimum));
    pHLayout->addWidget(pHelpButton);
    mpMapWidget->layout()->addItem(pHLayout);

    //--- connect help button to message box
    QObject::connect(pHelpButton, &QPushButton::clicked, [&]()
                     { QMessageBox::about(mpMapWidget, "GeoMap Plugin",
                                          QObject::tr("Select a group of keyframes by using right "
                                                      "mouse button to draw an encapsulating polygon."
                                                      "\n\n"
                                                      "Select or deselect individual keyframes by "
                                                      "clicking with the left mouse button "
                                                      "on the location markings.\n\n"
                                                      "A combination of both is also allowed.")); });

    //--- add map to the tab widget
    mpSettingsWidget->addTab(mpMapWidget, "Map");

    //--- create the sampling widget
    mpSamplingWidget = new QWidget(mpSettingsWidget);
    mpSamplingWidget->setLayout(new QVBoxLayout());
    mpSamplingWidget->layout()->setSpacing(10);
    mpSamplingWidget->layout()->setMargin(0);

    QWidget* spinBoxWidget = new QWidget(parent);
    spinBoxWidget->setLayout(new QHBoxLayout(parent));
    spinBoxWidget->layout()->setSpacing(0);
    spinBoxWidget->layout()->setMargin(0);
    spinBoxWidget->layout()->addWidget(new QLabel(tr("Select distance in meter"),parent));

    mpSpinBoxDist = new QDoubleSpinBox(parent);
    mpSpinBoxDist->setMinimum(0);
    mpSpinBoxDist->setMaximum(1000);
    mpSpinBoxDist->setDecimals(2);
    mpSpinBoxDist->setValue(mDistance);
    mpSpinBoxDist->setSingleStep(0.5);
    mpSpinBoxDist->setAlignment(Qt::AlignRight);
    spinBoxWidget->layout()->addWidget(mpSpinBoxDist);
    QObject::connect(mpSpinBoxDist, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &GeoMap::slot_distChanged);
    mpSamplingWidget->layout()->addWidget(spinBoxWidget);

    QLabel *LabelDistance = new QLabel(tr("Minimum distance between two keyframes"));
    LabelDistance->setStyleSheet(DESCRIPTION_STYLE);
    LabelDistance->setWordWrap(true);
    mpSamplingWidget->layout()->addWidget(LabelDistance);


    mpAltitudeCheckBox = new QCheckBox(parent);
    mpAltitudeCheckBox->setText("Include altitude in calculation");
    QObject::connect(mpAltitudeCheckBox, &QCheckBox::clicked, this, &GeoMap::slot_altitudeCheckChanged);
    if (!mAltitudeExisting) {
        mpAltitudeCheckBox->setVisible(false);
    }
    mpSamplingWidget->layout()->addWidget(mpAltitudeCheckBox);

    mpSamplingWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    mpSamplingWidget->adjustSize();

    //--- add sampling widget to tab widget
    mpSettingsWidget->addTab(mpSamplingWidget, "Sampling");

}

//==================================================================================================
void GeoMap::onGpsSelected(QPolygonF polyF)
{
    //--- if no gps is available, return
    if (!mIsGpsAvailable)
        return;

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

void GeoMap::slot_distChanged(double n)
{
    mDistance = n;
}

void GeoMap::slot_altitudeCheckChanged(bool check)
{
    mUseAltitude = check;
}

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

            QHash<QString, QVariant> metaHash = mMetaData[0].toHash();
            QHash<QString, QVariant>::iterator iter = metaHash.find("GPSAltitude");
            if (iter == metaHash.end()) {
                mAltitudeExisting = false;
            }
            else {
                mAltitudeExisting = true;
                if (mpAltitudeCheckBox) {
                    mpAltitudeCheckBox->setVisible(false);
                }
            }


            // Initial all frames are keyframes
            for (QVariant var : mMetaData)
            {
                mGpsData.append(QPair<QPointF, bool>(gpsHashToLatLong(var), true));
            }
            mIsGpsAvailable = true;

            return;
        }
    }
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
    QPointF latLong = gpsHashToLatLong(hash);

    return QGeoCoordinate(latLong.x(), latLong.y());
}

//==================================================================================================
void GeoMap::initializeQmlMap()
{
    //--- initialize qml map
    QQuickView* pQuickView     = new QQuickView();
    mpQuickViewContainerWidget = QWidget::createWindowContainer(pQuickView, mpMapWidget);
    mpMapHandler.reset(new MapHandler());
    pQuickView->engine()->clearComponentCache();
    pQuickView->rootContext()->setContextProperty("handler", mpMapHandler.get());
    pQuickView->setSource(QUrl("qrc:/map.qml"));

    //--- insert QuickView container widget at top
    dynamic_cast<QVBoxLayout*>(mpMapWidget->layout())
      ->insertWidget(0, mpQuickViewContainerWidget);

    if (!pQuickView->errors().isEmpty())
    {
        qCritical() << pQuickView->errors();
    }
    else
    {
        //--- make connections to qml map

        QObject* qmlRoot = pQuickView->rootObject();
        QObject::connect(qmlRoot, SIGNAL(gpsClicked(QString)),
                         mpMapHandler.get(), SLOT(onQmlGpsClicked(QString)));
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
    mpMapWidget->layout()->removeWidget(mpQuickViewContainerWidget);
    if (mpQuickViewContainerWidget)
        delete mpQuickViewContainerWidget;

    //--- initialize map
    initializeQmlMap();
}

double GeoMap::distanceBetweenPoints(int first, int second)
{
    QGeoCoordinate firstGPS = gpsHashtoGeoCo(mMetaData.at(first));
        QGeoCoordinate secondGPS = gpsHashtoGeoCo(mMetaData.at(second));
        QPointF firstLatLong = QPointF(firstGPS.latitude(), firstGPS.longitude());
        QPointF secondLatLong = QPointF(secondGPS.latitude(), secondGPS.longitude());
        double distance = greatCircleDistance(firstLatLong, secondLatLong);
        if (!mAltitudeExisting || !mUseAltitude) {
            return distance;
        }
        double pow = qPow(distance, 2);
        double sqrt = qSqrt(qPow(distance, 2));
        double pow2 = qPow(firstGPS.altitude() - secondGPS.altitude(), 2);
        double euclidDistance = qSqrt(qPow(distance, 2) + qPow(firstGPS.altitude() - secondGPS.altitude(), 2));
        return euclidDistance;

}

double GeoMap::greatCircleDistance(QPointF first, QPointF second)
{
    // caculate distance between to points using the haversine formula
    // earth radius in meters
    const int r = 6371008;
    // convert latitude and longitude to radiant
    double lat1 = first.x() * (M_PI / 180);
    double lat2 = second.x() * (M_PI / 180);
    double latDiff = (second.x() - first.x()) * (M_PI / 180);
    double longDiff = (second.y() - first.y()) * (M_PI / 180);
    // calculate haversine
    double a = pow(sin(latDiff/2), 2) + cos(lat1) * cos(lat2) * pow(sin(longDiff/2), 2);
    double distance = 2 * r * asin(sqrt(a));
    return distance;

}
