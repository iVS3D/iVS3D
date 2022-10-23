#ifndef EXPORTWIDGET_H
#define EXPORTWIDGET_H

#include <QWidget>
#include <QFileDialog>
#include <QCheckBox>

#include "applicationsettings.h" // used to determin text color depending on GUI style

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
     * @brief setResolutionList displays the given resList in the resolution dropdown.
     * @param resList the resolutions to display
     * @param idx the index of the resolution to select
     */
    void setResolutionList(QStringList resList, int idx = 0);

    /**
     * @brief setResolution displays the given resolution
     * @param resolution the resolution to display
     */
    void setResolution(QString resolution);

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
     * @brief enableReconstruct enables / disables the reconstruct button.
     * @param enabled button enabled if @a true, disabled otherwise
     */
    void enableReconstruct(bool enabled);

    /**
     * @brief setResolutionValid highlights the selected resolution if ist invalid.
     * @param valid Highlight resolution if @a false, normal resolution otherwise
     */
    void setResolutionValid(bool valid);

    /**
     * @brief returns the status of the use Crop Checkbox
     * @return @a true if the use Crop checkbox is checked, @a false otherwise
     */
    bool getCropStatus();

    /**
     * @brief Sets the status of the use Crop Checkbox
     * @return @a true if the use Crop checkbox should be checked, @a false otherwise
     */
    void setCropStatus(bool checked);

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

signals:

    /**
     * @brief [signal] sig_resChanged(...) is emitted if the resolution changed.
     * @param res the new resolution as string
     */
    void sig_resChanged(QString res);

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
     * @brief [signal] sig_cropExport() is emitted on crop export button pressed.
     */
    void sig_cropExport();

    /**
     * @brief [signal] sig_addAuto() is emitted on Add to automatic button pressed.
     */
    void sig_addAuto();

private slots:
    void on_pushButton_browse_clicked();
    void on_pushButton_export_clicked();
    void on_pushButton_reconstruct_clicked();
    void on_pushButton_cropExport_clicked();
    void on_pushButton_addAuto_clicked();
    void on_lineEdit_textChanged(const QString &text);
    void on_comboBox_currentTextChanged(const QString &text);

private:
    Ui::ExportWidget *ui;
    std::vector<QCheckBox*> m_checkboxes;
};

#endif // EXPORTWIDGET_H
