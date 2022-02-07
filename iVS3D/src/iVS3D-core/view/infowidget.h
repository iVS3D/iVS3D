#ifndef INFOWIDGET_H
#define INFOWIDGET_H

#include <QWidget>
#include "jsonEnum.h"

#include <QRegExp>
#include <QRegularExpression>
#include <QRegularExpressionMatch>

namespace Ui {
class InfoWidget;
}

/**
 * @class InfoWidget
 *
 * @ingroup View
 *
 * @brief The InfoWidget class displays given information to the user. The information is given as
 * key value pairs in a QMap and displayed in a table.
 *
 * @author Dominik Wüst
 *
 * @date 2021/03/03
 */
class InfoWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Creates an InfoWidget with the given title up top.
     * @param parent The parent QWidget
     * @param title The text to display up top
     */
    explicit InfoWidget(QWidget *parent = nullptr, QString title = "Info", bool dark = false);
    ~InfoWidget();

    /**
     * @brief setInfo displays the given key value pairs in a table.
     * @param info the key value pairs
     */
    void setInfo(QMap<QString,QString> info);

signals:
    void sig_openVideoPressed();
    void sig_openFolderPressed();

private slots:
    void on_toolButton_folder_clicked();
    void on_toolButton_video_clicked();

protected:
    Ui::InfoWidget *ui;
};

#endif // INFOWIDGET_H
