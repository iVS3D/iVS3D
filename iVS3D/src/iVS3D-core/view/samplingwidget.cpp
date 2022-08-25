#include "samplingwidget.h"

SamplingWidget::SamplingWidget(QWidget *parent, QStringList algorithmList, QStringList transformList) :
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

    // add algorithms
    ui->comboBoxAlgo->addItems(algorithmList);
    ui->comboBoxAlgo->insertSeparator(algorithmList.size());
    m_separatorIdx = algorithmList.size();
    ui->comboBoxAlgo->addItems(transformList);
    connect(ui->comboBoxAlgo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SamplingWidget::slot_selectedAlgoChanged);
    connect(ui->pushButton, &QPushButton::pressed, this, &SamplingWidget::slot_startSamplingPressed);
    connect(ui->pushButton_2, &QPushButton::pressed, this, &SamplingWidget::slot_startGeneratePressed);

    m_cbPreviewTransform = new QCheckBox("Enable preview", parent);
    m_cbPreviewTransform->setVisible(false);
    connect(m_cbPreviewTransform, &QCheckBox::stateChanged, this, &SamplingWidget::slot_enablePreviewChanged);

    // disable generate Settings buttons
    ui->pushButton_2->setVisible(false);
    ui->radioButton->setVisible(false);
    //
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
    emit sig_startSampling();
}

void SamplingWidget::slot_startGeneratePressed()
{
    emit sig_startGenerateSettings();
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
    ui->label_2->setVisible(true);
    ui->label_3->setVisible(true);
//    ui->pushButton_2->setVisible(true);
    SHOW_WIDGET(ui->pushButton);
//    ui->radioButton->setVisible(true);
    ui->addAuto->setVisible(true);
}

void SamplingWidget::showTransformBtns()
{
    ui->label_2->setVisible(false);
    ui->label_3->setVisible(false);
//    ui->pushButton_2->setVisible(false);
    HIDE_WIDGET(ui->pushButton);
    SHOW_WIDGET(m_cbPreviewTransform);
//    ui->radioButton->setVisible(false);
    ui->addAuto->setVisible(false);
}

void SamplingWidget::showNoBtns()
{
    ui->pushButton->setEnabled(false);
}
