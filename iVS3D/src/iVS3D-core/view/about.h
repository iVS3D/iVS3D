#ifndef ABOUT_H
#define ABOUT_H

#include <QDialog>

namespace Ui {
class About;
}

/**
 * @class About
 *
 * @ingroup View
 *
 * @brief The About class holds information about the authors and clients of this software
 *
 * @ingroup View
 *
 * @author Dominik Wüst
 *
 * @date 2021/02/08
 */
class About : public QDialog
{
    Q_OBJECT

public:
    explicit About(QWidget *parent = 0);
    ~About();

private:
    Ui::About *ui;
};

#endif // ABOUT_H
