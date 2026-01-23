#ifndef SAMPLINGWIDGET_H
#define SAMPLINGWIDGET_H

#include <QWidget>
#include <QCheckBox>
#include <QPushButton>
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
    explicit SamplingWidget(QWidget *parent = nullptr);
    ~SamplingWidget();

    /**
     * @brief showAlgorithmSettings shows the given QWidget to the user.
     * @param algoSettings the QWidget to display
     */
    void showPluginSettings(std::shared_ptr<QWidget> settingsWidget);

    void setResolutionList(QStringList resList, int idx);
    void setResolution(QString resolution);
    void setResolutionValid(bool valid);

    void setPluginList(const QStringList& pluginNames);
    void setSelectedPlugin(const QString& pluginName);

    enum PluginActions {
        PREVIEW_TOGGLE = 0,
        ADD_MASK = 1,
        START_SELECTION = 2,
        ALL_ACTIONS = 4
    };
    void setPluginActionVisible(PluginActions action, bool visible);

    void setPreviewEnabled(bool enabled);


signals:

    void sig_selectedPluginChanged(QString name);

    /**
     * @brief [signal] sig_startSampling(...) is emitted on start sampling button pressed.
     */
    void sig_startSampling();

    /**
     * @brief [signal] sig_enablePreviewChanged(...) is emitted on enable preview checkbox state changed.
     * @param enabled Is @a true if enable preview is checked, @a false otherwise.
     */
    void sig_enablePreviewChanged(bool enabled);

    /**
     * @brief [signal] sig_resChanged(...) is emitted if the resolution in the combo box is changed
     * @param resolution The selected resolution as a string
     */
    void sig_resChanged(QString resolution);

    void sig_addMask();

private slots:
    void slot_startSamplingPressed();
    void slot_enablePreviewChanged(bool enabled);
    void slot_addMaskPressed();
    void slot_selectedPluginChanged(QString name);

private:
    Ui::SamplingWidget *ui;
    std::shared_ptr<QWidget> m_currentPluginSettings;

    QPushButton* m_startSelectionBtn = nullptr;
    QCheckBox* m_previewCB = nullptr;
    QPushButton* m_addMaskBtn = nullptr;
};

#endif // SAMPLINGWIDGET_H
