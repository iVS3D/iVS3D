#include "readerfactory.h"

#include <qapplication.h>
#include <qfileinfo.h>
#include <qmessagebox.h>
#include <qobject.h>
#include <qthread.h>

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
    if (info.isFile() && (forceBackupVideoReader ||
                          (!reader->isValid() && allowBackupReader()))) {
        // backup video reader
        reader = new BackupVideoReader(path, params);
    }

    if (!reader->isValid()) return nullptr;

    return reader;
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
