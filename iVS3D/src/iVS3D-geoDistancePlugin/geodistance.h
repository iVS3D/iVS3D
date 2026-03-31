#ifndef IVS3D_GEODISTANCEPLUGIN_H
#define IVS3D_GEODISTANCEPLUGIN_H

#include <cmath>
#include <memory>

#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QGeoCoordinate>
#include <QMap>
#include <QObject>
#include <QVariant>
#include <QWidget>

#include "ibase.h"
#include "iselection.h"

#define DESCRIPTION_STYLE                                               \
    "color: rgb(58, 58, 58); border-left: 6px solid  rgb(58, 58, 58); " \
    "border-top-right-radius: 5px; border-bottom-right-radius: 5px; "   \
    "background-color: lightblue;"

#define NAME_Distance "Distance"
#define NAME_Altitude "Altitude"

class Reader;
class MetaData;

/**
 * @class GeoDistance
 *
 * @ingroup GeoDistancePlugin
 *
 * @brief Selects images based on traveled geographic distance between frames.
 *
 * @author Daniel Brommer
 * @author Dominik Wüst
 * @author Boitumelo Ruf
 */
class GeoDistance : public PLUG::IBase, public PLUG::ISelection
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "iVS3D.IBase")

    Q_INTERFACES(PLUG::IBase PLUG::ISelection)

  public:
    GeoDistance();
    ~GeoDistance() override = default;

    PLUG::SettingsWidgetResult getSettingsWidget() override;
    QString getName() const override;
    QMap<QString, QVariant> getSettings() const override;
    PLUG::ApplySettingsResult applySettings(
        const QMap<QString, QVariant>& settings) override;

    PLUG::InputLoadedResult onInputLoaded(const PLUG::InputData& input) override;
    PLUG::MetaDataLoadedResult onMetaDataLoaded(
        const PLUG::InputMetaData& inputMetaData) override;
    void onSelectedImagesChanged(
        const std::vector<uint>& selectedImages) override;

    PLUG::SelectionResult selectImages(const PLUG::SelectionData& data,
                                       volatile bool& cancelFlag) override;

  signals:
    void syncSettingsWidget(double distance, bool useAltitude,
                            bool altitudeVisible);

  private slots:
    void slot_distChanged(double n);
    void slot_altitudeCheckChanged(bool check);

  private:
    std::unique_ptr<QWidget> createSettingsWidget();
    void readMetaData(MetaData* metaData);

    QPointF gpsHashToLatLong(const QVariant& hash, bool* ok = nullptr) const;
    QGeoCoordinate gpsHashToGeoCo(const QVariant& hash) const;
    double distanceBetweenPoints(int first, int second) const;
    double greatCircleDistance(QPointF first, QPointF second) const;

  private:
    Reader* mReader = nullptr;

    QList<QVariant> mMetaData;
    QList<QPair<QPointF, bool>> mGpsData;

    bool mIsGpsAvailable = false;
    bool mAltitudeExisting = false;
    bool mUseAltitude = false;
    double mDistance = 1.0;

    QDoubleSpinBox* mSpinBoxDist = nullptr;
    QCheckBox* mAltitudeCheckBox = nullptr;
};

#endif // IVS3D_GEODISTANCEPLUGIN_H
