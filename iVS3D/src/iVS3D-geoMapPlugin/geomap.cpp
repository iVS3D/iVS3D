#include "geomap.h"

// Qt
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickView>
#include <QTranslator>

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
    return imageList;
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
}

//==================================================================================================
QMap<QString, QVariant> GeoMap::getSettings()
{
    return {};
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
    mpSettingsWidget = new QWidget(parent);
    mpSettingsWidget->setLayout(new QVBoxLayout());
    mpSettingsWidget->layout()->setSpacing(10);
    mpSettingsWidget->layout()->setMargin(0);

    //--- initialize qml map
    initializeQmlMap();
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
    mpQuickViewContainerWidget = QWidget::createWindowContainer(pQuickView, mpSettingsWidget);
    mpMapHandler.reset(new MapHandler());
    pQuickView->engine()->clearComponentCache();
    pQuickView->rootContext()->setContextProperty("handler", mpMapHandler.get());
    pQuickView->setSource(QUrl("qrc:/map.qml"));
    mpSettingsWidget->layout()->addWidget(mpQuickViewContainerWidget);

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
    mpSettingsWidget->layout()->removeWidget(mpQuickViewContainerWidget);
    if (mpQuickViewContainerWidget)
        delete mpQuickViewContainerWidget;

    //--- initialize map
    initializeQmlMap();
}