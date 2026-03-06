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
    explicit OutputWidget(QWidget *parent = nullptr, QString title = "Output");

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
     * @brief enableExportPathChange enables / disables modification of the export path
     * @param enabled @a true = modify path, @a false = fixed path
     */
    void enableExportPathChange(bool enabled);
    /**
     * @brief enableReconstruct enables / disables the reconstruct button
     * @param enabled @a true = enable button, @a false = disable button
     */
    void enableReconstruct(bool enabled);
    /**
     * @brief showProgress switches from showing export settings to export progress bar
     */
    void showProgress();
    /**
     * @brief showExportOptions switches from showing export progress bar to export settings
     */
    void showExportOptions();

    /**
     * @brief disableCreateFilesWidget will disable the create files for widget on the exportWidget
     * @param @a true shows the widget, @a false will hide it
     */
    void enableCreateFilesWidget(bool enable);

    void setResolutionList(QStringList resList, int idx);
    void setResolution(QString resolution);
    void setResolutionValid(bool valid);

    QString getExportFormat();
    bool setOutputFormat(QString format);

    /**
     * @brief setAltitudeVisible will show or hide the altitude selector
     * @param visible @a true shows the selector, @a false will hide it
     */
    void setAltitudeVisible(bool visible);

    /**
     * @brief setAltitude sets the value of the altitude selector
     * @param altitude
     */
    void setAltitude(double altitude);

    double getAltitude();
    void enableFormat(QString format, bool enable);

    std::shared_ptr<MaskStackView> getMaskStackView();

signals:
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

    /**
     * @brief [signal] sig_resChanged(...) is emitted if the resolution in the combo box is changed
     * @param resolution The selected resolution as a string
     */
    void sig_resChanged(QString resolution);

    /**
     * @brief [signal] sig_altitudeChanged() is emitted when the altitude is changed by the user.
     */
    void sig_altitudeChanged(double altitude);



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

    /**
     * @brief [slot] slot_displayWarning appends a warning to the warning list
     * @param warning Warning text
     */
    void slot_displayWarning(QString warning);

    /**
     * @brief [slot] slot_clearWarnings clears all currently shown warnings
     */
    void slot_clearWarnings();

private slots:
    void slot_pathChanged(const QString &path);
    void slot_addAuto();
    void slot_export();
    void slot_reconstruct();
    void slot_abort();

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
