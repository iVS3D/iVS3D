#ifndef SAMPLINGWIDGET_H
#define SAMPLINGWIDGET_H

#include <QWidget>
#include <QCheckBox>
#include "ui_samplingwidget.h"


namespace Ui {
class SamplingWidget;
}

/**
 * @class SamplingWidget
 *
 * @ingroup View
 *
 * @brief The SamplingWidget class is a graphical user interface to select and edit sampling algorithms
 * and transforms.
 * The algorithm settings can be modified using the QWidget provided by IAlgorithm::getSettingsWidget().
 * The transfrom settings can be modified using the QWidget provided by ITransform::getSettingsWidget().
 * The user can select algorithms provided by AlgorithmManager::instance and transforms provided by
 * TransformManager::instance as well as define the image base. Image bases are:
 *  - all images
 *  - keyframes only
 *  - all images (use boundaries)
 *  - keyframes only (use boundaries)
 *
 * The SamplingWidget provides signals on selected algorithm changed and start button pressed as well as
 * selected transform changed.
 *
 * @author Dominik Wüst
 *
 * @date 2021/03/03
 */
class SamplingWidget : public QWidget
{
    Q_OBJECT

public:

    /**
     * @brief Creates a SamplingWidget with the title displayed up top and the algorithmList to select algorithms from. Returned indices refer to this algorithmList.
     * @param parent The parent QWidget for this widget
     * @param title Text to display up top
     * @param algorithmList The algorithms the user can select
     * @param transformList The transforms the user can select
     *
     * @see IAlgorithm, ITransform
     */
    explicit SamplingWidget(QWidget *parent = nullptr, QStringList algorithmList = QStringList("no algorithm"), QStringList transformList = QStringList(""));
    ~SamplingWidget();

    /**
     * @brief showAlgorithmSettings shows the given QWidget to the user.
     * @param algoSettings the QWidget to display
     */
    void showAlgorithmSettings(QWidget *algoSettings);


    /**
     * @brief getSelectedAlgorithm returns the currently selected Algorithm
     * @return int wich corresponds to the index in the pluginlist
     */
    int getSelectedAlgorithm();

    /**
     * @brief getSelctedType returns the type of the curently selected Algorithm
     * @return @a 1 if algorithm, @a 0 if transformation
     */
    int getSelctedType();

    /**
     * @brief setAlgorithm sets the currently selceted algoritm to the algorithm corresponding to the index
     * @param idx the index in the pluginList
     */
    void setAlgorithm(int idx);

    void disablePreview();


signals:

    /**
     * @brief [signal] sig_selectedAlgorithmChanged(...) is emitted on dropdown index changed to an IAlgorithm
     * instance. @see IAlgorithm
     * @param idx The index of the new algorithm refering to algorithmList given to constructor
     */
    void sig_selectedAlgorithmChanged(int idx);

    /**
     * @brief [signal] sig_selectedTransformChanged(...) is emitted on dropdown index changed to an ITransfrom
     * instance. @see ITransfrom
     * @param idx The index of the new transform refering to transformList
     */
    void sig_selectedTransformChanged(int idx);

    /**
     * @brief [signal] sig_startSampling(...) is emitted on start sampling button pressed.
     * @param onlyKeyframes @a true if image base is only keyframes, @a false if all images
     * @param useBounds @a true if bounds are used, @a false if bounds are ignored
     */
    void sig_startSampling();

    /**
     * @brief [signal] sig_enablePreviewChanged(...) is emitted on enable preview checkbox state changed.
     * @param enabled Is @a true if enable preview is checked, @a false otherwise.
     */
    void sig_enablePreviewChanged(bool enabled);

    /**
     * @brief [signal] sig_addAuto(...) is emitted when add automatic execution is clicked
     * @param idx The index of the currently selected plugin
     * @param generate @a true if generate Settings is used @a false otherwise
     */
    void sig_addAuto(int idx, bool generate);

private slots:
    void slot_selectedAlgoChanged(int idx);
    void slot_startSamplingPressed();
    void slot_enablePreviewChanged(bool enabled);
    void on_addAuto_clicked();

private:
    Ui::SamplingWidget *ui;
    QWidget *m_algoSettings;
    QWidget *m_placeholder;
    QCheckBox *m_cbPreviewTransform;
    int m_separatorIdx;

    void showSamplingBtns();
    void showTransformBtns();
    void showNoBtns();
};

#define HIDE_WIDGET(W) ui->gridLayout->removeWidget(W); W->setVisible(false);
#define SHOW_WIDGET(W) ui->gridLayout->addWidget(W,3,2); W->setVisible(true);

#endif // SAMPLINGWIDGET_H
