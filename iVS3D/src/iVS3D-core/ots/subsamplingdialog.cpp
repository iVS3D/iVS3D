#include "subsamplingdialog.h"
#include "ui_subsamplingdialog.h"

namespace lib3d {
namespace ots {
namespace ui {

//==================================================================================================
SubsamplingDialog::SubsamplingDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::SubsamplingDialog)
{
  ui->setupUi(this);
}

//==================================================================================================
SubsamplingDialog::~SubsamplingDialog()
{
  delete ui;
}

//==================================================================================================
bool SubsamplingDialog::isSubsamplingActivated()
{
  return ui->gb_subsample->isChecked();
}

//==================================================================================================
uint SubsamplingDialog::getNthFrameValue()
{
  if(isSubsamplingActivated())
    return static_cast<uint>(ui->sb_everyNthFrame->value());
  else
    return 1;
}

} // namespace ui
} // namespace ots
} // namespeace lib3d