#include "readerfactory.h"

ReaderFactory::ReaderFactory()
{

}

Reader *ReaderFactory::createReader(QString path)
{

    for (std::pair<std::string, AbstractReader> a : ReaderFactory::instance().m_availablerReader) {
        Reader* current = a.second(path);
        //TODO: if (current->isValid)
        return current;
    }

}

bool ReaderFactory::reg(std::string name, AbstractReader reader)
{
    return m_availablerReader.insert(std::make_pair(name, reader)).second;
}
