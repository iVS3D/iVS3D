#include "exportwidget.h"

#include "ui_exportwidget.h"

ExportWidget::ExportWidget(QWidget *parent)
    : QWidget(parent), ui(new Ui::ExportWidget) {
    ui->setupUi(this);
    m_maskStackView = std::make_shared<MaskStackView>(this);
    ui->verticalLayout_transforms->addWidget(m_maskStackView.get());

    connect(ui->comboBox_resolution, &QComboBox::currentTextChanged,
            [=](const QString &text) { emit sig_resChanged(text); });

    m_altitudeSpinBox =
        nullptr;  // only display the altitude box if we have metadata!
}

ExportWidget::~ExportWidget() {
    delete ui;
}

std::shared_ptr<MaskStackView> ExportWidget::getMaskStackView() {
    return m_maskStackView;
}

void ExportWidget::setOutputPath(QString path) { ui->lineEdit->setText(path); }

void ExportWidget::enableExport(bool enabled) {
    ui->pushButton_export->setEnabled(enabled);
}

void ExportWidget::enableExportPathEditable(bool enabled) {
    ui->lineEdit->setEnabled(enabled);
    ui->lineEdit->setToolTip(
        enabled ? tr("location to export keyframes to.")
                : tr("export location ha been passed as a start argument. Thus "
                     "it can not be changed!"));
    ui->pushButton_browse->setEnabled(enabled);
    ui->pushButton_browse->setToolTip(
        enabled ? tr("location to export keyframes to.")
                : tr("export location ha been passed as a start argument. Thus "
                     "it can not be changed!"));
}

void ExportWidget::enableReconstruct(bool enabled) { (void)enabled; }

void ExportWidget::on_pushButton_browse_clicked() {
    QString newPath = QFileDialog::getExistingDirectory(
        this, tr("choose output folder"), ui->lineEdit->text(),
        QFileDialog::DontUseNativeDialog);
    if (!newPath.isEmpty()) {
        ui->lineEdit->setText(newPath);
    }
}

void ExportWidget::on_pushButton_export_clicked() { emit sig_export(); }

void ExportWidget::on_pushButton_reconstruct_clicked() {
    emit sig_reconstruct();
}

void ExportWidget::on_pushButton_addAuto_clicked() { emit sig_addAuto(); }

void ExportWidget::on_lineEdit_textChanged(const QString &text) {
    emit sig_pathChanged(text);
}

void ExportWidget::on_spinBox_altitude_valueChanged(double d) {
    emit sig_altitudeChanged(d);
}

void ExportWidget::setResolutionList(QStringList resList, int idx) {
    Q_ASSERT(!resList.empty());
    Q_ASSERT(idx >= 0);
    Q_ASSERT(idx < resList.size());

    ui->comboBox_resolution->clear();
    ui->comboBox_resolution->addItems(resList);
    ui->comboBox_resolution->setCurrentIndex(idx);
}

void ExportWidget::setResolution(QString resolution) {
    ui->comboBox_resolution->setEditText(resolution);
}

void ExportWidget::setResolutionValid(bool valid) {
    QPalette colorPalette = ui->comboBox_resolution->palette();
    if (valid) {
        ApplicationSettings as = ApplicationSettings::instance();
        if (as.getColorTheme() == DARK) {
            // darkstyle on
            colorPalette.setColor(QPalette::Text, Qt::white);
        } else {
            // darkstyle off
            colorPalette.setColor(QPalette::Text, Qt::black);
        }
    } else {
        colorPalette.setColor(QPalette::Text, Qt::red);
    }
    ui->comboBox_resolution->setPalette(colorPalette);
}

QString ExportWidget::getExportFormat() {
    return ui->comboBox_format->currentText();
}

bool ExportWidget::setOutputFormat(QString format) {
    int idx = ui->comboBox_format->findText(format);
    if (idx >= 0) {
        ui->comboBox_format->setCurrentIndex(idx);
        return true;
    }
    return false;
}

void ExportWidget::setAltitudeVisible(bool visible) {
    if (visible && !m_altitudeSpinBox) {
        // altitude spinbox has not been created yet
        m_altitudeSpinBox = new QDoubleSpinBox(this);
        m_altitudeSpinBox->setRange(-430.5, 8848.86);

        ui->formLayout->insertRow(1, tr("Altitude above sea-level"),
                                  m_altitudeSpinBox);
        connect(m_altitudeSpinBox,
                QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
                &ExportWidget::on_spinBox_altitude_valueChanged);
        return;
    }
    if (!visible && m_altitudeSpinBox) {
        // the altitude spinbox exists but is not needed anymore, delete it!
        connect(m_altitudeSpinBox,
                QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
                &ExportWidget::on_spinBox_altitude_valueChanged);
        ui->formLayout->removeRow(
            1);  // this deleted the m_altitudeSpinBox and the label!
        m_altitudeSpinBox = nullptr;
        return;
    }
}

void ExportWidget::setAltitude(double altitude) {
    if (m_altitudeSpinBox) m_altitudeSpinBox->setValue(altitude);
}

double ExportWidget::getAltitude() {
    if (m_altitudeSpinBox) return m_altitudeSpinBox->value();
    return 0.0;
}

void ExportWidget::enableFormat(QString format, bool enable) {
    int idx = ui->comboBox_format->findText(format);
    if (idx >= 0)
    {
        if (enable) {
            // Enable first item again
            ui->comboBox_format->setItemData(idx, QVariant(Qt::ItemFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable)),
                                             Qt::UserRole - 1);
        } else {
            ui->comboBox_format->setItemData(idx, QVariant(0),
                                             Qt::UserRole - 1);
            // Disable first item
            ui->comboBox_format->setItemData(idx, QVariant(0),
                                             Qt::UserRole - 1);

            // If it was selected, move to next available format
            if (ui->comboBox_format->currentIndex() == idx)
                ui->comboBox_format->setCurrentIndex(idx == 0 ? 1 : 0);
        }
    }
}
