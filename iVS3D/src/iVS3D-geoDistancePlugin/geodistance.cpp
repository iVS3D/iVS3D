#include "geodistance.h"

#include <QCoreApplication>
#include <QDebug>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QSignalBlocker>
#include <QTranslator>
#include <QtMath>

#include "../iVS3D-core/model/metaData/metadata.h"

using PLUG::ApplySettingsResult;
using PLUG::Error;
using PLUG::ErrorCode;
using PLUG::InputData;
using PLUG::InputLoadedResult;
using PLUG::InputMetaData;
using PLUG::MetaDataLoadedResult;
using PLUG::SelectionData;
using PLUG::SelectionResult;
using PLUG::SettingsWidgetResult;

//==================================================================================================
GeoDistance::GeoDistance()
    : IBase()
{
    QLocale locale = qApp->property("translation").toLocale();
    QTranslator* translator = new QTranslator();
    translator->load(locale, "geodistance", "_", ":/translations", ".qm");
    qApp->installTranslator(translator);
}

//==================================================================================================
SettingsWidgetResult GeoDistance::getSettingsWidget()
{
    auto widget = createSettingsWidget();
    if (!widget) {
        return tl::make_unexpected(Error(
            ErrorCode::ResourceUnavailable,
            tr("Failed to create GeoDistance settings widget.")));
    }
    return widget;
}

//==================================================================================================
QString GeoDistance::getName() const
{
    return tr("GeoDistance");
}

//==================================================================================================
QMap<QString, QVariant> GeoDistance::getSettings() const
{
    QMap<QString, QVariant> settings;
    settings.insert(NAME_Distance, mDistance);
    settings.insert(NAME_Altitude, mUseAltitude);
    return settings;
}

//==================================================================================================
ApplySettingsResult GeoDistance::applySettings(
    const QMap<QString, QVariant>& settings)
{
    const double distance = settings.value(NAME_Distance, mDistance).toDouble();
    const bool useAltitude = settings.value(NAME_Altitude, mUseAltitude).toBool();

    if (distance < 0.0) {
        return tl::make_unexpected(
            Error(ErrorCode::InvalidInput,
                  tr("Distance must be greater than or equal to 0.")));
    }

    mDistance = distance;
    mUseAltitude = useAltitude;

    emit syncSettingsWidget(mDistance, mUseAltitude, mAltitudeExisting);
    return {};
}

//==================================================================================================
InputLoadedResult GeoDistance::onInputLoaded(const InputData& input)
{
    mReader = input.reader;
    mMetaData.clear();
    mGpsData.clear();
    mIsGpsAvailable = false;
    mAltitudeExisting = false;
    emit syncSettingsWidget(mDistance, mUseAltitude, mAltitudeExisting);
    return {};
}

//==================================================================================================
MetaDataLoadedResult GeoDistance::onMetaDataLoaded(
    const InputMetaData& inputMetaData)
{
    MetaData* metaData = inputMetaData.metaData;
    if (!metaData && mReader) {
        metaData = mReader->getMetaData();
    }

    readMetaData(metaData);
    emit syncSettingsWidget(mDistance, mUseAltitude, mAltitudeExisting);
    return {};
}

//==================================================================================================
void GeoDistance::onSelectedImagesChanged(
    const std::vector<uint>& selectedImages)
{
    if (!mIsGpsAvailable || mGpsData.isEmpty()) {
        return;
    }

    for (auto& gpsPoint : mGpsData) {
        gpsPoint.second = false;
    }

    for (uint idx : selectedImages) {
        if (idx < static_cast<uint>(mGpsData.size())) {
            mGpsData[int(idx)].second = true;
        }
    }
}

//==================================================================================================
SelectionResult GeoDistance::selectImages(const SelectionData& data,
                                          volatile bool& cancelFlag)
{
    const std::vector<uint>& imageList = data.selectedIndices;
    if (!mIsGpsAvailable || imageList.empty()) {
        return imageList;
    }
    if (cancelFlag) {
        return imageList;
    }

    std::vector<uint> ret;
    ret.reserve(imageList.size());

    for (auto& gpsPoint : mGpsData) {
        gpsPoint.second = false;
    }

    const int firstIndex = int(imageList.front());
    if (firstIndex < 0 || firstIndex >= mGpsData.size()) {
        return imageList;
    }

    int prevIndex = firstIndex;
    mGpsData[prevIndex].second = true;
    ret.push_back(imageList.front());

    double currentDistance = 0;
    for (int i = 1; i < int(imageList.size()); i++) {
        if (cancelFlag) {
            break;
        }

        const int currentIndex = int(imageList[size_t(i)]);
        if (currentIndex < 0 || currentIndex >= mGpsData.size()) {
            continue;
        }

        currentDistance += distanceBetweenPoints(prevIndex, currentIndex);

        prevIndex = currentIndex;
        if (currentDistance < mDistance) {
            mGpsData[currentIndex].second = false;
            continue;
        }

        mGpsData[currentIndex].second = true;
        currentDistance = 0;
        ret.push_back(imageList[size_t(i)]);
    }
    return ret;
}

//==================================================================================================
std::unique_ptr<QWidget> GeoDistance::createSettingsWidget()
{
    auto widget = std::make_unique<QWidget>(nullptr);
    widget->setLayout(new QVBoxLayout());
    widget->layout()->setSpacing(10);
    widget->layout()->setContentsMargins(0, 0, 0, 0);
    widget->layout()->setAlignment(Qt::AlignTop);

    auto* labelDescription = new QLabel(tr("This plugin uses the geo location "
                                           "provided in the meta data to "
                                           "calculate the distance between "
                                           "images. An image is selected as a "
                                           "new keyframe, if the distance to "
                                           "the previous keyframe is greater "
                                           "than the specified threshold."));
    labelDescription->setStyleSheet(DESCRIPTION_STYLE);
    labelDescription->setWordWrap(true);
    labelDescription->setMinimumWidth(50);
    labelDescription->setMargin(10);
    widget->layout()->addWidget(labelDescription);

    auto* spinBoxWidget = new QWidget(widget.get());
    spinBoxWidget->setLayout(new QHBoxLayout());
    spinBoxWidget->layout()->setSpacing(0);
    spinBoxWidget->layout()->setContentsMargins(0, 0, 0, 0);
    spinBoxWidget->layout()->addWidget(new QLabel(tr("Select distance in meter")));

    mSpinBoxDist = new QDoubleSpinBox(widget.get());
    mSpinBoxDist->setMinimum(0.0);
    mSpinBoxDist->setMaximum(1000.0);
    mSpinBoxDist->setDecimals(2);
    mSpinBoxDist->setValue(mDistance);
    mSpinBoxDist->setSingleStep(0.5);
    mSpinBoxDist->setAlignment(Qt::AlignRight);
    spinBoxWidget->layout()->addWidget(mSpinBoxDist);
    spinBoxWidget->setToolTip(tr("The minimum distance between two consecutive keyframes."));
    QObject::connect(mSpinBoxDist, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                     this, &GeoDistance::slot_distChanged);
    widget->layout()->addWidget(spinBoxWidget);

    mAltitudeCheckBox = new QCheckBox(widget.get());
    mAltitudeCheckBox->setText(tr("Include altitude in calculation"));
    mAltitudeCheckBox->setChecked(mUseAltitude);
    mAltitudeCheckBox->setVisible(mAltitudeExisting);
    QObject::connect(mAltitudeCheckBox, &QCheckBox::clicked,
                     this, &GeoDistance::slot_altitudeCheckChanged);
    widget->layout()->addWidget(mAltitudeCheckBox);

    QObject::connect(this, &GeoDistance::syncSettingsWidget, widget.get(),
                     [this](double distance, bool useAltitude,
                            bool altitudeVisible) {
                         if (mSpinBoxDist) {
                             QSignalBlocker blocker(mSpinBoxDist);
                             mSpinBoxDist->setValue(distance);
                         }
                         if (mAltitudeCheckBox) {
                             QSignalBlocker blocker(mAltitudeCheckBox);
                             mAltitudeCheckBox->setChecked(useAltitude);
                             mAltitudeCheckBox->setVisible(altitudeVisible);
                         }
                     });

    emit syncSettingsWidget(mDistance, mUseAltitude, mAltitudeExisting);

    widget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    widget->adjustSize();
    return widget;
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
void GeoDistance::readMetaData(MetaData* metaData)
{
    mMetaData.clear();
    mGpsData.clear();
    mIsGpsAvailable = false;
    mAltitudeExisting = false;

    if (!metaData) {
        return;
    }

    QStringList metaReader = metaData->availableMetaData();
    for (const QString& metaName : metaReader)
    {
        if (metaName.startsWith("GPS"))
        {
            MetaDataReader* reader = metaData->loadMetaData(metaName);
            if (!reader) {
                continue;
            }

            mMetaData = reader->getAllMetaData();
            if (mMetaData.isEmpty()) {
                continue;
            }

            QHash<QString, QVariant> metaHash = mMetaData[0].toHash();
            mAltitudeExisting = metaHash.contains("GPSAltitude");


            for (const QVariant& var : mMetaData)
            {
                bool ok = false;
                const QPointF point = gpsHashToLatLong(var, &ok);
                mGpsData.append(QPair<QPointF, bool>(point, ok));
            }

            mIsGpsAvailable = !mGpsData.isEmpty();

            return;
        }
    }
}

//==================================================================================================
QPointF GeoDistance::gpsHashToLatLong(const QVariant& hash, bool* ok) const
{
    bool latitudeOk = false;
    bool longitudeOk = false;
    QHash<QString, QVariant> gpsHash = hash.toHash();

    const double latitudeAbs = gpsHash.value("GPSLatitude").toDouble(&latitudeOk);
    const double longitudeAbs = gpsHash.value("GPSLongitude").toDouble(&longitudeOk);
    const QString latitudeRef = gpsHash.value("GPSLatitudeRef").toString();
    const QString longitudeRef = gpsHash.value("GPSLongitudeRef").toString();

    const bool validRef = (latitudeRef == "N" || latitudeRef == "S") &&
                          (longitudeRef == "E" || longitudeRef == "W");
    const bool valid = latitudeOk && longitudeOk && validRef;
    if (ok) {
        *ok = valid;
    }
    if (!valid) {
        return QPointF();
    }

    const double latitude = (latitudeRef == "N") ? latitudeAbs : -latitudeAbs;
    const double longitude = (longitudeRef == "E") ? longitudeAbs : -longitudeAbs;
    return QPointF(latitude, longitude);
}

//==================================================================================================
QGeoCoordinate GeoDistance::gpsHashToGeoCo(const QVariant& hash) const
{
    bool ok = false;
    const QPointF latLong = gpsHashToLatLong(hash, &ok);
    if (!ok) {
        return QGeoCoordinate();
    }

    if (!mAltitudeExisting || !mUseAltitude) {
        return QGeoCoordinate(latLong.x(), latLong.y());
    }

    QHash<QString, QVariant> gpsHash = hash.toHash();
    const double altitudeAbs = gpsHash.value("GPSAltitude").toDouble();
    const QString altitudeRef = gpsHash.value("GPSAltitudeRef").toString();
    const double altitude = (altitudeRef == "0") ? altitudeAbs : -altitudeAbs;

    return QGeoCoordinate(latLong.x(), latLong.y(), altitude);
}

double GeoDistance::distanceBetweenPoints(int first, int second) const
{
    if (first < 0 || second < 0 || first >= mMetaData.size() ||
        second >= mMetaData.size()) {
        return 0.0;
    }

    const QGeoCoordinate firstGPS = gpsHashToGeoCo(mMetaData.at(first));
    const QGeoCoordinate secondGPS = gpsHashToGeoCo(mMetaData.at(second));
    const QPointF firstLatLong(firstGPS.latitude(), firstGPS.longitude());
    const QPointF secondLatLong(secondGPS.latitude(), secondGPS.longitude());
    const double distance = greatCircleDistance(firstLatLong, secondLatLong);
    if (!mAltitudeExisting || !mUseAltitude) {
        return distance;
    }

    return qSqrt(qPow(distance, 2) +
                 qPow(firstGPS.altitude() - secondGPS.altitude(), 2));

}

double GeoDistance::greatCircleDistance(QPointF first, QPointF second) const
{
    const int r = 6371008;
    const double lat1 = first.x() * (M_PI / 180);
    const double lat2 = second.x() * (M_PI / 180);
    const double latDiff = (second.x() - first.x()) * (M_PI / 180);
    const double longDiff = (second.y() - first.y()) * (M_PI / 180);
    const double a = pow(sin(latDiff / 2), 2) +
                     cos(lat1) * cos(lat2) * pow(sin(longDiff / 2), 2);
    const double distance = 2 * r * asin(sqrt(a));
    return distance;

}
