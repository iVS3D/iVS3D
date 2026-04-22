#ifndef READERFACTORY_H
#define READERFACTORY_H

#include <qobject.h>

#include "reader.h"
#include "readerparams.h"

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
