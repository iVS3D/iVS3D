#ifndef NOUIEXPORT_H
#define NOUIEXPORT_H

#include <QObject>
#include <QMap>
#include <QPoint>
#include <QRect>
#include <QVariant>

#include "exportcontroller.h"
#include "DataManager.h"
#include "logfile.h"
#include "logmanager.h"
#include "stringcontainer.h"
#include "terminalinteraction.h"

/**
 * @class noUIExport
 *
 * @ingroup Model
 *
 * @brief The noUIExport class is used to run the export without a ui.
 *
 * @author Daniel Brommer
 *
 * @date 2021/07/13
 */

class noUIExport: public QObject
{
    Q_OBJECT
public:
    /**
     * @brief noUIExport Consturctor to set the given settings
     * @param exportSettings Settings of the export
     * @param dm Pointer to the DataManager
     */
    noUIExport(QMap<QString, QVariant> exportSettings, DataManager* dm);
    /**
     * @brief runExport Runs the export. This method is similar to ExportController::slot_export
     */
    void runExport();

public slots:
    /**
     * @brief slot_exportFinished Connected to the ExportExecutor, called when the export is finished
     */
    void slot_exportFinished(int result);

signals:
    /**
     * @brief sig_exportFinished Connected to the AutomaticExecutor to signal that the export is finished
     */
    void sig_exportFinished();


private:
    QPoint m_resolution;
    QString m_path;
    QRect m_roi = QRect(0,0,0,0);
    bool m_useCrop;
    std::vector<bool> m_ITransfromSelection = std::vector<bool>();
    DataManager* m_dataManager;
    ExportExecutor* m_exportExec = nullptr;
    LogFile *m_logFile;
    TerminalInteraction* m_terminal;

    QPoint parseResolution(QString resolutionString);
};

#endif // NOUIEXPORT_H
