#include "infowidget.h"
#include "ui_infowidget.h"
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
    QWidget(parent),
    ui(new Ui::InfoWidget)
{
    // open folder / video / meta data buttons
    ui->setupUi(this);
    ui->toolButton_folder->setIcon(QIcon(theme == DARK ? ":/icons/openFolderIconW" : ":/icons/openFolderIconB"));
    ui->toolButton_video->setIcon( QIcon(theme == DARK ? ":/icons/openVideoIconW"  : ":/icons/openVideoIconB"));
    ui->toolButton_meta->setIcon( QIcon(theme == DARK ? ":/icons/openMetaIconW"  : ":/icons/openMetaIconB"));
    ui->toolButton_meta->setEnabled(false);

    // resolution, altitude, crop settings
    m_settingsWidget = new QWidget(this);
    m_settingsLayout = new QFormLayout(m_settingsWidget);
    m_settingsWidget->setLayout(m_settingsLayout);
    m_settingsLayout->setMargin(0);

    m_resolutionComboBox = new QComboBox(this);
    m_settingsLayout->addRow(tr("Working resolution"), m_resolutionComboBox);

    m_altitudeSpinBox = nullptr; // only display the altitude box if we have metadata!

    {
        m_cropCheckBox = new QCheckBox(tr("Use ROI"), this);
        m_cropPushButton = new QPushButton(tr("Edit"),this);
        auto wrapper = new QWidget(this);
        wrapper->setLayout(new QHBoxLayout(this));
        wrapper->layout()->setMargin(0);
        wrapper->layout()->addWidget(m_cropCheckBox);
        wrapper->layout()->addWidget(m_cropPushButton);
        m_settingsLayout->addRow(tr("Region of interest (ROI)"), wrapper);
    }

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
    delete ui;
}

void InfoWidget::enableOpenMetaData(bool status, QString tooltip)
{
    ui->toolButton_meta->setEnabled(status);
    if (!tooltip.isEmpty()) ui->toolButton_meta->setToolTip(tooltip);
}

void InfoWidget::enableOpenImages(bool status, QString tooltip)
{
    ui->toolButton_folder->setEnabled(status);
    if (!tooltip.isEmpty()) ui->toolButton_folder->setToolTip(tooltip);
}

void InfoWidget::enableOpenVideo(bool status, QString tooltip)
{
    ui->toolButton_video->setEnabled(status);
    if (!tooltip.isEmpty()) ui->toolButton_video->setToolTip(tooltip);
}

void InfoWidget::setColorTheme(ColorTheme theme)
{
    ui->toolButton_folder->setIcon(QIcon(theme == DARK ? ":/icons/openFolderIconW" : ":/icons/openFolderIconB"));
    ui->toolButton_video->setIcon( QIcon(theme == DARK ? ":/icons/openVideoIconW"  : ":/icons/openVideoIconB"));
    ui->toolButton_meta->setIcon(  QIcon(theme == DARK ? ":/icons/openMetaIconW"  : ":/icons/openMetaIconB"));
}

OperationStack *InfoWidget::getOpStack()
{
    return m_opStack;
}

void InfoWidget::setResolutionList(QStringList resList, int idx)
{
    Q_ASSERT(!resList.empty());
    Q_ASSERT(idx>=0);
    Q_ASSERT(idx < resList.size());

    m_resolutionComboBox->clear();
    m_resolutionComboBox->addItems(resList);
    m_resolutionComboBox->setCurrentIndex(idx);
}

void InfoWidget::setResolution(QString resolution)
{
    m_resolutionComboBox->setEditText(resolution);
}

void InfoWidget::setResolutionValid(bool valid)
{
    QPalette colorPalette = m_resolutionComboBox->palette();
    if (valid) {
        ApplicationSettings as = ApplicationSettings::instance();
        if (as.getColorTheme() == DARK) {
            //darkstyle on
            colorPalette.setColor(QPalette::Text, Qt::white);
        }
        else {
            //darkstyle off
            colorPalette.setColor(QPalette::Text, Qt::black);
        }
    }
    else {
        colorPalette.setColor(QPalette::Text, Qt::red);
    }
    m_resolutionComboBox->setPalette(colorPalette);
}

bool InfoWidget::getCropStatus()
{
    return m_cropCheckBox->checkState() == Qt::Checked;
}

void InfoWidget::setCropStatus(bool checked)
{
    m_cropCheckBox->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
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

void InfoWidget::on_pushButton_cropEdit_clicked()
{
    emit sig_cropEdit();
}

void InfoWidget::on_spinBox_altitude_valueChanged(double d)
{
    emit sig_altitudeChanged(d);
}

void InfoWidget::on_comboBox_resolution_currentTextChanged(const QString &text)
{
    emit sig_resChanged(text);
}
