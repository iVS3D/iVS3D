#include "geodistance.h"

// Qt
#include <QTranslator>
#include <QtMath>

// iVS3D-core
#include "../iVS3D-core/model/metaData/metadata.h"


//==================================================================================================
GeoDistance::GeoDistance()
    : mpSigObj(nullptr)
    , mpReader(nullptr)
    , mpSamplingWidget(nullptr)
    , mBuffer({})
    , mMetaData({})
    , mGpsData({})
    , mIsGpsAvailable(false)
{
    //--- load and install translations
    QLocale locale = qApp->property("translation").toLocale();
    QTranslator* translator = new QTranslator();
    translator->load(locale, "geodistance", "_", ":/translations", ".qm");
    qApp->installTranslator(translator);
}

//==================================================================================================
GeoDistance::~GeoDistance()
{

}

//==================================================================================================
QWidget* GeoDistance::getSettingsWidget(QWidget* parent)
{
    //--- if settings widget is not yet created, do so
    if (!mpSamplingWidget)
        createSettingsWidget(parent);
    mpAltitudeCheckBox->setVisible(mAltitudeExisting);
    return mpSamplingWidget;
}

//==================================================================================================
QString GeoDistance::getName() const
{
    return tr("GeoDistance");
}

//==================================================================================================
std::vector<uint> GeoDistance::sampleImages(const std::vector<uint>& imageList,
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
void GeoDistance::initialize(Reader* reader, QMap<QString, QVariant> buffer, signalObject* sigObj)
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
void GeoDistance::setSettings(QMap<QString, QVariant> settings)
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
QMap<QString, QVariant> GeoDistance::getSettings()
{
    QString valueDev = QString::number(mDistance);
    QMap<QString, QVariant> settings;
    settings.insert(NAME_Distance, valueDev);
    settings.insert(NAME_Altitude, mUseAltitude);
    return settings;

}

//==================================================================================================
QMap<QString, QVariant> GeoDistance::generateSettings(Progressable* receiver, bool useCuda,
                                                 volatile bool* stopped)
{
    return QMap<QString, QVariant>();
}

//==================================================================================================
void GeoDistance::onNewMetaData()
{
    //--- clear all items that might have already been drawn on map
    mGpsData.clear();
    readMetaData();
}

//==================================================================================================
void GeoDistance::onKeyframesChanged(std::vector<uint> keyframes)
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
void GeoDistance::createSettingsWidget(QWidget* parent)
{
    //--- create the sampling widget
    mpSamplingWidget = new QWidget(parent);
    mpSamplingWidget->setLayout(new QVBoxLayout());
    mpSamplingWidget->layout()->setSpacing(10);
    mpSamplingWidget->layout()->setMargin(0);
    mpSamplingWidget->layout()->setAlignment(Qt::AlignTop);

    QLabel *LabelDescription = new QLabel(tr("This plugin uses the geo location provided in the meta data to calculate the distance between images. An image is selected as a new keyframe, if the distance to the previous keyframe is greater than the specified threshold."));
    LabelDescription->setStyleSheet(DESCRIPTION_STYLE);
    LabelDescription->setWordWrap(true);
    LabelDescription->setMinimumWidth(50);
    LabelDescription->setMargin(10);
    mpSamplingWidget->layout()->addWidget(LabelDescription);

    QWidget* spinBoxWidget = new QWidget(parent);
    spinBoxWidget->setLayout(new QHBoxLayout(parent));
    spinBoxWidget->layout()->setSpacing(0);
    spinBoxWidget->layout()->setMargin(0);
    spinBoxWidget->layout()->addWidget(new QLabel(tr("Select distance in meter")));

    mpSpinBoxDist = new QDoubleSpinBox(parent);
    mpSpinBoxDist->setMinimum(0);
    mpSpinBoxDist->setMaximum(1000);
    mpSpinBoxDist->setDecimals(2);
    mpSpinBoxDist->setValue(mDistance);
    mpSpinBoxDist->setSingleStep(0.5);
    mpSpinBoxDist->setAlignment(Qt::AlignRight);
    spinBoxWidget->layout()->addWidget(mpSpinBoxDist);
    spinBoxWidget->setToolTip(tr("The minimum distance between two consecutive keyframes."));
    QObject::connect(mpSpinBoxDist, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &GeoDistance::slot_distChanged);
    mpSamplingWidget->layout()->addWidget(spinBoxWidget);


    mpAltitudeCheckBox = new QCheckBox(parent);
    mpAltitudeCheckBox->setText(tr("Include altitude in calculation"));
    QObject::connect(mpAltitudeCheckBox, &QCheckBox::clicked, this, &GeoDistance::slot_altitudeCheckChanged);
    if (!mAltitudeExisting) {
        mpAltitudeCheckBox->setVisible(false);
    }
    mpSamplingWidget->layout()->addWidget(mpAltitudeCheckBox);

    mpSamplingWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    mpSamplingWidget->adjustSize();

}

void GeoDistance::slot_distChanged(double n)
{
    mDistance = n;
}

void GeoDistance::slot_altitudeCheckChanged(bool check)
{
    mUseAltitude = check;
}

//==================================================================================================
void GeoDistance::readMetaData()
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
            }

            if (mpAltitudeCheckBox) {
                mpAltitudeCheckBox->setVisible(mAltitudeExisting);
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
std::vector<unsigned int> GeoDistance::getKeyframesFromGps()
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
QPointF GeoDistance::gpsHashToLatLong(QVariant hash)
{
    QHash<QString, QVariant> gpsHash = hash.toHash();
    double latitude_abs = gpsHash.find("GPSLatitude").value().toDouble();
    double longitude_abs = gpsHash.find("GPSLongitude").value().toDouble();
    double latitude = (gpsHash.find("GPSLatitudeRef").value().toString() == "N") ? latitude_abs : latitude_abs * -1;
    double longitude = (gpsHash.find("GPSLongitudeRef").value().toString() == "E") ? longitude_abs : longitude_abs * -1;
    return QPointF(latitude, longitude);
}

//==================================================================================================
QGeoCoordinate GeoDistance::gpsHashtoGeoCo(QVariant hash)
{
    QHash<QString, QVariant> gpsHash = hash.toHash();
    double latitude_abs = gpsHash.find("GPSLatitude").value().toDouble();
    double longitude_abs = gpsHash.find("GPSLongitude").value().toDouble();
    double latitude = (gpsHash.find("GPSLatitudeRef").value().toString() == "N") ? latitude_abs : latitude_abs * -1;
    double longitude = (gpsHash.find("GPSLongitudeRef").value().toString() == "E") ? longitude_abs : longitude_abs * -1;
    if (!mAltitudeExisting) {
        return QGeoCoordinate(latitude, longitude);
    }
    double altitude_abs = gpsHash.find("GPSAltitude").value().toDouble();
    double altitude = (gpsHash.find("GPSAltitudeRef").value().toString() == "0") ? altitude_abs : altitude_abs * -1;

    return QGeoCoordinate(latitude, longitude, altitude);
}

double GeoDistance::distanceBetweenPoints(int first, int second)
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

double GeoDistance::greatCircleDistance(QPointF first, QPointF second)
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
