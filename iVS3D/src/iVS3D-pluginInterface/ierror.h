#pragma once

#include <QString>

namespace PLUG {

/**
 * @enum ErrorCode
 * @brief Enumeration of error codes used in iVS3D plugin interfaces.
 * @ingroup Plugin
 */
enum class ErrorCode {
    RuntimeError,
    InvalidInput,
    NotImplemented,
    ResourceUnavailable,
    GpuOutOfMemory
};

/**
 * @class Error
 * @brief Represents an error with a code and a message. The message is intended
 * for display to the user.
 *
 * @ingroup Plugin
 * @see @ref plugin_interface_doc "PluginInterface.md"
 */
class Error {
public:
    Error(ErrorCode code, QString message)
        : code(code), message(std::move(message)) {}

    ErrorCode code;
    QString message;
};

} // namespace PLUG