#include "samplingwidget.h"
#include "applicationsettings.h"

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

    m_cbPreviewTransform = new QCheckBox(tr("Enable preview"), parent);
    m_cbPreviewTransform->setVisible(false);
    connect(m_cbPreviewTransform, &QCheckBox::stateChanged, this, &SamplingWidget::slot_enablePreviewChanged);

    // disable generate Settings buttons
    ui->label_3->setVisible(false);
    //

    connect(ui->comboBoxResolution, &QComboBox::currentTextChanged, [=](const QString& text) { emit sig_resChanged(text); });
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
    emit sig_selectedPluginChanged(ui->comboBoxAlgo->currentText());
}

void SamplingWidget::slot_startSamplingPressed()
{
    emit sig_startSampling();
}

void SamplingWidget::slot_enablePreviewChanged(bool enabled)
{
    emit sig_enablePreviewChanged(enabled);
}

void SamplingWidget::showSamplingBtns()
{
    HIDE_WIDGET(m_cbPreviewTransform)
//    ui->label_2->setVisible(true);
//    ui->label_3->setVisible(true);
//    ui->pushButton_2->setVisible(true);
    SHOW_WIDGET(ui->pushButton);
//    ui->radioButton->setVisible(true);
//    ui->addAuto->setVisible(true);
}

void SamplingWidget::showTransformBtns()
{
    ui->label_3->setVisible(false);
//    ui->pushButton_2->setVisible(false);
    HIDE_WIDGET(ui->pushButton);
    SHOW_WIDGET(m_cbPreviewTransform);
//    ui->radioButton->setVisible(false);
}

void SamplingWidget::showNoBtns()
{
    ui->pushButton->setEnabled(false);
}

void SamplingWidget::setResolutionList(QStringList resList, int idx)
{
    Q_ASSERT(!resList.empty());
    Q_ASSERT(idx>=0);
    Q_ASSERT(idx < resList.size());

    ui->comboBoxResolution->clear();
    ui->comboBoxResolution->addItems(resList);
    ui->comboBoxResolution->setCurrentIndex(idx);
}

void SamplingWidget::setResolution(QString resolution)
{
    ui->comboBoxResolution->setEditText(resolution);
}

void SamplingWidget::setResolutionValid(bool valid)
{
    QPalette colorPalette = ui->comboBoxResolution->palette();
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
    ui->comboBoxResolution->setPalette(colorPalette);
}

void SamplingWidget::setPluginList(const QStringList& pluginNames) {
    ui->comboBoxAlgo->clear();
    ui->comboBoxAlgo->addItems(pluginNames);
}

void SamplingWidget::setSelectedPlugin(const QString& pluginName) {
    int index = ui->comboBoxAlgo->findText(pluginName);
    if (index != -1) {
        ui->comboBoxAlgo->setCurrentIndex(index);
    }
}

void SamplingWidget::setPreviewVisible(bool visible) {
    if (visible) {
        SHOW_WIDGET(m_cbPreviewTransform);
    } else {
        HIDE_WIDGET(m_cbPreviewTransform);
    }
}

void SamplingWidget::setSelectionVisible(bool visible) {
    if (visible) {
        SHOW_WIDGET(ui->pushButton);
    } else {
        HIDE_WIDGET(ui->pushButton);
    }
}
