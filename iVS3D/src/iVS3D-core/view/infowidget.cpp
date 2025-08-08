#include "infowidget.h"
#include "applicationsettings.h"


#include <QObject>
/*!
 * \fn InfoWidget::InfoWidget(QWidget *parent, QString title)
 * \param parent
 * \param title
 *
 * Creates an InfoWidget with and displays the given \a title at the top.
 */
InfoWidget::InfoWidget(QWidget *parent, QString title, ColorTheme theme) :
    QWidget(parent)
{
    this->setLayout(new QVBoxLayout(this));
    
    // open folder / video / meta data buttons  
    m_inputButtonLayout = new QHBoxLayout(this);
    
    m_openImagesButton = new AdaptiveToolButton(tr(" load images"), tr(" images"), this);
    m_openImagesButton->setIconForTheme(QIcon(":/icons/openFolderIconW"), DARK);
    m_openImagesButton->setIconForTheme(QIcon(":/icons/openFolderIconB"), LIGHT);
    m_openImagesButton->setColorTheme(theme);
    m_inputButtonLayout->addWidget(m_openImagesButton);
    connect(m_openImagesButton, &AdaptiveToolButton::clicked, this, &InfoWidget::on_toolButton_folder_clicked);

    m_openVideoButton = new AdaptiveToolButton(tr(" load video"), tr(" video"), this);
    m_openVideoButton->setIconForTheme(QIcon(":/icons/openVideoIconW"), DARK);
    m_openVideoButton->setIconForTheme(QIcon(":/icons/openVideoIconB"), LIGHT);
    m_openVideoButton->setColorTheme(theme);
    m_inputButtonLayout->addWidget(m_openVideoButton);
    connect(m_openVideoButton, &AdaptiveToolButton::clicked, this, &InfoWidget::on_toolButton_video_clicked);

    m_openMetaDataButton = new AdaptiveToolButton(tr(" load meta data"), tr(" meta data"), this);
    m_openMetaDataButton->setIconForTheme(QIcon(":/icons/openMetaIconW"), DARK);
    m_openMetaDataButton->setIconForTheme(QIcon(":/icons/openMetaIconB"), LIGHT);
    m_openMetaDataButton->setColorTheme(theme);
    m_openMetaDataButton->setEnabled(false);
    m_inputButtonLayout->addWidget(m_openMetaDataButton);
    connect(m_openMetaDataButton, &AdaptiveToolButton::clicked, this, &InfoWidget::on_toolButton_meta_clicked);

    this->layout()->addItem(m_inputButtonLayout);

    // resolution, altitude, crop settings
    m_settingsWidget = new QWidget(this);
    m_settingsLayout = new QFormLayout(m_settingsWidget);
    m_settingsWidget->setLayout(m_settingsLayout);
    m_settingsLayout->setMargin(0);

    m_altitudeSpinBox = nullptr; // only display the altitude box if we have metadata!

    this->layout()->addWidget(m_settingsWidget);
    m_settingsWidget->setEnabled(false);

    // operation stack
    m_opStack = new OperationStack(this);
    m_opStack->layout()->setMargin(0);
    this->layout()->addWidget(m_opStack);
    //m_opStack->setEnabled(false);
}

InfoWidget::~InfoWidget()
{
    
}

void InfoWidget::enableOpenMetaData(bool status, QString tooltip)
{
    m_openMetaDataButton->setEnabled(status);
    if (!tooltip.isEmpty()) m_openMetaDataButton->setToolTip(tooltip);
}

void InfoWidget::enableOpenImages(bool status, QString tooltip)
{
    m_openImagesButton->setEnabled(status);
    if (!tooltip.isEmpty()) m_openImagesButton->setToolTip(tooltip);
}

void InfoWidget::enableOpenVideo(bool status, QString tooltip)
{
    m_openVideoButton->setEnabled(status);
    if (!tooltip.isEmpty()) m_openVideoButton->setToolTip(tooltip);
}

void InfoWidget::setColorTheme(ColorTheme theme)
{
    for(AdaptiveToolButton *button : { m_openImagesButton, m_openVideoButton, m_openMetaDataButton }) {
        button->setColorTheme(theme);
    }
}

OperationStack *InfoWidget::getOpStack()
{
    return m_opStack;
}

void InfoWidget::setAltitudeVisible(bool visible)
{
    if (visible && !m_altitudeSpinBox) {
        // altitude spinbox has not been created yet
        m_altitudeSpinBox = new QDoubleSpinBox(this);
        m_settingsLayout->insertRow(1,tr("Altitude of the first image"), m_altitudeSpinBox);
        connect(m_altitudeSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &InfoWidget::on_spinBox_altitude_valueChanged);
        return;
    }
    if(!visible && m_altitudeSpinBox) {
        // the altitude spinbox exists but is not needed anymore, delete it!
        connect(m_altitudeSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &InfoWidget::on_spinBox_altitude_valueChanged);
        m_settingsLayout->removeRow(1); // this deleted the m_altitudeSpinBox and the label!
        m_altitudeSpinBox = nullptr;
        return;
    }
}

void InfoWidget::setAltitude(double altitude)
{
    if(m_altitudeSpinBox) m_altitudeSpinBox->setValue(altitude);
}

void InfoWidget::enableSettings(bool enabled)
{
    m_settingsWidget->setEnabled(enabled);
}

void InfoWidget::on_toolButton_folder_clicked()
{
    emit sig_openFolderPressed();
}

void InfoWidget::on_toolButton_video_clicked()
{
    emit sig_openVideoPressed();
}

void InfoWidget::on_toolButton_meta_clicked()
{
    qDebug("openmeta");
    emit sig_openMetaPressed();
}

void InfoWidget::on_spinBox_altitude_valueChanged(double d)
{
    emit sig_altitudeChanged(d);
}
