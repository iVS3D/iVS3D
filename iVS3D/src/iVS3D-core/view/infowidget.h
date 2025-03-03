#ifndef INFOWIDGET_H
#define INFOWIDGET_H

#include <QWidget>
#include "operationstack.h"
#include "stringcontainer.h"
#include "cvmat_qmetadata.h"

#include <QRegExp>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QFormLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QDoubleSpinBox>

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
     * @brief setAltitudeVisible will show or hide the altitude selector
     * @param visible @a true shows the selector, @a false will hide it
     */
    void setAltitudeVisible(bool visible);

    /**
     * @brief setAltitude sets the value of the altitude selector
     * @param altitude
     */
    void setAltitude(double altitude);

    void enableSettings(bool enabled);


signals:
    void sig_openVideoPressed();
    void sig_openFolderPressed();
    void sig_openMetaPressed();

    /**
     * @brief [signal] sig_resChanged(...) is emitted if the resolution changed.
     * @param res the new resolution as string
     */
    void sig_resChanged(QString res);

    /**
     * @brief [signal] sig_cropExport() is emitted on crop export button pressed.
     */
    void sig_cropEdit();

    /**
     * @brief [signal] sig_altitudeChanged() is emitted when the altitude is changed by the user.
     */
    void sig_altitudeChanged(double altitude);

private slots:
    void on_toolButton_folder_clicked();
    void on_toolButton_video_clicked();
    void on_toolButton_meta_clicked();

    void on_pushButton_cropEdit_clicked();
    void on_spinBox_altitude_valueChanged(double d);
    void on_comboBox_resolution_currentTextChanged(const QString &text);

protected:
    Ui::InfoWidget *ui;
    QWidget *m_settingsWidget;
    QFormLayout *m_settingsLayout;
    QComboBox *m_resolutionComboBox;
    QDoubleSpinBox *m_altitudeSpinBox;
    QPushButton *m_cropPushButton;
    QCheckBox *m_cropCheckBox;
    OperationStack *m_opStack;
};

#endif // INFOWIDGET_H
