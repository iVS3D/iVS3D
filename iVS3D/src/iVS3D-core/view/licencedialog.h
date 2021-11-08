#ifndef LICENCEDIALOG_H
#define LICENCEDIALOG_H

#include <QDialog>

namespace Ui {
class LicenceDialog;
}

/**
 * @class LicenceDialog
 *
 * @ingroup View
 *
 * @brief The LicenceDialog class holds information about licences used in this project
 *
 * @ingroup View
 *
 * @author Patrick Binder
 *
 * @date 2021/04/15
 */
class LicenceDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LicenceDialog(QWidget *parent = nullptr);
    ~LicenceDialog();

private:
    Ui::LicenceDialog *ui;
};

#endif // LICENCEDIALOG_H
