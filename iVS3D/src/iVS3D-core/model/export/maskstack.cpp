#include "maskstack.h"

MaskStack::MaskStack(QObject* parent)
    : QObject(parent), m_nextId(0)
{
}

void MaskStack::addRecord(const MaskRecord& record)
{
    // Create a new record with the next available ID
    MaskRecord newRecord = record;
    newRecord.id = m_nextId++;

    m_records.append(newRecord);

    emit sig_recordAdded(newRecord);
    emit sig_stackChanged();
}

bool MaskStack::removeRecord(int index)
{
    if (index < 0 || index >= m_records.size()) {
        return false;
    }

    MaskRecord removed = m_records[index];
    m_records.removeAt(index);

    emit sig_recordRemoved(removed);
    emit sig_stackChanged();

    return true;
}

bool MaskStack::removeRecordById(int id)
{
    for (int i = 0; i < m_records.size(); ++i) {
        if (m_records[i].id == id) {
            return removeRecord(i);
        }
    }
    return false;
}

void MaskStack::clear()
{
    if (m_records.isEmpty()) {
        return;  // No need to emit if already empty
    }

    m_records.clear();

    emit sig_stackCleared();
    emit sig_stackChanged();
}

int MaskStack::size() const
{
    return m_records.size();
}

bool MaskStack::isEmpty() const
{
    return m_records.isEmpty();
}

const MaskRecord* MaskStack::getRecord(int index) const
{
    if (index < 0 || index >= m_records.size()) {
        return nullptr;
    }
    return &m_records[index];
}

QVector<MaskRecord> MaskStack::getAllRecords() const
{
    return m_records;
}

const MaskRecord* MaskStack::getRecordById(int id) const
{
    for (const auto& record : m_records) {
        if (record.id == id) {
            return &record;
        }
    }
    return nullptr;
}
