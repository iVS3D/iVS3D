#pragma once

/**
 * @file ModelConfig.h
 * @brief Model configuration container and JSON loader for neural-network metadata.
 * @author Dominik Wüst
 * @date March 2026
 */

#include <QColor>
#include <QDir>
#include <QMap>
#include <QMetaType>
#include <QString>
#include <QVector>
#include <cstdint>
#include <string>
#include <vector>

#include <tl/expected.hpp>

/**
 * @defgroup ModelConfig ModelConfig
 * @brief Model configuration and management components for neural-network models.
 *
 * This group contains the configuration container, model manager, and UI settings widget.
 */

/**
 * @brief Namespace containing model-configuration components used by iVS3D plugins.
 *
 * See also the module overview and JSON format in
 * @ref modelconfig_readme "ModelConfig.md".
 */
namespace MCFG {

/**
 * @class ModelConfig
 * @brief Stores one parsed model configuration and runtime class-selection state.
 *
 * @details
 * A configuration is loaded from a JSON file via loadFromFile(), validated,
 * and then used by ModelManager and UI components.
 *
 * The expected JSON format is documented in @ref modelconfig_readme "ModelConfig.md".
 *
 * @ingroup ModelConfig
 * 
 * @author Dominik Wüst (dominik.wuest@iosb.fraunhofer.de)
 * @date March 2026
 */
class ModelConfig {
public:
    /**
     * @brief Error codes returned by loadFromFile().
     */
    enum class ErrorCode {
        ConfigFileNotFound = 0,
        ModelFileNotFound = 1,
        IoError = 2,
        ConfigParseError = 3,
        DuplicateClassIds = 4,
    };

    /**
     * @brief Error object returned on failed config loading.
     */
    struct Error {
        /**< Machine-readable error code. */
        ErrorCode code;
        /**< Human-readable error message. */
        QString message;
    };

    /** @brief Unsigned integer type used for class identifiers. */
    using ClassId = uint32_t;

    /**
     * @brief Description of one output class.
     */
    struct ClassInfo {
        /**< Stable class identifier used by model outputs and UI selections. */
        ClassId id;
        /**< Display name of the class. */
        QString name;
        /**< Display color used by visualizations and UI. */
        QColor color;
        /**< Runtime selection state used to enable/disable classes. */
        bool selected = false;
    };

    /**
     * @brief Load a model configuration from a JSON file
     * Performs validation:
     * - Checks for duplicate class IDs
     * - Verifies model file exists (or defaults to config path with .onnx extension)
     * @param jsonPath Path to the JSON configuration file
     * @return ModelConfig if successful, Error otherwise
    *
    * @see @ref modelconfig_readme "ModelConfig.md".
     */
    static tl::expected<ModelConfig, ModelConfig::Error> loadFromFile(const QString& jsonPath);

    /** @brief Get the per-channel mean vector from config. */
    const std::vector<float>& getMean() const noexcept;

    /** @brief Get the per-channel standard deviation vector from config. */
    const std::vector<float>& getStd() const noexcept;

    /** @brief Get the resolved absolute path to the ONNX model file. */
    const std::string& getModelPath() const noexcept;

    /** @brief Get input alignment requirement (multiple-of value for H/W). */
    uint getInputAlignment() const noexcept;

    /** @brief Check whether mean/std normalization is enabled. */
    bool getApplyMeanStd() const noexcept;

    /** @brief Check whether [0,255] -> [0,1] normalization is enabled. */
    bool getNormalizeTo01() const noexcept;

    /** @brief Get all configured classes. */
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

} // namespace MCFG

// Register custom types for use in signals/slots
Q_DECLARE_METATYPE(MCFG::ModelConfig::ClassInfo)
Q_DECLARE_METATYPE(QVector<MCFG::ModelConfig::ClassInfo>)

// Backward-compatible alias (prefer MCFG::ModelConfig in new code).
using ModelConfig = MCFG::ModelConfig;