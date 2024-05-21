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
    , mpQuickViewContainerWidget(nullptr)
    , mpMapWidget(nullptr)
    , mpMapHandler(nullptr)
    , mBuffer({})
    , mMetaData({})
    , mGpsData({})
    , mPolygon()
    , mIsQmlMapInitialized(false)
    , mIsGpsAvailable(false)
{
    //--- load and install translations
    QLocale locale = qApp->property("translation").toLocale();
    QTranslator* translator = new QTranslator();
    translator->load(locale, "geomap", "_", ":/translations", ".qm");
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
    if (!mpMapWidget) {
        //--- if settings widget is not yet created, do so
        createSettingsWidget(parent);
    } else {
        //--- otherwise make sure to update the selected frames
        // this means drawing mGpsData to the map
        reinitializeQmlMap();
    }

    return mpMapWidget;
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
    if (!mIsGpsAvailable) {
        return imageList;
    }

    // get all frames inside the polygon from the map
    auto frames_inside_polygon = this->getFramesInsidePolygon();

    std::vector<uint> ret;
    ret.resize(imageList.size());

    // intersect given images (selection from previous algorithms) and images selected on map
    auto end = std::set_intersection(
        imageList.begin(),imageList.end(),
        frames_inside_polygon.begin(),frames_inside_polygon.end(),
        ret.begin()
    );
    auto size = end - ret.begin();
    ret.resize(size);

    mPolygon = QPolygonF();
    mpMapHandler->setPolygon(mPolygon);

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
    //mPolygon = settings.find(NAME_Polygon).value().value<QPolygonF>();
    //mpMapHandler->setPolygon(mPolygon);
}

//==================================================================================================
QMap<QString, QVariant> GeoMap::getSettings()
{
    auto settings = QMap<QString, QVariant>();
    settings[NAME_Polygon] = QVariant::fromValue(mPolygon);
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
    mPolygon = QPolygonF();
    readMetaData();

    if (mpQuickViewContainerWidget)
        reinitializeQmlMap();
}

//==================================================================================================
void GeoMap::onKeyframesChanged(std::vector<uint> keyframes)
{
    // nothing to visualize without gps data
    if (!mIsGpsAvailable)
    {
        return;
    }

    // gps map already have the new keyframes, this happens
    // if the map caused the change -> nothing to update
    if(keyframes == getKeyframesFromGps()) {
        return;
    }
    // create a copy of the old list
    QList<QPair<QPointF, bool>> oldGpsData = mGpsData;

    // reset all frames to unselected
    for (int gpsIndex = 0; gpsIndex < mGpsData.length(); gpsIndex++)
    {
        mGpsData[gpsIndex].second = false;
    }

    // select the new keyframes
    for (int index : keyframes)
    {
        mGpsData[index].second = true;
    }

    // find all keyframes that changed
    QList<QPair<QPointF, bool>> changedGpsData;
    for (int gpsIndex = 0; gpsIndex < mGpsData.length(); gpsIndex++) {
        if(mGpsData[gpsIndex].second != oldGpsData[gpsIndex].second) {
            changedGpsData.append(mGpsData[gpsIndex]);
        }
    }



    // only if the map exists and is currently visible, update it!
    if (mpQuickViewContainerWidget && mpMapWidget && mpMapWidget->isVisible())
    {
        // updating individual qml items is expensive, so only do it if there are few items to update!
        if (changedGpsData.length() < 10) {
            mpMapHandler->updatePoints(changedGpsData);
            mpMapHandler->setPolygon(mPolygon);
        } else {
            reinitializeQmlMap(); // oterwise just draw the map from scratch!
        }
    }


/*
    // only if the map exists and is currently visible, redraw it!
    if (mpQuickViewContainerWidget && mpMapWidget && mpMapWidget->isVisible())
    {
        reinitializeQmlMap();
    }*/
}

//==================================================================================================
void GeoMap::createSettingsWidget(QWidget* parent)
{
    mpMapWidget = new QWidget(parent);
    mpMapWidget->setLayout(new QVBoxLayout());
    mpMapWidget->layout()->setSpacing(3);
    mpMapWidget->layout()->setMargin(3);

    //--- initialize qml map
    initializeQmlMap();



    //--- add Reset Button
    QPushButton* pResetButton = new QPushButton(QObject::tr("Reset selection"), mpMapWidget);
    QObject::connect(pResetButton, &QPushButton::clicked, [&]() {
        mpMapHandler->onQmlDeleteSelection();
    });

    //--- add Help Button
    QPushButton* pHelpButton = new QPushButton(QObject::tr("Help"), mpMapWidget);

    //--- add Layout with Buttons
    QHBoxLayout* pHLayout = new QHBoxLayout();
    pHLayout->layout()->setSpacing(3);
    pHLayout->layout()->setMargin(3);
    pHLayout->addWidget(pResetButton);
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

}

//==================================================================================================
void GeoMap::onGpsSelected(QPolygonF polyF)
{
    //--- if no gps is available, return
    if (!mIsGpsAvailable)
        return;

    mPolygon = polyF;
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

std::vector<unsigned int> GeoMap::getFramesInsidePolygon()
{
    // If polygon has length 0 no points are inside
    if (mPolygon.length() == 0)
    {
        return std::vector<unsigned int>();
    }

    std::vector<unsigned int> keyframes;
    // mPolygon is the perimeter of the polygon drwan on the map
    for (int i = 0; i < mGpsData.size(); i++)
    {
        // Check for every gps point, if it's inside the polygon and set the mPointInsidePolygon list accordingly
        QPointF point = mGpsData[i].first;
        if (mPolygon.containsPoint(point, Qt::OddEvenFill))
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

        //--- set initial polygon
        mpMapHandler->setPolygon(mPolygon);

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
        return distance;

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
