#include "samplingwidget.h"

SamplingWidget::SamplingWidget(QWidget *parent, QString title, QStringList algorithmList, QStringList transformList) :
    QWidget(parent),
    ui(new Ui::SamplingWidget)
{
    ui->setupUi(this);
    m_placeholder = new QWidget(parent);
    m_placeholder->setVisible(false);

    // setup area for algo settings widget
    m_algoSettings = new QWidget(this);
    ui->scrollAreaWidgetContents->setLayout(new QVBoxLayout(this));
    ui->scrollAreaWidgetContents->layout()->setContentsMargins(0,0,0,0);
    ui->scrollAreaWidgetContents->layout()->addWidget(m_algoSettings);

    // set widget title
    //ui->groupBox->setTitle(title);

    // add algorithms
    ui->comboBoxAlgo->addItems(algorithmList);
    ui->comboBoxAlgo->insertSeparator(algorithmList.size());
    m_separatorIdx = algorithmList.size();
    ui->comboBoxAlgo->addItems(transformList);
    connect(ui->comboBoxAlgo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SamplingWidget::slot_selectedAlgoChanged);
    connect(ui->pushButton, &QPushButton::pressed, this, &SamplingWidget::slot_startSamplingPressed);

    // add sampling options
    ui->comboBox->addItem("All images");
    ui->comboBox->addItem("Only keyframes");
    ui->comboBox->addItem("All images (Use bounds)");
    ui->comboBox->addItem("Only keyframes (Use bounds)");

    m_cbPreviewTransform = new QCheckBox("Enable preview", parent);
    m_cbPreviewTransform->setVisible(false);
    connect(m_cbPreviewTransform, &QCheckBox::stateChanged, this, &SamplingWidget::slot_enablePreviewChanged);
}

SamplingWidget::~SamplingWidget()
{
    disconnect(ui->comboBoxAlgo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SamplingWidget::slot_selectedAlgoChanged);
    disconnect(ui->pushButton, &QPushButton::pressed, this, &SamplingWidget::slot_startSamplingPressed);

    delete ui;
    delete m_cbPreviewTransform;
}

void SamplingWidget::showAlgorithmSettings(QWidget *algoSettings)
{
    ui->scrollAreaWidgetContents->layout()->replaceWidget(m_algoSettings, algoSettings);
    m_algoSettings->setVisible(false);
    algoSettings->setVisible(true);
    m_algoSettings = algoSettings;
}


int SamplingWidget::getSelectedAlgorithm()
{
    return ui->comboBoxAlgo->currentIndex();
}

int SamplingWidget::getSelctedType()
{
    int idx = ui->comboBoxAlgo->currentIndex();
    if(idx < m_separatorIdx){
        return 1; // Algorithm
    } else {
        return 0; // Transformable
    }
}

void SamplingWidget::setAlgorithm(int idx)
{
    ui->comboBoxAlgo->setCurrentIndex(idx);
}

void SamplingWidget::resetSelectedImages()
{
    ui->comboBox->setCurrentIndex(0);
}

void SamplingWidget::disablePreview()
{
    m_cbPreviewTransform->setChecked(false);
}

void SamplingWidget::slot_selectedAlgoChanged(int idx)
{
    if(idx<m_separatorIdx){
        emit sig_selectedAlgorithmChanged(idx);
        showSamplingBtns();
    } else if(idx > m_separatorIdx){
        emit sig_selectedTransformChanged(idx-m_separatorIdx-1);
        showTransformBtns();
    } else {
        showAlgorithmSettings(m_placeholder);
        showNoBtns();
    }
}

void SamplingWidget::slot_startSamplingPressed()
{
    int index = ui->comboBox->currentIndex();
    //Order of comboBox
    /*
    "All images"
    "Only keyframes"
    "All images (Use bounds)"
    "Only keyframes (Use bounds)"*/
    switch (index) {
        case 0 : emit sig_startSampling(false, false); break;
        case 1 : emit sig_startSampling(true, false); break;
        case 2 : emit sig_startSampling(false, true); break;
        case 3 : emit sig_startSampling(true, true); break;
    }
}

void SamplingWidget::slot_enablePreviewChanged(bool enabled)
{
    emit sig_enablePreviewChanged(enabled);
}

void SamplingWidget::on_addAuto_clicked()
{
    //Ignore ITransform
    int idx = ui->comboBoxAlgo->currentIndex();
    if(idx < m_separatorIdx){
        bool generate = ui->radioButton->isChecked();
        emit sig_addAuto(idx, generate);
    } else if(idx > m_separatorIdx){
        return;
    }
}

void SamplingWidget::showSamplingBtns()
{
    HIDE_WIDGET(m_cbPreviewTransform)
    SHOW_WIDGET(ui->label_2)
    SHOW_WIDGET(ui->comboBox)
    SHOW_WIDGET(ui->pushButton)
}

void SamplingWidget::showTransformBtns()
{
    HIDE_WIDGET(ui->comboBox)
    HIDE_WIDGET(ui->label_2)
    HIDE_WIDGET(ui->pushButton)
    SHOW_WIDGET(m_cbPreviewTransform)
}

void SamplingWidget::showNoBtns()
{
    ui->pushButton->setEnabled(false);
}
