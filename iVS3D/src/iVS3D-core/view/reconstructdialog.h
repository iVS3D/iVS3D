#ifndef RECONSTRUCTDIALOG_H
#define RECONSTRUCTDIALOG_H

#include <QDialog>

namespace Ui {
class ReconstructDialog;
}

/**
 * @class ReconstructDialog
 *
 * @ingroup View
 *
 * @brief The ReconstructDialog class handles the dialog for selecting the startup of a reconstruction software aswell as creating batchfiles to easily execute the given parameters at a later point
 *
 * @author Lennart Ruck
 *
 * @date 2021/04/14
 */
class ReconstructDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief ReconstructDialog constructor configuring ui elements
     * @param parent constructor that sets up ui elements
     */
    explicit ReconstructDialog(QWidget *parent = 0);
    ~ReconstructDialog();

    /**
     * @brief ReconstructDialog constructor configuring ui elements
     * @param parent constructor that sets up ui elements
     * @param exportnameList list of all exports that were created in this session to pick from
     * @param reconstructtoolList list of all reconstruct tools from the ApplicationSettings
     */
    ReconstructDialog(QWidget *parent, QStringList exportnameList, QStringList reconstructtoolList);

    /**
     * @brief getExportName getter for the selected export
     * @return returns name of selected export
     */
    QString getExportName();
    /**
     * @brief getReconstructtool getter for the selected reconstruct software
     * @return returns name of selected reconstruct software
     */
    QString getReconstructtool();
    /**
     * @brief getStartArguments getter for the written start parameters for the reconstruct software
     * @return returns the written String containing the start parameters
     */
    QString getStartArguments();
    /**
     * @brief getCreateProject getter for the checkbox on whether to create a project file (for colmap) aswell or not
     * @return returns state of the "create project file" checkbox, @a true = create project file, @a false = don't create project file
     */
    bool getCreateProject();

signals:
    /**
     * @brief sig_cancel signals click on the abort button
     */
    void sig_cancel();
    /**
     * @brief sig_reconstruct signals click on reconstruct button
     */
    void sig_reconstruct();
    /**
     * @brief sig_reconstructtoolChange signals when the selected reconstruct software has changed
     */
    void sig_reconstructtoolChange();
    /**
     * @brief sig_exportChange signals when the selected export has changed
     */
    void sig_exportChange();
private slots:
    void on_cancelbutton_clicked();
    void on_reconstructButton_clicked();

private:
    Ui::ReconstructDialog *ui;
};

#endif // RECONSTRUCTDIALOG_H
