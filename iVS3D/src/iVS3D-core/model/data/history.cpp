#include "history.h"
#include <QDebug>

History::History(ModelInputPictures *mip) : m_mip(mip)
{
    // create snapshot of initial state
    m_currentIndex = 0;
    m_history.append(mip->save());
}

History::~History()
{
    // wipe future
    for(auto m : m_history){
        delete m;
    }
    m_history.clear();
}

bool History::hasFuture()
{
    return m_currentIndex < m_history.size() - 1;
}

bool History::hasPast()
{
    return m_currentIndex != 0;
}

bool History::undo()
{
    // only undo if we have a past state to restore
    if(!hasPast()) return false;

    // get last memento and restore its state
    auto last = m_history.at(m_currentIndex - 1);
    m_mip->restore(last);

    m_currentIndex--;
    emit sig_historyChanged();
    return true;
}

bool History::redo()
{
    // only redo if we have a future state to restore
    if(!hasFuture()) return false;

    // get next memento and restore its state
    auto next = m_history.at(m_currentIndex + 1);
    m_mip->restore(next);

    m_currentIndex++;
    emit sig_historyChanged();
    return true;
}


bool History::restoreState(int index)
{
    if (index < 0 || index >= m_history.size()) {
        return false;
    }
    auto next = m_history.at(index);
    m_mip->restore(next);
    m_currentIndex = index;
    emit sig_historyChanged();
    return true;
}

int History::getCurrentIndex()
{
    return m_currentIndex;
}

void History::slot_save()
{
    // save state of mip
    auto mem = m_mip->save();
    int indexToRemove = m_history.size() - 1;
    while (indexToRemove > m_currentIndex) {
        auto toDelete = m_history.at(indexToRemove);
        delete toDelete;
        m_history.remove(indexToRemove);
        indexToRemove--;
    }

    m_history.append(mem);
    m_currentIndex++;
    emit sig_historyChanged();
}
