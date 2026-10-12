#ifndef READERFACTORY_H
#define READERFACTORY_H

#include <qabstractbutton.h>
#include <qmessagebox.h>
#include <qobject.h>
#include <qobjectdefs.h>

#include "reader.h"
#include "readerparams.h"
#include "stringcontainer.h"

#define CONFIRM_BT QMessageBox::Yes
#define DECLINE_BT QMessageBox::Cancel

class ReaderFactory : QObject {
   public:
    static ReaderFactory& instance() {
        static ReaderFactory INSTANCE;
        return INSTANCE;
    }

    Reader* createReader(QString path, std::shared_ptr<ReaderParams> params,
                         QString backend = stringContainer::fastVideoReader);

   private:
    ReaderFactory();
    QString switchReaderPrompt();
};

#endif  // READERFACTORY_H
