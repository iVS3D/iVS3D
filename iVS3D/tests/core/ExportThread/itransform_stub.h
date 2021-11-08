#ifndef ITRANSFORM_STUB_H
#define ITRANSFORM_STUB_H

#include "itransform.h"

#include <opencv2/video.hpp>

class ITransform_stub : public ITransform
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "pse.iVS3D.ITransform")   // implement interface as plugin, use the iid as identifier
    Q_INTERFACES(ITransform)                        // declare this as implementation of ITransform interface

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
