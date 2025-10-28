#ifndef LIB3D_OTS_UI_COLMAPWRAPPER_NEWPRODUCTDIALOG_H
#define LIB3D_OTS_UI_COLMAPWRAPPER_NEWPRODUCTDIALOG_H

// Std
#include <vector>

// Qt
#include <QDialog>

#include "../colmapwrapper.h"

namespace lib3d {
namespace ots {
namespace ui {
namespace colmapwrapper {

namespace Ui {
class NewProductDialog;
}

/**
 * @brief Class for ui dialog to create new product.
 * @author Ruf, Boitumelo <boitumelo.ruf@iosb.fraunhofer.de>
 */
class NewProductDialog : public QDialog
{
    Q_OBJECT

    //--- METHOD DECLERATION ---//

  public:
    explicit NewProductDialog(ColmapWrapper *ipWrapper, QWidget *parent = nullptr);
    ~NewProductDialog();

    std::vector<ColmapWrapper::SJob> getNewJobList() const;

  public slots:
    void onUpdateToDarkTheme();
    void onUpdateToLightTheme();
    void onShow();


  private slots:

    void onCustomCommandClicked();
    void onProdCameraPosesClicked();
    void onProdPointCloudClicked();
    void onProdMeshClicked();

    void onPbSelectImageDirectoryClicked();
    void onPbSelectMaskDirectoryClicked();

    void updateSettingsVisibility();

    void onAccepted();

    void validateImagePath();
    void validateMaskPath();

    void validateSequenceName();

    //--- MEMBER DECLERATION ---//

  private:
    Ui::NewProductDialog *ui;

    /// Member pointer to wrapper
    lib3d::ots::ColmapWrapper* mpColmapWrapper;

    /// list of available sequences
    std::vector<ColmapWrapper::SSequence> mAvailableSeqs;

    /// list of new jobs created
    std::vector<ColmapWrapper::SJob> mNewJobList;

    void enableSaveButtonState();

    bool isImagePathValid;
    bool isSequenceNameValid;
};

} // namespace colmapwrapper
} // namespace ui
} // namespace ots
} // namespace lib3d
#endif // LIB3D_OTS_UI_COLMAPWRAPPER_NEWPRODUCTDIALOG_H
