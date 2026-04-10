#pragma once
#include "reader.h"

class ReaderFactory {
   public:
    static ReaderFactory& instance() {
        static ReaderFactory INSTANCE;
        return INSTANCE;
    }

    std::pair<ReaderResult, std::unique_ptr<iReader>> createReader(
        const std::string& path);

   private:
    ReaderFactory();
};
