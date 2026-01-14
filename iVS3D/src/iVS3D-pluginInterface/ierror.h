#pragma once

#include <QString>

/**
 * @enum ErrorCode
 * @brief Enumeration of error codes used in iVS3D plugin interfaces.
 */
enum class ErrorCode {
    RuntimeError
};

/**
 * @class Error
 * @brief Represents an error with a code and a message. The message is intended
 * for display to the user.
 */
class Error {
public:
    Error(ErrorCode code, QString message)
        : code(code), message(std::move(message)) {}

    ErrorCode code;
    QString message;
};