#include "readerfactory.h"

#include <qfileinfo.h>
#include <qmessagebox.h>

#include "backupvideoreader.h"
#include "imagereader.h"
#include "videoreader.h"

ReaderFactory::ReaderFactory() {}

Reader* ReaderFactory::createReader(QString path,
                                    std::shared_ptr<ReaderParams> params,
                                    bool forceBackupVideoReader) {
    QFileInfo info(path);
    Reader* reader;
    if (!info.exists()) return nullptr;
    if (info.isDir()) {
        // image reader
        reader = new ImageReader(path, params);
    }
    if (info.isFile()) {
        // ffmpeg video reader
        reader = new VideoReader(path, params);
    }
    bool enableBackupVidReader = !reader->isValid() && allowBackupReader();
    if (info.isFile() && (forceBackupVideoReader || enableBackupVidReader)) {
        // backup video reader
        reader = new BackupVideoReader(path, params);
    }

    if (!reader->isValid()) return nullptr;

    return reader;
}

bool ReaderFactory::allowBackupReader() {
    QMessageBox::StandardButton confirmBt = QMessageBox::Yes;
    QMessageBox::StandardButton declineBt = QMessageBox::Abort;

    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setText(tr("Do you want to use the backup video reader?"));
    msgBox.setDetailedText(
        tr("The frame precise custom video reader could not load the "
           "video. If you do not need frame perfect selection you can use "
           "the backup video reader, which is based on OpenCV. If you "
           "abort this the video can not be loaded."));
    msgBox.setStandardButtons(confirmBt | declineBt);
    msgBox.setDefaultButton(confirmBt);
    int ret = msgBox.exec();

    return ret == confirmBt;
}
