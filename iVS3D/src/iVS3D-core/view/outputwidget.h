#ifndef OUTPUTWIDGET_H
#define OUTPUTWIDGET_H

#include <QWidget>
#include <QLayout>
#include <QGroupBox>

#include "exportwidget.h"
#include "progresswidget.h"
#include "progressdisplay.h"

/**
 * @class OutputWidget
 *
 * @ingroup View
 *
 * @brief The OutputWidget class is responsible for controlling the Widget on the bottom right holding export details and the export progress bar
 * It uses different Signals&Slots to communicate with the Exportcontroller, aswell as controlling and parsing the data in its ui elements.
 * Holds two different Widgets ExportWidget m_exportW and ProgressWidget m_progressW to easily switch between showing export settings and export progress bar.
 *
 * @author Lennart Ruck
 *
 * @date 2021/02/08
 */
class OutputWidget : public QWidget
{
    Q_OBJECT
public:
    /**
     * @brief OutputWidget constructor, initializes member variables aswell as connecting its Signals&Slots and configuring its ui elements
     * @param parent (ui-)class which holds this OutputWidget
     * @param title name of the Widget on the MainWindow
     */
    explicit OutputWidget(QWidget *parent = nullptr, QString title = "Output", QStringList transformList = QStringList());
    /**
     * @brief setResolutionList sets the availible resolutions to pick from the combobox for the export
     * This method is a passthrough to the ExportWidget having the exact same signature
     * @param resList contains all resolutions as a String (f.e. "1280 x 720 (HD)")
     * @param idx index to be preselected in the drop down
     */
    void setResolutionList(QStringList resList, int idx = 0);
    /**
     * @brief setResolution displays the given resolution
     * @param resolution the resolution to display
     */
    void setResolution(QString resolution);
    /**
     * @brief setOutputPath sets the path text in the output folder text field
     * @param path String to display
     */
    void setOutputPath(QString path);
    /**
     * @brief enableExport enables / disables the export button
     * @param enabled @a true = enable button, @a false = disable button
     */
    void enableExport(bool enabled);
    /**
     * @brief enableReconstruct enables / disables the reconstruct button
     * @param enabled @a true = enable button, @a false = disable button
     */
    void enableReconstruct(bool enabled);
    /**
     * @brief setResolutionValid sets validity of (export) resolution (black = valid, red = invalid)
     * @param valid @a true = valid, @a false = invalid
     */
    void setResolutionValid(bool valid);
    /**
     * @brief showProgress switches from showing export settings to export progress bar
     */
    void showProgress();
    /**
     * @brief showExportOptions switches from showing export progress bar to export settings
     */
    void showExportOptions();
    /**
     * @brief getCropStatus getter for the state of the "use crop" checkbox
     * @return state of "use crop" checkbox, @a true = use cropping, @a false = do not use cropping
     */
    bool getCropStatus();
    /**
     * @brief setCropStatus setter for the state of the "use crop" checkbox
     * @param checked state of "use crop" checkbox, @a true = use cropping, @a false = do not use cropping
     */
    void setCropStatus(bool checked);

    /**
     * @brief getSelectedITransformMasks getter for all selected masks that should be exported
     * @return returns list of all masks to be exported
     */
    std::vector<bool> getSelectedITransformMasks();

    /**
     * @brief setSelectedITransformMasks setter for masks that should be exported
     * @param selection masks to select
     * @return @a true if set successful
     */
    bool setSelectedITransformMasks(std::vector<bool> selection);

    /**
     * @brief disableCreateFilesWidget will disable the create files for widget on the exportWidget
     * @param @a true shows the widget, @a false will hide it
     */
    void enableCreateFilesWidget(bool enable);

signals:
    /**
     * @brief sig_resChanged gets emitted once the export resolution has been altered
     * @param res String holding the latest export resolution
     */
    void sig_resChanged(QString res);
    /**
     * @brief sig_pathChanged gets emitted once the export path has been altered
     * @param path String holding the latest export path
     */
    void sig_pathChanged(QString path);
    /**
     * @brief sig_export signal for the ExportController to start the export (without cropping)
     */
    void sig_export();
    /**
     * @brief sig_cropExport signal for the ExportController to start the export with cropping
     */
    void sig_cropExport();
    /**
     * @brief sig_reconstruct signal for the ExportController to start the reconstruction software (and create projectfiles and batchfiles)
     */
    void sig_reconstruct();
    /**
     * @brief sig_abort signal for the ExportController to abort the export (triggered by user)
     */
    void sig_abort();
    /**
     * @brief sig_addAuto signal to add the current output to the automatic execution (triggered by user)
     */
    void sig_addAuto();

public slots:
    /**
     * @brief [slot] slot_displayProgress gets invoked by the ExportThread giving new progress information
     * @param progress [0-100] progress percentage
     * @param currentOperation QString describing the current operation
     */
    void slot_displayProgress(int progress, QString currentOperation);
    /**
     * @brief [slot] slot_displayMessage gets invoked by the ExportThread giving a new information/message
     * @param message QString holds the message
     */
    void slot_displayMessage(QString message);

private slots:
    void slot_resChanged(const QString &res);
    void slot_pathChanged(const QString &path);
    void slot_addAuto();
    void slot_export();
    void slot_reconstruct();
    void slot_abort();
    void slot_cropExport();

private:
    /**
     * @brief m_exportW Widget holding the ui elements for export settings
     */
    ExportWidget *m_exportW;
    /**
     * @brief m_progressW holding the ui elements for the export progress bar
     */
    ProgressWidget *m_progressW;
    QLayout *m_layout;
};

#endif // OUTPUTWIDGET_H
