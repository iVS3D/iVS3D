#ifndef HISTORY_H
#define HISTORY_H

#include <QObject>
#include "modelinputpictures.h"

/**
 * @class History
 *
 * @ingroup Model
 *
 * @brief The History class stores and restores historical states of ModelInputPictrues in order to provide undo and redo functionality.
 * Therefor the current state of mip is stored by calling ore invoking slot_save(). undo() and redo() then allow to jump back and forth
 * between these historical states. Saving a new state will whipe the "future" states (f.e. states that were discareded by undo()). This
 * ensures a linear history.
 *
 * @author Dominik Wuest
 *
 * @date 2022/05/16
 */
class History : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief History creates a history for the given mip.
     * @param mip the ModelInputPictures object to store and restore
     */
    History(ModelInputPictures *mip);
    ~History();
    /**
     * @brief hasFuture checks if there is a future state to restore.
     * @return returns @a true if there is a state to restore on redo()
     */
    bool hasFuture();
    /**
     * @brief hasPastchecks if there is a past state to restore.
     * @return returns @a true if there is a state to restore on undo()
     */
    bool hasPast();
    /**
     * @brief undo restores the past state of mip if one was saved.
     * @return returns @a true if a past state was restored
     */
    bool undo();
    /**
     * @brief redo restores the future state of mip if one was saved.
     * @return returns @a true if a future state was restored
     */
    bool redo();


    bool restoreState(int index);

    int getCurrentIndex();

    void clear();

public slots:
    /**
     * @brief slot_save stores the current state of mip.
     */
    void slot_save();

signals:
    /**
     * @brief sig_historyChanged is emitted whenever a new state is added to the history.
     */
    void sig_historyChanged();

private:
    ModelInputPictures *m_mip;
    QVector<ModelInputPictures::Memento*> m_history;
    int m_currentIndex;

};

#endif // HISTORY_H
