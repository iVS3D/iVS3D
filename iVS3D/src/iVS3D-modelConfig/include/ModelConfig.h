#pragma once

#include <QColor>
#include <QDir>
#include <QMap>
#include <QMetaType>
#include <QString>
#include <string>
#include <vector>

#include <tl/expected.hpp>



class ModelConfig {
public:
    enum class ErrorCode {
        ConfigFileNotFound = 0,
        ModelFileNotFound = 1,
        IoError = 2,
        ConfigParseError = 3,
        DuplicateClassIds = 4,
    };

    struct Error {
        ErrorCode code;
        QString message;
    };

    using ClassId = uint32_t;
    struct ClassInfo {
        ClassId id;
        QString name;
        QColor color;
        bool selected = false;
    };

    /**
     * @brief Load a model configuration from a JSON file
     * Performs validation:
     * - Checks for duplicate class IDs
     * - Verifies model file exists (or defaults to config path with .onnx extension)
     * @param jsonPath Path to the JSON configuration file
     * @return ModelConfig if successful, Error otherwise
     */
    static tl::expected<ModelConfig, ModelConfig::Error> loadFromFile(const QString& jsonPath);

    // Getters for configuration data
    const std::vector<float>& getMean() const noexcept;
    const std::vector<float>& getStd() const noexcept;
    const std::string& getModelPath() const noexcept;
    uint getInputAlignment() const noexcept;
    bool getApplyMeanStd() const noexcept;
    bool getNormalizeTo01() const noexcept;

    // Class information getters
    const std::vector<ClassInfo>& getClasses() const noexcept;
    
    /**
     * @brief Get class info by ID
     * @param id Class ID to look up
     * @return Pointer to ClassInfo if found, nullptr otherwise
     */
    const ClassInfo* getClassById(ClassId id) const noexcept;

    /**
     * @brief Set the selected state of a class
     * @param id Class ID to update
     * @param selected Whether the class should be selected
     * @return true if class was found and updated, false otherwise
     */
    bool setClassSelected(ClassId id, bool selected) noexcept;

    /**
     * @brief Set whether to apply mean/std normalization
     * @param apply true to enable mean/std application, false to disable
     */
    void setApplyMeanStd(bool apply) noexcept;

    /**
     * @brief Set whether to normalize input to [0,1]
     * @param normalize true to enable normalization, false to disable
     */
    void setNormalizeTo01(bool normalize) noexcept;

    /**
     * @brief Set the input alignment value
     * @param alignment Alignment value for input dimensions
     */
    void setInputAlignment(uint alignment) noexcept;


private:
    // Private members - only accessible via getters
    std::vector<float> mean_;
    std::vector<float> std_;
    std::vector<ClassInfo> classes_;
    std::string modelPath_;
    uint inputAlignment_ = 1;
    bool applyMeanStd_ = true;      // Whether to apply mean/std normalization
    bool normalizeTo01_ = true;     // Whether to normalize input to [0,1]
};

// Register custom types for use in signals/slots
Q_DECLARE_METATYPE(ModelConfig::ClassInfo)
Q_DECLARE_METATYPE(QVector<ModelConfig::ClassInfo>)