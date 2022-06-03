#ifndef READERFACTORY_H
#define READERFACTORY_H

#include "reader.h"

typedef std::function<Reader *(QString path)> AbstractReader;

class ReaderFactory
{
public:


    static ReaderFactory &instance(){
        static ReaderFactory INSTANCE;
        return INSTANCE;
    }

    Reader* createReader(QString path);

     bool reg(std::string name, AbstractReader builder);

private:
    std::map<std::string, AbstractReader> m_availablerReader;
    ReaderFactory();

};

template<typename Implementation>
Reader *builder(){
    return new Implementation();
}

#define REGISTER_READER(name, impl) const bool res = ReaderFactory::instance().reg(name, builder<impl>);

#endif // READERFACTORY_H
