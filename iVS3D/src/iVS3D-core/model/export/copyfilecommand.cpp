#include "copyfilecommand.h"

CopyFileCommand::CopyFileCommand(std::vector<std::string> files_, QString folderpath_) : files(files_), folderpath(folderpath_), initialized(false) { }

std::optional<QString> CopyFileCommand::execute(ImageContext &ctx)
{
    QString sourcepath = QString::fromStdString(files[ctx.index]);
    QString name = sourcepath.split("/").last();
    ctx.filename = QDir::cleanPath(folderpath + "/" + name);

    if(!initialized){
        if(!QDir().exists(folderpath) && !QDir().mkpath(folderpath)) {
            return "ERROR: failed to create output folder for plugin: " + folderpath;
        }
        initialized = true;
    }

    // check if the source file and destination file are the same
    if (QFile::exists(ctx.filename) && QFileInfo(sourcepath) == QFileInfo(ctx.filename)) {
        return std::nullopt; // no need to copy, source and destination are the same
    }

    if (QFile::exists(ctx.filename) && !QFile::remove(ctx.filename)) {
        return "ERROR: failed to remove existing file '" + ctx.filename + "'!";
    }

    if(!QFile::copy(sourcepath, ctx.filename)) {
        return "ERROR: failed to copy image file '" + sourcepath + "' to '" + ctx.filename + "'!";
    }
    return std::nullopt;
}
