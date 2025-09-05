#ifndef EXPORTWIDGET_H
#define EXPORTWIDGET_H

#include <QWidget>
#include <QFileDialog>
#include <QCheckBox>
#include <QPushButton>
#include <QDoubleSpinBox>

#include "applicationsettings.h" // used to determin text color depending on GUI style

#define EXPORT_FORMAT_SAME_AS_INPUT "same as input"

namespace Ui {
class ExportWidget;
}

/**
 * @class ExportWidget
 *
 * @ingroup View
 *
 * @brief The ExportWidget class provides a user interface to configure export:
 *  - export path
 *  - export resolution
 *  - start export
 * It also allows to start a recnstruction using the reconstruct button.
 *
 * @author Dominik Wüst
 *
 * @date 2021/03/03
 */
class ExportWidget : public QWidget
{
    Q_OBJECT

public:

    /**
     * @brief Creates an ExportWidget with the given QWidget as parent.
     * @param parent The parent for displaying
     */
    explicit ExportWidget(QWidget *parent = nullptr, QStringList transformList = QStringList());
    ~ExportWidget();

    /**
     * @brief setOutputPath sets the text in the path text box to given path.
     * @param path the path to display
     */
    void setOutputPath(QString path);

    /**
     * @brief enableExport enables / disables the export button.
     * @param enabled button enabled if @a true, disabled otherwise
     */
    void enableExport(bool enabled);

    /**
     * @brief enableExportPathEditable enables / disables editting of the export path.
     * @param editting enabled if @a true, disabled otherwise
     */
    void enableExportPathEditable(bool enabled);

    /**
     * @brief enableReconstruct enables / disables the reconstruct button.
     * @param enabled button enabled if @a true, disabled otherwise
     */
    void enableReconstruct(bool enabled);

    /**
     * @brief getSelectedITransforms returns a list with @a list[i] = @a true if @a iTransform[i] is selected.
     * @return The list
     */
    std::vector<bool> getSelectedITransforms();

    /**
     * @brief setSelectedITransforms selects the @a iTransform[i] if @a selected[i] = @a true.
     * @param selection the iTransforms selection
     * @return @a true if selection.size matches iTransform.count
     */
    bool setSelectedITransforms(std::vector<bool> selection);

    /**
     * @brief disableCreateFilesWidget will disable the create files for widget
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

signals:

    /**
     * @brief [signal] sig_pathChanged(...) is emitted if the selected path changed.
     * @param path the new path as string
     */
    void sig_pathChanged(QString path);

    /**
     * @brief [signal] sig_export() is emitted on export button pressed.
     */
    void sig_export();

    /**
     * @brief [signal] sig_reconstruct() is emitted on reconstruct button pressed.
     */
    void sig_reconstruct();

    /**
     * @brief [signal] sig_addAuto() is emitted on Add to automatic button pressed.
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

private slots:
    void on_pushButton_browse_clicked();
    void on_pushButton_export_clicked();
    void on_pushButton_reconstruct_clicked();
    void on_pushButton_addAuto_clicked();
    void on_lineEdit_textChanged(const QString &text);
    void on_spinBox_altitude_valueChanged(double d);

private:
    Ui::ExportWidget *ui;
    std::vector<QCheckBox*> m_checkboxes;

    QDoubleSpinBox *m_altitudeSpinBox;

};

#endif // EXPORTWIDGET_H
