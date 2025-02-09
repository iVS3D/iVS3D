#ifndef WRITETODISKCOMMAND_H
#define WRITETODISKCOMMAND_H

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

class WriteToDiskCommand : public ImageCommand {
    QString folder;
    QString prefix;
    bool initialized;

    bool ensureFolderExists();

public:
    WriteToDiskCommand(QString folderpath, QString prefix = "");
    std::optional<QString> execute(ImageContext &ctx) override;
};

#endif //WRITETODISKCOMMAND_H
