#include "readerfactory.h"

#include <qapplication.h>
#include <qfileinfo.h>
#include <qglobal.h>
#include <qmessagebox.h>
#include <qobject.h>
#include <qtextedit.h>
#include <qthread.h>

#include <qdebug>

#include <memory>

#include "fastvideoreader.h"
#include "imagereader.h"
#include "stringcontainer.h"
#include "videoreader.h"

ReaderFactory::ReaderFactory() {}

Reader* ReaderFactory::createReader(QString path,
                                    std::shared_ptr<ReaderParams> params,
                                    QString backend) {
    QFileInfo info(path);
    std::unique_ptr<Reader> reader = nullptr;

    if (!info.exists()) return nullptr;
    if (info.isDir()) {
        // image reader
        reader = std::make_unique<ImageReader>(path, params);
    } else {
        // video reader
        if (backend == stringContainer::preciseVideoReader) {
            reader = std::make_unique<VideoReader>(path, params);
            if (!reader->isValid()) {
                reader.release();
                reader = nullptr;
                backend = switchReaderPrompt();
            }
        }
        if (backend == stringContainer::fastVideoReader) {
            reader = std::make_unique<FastVideoReader>(path, params);
        }
    }
    if (!reader) return nullptr;

    if (!reader->isValid()) {
        return nullptr;
    }

    return reader.release();
}

QString ReaderFactory::switchReaderPrompt() {
    auto f = []() -> QString {
        QMessageBox msgBox(QApplication::activeWindow());
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText(
            tr("The PreciseVideoReader backend is not available for this video. "
               "Do you want to switch the backend?"));
        msgBox.setDetailedText(
            tr("The frame precise video reader could not load the "
               "video. If you do not need frame perfect selection you can use "
               "the FastVideoReader backend, which is less robust during frame "
               "jumps. In case frame precise jumps are necessary we recommend "
               "reencoding the video with ffmpeg:"
               "\n\n \"ffmpeg -i VIDEO.mp4 -vf fps=FPS -vsync cfr -c:v libx264 "
               "-crf 0 -c:an OUTVIDEO.mp4\" "));
        msgBox.setStandardButtons(CONFIRM_BT | DECLINE_BT);
        msgBox.setDefaultButton(CONFIRM_BT);

        return msgBox.exec() == CONFIRM_BT ? stringContainer::fastVideoReader
                                           : QString();
    };

    QString result;
    QMetaObject::invokeMethod(qApp, f, Qt::BlockingQueuedConnection, &result);
    return result;
}