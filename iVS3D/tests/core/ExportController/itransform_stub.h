#ifndef ITRANSFORM_STUB_H
#define ITRANSFORM_STUB_H

#include "itransform.h"

#include <QMap>
#include <opencv2/video.hpp>

class ITransform_stub : public ITransform
{
public:
    ITransform_stub();
    QWidget* getSettingsWidget(QWidget*) {throw "not implemented";}
    QString getName() const ;
    QStringList getOutputNames();
    ITransform *copy() {throw "not implemented";}
    ImageList transform(uint idx, const cv::Mat &img);
    void enableCuda(bool) {throw "not implemented";}
    void setSettings(QMap<QString, QVariant>) {throw "not implemented";}
    QMap<QString, QVariant> getSettings() {throw "not implemented";}

};

#endif // ITRANSFORM_STUB_H
