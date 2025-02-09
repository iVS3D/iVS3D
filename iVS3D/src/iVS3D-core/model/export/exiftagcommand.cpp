#include "exiftagcommand.h"

ExifTagCommand::ExifTagCommand(MetaDataReader *gpsreader_) : gpsreader(gpsreader_) { }

std::optional<QString> ExifTagCommand::execute(ImageContext &ctx)
{
    if (ctx.filename.isEmpty()) {
        return "ERROR: no output file avaliable for adding EXIF data.";
    }

    QVariant gpsdata = gpsreader->getImageMetaData(ctx.index);

    std::vector<char> exifdata = exif.saveExif(ctx.filename, gpsdata);

    QFile file(ctx.filename);
    if (!file.open(QIODevice::ReadOnly)) {
        return "ERROR: Failed to open file for reading: " + ctx.filename;
    }

    QByteArray fileData = file.readAll();  // Read entire file into QByteArray
    file.close();

    QByteArray newData;
    QString fileExtension = QFileInfo(ctx.filename).suffix().toLower();

    if (fileExtension == "jpeg" || fileExtension == "jpg") {
        // JPEG: Insert EXIF data after the first 2 bytes (FF D8)
        newData.append(fileData.left(2));   // First 2 bytes
        newData.append(QByteArray::fromRawData(exifdata.data(), exifdata.size()));
        newData.append(fileData.mid(2));    // Remaining data
    }
    else if (fileExtension == "png") {
        // PNG: Insert EXIF data after the first 33 bytes (PNG header + IHDR)
        newData.append(fileData.left(33));  // First 33 bytes
        newData.append(QByteArray::fromRawData(exifdata.data(), exifdata.size()));
        newData.append(fileData.mid(33));   // Remaining data
    }
    else {
        return "ERROR: Unsupported file format " + fileExtension;
    }

    // Write the modified data back to the file
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return "ERROR: Failed to open file for writing: " + ctx.filename;
    }
    file.write(newData);
    file.close();

    return std::nullopt;
}
