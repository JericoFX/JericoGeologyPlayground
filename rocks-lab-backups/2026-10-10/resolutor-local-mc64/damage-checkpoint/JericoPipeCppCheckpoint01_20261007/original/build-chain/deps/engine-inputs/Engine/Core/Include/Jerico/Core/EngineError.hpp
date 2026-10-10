#pragma once

#include <string>

namespace Jerico
{
enum class ErrorSeverity
{
    Error,
    Fatal
};

struct EngineError
{
    std::string code;
    std::string message;
    std::string stage;
    std::string subject;
    std::string cause;
    std::string suggestedAction;
    ErrorSeverity severity{ErrorSeverity::Error};
};
}
