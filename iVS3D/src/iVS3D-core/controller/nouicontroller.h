#ifndef NOUICONTROLLER_H
#define NOUICONTROLLER_H

#include <QObject>
#include <QElapsedTimer>

#include "applicationsettings.h"
#include "transformmanager.h"
#include "DataManager.h"
#include "openexecutor.h"
#include "automaticexecsettings.h"
#include "automaticexecutor.h"
#include "logmanager.h"
#include "terminalinteraction.h"

/**
 * @class noUIController
 *
 * @ingroup Controller
 *
 * @brief The noUIController class is used to execute the noUI version of iVS3D. It will read and process the files given trough command line arguments.
 *
 * @author Daniel Brommer
 *
 * @date 2021/06/24
 */

class noUIController : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief noUIController Constructor which extracts the command line arguments
     * @param arguments QStringList with the command line arguments
     */
    explicit noUIController(QString inputPath, QString settingsPath, QString outputPath, QString logPath);

public slots:
    /**
     * @brief exec Imports the path given with -in then reads the exec file given with -auto and executes the file and then closes the application
     * @return
     */
    int exec();



private:
    QString m_autoPath;
    QString m_inputPath;
    QString m_outputPath;
    DataManager* m_dataManager = nullptr;
    bool m_importFinished = false;
    TerminalInteraction* m_terminal;
};

#endif // NOUICONTROLLER_H
