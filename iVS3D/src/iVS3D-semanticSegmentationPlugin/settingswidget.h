#ifndef SETTINGSWIDGET_H
#define SETTINGSWIDGET_H

#define DESCRIPTION_STYLE "color: rgb(58, 58, 58); border-left: 6px solid  rgb(58, 58, 58); border-top-right-radius: 5px; border-bottom-right-radius: 5px; background-color: lightblue;"

#include <QObject>
#include <QWidget>
#include <QLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QComboBox>
#include <QLabel>
#include <QCheckBox>
#include <QSlider>
#include <QPushButton>

#include <opencv2/core.hpp>

#include "cvmat_qmetadata.h"

/**
 * @class SettingsWidget
 *
 * @ingroup SemanticSegmentationPlugin
 *
 * @brief The SettingsWidget class provides user access to the parameters of SemanticSegmentation. The available .onnx models are
 * listed and one can be seleted for calculating the score in SemanticSegmentation::transform. The classes of the selected model
 * are displayed with their associated color and can be ckecked to be included in the output masks. The overlay alpha for the semantic
 * map can be adjusted.
 *
 * @date 2021/03/31
 *
 * @author Dominik Wüst
 */
class SettingsWidget : public QWidget
{
    Q_OBJECT
public:
    /**
     * @brief SettingsWidget creates a new Instance with the given model names to select from and a default alpha value.
     * @param parent The parent for the QWidget
     * @param ONNXmodelList The model names to display
     * @param blendAlpha The default alpha value for blending
     */
    explicit SettingsWidget(QWidget *parent = nullptr, QStringList ONNXmodelList = QStringList(""), float blendAlpha = 0.5f);
    ~SettingsWidget();

    void setClasses(QBoolList boolList);
    void setModel(uint modelIndex);

signals:
    /**
     * @brief [signal] sig_selectedONNXIndexChanged is emitted if a different model is chosen.
     * @param idx The index of the chosen model
     */
    void sig_selectedONNXIndexChanged(int idx);

    /**
     * @brief [signal] sig_selectedClassesChanged is emitted if a class is selected / deselected.
     * @param classes A List of bools for each class, is @a true if class is selected
     */
    void sig_selectedClassesChanged(QBoolList classes);

    /**
     * @brief [signal] sig_blendAlphaChanged is emitted on blend alpha value changed.
     * @param alpha The new alpha value
     */
    void sig_blendAlphaChanged(float alpha);

public slots:
    /**
     * @brief [slot] slot_classesAndColorsChanged updates the class list for the chosen model.
     * @param classes The class names
     * @param colors The color for each class
     * @param selectedClasses bools for the currently selected classes
     */
    void slot_classesAndColorsChanged(QStringList classes, QColorList colors, QBoolList selectedClasses);

    /**
     * @brief [slot] slot_showTask displays the given task message and the processor used .
     * @param processor The processor used (cpu or gpu)
     * @param task The task currently performed by the processor
     */
    void slot_showTask(QString processor, QString task);

private slots:
    // --- slots for gui elements to process events and emit signals ---
    void slot_comboBoxIdxChanged(int idx);
    void slot_selectedClassesChanged(bool);
    void slot_overlayAlphaChanged(int percent);
    void slot_invertSelectionPressed();

private:
    std::vector<QCheckBox*> m_classBoxes;
    QGridLayout *m_gridLayout;
    QComboBox *m_comboBox;
    QSlider *m_alphaSlider;
    QPushButton *m_invertButton;
    QLabel *m_processLabel;
    QLabel *m_processorLabel;
};

#endif // SETTINGSWIDGET_H
