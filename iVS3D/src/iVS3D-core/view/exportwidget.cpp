#include "exportwidget.h"
#include "ui_exportwidget.h"


ExportWidget::ExportWidget(QWidget *parent, QStringList transformList) :
    QWidget(parent),
    ui(new Ui::ExportWidget)
{
    ui->setupUi(this);
    for(const auto &t : transformList){
        auto *cb = new QCheckBox(t);
        m_checkboxes.push_back(cb);
        ui->verticalLayout_transforms->addWidget(cb);
    }
#if defined(Q_OS_WIN)
    m_reconstructBtn = new QPushButton(tr("reconstruct"));
    ui->horizontalLayout->addWidget(m_reconstructBtn);
    connect(m_reconstructBtn, &QPushButton::clicked, this, &ExportWidget::on_pushButton_reconstruct_clicked);
#endif
    this->ui->altitudeWidget->setVisible(false);
}

ExportWidget::~ExportWidget()
{
    for(auto cb : m_checkboxes){
        delete cb;
    }
    delete ui;
}

void ExportWidget::setResolutionList(QStringList resList, int idx)
{
    Q_ASSERT(!resList.empty());
    Q_ASSERT(idx>=0);
    Q_ASSERT(idx < resList.size());

    ui->comboBox->clear();
    ui->comboBox->addItems(resList);
    ui->comboBox->setCurrentIndex(idx);
}

void ExportWidget::setResolution(QString resolution)
{
    ui->comboBox->setEditText(resolution);
}

void ExportWidget::setOutputPath(QString path)
{
    ui->lineEdit->setText(path);
}

void ExportWidget::enableExport(bool enabled)
{
    ui->pushButton_export->setEnabled(enabled);
}

void ExportWidget::enableExportPathEditable(bool enabled)
{
    ui->lineEdit->setEnabled(enabled);
    ui->lineEdit->setToolTip(enabled ? tr("location to export keyframes to.") : tr("export location ha been passed as a start argument. Thus it can not be changed!"));
    ui->pushButton_browse->setEnabled(enabled);
    ui->pushButton_browse->setToolTip(enabled ? tr("location to export keyframes to.") : tr("export location ha been passed as a start argument. Thus it can not be changed!"));
}

void ExportWidget::enableReconstruct(bool enabled)
{
#if defined(Q_OS_WIN)
    m_reconstructBtn->setEnabled(enabled);
#else
    (void)enabled;
#endif
}

void ExportWidget::setResolutionValid(bool valid)
{
    QPalette colorPalette = this->ui->comboBox->palette();
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
    this->ui->comboBox->setPalette(colorPalette);
}

bool ExportWidget::getCropStatus()
{
    if (this->ui->checkBox->checkState() == 0) {
        return false;
    }
    return true;
}

void ExportWidget::setCropStatus(bool checked)
{
    if (checked) {
        this->ui->checkBox->setCheckState(Qt::Checked);
        return;
    }
    this->ui->checkBox->setCheckState(Qt::Unchecked);
    return;

}

std::vector<bool> ExportWidget::getSelectedITransforms()
{
    std::vector<bool> transforms;
    for(const auto &cb : m_checkboxes){
        transforms.push_back(cb->isChecked());
    }
    return transforms;
}

void ExportWidget::on_pushButton_browse_clicked()
{
    QString newPath = QFileDialog::getExistingDirectory(this,tr("choose output folder"), ui->lineEdit->text(), QFileDialog::DontUseNativeDialog);
    if(!newPath.isEmpty()){
        ui->lineEdit->setText(newPath);
    }
}

void ExportWidget::on_pushButton_export_clicked()
{
    emit sig_export();
}

void ExportWidget::on_pushButton_reconstruct_clicked()
{
    emit sig_reconstruct();
}

void ExportWidget::on_pushButton_cropExport_clicked()
{
    emit sig_cropExport();
}

void ExportWidget::on_pushButton_addAuto_clicked()
{
    emit sig_addAuto();
}

void ExportWidget::on_lineEdit_textChanged(const QString &text)
{
    emit sig_pathChanged(text);
}

void ExportWidget::on_comboBox_currentTextChanged(const QString &text)
{
    emit sig_resChanged(text);
}

void ExportWidget::on_spinBox_Altitude_valueChanged(double i)
{
    emit sig_altitudeChanged(i);
}

bool ExportWidget::setSelectedITransforms(std::vector<bool> selection)
{
    if(m_checkboxes.size() != selection.size()){
        return false;
    }
    for(int i = 0; i<(int)m_checkboxes.size();i++){
        m_checkboxes[i]->setChecked(selection[i]);
    }
    return true;
}

void ExportWidget::enableCreateFilesWidget(bool enable)
{
    if (enable) {
        ui->createFilesWidget->show();
    }
    else {
        ui->createFilesWidget->hide();
    }
}

void ExportWidget::enableAltitude(bool enable)
{
    this->ui->altitudeWidget->setVisible(enable);
}

void ExportWidget::setAltitude(double altitude)
{
    this->ui->spinBox_Altitude->setValue(altitude);
}

