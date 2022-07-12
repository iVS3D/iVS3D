#include "history.h"

History::History(ModelInputPictures *mip) : m_mip(mip)
{
    // create snapshot of initial state
    m_curr = mip->save();
}

History::~History()
{
    delete m_curr;
    // wipe future
    for(auto m : m_future){
        delete m;
    }
    m_future.clear();
    // wipe past
    for(auto m : m_past){
        delete m;
    }
    m_past.clear();
}

bool History::hasFuture()
{
    return !m_future.isEmpty();
}

bool History::hasPast()
{
    return !m_past.isEmpty();
}

bool History::undo()
{
    // only undo if we have a past state to restore
    if(!hasPast()) return false;

    // get last memento and restore its state
    auto last = m_past.pop();
    m_mip->restore(last);

    // store as future state for potential redo
    m_future.push(m_curr);
    m_curr = last;
    emit sig_historyChanged();
    return true;
}

bool History::redo()
{
    // only redo if we have a future state to restore
    if(!hasFuture()) return false;

    // get next memento and restore its state
    auto next = m_future.pop();
    m_mip->restore(next);

    // store as past state for potential redo
    m_past.push(m_curr);
    m_curr = next;
    emit sig_historyChanged();
    return true;
}

QStringList History::historyList()
{
    QStringList hist;
    for(auto mem : m_past){
        auto dat = mem->getSnapshotDate().toString("dd.MM.yyyy");
        auto time = mem->getSnapshotDate().toString("hh:mm");
        hist.append(dat + " - " + time);
    }
    return hist;
}

void History::slot_save()
{
    // save state of mip
    auto mem = m_mip->save();

    // store in past
    if(m_curr)
        m_past.push(m_curr);

    m_curr = mem;

    // wipe future
    for(auto m : m_future){
        delete m;
    }
    m_future.clear();

    emit sig_historyChanged();
}
