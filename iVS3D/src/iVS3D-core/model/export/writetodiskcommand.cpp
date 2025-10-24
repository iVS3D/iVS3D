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

WriteToDiskCommand::WriteToDiskCommand(QString folderpath, QString prefix_, QString format_, std::vector<std::string> files_) : folder(folderpath), prefix(prefix_), format(format_), initialized(false), files(files_) { }

std::optional<QString> WriteToDiskCommand::execute(ImageContext &ctx)
{
    if(!initialized && !ensureFolderExists()) {
        return "ERROR: failed to create /images folder for export: " + folder;
    }

    if (!files.empty() && !files[ctx.index].empty()) {
        // use original filename if available
        // extract filename from full path
        QString sourcepath = QString::fromStdString(files[ctx.index]);
        QString name = sourcepath.split("/").last();
        // remove file extension
        name = name.left(name.lastIndexOf('.'));
        name += "." + format;
        ctx.filename = QDir::cleanPath(folder + "/" + name);
    } else {
        // generate filename based on index
        ctx.filename = QDir::cleanPath(folder + "/" + prefix + QString::number(ctx.index, 10).rightJustified(8, '0') + "." + format);
    }
    if (QFile::exists(ctx.filename) && !QFile::remove(ctx.filename)) {
        return "ERROR: failed to remove existing file '" + ctx.filename + "'!";
    }
    
    if (!cv::imwrite(ctx.filename.toStdString(), ctx.image, {cv::IMWRITE_JPEG_QUALITY, 100})) {
        return "ERROR: failed to export image: " + ctx.filename;
    }

    return std::nullopt;
}
