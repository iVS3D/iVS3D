#ifndef EXIFTAGCOMMAND_H
#define EXIFTAGCOMMAND_H

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <QString>
#include <QDir>
#include <QRect>

#include <vector>
#include <memory>
#include <optional>

#include "reader.h"
#include "metadatareader.h"
#include "exportexif.h"
#include "imageprocessor.h"

class ExifTagCommand : public ImageCommand {
    MetaDataReader *gpsreader;
    ExportExif exif;

public:
    ExifTagCommand(MetaDataReader *gpsreader);
    std::optional<QString> execute(ImageContext &ctx) override;
};

#endif //EXIFTAGCOMMAND_H
