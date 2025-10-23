#include "writetodiskcommand.h"

bool WriteToDiskCommand::ensureFolderExists()
{
    QDir dir;
    if(dir.exists(folder) || dir.mkpath(folder)) {
        return true;
    } else {
        qDebug() << "Failed to create output folder for images!";
        return false;
    }
}

WriteToDiskCommand::WriteToDiskCommand(QString folderpath, QString prefix_, QString format_) : folder(folderpath), prefix(prefix_), format(format_), initialized(false) { }

std::optional<QString> WriteToDiskCommand::execute(ImageContext &ctx)
{
    if(!initialized && !ensureFolderExists()) {
        return "ERROR: failed to create /images folder for export: " + folder;
    }

    ctx.filename = QDir::cleanPath(folder + "/" + prefix + QString::number(ctx.index, 10).rightJustified(8, '0') + "." + format);
    if (QFile::exists(ctx.filename) && !QFile::remove(ctx.filename)) {
        return "ERROR: failed to remove existing file '" + ctx.filename + "'!";
    }
    
    if (!cv::imwrite(ctx.filename.toStdString(), ctx.image, {cv::IMWRITE_JPEG_QUALITY, 100})) {
        return "ERROR: failed to export image: " + ctx.filename;
    }

    return std::nullopt;
}
