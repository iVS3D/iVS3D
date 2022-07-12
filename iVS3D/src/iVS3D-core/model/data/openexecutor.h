#ifndef OPENEXECUTOR_H
#define OPENEXECUTOR_H

#include "DataManager.h"

#include <QtConcurrent/QtConcurrent>
#include <QFuture>
#include <QFutureWatcher>

/**
 * @class OpenExecutor
 *
 * @ingroup Model
 *
 * @brief The OpenExecutor class tries to import files from given path. It does this in a concurrent thread and reports the import progress using signals.
 *
 * @author Dominik Wüst
 *
 * @date 2021/03/02
 */
class OpenExecutor : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief OpenExecutor Constructor, which stores the given pathToOpen and dataManger pointer as members
     *
     * @param pathToOpen is the path to a file, which should be opened
     * @param dataManager is usde to open the file
     */
    OpenExecutor(const QString &pathToOpen, DataManager *dataManager);
    /**
      * @brief ~OpenExecuter Destructor, which disconnects the futureWatcher and deletes it
      */
    ~OpenExecutor();
    /**
     * @brief open uses the given DataManger to open the file (defined through pathToOpen) in a seperate thread
     */
    void open();

public slots:
    /**
     * @brief [slot] slot_finished emits sig_process to show that it is done
     */
    void slot_finished();

signals:
    /**
     * @brief [signal] sig_finished signals the connected element that it is done
     * @param result, which shows if the progress was successfull
     */
    void sig_finished(int result);

private:
    QFuture<int> m_futurePics;
    QFutureWatcher<int> *m_futurePicsWatcher;
    DataManager *m_dataManager;
    QString m_path;

    bool isProjectPath();
};

#endif // OPENEXECUTOR_H
