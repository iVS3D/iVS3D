#include "readerfactory.h"

#include <qapplication.h>
#include <qfileinfo.h>
#include <qmessagebox.h>
#include <qobject.h>
#include <qthread.h>

#include <memory>

#include "backupvideoreader.h"
#include "imagereader.h"
#include "videoreader.h"

ReaderFactory::ReaderFactory() {}

Reader* ReaderFactory::createReader(QString path,
                                    std::shared_ptr<ReaderParams> params,
                                    bool forceBackupVideoReader) {
    QFileInfo info(path);
    std::unique_ptr<Reader> reader;
    if (!info.exists()) return nullptr;
    if (info.isDir()) {
        // image reader
        reader = std::make_unique<ImageReader>(path, params);
    } else {
        // video reader
        bool useBackupReader = forceBackupVideoReader;
        // ffmpeg reader
        if (!useBackupReader) {
            reader = std::make_unique<VideoReader>(path, params);
        }
        // switch to backup reader if video could not be read
        if (!reader->isValid()) {
            useBackupReader = allowBackupReader();
        }
        // backup reader
        if (useBackupReader) {
            reader = std::make_unique<BackupVideoReader>(path, params);
        }
    }
    if (!reader) return nullptr;

    if (!reader->isValid()) {
        return nullptr;
    }

    return reader.release();
}

bool ReaderFactory::allowBackupReader() {
    QMessageBox msgBox;
    msgBox.setModal(true);
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setText(tr("Do you want to use the backup video reader?"));
    msgBox.setDetailedText(
        tr("The frame precise custom video reader could not load the "
           "video. If you do not need frame perfect selection you can use "
           "the backup video reader, which is based on OpenCV. If you "
           "abort this the video can not be loaded."));
    msgBox.setStandardButtons(CONFIRM_BT | DECLINE_BT);
    msgBox.setDefaultButton(CONFIRM_BT);

    msgBox.show();
    int res = -1;
    while (res != CONFIRM_BT && res != DECLINE_BT) {
        QApplication::processEvents();
        res = msgBox.result();
        QThread::msleep(100);
    }

    return res == CONFIRM_BT;
}
