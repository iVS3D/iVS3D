#ifndef INFOWIDGET_H
#define INFOWIDGET_H

#include <QWidget>
#include "operationstack.h"
#include "stringcontainer.h"
#include "cvmat_qmetadata.h"
#include "adaptivetoolbutton.h"

#include <QRegExp>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QFormLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QDoubleSpinBox>

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
     * @brief [signal] sig_altitudeChanged() is emitted when the altitude is changed by the user.
     */
    void sig_altitudeChanged(double altitude);

private slots:
    void on_toolButton_folder_clicked();
    void on_toolButton_video_clicked();
    void on_toolButton_meta_clicked();
    void on_spinBox_altitude_valueChanged(double d);

protected:
// input buttons
    AdaptiveToolButton *m_openMetaDataButton;
    AdaptiveToolButton *m_openImagesButton;
    AdaptiveToolButton *m_openVideoButton;
    QHBoxLayout *m_inputButtonLayout;

// resolution, altitude, crop settings
    QWidget *m_settingsWidget;
    QFormLayout *m_settingsLayout;
    QDoubleSpinBox *m_altitudeSpinBox;

// operation stack
    OperationStack *m_opStack;
};

#endif // INFOWIDGET_H
