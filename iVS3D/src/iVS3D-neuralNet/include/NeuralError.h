#pragma once

/**
 * @file NeuralError.h
 * @brief Defines error handling classes for the neural network module.
 * @author Dominik Wüst (dominik.wuest@iosb.faunhofer.de)
 * @date August 2025
 */

#include <ostream>
#include <string>

namespace NN {

/**
 * @brief Error codes for the neural network module.
 * 
 * @ingroup NeuralNet
 *
 * @details This enum class defines the various error codes 
 * that can be returned by the neural network module:
 * - InvalidArgument: The provided argument i.e. the path to the onnx model is invalid.
 * - OutOfMemory: The system/gpu ran out of memory.
 * - RuntimeError: A runtime error occurred.
 *
 * @author Dominik Wüst (dominik.wuest@iosb.faunhofer.de)
 * @date August 2025
 */
enum class ErrorCode {
    InvalidArgument = 0,
    OutOfMemory = 1,
    RuntimeError = 2
};


/**
 * @brief Represents an error that occurred in the neural network module and contains the error type and message.
 *
 * @ingroup NeuralNet
 *
 * @details This class encapsulates the error code and message for errors occurring in the neural network module.
 * 
 * @see ErrorType for available error types.
 * 
 * @date August 2025
 * @author Dominik Wüst (dominik.wuest@iosb.faunhofer.de)
 */
class NeuralError {
public:
    NeuralError(ErrorCode code, const std::string& message)
        : m_code(code), m_message(message) {}

    ErrorCode code() const { return m_code; }
    std::string message() const { return m_message; }

private:
    ErrorCode m_code;
    std::string m_message;
};

// Helper function to convert ErrorCode to string
inline const char* to_string(ErrorCode code) {
    switch (code) {
        case ErrorCode::InvalidArgument: return "InvalidArgument";
        case ErrorCode::OutOfMemory:     return "OutOfMemory";
        case ErrorCode::RuntimeError:    return "RuntimeError";
        default:                         return "UnknownError";
    }
}

// Overload operator<< for NeuralError
inline std::ostream& operator<<(std::ostream& os, const NeuralError& err) {
    os << "[" << to_string(err.code()) << "] " << err.message();
    return os;
}

} // namespace NN
