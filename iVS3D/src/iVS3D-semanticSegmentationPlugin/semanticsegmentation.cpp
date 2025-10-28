#include "semanticsegmentation.h"

#include <QApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>

SemanticSegmentation::SemanticSegmentation() {
    // install translator
    QLocale locale = qApp->property("translation").toLocale();
    QTranslator *translator = new QTranslator();
    translator->load(locale, "semanticsegmentation", "_", ":/translations",
                     ".qm");
    qApp->installTranslator(translator);

    // filter for semnatic segmentation models
    QStringList filter("Segmentation_*.onnx");
    QString path = QCoreApplication::applicationDirPath() + MODEL_PATH;
    QDir dir(path);
    qDebug() << dir.currentPath();
    m_ONNXmodelList = dir.entryList(filter);
    QStringList filteredModelList;
    for (int i = 0; i < m_ONNXmodelList.size(); i++) {
        m_ONNXmodelIdx = i;
        if (loadModelInfo()) {  // we only display models with valid model info
                                // file (same name but .json ending)
            filteredModelList.push_back(m_ONNXmodelList[i]);
        }
    }
    m_ONNXmodelList = filteredModelList;
    m_ONNXmodelIdx = 0;
    if (m_ONNXmodelList.size() && loadModelInfo()) {
        for (int i = 0; i < m_modelInfo.classes.size(); i++) {
            m_ONNXselectedClasses.push_back(true);
        }
    }

    m_imageIdx = UINT_MAX;
    m_blendAlpha = 0.5f;
    m_guiUpToDate = false;

    cv::ocl::setUseOpenCL(true);
    m_useCuda = false;
    m_model = nullptr;
    m_settingsWidget = nullptr;
}

SemanticSegmentation::~SemanticSegmentation() {
    if (m_settingsWidget != nullptr) {
        delete m_settingsWidget;
    }
}

QWidget *SemanticSegmentation::getSettingsWidget(QWidget *parent) {
    if (m_settingsWidget != nullptr) {
        return m_settingsWidget;
    }

    // create settings widget and connect to it
    m_settingsWidget =
        new SettingsWidget(parent, m_ONNXmodelList, 0.5,
                           QCoreApplication::applicationDirPath() + MODEL_PATH);
    m_ONNXmodelIdx = 0;

    if (m_ONNXmodelList.empty()) {
        return m_settingsWidget;  // no models found, return empty widget
    }

    m_ONNXselectedClasses.clear();
    for (int i = 0; i < m_modelInfo.classes.size(); i++) {
        m_ONNXselectedClasses.push_back(true);
    }
    QStringList classes;
    QList<QColor> colors;
    for (const auto &cls : m_modelInfo.classes) {
        classes.append(cls.name);
        colors.append(cls.color);
    }
    m_settingsWidget->slot_classesAndColorsChanged(classes, colors,
                                                   m_ONNXselectedClasses);

    connect(m_settingsWidget, &SettingsWidget::sig_selectedONNXIndexChanged,
            this, &SemanticSegmentation::slot_ONNXindexChanged);
    connect(m_settingsWidget, &SettingsWidget::sig_selectedClassesChanged, this,
            &SemanticSegmentation::slot_selectedClassesChanged);
    connect(m_settingsWidget, &SettingsWidget::sig_blendAlphaChanged, this,
            &SemanticSegmentation::slot_blendAlphaChanged);
    connect(this, &SemanticSegmentation::sig_classesAndColorsChanged,
            m_settingsWidget, &SettingsWidget::slot_classesAndColorsChanged);
    connect(this, &SemanticSegmentation::sig_message, m_settingsWidget,
            &SettingsWidget::slot_showTask);
    connect(this, &SemanticSegmentation::sig_error, m_settingsWidget,
            &SettingsWidget::slot_showError);

    return m_settingsWidget;
}

QString SemanticSegmentation::getName() const {
    return tr("Semantic Segmentation");
}

ITransform *SemanticSegmentation::copy() {
    auto *copy = new SemanticSegmentation();
    copy->m_ONNXmodelIdx = this->m_ONNXmodelIdx;
    copy->m_ONNXselectedClasses = this->m_ONNXselectedClasses;
    copy->m_useCuda = this->m_useCuda;
    copy->m_model = this->m_model;
    copy->m_modelInfo = this->m_modelInfo;

    this->m_model = nullptr;
    this->m_useCuda = false;
    return copy;
}

TransformResult SemanticSegmentation::transform(uint idx, const cv::Mat &img,
                                                const Resolution &resolution,
                                                const ROI &roi) {
    QMutexLocker lock(&m_mutex);

    if (m_ONNXmodelList.empty()) {
        cv::Mat resized;
        resolution.resize(img, resized);
        emit sendToGui(idx, resized);
        return tl::unexpected<Error>(createError(
            tr("No models found in %1")
                .arg(QCoreApplication::applicationDirPath() + MODEL_PATH)));
    }

    // update resolution, roi and index buffer
    m_resolution = resolution;
    m_roi = roi;
    m_imageIdx = idx;

    // resize to working resolution for processing in NN
    resolution.resize(img, m_originalImage);

    if (!m_useCuda) {
        // create preview of image before calculation started
        cv::Mat preview(m_originalImage.rows, m_originalImage.cols,
                        m_originalImage.type());
        preview.setTo(cv::Scalar(255, 255, 255));
        cv::hconcat(m_originalImage, preview, preview);
        emit sendToGui(idx, preview);
    }

    // crop before processing
    m_roi.crop(m_originalImage, m_image);

    // only start calculation if models found
    if (m_ONNXmodelList.size() == 0) {
        return tl::unexpected<Error>(createError(
            tr("No models found in %1")
                .arg(QCoreApplication::applicationDirPath() + MODEL_PATH)));
    }

    // load selected model
    if (!m_model) {
        QString modelPath = QCoreApplication::applicationDirPath() +
                            MODEL_PATH + "/" + m_ONNXmodelList[m_ONNXmodelIdx];
        emit sig_message(HW_NAME(m_useCuda), tr("Loading model..."), true);

        auto modelResult =
            NN::NeuralNetFactory::create(modelPath.toStdString(), m_useCuda);
        if (!modelResult) {
            return tl::unexpected<Error>(createError(
                tr("Failed to load model: %1 \n %2")
                    .arg(modelPath, QString::fromStdString(
                                        modelResult.error().message()))));
        }
        m_model = std::move(modelResult.value());
    }

    // get last two dimensions of the input shape
    auto shape = m_model->inputShape();
    if (shape.size() < 2) {
        m_model = nullptr;
        return tl::unexpected<Error>(createError(
            tr("Failed to load model: Invalid input shape %1")
                .arg(QString::fromStdString(NN::shapeToString(shape)))));
    }

    emit sig_message(HW_NAME(m_useCuda), tr("Computing preview..."), true);
    auto start = std::chrono::high_resolution_clock::now();  // start clock

    std::cout << "Running inference for semseg..." << std::endl;
    auto result =
        NN::Tensor::fromCvMat(m_image, m_model->inputShape(), 1.0f,
                              m_modelInfo.mean, m_modelInfo.std)
            .and_then(NN::Util::bind_inference(m_model))
            .and_then([](NN::Tensor &&tensor)
                          -> tl::expected<NN::Tensor, NN::NeuralError> {
                // squeeze the tensor to remove leading dimensions of size 1
                if (tensor.dtype() == NN::TensorType::Float) {
                    return tensor.reduceWithIndex(NN::ReduceArgMax{}, 1);
                }
                return tl::expected<NN::Tensor, NN::NeuralError>(
                    std::move(tensor));
            })
            .and_then(NN::Util::bind_squeeze());

    std::cout << "...done!" << std::endl;
    auto end = std::chrono::high_resolution_clock::now();  // stop clock
    auto durationMs =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
            .count();

    if (!result) {
        m_model = nullptr;
        if (result.error().code() == NN::ErrorCode::OutOfMemory) {
            return tl::unexpected<Error>(createError(
                tr("Ran out of memory during segmentation! Lower the working "
                   "resolution to reduce memory usage.")));
        } else {
            return tl::unexpected<Error>(createError(
                tr("Failed to compute segmentation: %1")
                    .arg(QString::fromStdString(result.error().message()))));
        }
    }

    std::cout << "Tensor after inference: " << result.value().toString()
              << std::endl;

    emit sig_message(
        HW_NAME(m_useCuda),
        tr("Finished preview in ") + QString::number(durationMs) + tr("ms"),
        false);

    m_segmentationClasses = std::move(result.value());
    TENSOR_DEBUG_PRINT(m_segmentationClasses);

    if (!computeColorization() || !computeMask()) {
        m_model = nullptr;
        return tl::unexpected<Error>(
            createError(tr("Failed to compute colorization or mask!")));
    }
    std::cout << "Colorization and mask done!" << std::endl;
    auto print_mat = [](const cv::Mat &mat) {
        std::cout << "cv::Mat(size=" << mat.size()
                  << ", channels=" << mat.channels() << ", type=" << mat.type()
                  << ", dims=" << mat.dims << ")" << std::endl;
    };

    m_guiUpToDate = false;
    sendGuiPreview();           // visualize result on gui
    return m_segmentationMask;  // return the result
}

void SemanticSegmentation::enableCuda(bool enabled) {
    m_useCuda = enabled;
    m_model = nullptr;
}

void SemanticSegmentation::setSettings(QMap<QString, QVariant> settings) {
    // find index of the model
    QString modelName = settings.find(USED_MODEL).value().toString();
    int modelIndex = 0;
    for (QString model : m_ONNXmodelList) {
        if (model.compare(modelName) == 0) {
            m_ONNXmodelIdx = modelIndex;
            break;
        }
        modelIndex++;
    }
    QList<QVariant> boolVar = settings.find(SELECTED_CLASSES).value().toList();
    QList<bool> boolList;
    for (QVariant var : boolVar) {
        boolList.append(var.toBool());
    }

    m_ONNXselectedClasses = boolList;
    if (m_settingsWidget) {
        // Disbale class update to prevent classes to be set to default
        m_updateClasses = false;
        m_settingsWidget->setModel(m_ONNXmodelIdx);
        m_settingsWidget->setClasses(m_ONNXselectedClasses);
    }
}

QMap<QString, QVariant> SemanticSegmentation::getSettings() {
    QMap<QString, QVariant> settings;
    // Save name of the model. Index may change based on order in which models
    // are read
    if (m_ONNXmodelList.size() != 0) {
        settings.insert(USED_MODEL, m_ONNXmodelList[m_ONNXmodelIdx]);
    } else {
        settings.insert(USED_MODEL, "no model selected");
    }
    QList<QVariant> boolList;
    for (bool b : m_ONNXselectedClasses) {
        boolList.append(b);
    }
    QVariant selectedClasses = QVariant(boolList);
    settings.insert(SELECTED_CLASSES, selectedClasses);
    return settings;
}

void SemanticSegmentation::deactivate() {
    std::cout << "Deactivating SemanticSegmentation plugin..." << std::endl;
    m_model = nullptr;
}

void SemanticSegmentation::slot_ONNXindexChanged(int n) {
    // update model
    m_model = nullptr;
    m_ONNXmodelIdx = n;
    m_guiUpToDate = false;

    // update class checkboxes on gui
    if (!loadModelInfo()) {
        createError(tr("Failed to load config file for model: %1")
                        .arg(m_ONNXmodelList[m_ONNXmodelIdx]));
        return;
    }

    if (m_updateClasses) {
        // update local class list
        m_ONNXselectedClasses.clear();
        for (int i = 0; i < m_modelInfo.classes.size(); i++) {
            m_ONNXselectedClasses.push_back(true);
        }
    }
    m_updateClasses = true;

    QStringList classes;
    QColorList colors;
    for (const auto &cls : m_modelInfo.classes) {
        classes.append(cls.name);
        colors.append(cls.color);
    }
    emit sig_classesAndColorsChanged(classes, colors, m_ONNXselectedClasses);

    // update gui with new model
    if (m_imageIdx == UINT_MAX) {
        return;
    }
    QTimer::singleShot(0, this, [=]() {
        transform(m_imageIdx, m_image, m_resolution, m_roi);
    });
}

void SemanticSegmentation::slot_selectedClassesChanged(QBoolList classes) {
    m_ONNXselectedClasses = classes;
    m_guiUpToDate = false;

    if (m_imageIdx == UINT_MAX) {
        return;
    }

    QTimer::singleShot(0, this, [=]() {
        computeMask();
        sendGuiPreview();
        m_guiUpToDate = true;
    });
}

void SemanticSegmentation::slot_blendAlphaChanged(float alpha) {
    qDebug() << "Blend alpha set to " << QString::number(alpha);
    m_blendAlpha = alpha;
    m_guiUpToDate = false;
    if (m_imageIdx == UINT_MAX) {
        return;
    }
    // update preview
    QTimer::singleShot(0, this, &SemanticSegmentation::sendGuiPreview);
}

bool SemanticSegmentation::loadModelInfo() {
    // find the current model path
    QString modelPath = QCoreApplication::applicationDirPath() + MODEL_PATH +
                        "/" + m_ONNXmodelList[m_ONNXmodelIdx];
    // change ending from .onnx to .json to get the config path
    QString configPath = modelPath.left(modelPath.lastIndexOf('.')) + ".json";
    if (!QFile::exists(configPath)) {
        qDebug() << "Config file does not exist: " << configPath;
        return false;
    }
    // read the config file and parse to ModelInfo
    QFile file(configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to open config file: " << configPath;
        return false;
    }
    QByteArray data = file.readAll();
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError) {
        qDebug() << "JSON parse error: " << err.errorString();
        return false;
    }

    QJsonObject obj = doc.object();

    m_modelInfo = ModelInfo();

    for (auto val : obj["mean"].toArray())
        m_modelInfo.mean.push_back((float)val.toDouble());
    for (auto val : obj["std"].toArray())
        m_modelInfo.std.push_back((float)val.toDouble());

    for (auto cls_val : obj["classes"].toArray()) {
        QJsonObject cls_obj = cls_val.toObject();
        ClassInfo cls;
        cls.name = cls_obj["name"].toString();

        auto c = cls_obj["color"].toArray();
        if (c.size() != 3) {
            qDebug() << "Invalid color format for class" << cls.name;
            continue;  // skip this class if color is not valid
        }
        cls.color = QColor(c[0].toInt(), c[1].toInt(), c[2].toInt());
        m_modelInfo.classes.push_back(cls);
    }

    return true;
}

void SemanticSegmentation::sendGuiPreview() {
    if (m_guiUpToDate) {
        return;
    }
    cv::Mat preview;
    alphaBlend(m_segmentationColorized, m_image, preview, m_blendAlpha);

    // resize preview to original size of cropped area
    Resolution targetRes(m_originalImage);

    if (m_roi.isDefault()) {
        // No ROI, scale back to the full resolution for preview
        targetRes.resize(preview);

        cv::Mat mask;
        targetRes.resize(m_segmentationMask, mask);  // scale the mask as well

        cv::hconcat(preview, mask, preview);
    } else {
        // scale back to the ROI of the full resolution
        cv::Rect targetRoi = m_roi.cropAsCvRect(targetRes);
        targetRes = Resolution(targetRoi.size());

        targetRes.resize(preview);  // scale the image preview

        cv::Mat fullPreview = m_originalImage.clone();
        preview.copyTo(
            fullPreview(targetRoi));  // fullPreview(targetRoi) = preview;

        cv::Mat mask;
        targetRes.resize(m_segmentationMask, mask);  // scale the mask preview

        // create black background for the area outside the ROI
        cv::Mat maskPreview(m_originalImage.rows, m_originalImage.cols,
                            m_originalImage.type());
        maskPreview.setTo(cv::Scalar(0, 0, 0));
        mask.copyTo(maskPreview(targetRoi));  // maskPreview(targetRoi) = mask;

        cv::hconcat(fullPreview, maskPreview, preview);
    }

    emit sendToGui(m_imageIdx, preview);
    m_guiUpToDate = true;
}

void SemanticSegmentation::alphaBlend(const cv::Mat &foreground,
                                      const cv::Mat &background,
                                      cv::Mat &destionation, float alpha) {
    destionation = alpha * foreground + (1.0f - alpha) * background;
}

bool SemanticSegmentation::computeColorization() {
    // compute the colorized segmentation for the gui
    auto colorResult =
        m_segmentationClasses
            .map(
                [this](const int64_t &value) -> std::array<uint8_t, 3> {
                    return {static_cast<uint8_t>(
                                m_modelInfo.classes[value].color.blue()),
                            static_cast<uint8_t>(
                                m_modelInfo.classes[value].color.green()),
                            static_cast<uint8_t>(
                                m_modelInfo.classes[value].color.red())};
                },
                0)
            .and_then(NN::Util::bind_toCvMat());

    if (!colorResult) {
        // TODO: display something!
        createError(
            tr("Failed to compute colorized segmentation: %1")
                .arg(QString::fromStdString(colorResult.error().message())));
        m_model = nullptr;
        return false;
    }
    m_segmentationColorized = std::move(colorResult.value());
    // resize the result to the original width and height
    if (m_segmentationColorized.cols != m_image.cols ||
        m_segmentationColorized.rows != m_image.rows) {
        cv::resize(m_segmentationColorized, m_segmentationColorized,
                   cv::Size(m_image.cols, m_image.rows), 0, 0,
                   cv::INTER_NEAREST);
    }
    return true;
}

bool SemanticSegmentation::computeMask() {
    // compute the binary mask for the selected classes
    auto classList = m_ONNXselectedClasses;

    auto maskResult = m_segmentationClasses
                          .map([classList](const int64_t &value) -> uint8_t {
                              return classList[value]
                                         ? 255
                                         : 0;  // only keep selected classes
                          })
                          .and_then(NN::Util::bind_toCvMat());

    if (!maskResult) {
        // TODO: Display something
        createError(
            tr("Failed to compute binary mask: %1")
                .arg(QString::fromStdString(maskResult.error().message())));
        m_model = nullptr;
        return false;
    }
    m_segmentationMask = std::move(maskResult.value());
    // resize the result to the original width and height
    if (m_segmentationMask.cols != m_image.cols ||
        m_segmentationMask.rows != m_image.rows) {
        cv::resize(m_segmentationMask, m_segmentationMask,
                   cv::Size(m_image.cols, m_image.rows), 0, 0,
                   cv::INTER_NEAREST);
    }

    cv::cvtColor(m_segmentationMask, m_segmentationMask, cv::COLOR_GRAY2RGB);
    return true;
}

Error SemanticSegmentation::createError(const QString &message) {
    emit sendToGui(m_imageIdx, m_image);
    emit sig_error(message);
    return Error(ErrorCode::RuntimeError, message);
}
