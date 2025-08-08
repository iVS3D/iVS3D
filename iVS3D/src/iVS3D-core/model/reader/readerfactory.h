#ifndef READERFACTORY_H
#define READERFACTORY_H

#include "reader.h"
#include "readerparams.h"

typedef std::function<Reader *(QString path, std::shared_ptr<ReaderParams> params)> AbstractReader;

class ReaderFactory
{
public:

    static ReaderFactory &instance(){
        static ReaderFactory INSTANCE;
        return INSTANCE;
    }

    Reader* createReader(QString path, std::shared_ptr<ReaderParams> params);

    bool reg(std::string name, AbstractReader builder);

private:
    std::map<std::string, AbstractReader> m_availablerReader;
    ReaderFactory();

};

template<typename Implementation>
Reader *builder(QString path, std::shared_ptr<ReaderParams> params){
    return new Implementation(path, params);
}

#define REGISTER_READER(name, impl) const bool res = ReaderFactory::instance().reg(name, builder<impl>);


#endif // READERFACTORY_H
