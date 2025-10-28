#include <QString>

enum class ErrorCode {
    RuntimeError
};

class Error {
public:
    Error(ErrorCode code, QString message)
        : code(code), message(std::move(message)) {}

    ErrorCode code;
    QString message;
};