#ifndef LIB3D_OTS_UI_SUBSAMPLINGDIALOG_H
#define LIB3D_OTS_UI_SUBSAMPLINGDIALOG_H

#include <QDialog>

namespace lib3d {
namespace ots {
namespace ui {

namespace Ui {
class SubsamplingDialog;
}

/**
 * @brief Ui dialog class to allow user to set a subsampling rate when importing images from a sequence.
 */
class SubsamplingDialog : public QDialog
{
    Q_OBJECT

  public:
    explicit SubsamplingDialog(QWidget *parent = nullptr);
    ~SubsamplingDialog();

    /**
     * @return True, if subsampling is activated. I.e. if corresponding checkbox in dialog is set.
     */
    bool isSubsamplingActivated();

    /**
     * @return Value of subsampling rate, represented by every n-th frame that is to be copied.
     */
    uint getNthFrameValue();

  private:

    /// Pointer to dialog object.
    Ui::SubsamplingDialog *ui;
};

} // namespace ui
} // namespace ots
} // namespeace lib3d

#endif // LIB3D_OTS_UI_SUBSAMPLINGDIALOG_H
