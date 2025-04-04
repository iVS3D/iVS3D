#ifndef LIB3D_OTS_UI_COLMAPWRAPPER_SETTINGSDIALOG_H
#define LIB3D_OTS_UI_COLMAPWRAPPER_SETTINGSDIALOG_H

// Qt
#include <QObject>
#include <QDialog>
#include <QLineEdit>
#include <QTableWidgetItem>

#include "../colmapwrapper.h"
#include "applicationsettings.h"

namespace lib3d {
namespace ots {
namespace ui {
namespace colmapwrapper {

namespace Ui {
  class SettingsDialog;
}

/**
 * @brief Class providing dialog for settings of ColmapWrapper
 * @author Ruf, Boitumelo <boitumelo.ruf@iosb.fraunhofer.de>
 */
class SettingsDialog : public QDialog
{
    Q_OBJECT

  public:
    explicit SettingsDialog(lib3d::ots::ColmapWrapper* ipWrapper,
                                            QWidget *parent = nullptr);
    ~SettingsDialog();

  public slots:
    void onUpdateToDarkTheme();
    void onUpdateToLightTheme();
    void onShow();
    void onStatusChanged();
    void onQuit();

  private slots:
    void onLocalColmapBinaryPushButtonPressed();
    void onLocalOpenMVSBinaryPushButtonPressed();
    void onLocalWorkspacePushButtonPressed();
    void onSelectMountPntPushButtonPressed();
    void onConnectionComboBoxIdxChanged(int idx);
    void onInstallScriptsPushButtonPressed();
    void onApply();
    void onCancel();
    void onRestoreDefaults();
    void onAddCustomCommand();
    void onRemoveCustomCommand(int row);
    void onClickonExperimental();

  private:
    Ui::SettingsDialog *ui;

    /// Member pointer to wrapper
    lib3d::ots::ColmapWrapper* mpColmapWrapper;

    lib3d::ots::ColmapWrapper::SSetupResults mSetupResults;

    void settingsChanged();
    void updateStatusMsg(const QPair<ColmapWrapper::ESetupTestResult,QString> &test, QLineEdit *input = nullptr);
    void loadCustomCommands();
    void updateRemoveButtonCallbacks();
    void addEmptyRow();



};

} // namespace colmapwrapper
} // namespace ui
} // namespace ots
} // namespeace lib3d

#endif // LIB3D_OTS_UI_COLMAPWRAPPER_SETTINGSDIALOG_H
