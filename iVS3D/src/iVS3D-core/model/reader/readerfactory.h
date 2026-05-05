#ifndef READERFACTORY_H
#define READERFACTORY_H

#include <qabstractbutton.h>
#include <qmessagebox.h>
#include <qobject.h>
#include <qobjectdefs.h>

#include "reader.h"
#include "readerparams.h"

#define CONFIRM_BT QMessageBox::Yes
#define DECLINE_BT QMessageBox::Abort

class ReaderFactory : QObject {
   public:
    static ReaderFactory& instance() {
        static ReaderFactory INSTANCE;
        return INSTANCE;
    }

    Reader* createReader(QString path, std::shared_ptr<ReaderParams> params,
                         bool forceBackupVideoReader = false);

   private:
    ReaderFactory();
    bool allowBackupReader();
};

#endif  // READERFACTORY_H
