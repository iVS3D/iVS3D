#ifndef COPYFILECOMMAND_H
#define COPYFILECOMMAND_H

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
#include "imageprocessor.h"

class CopyFileCommand : public ImageCommand {
    std::vector<std::string> files;
    QString folderpath;
    bool initialized;

public:
    CopyFileCommand(std::vector<std::string> &files_, QString folderpath_);
    std::optional<QString> execute(ImageContext &ctx) override;
};

#endif //COPYFILECOMMAND_H
