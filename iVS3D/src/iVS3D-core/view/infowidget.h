#ifndef INFOWIDGET_H
#define INFOWIDGET_H

#include <QWidget>
#include "operationstack.h"
#include "stringcontainer.h"
#include "cvmat_qmetadata.h"

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
    explicit InfoWidget(QWidget *parent = nullptr, QString title = "Info", ColorTheme theme = LIGHT);
    ~InfoWidget();

    /**
     * @brief setInfo displays the given key value pairs in a table.
     * @param info the key value pairs
     */
    void setInfo(QMap<QString,QString> info);

    /**
     * @brief enableOpenMetaData enables "Open Meta Data" button in the widget
     *
     * @param status true = enable
     */
    void enableOpenMetaData(bool status, QString tooltip = QString());

    /**
     * @brief enableInputImages enables "Open Images" button in the widget
     *
     * @param status true = enable
     */
    void enableOpenImages(bool status, QString tooltip = QString());

    /**
     * @brief enableInputVideo enables "Open Video" button in the widget
     *
     * @param status true = enable
     */
    void enableOpenVideo(bool status, QString tooltip = QString());

    /**
     * @brief setColorTheme updates text and icon colors according to a given theme
     * @param theme
     */
    void setColorTheme(ColorTheme theme);

    /**
     * @brief getOpStack returns the OperationStack displayed on the InfoWidget
     *
     * @return Pointer to the current OperationStack
     */
    OperationStack* getOpStack();


signals:
    void sig_openVideoPressed();
    void sig_openFolderPressed();
    void sig_openMetaPressed();

private slots:
    void on_toolButton_folder_clicked();
    void on_toolButton_video_clicked();
    void on_toolButton_meta_clicked();

protected:
    Ui::InfoWidget *ui;
};

#endif // INFOWIDGET_H
