#include "readerfactory.h"

#include <QDir>
#include <QFileInfo>
#include <QString>

#include "imagereader.h"
#include "readererror.h"
#include "videoreader.h"

ReaderFactory::ReaderFactory() {}

std::pair<ReaderResult, std::unique_ptr<iReader>> ReaderFactory::createReader(
    const std::string& path) {
    QFileInfo fileInfo = QFileInfo(QString::fromStdString(path));
    if (!fileInfo.exists()) return {ReaderResult::UnkownError, nullptr};

    std::unique_ptr<iReader> reader;
    if (fileInfo.isDir()) {
        // Directory -> ImageReader
        reader = std::make_unique<ImageReader>();
    } else if (fileInfo.isFile() && VideoReader::suffixValidation(fileInfo)) {
        // File -> VideoReader
        reader = std::make_unique<ImageReader>();
    } else {
        return {ReaderResult::FileError, nullptr};
    }
    ReaderResult initRes = reader->init(path);
    if (initRes != ReaderResult::Success) return {initRes, nullptr};

    return {ReaderResult::Success, std::move(reader)};
}
