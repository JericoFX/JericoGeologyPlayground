#pragma once

#include <Jerico/Core/EngineError.hpp>

#include <optional>
#include <stdexcept>
#include <utility>
#include <variant>

namespace Jerico
{
template<typename T>
class Result
{
public:
    static Result success(T value)
    {
        return Result(std::move(value));
    }

    static Result failure(EngineError error)
    {
        return Result(std::move(error));
    }

    [[nodiscard]] bool hasValue() const noexcept
    {
        return std::holds_alternative<T>(storage_);
    }

    T& value()
    {
        if (!hasValue())
        {
            throw std::logic_error("Result does not contain a value");
        }
        return std::get<T>(storage_);
    }

    const T& value() const
    {
        if (!hasValue())
        {
            throw std::logic_error("Result does not contain a value");
        }
        return std::get<T>(storage_);
    }

    EngineError& error()
    {
        if (hasValue())
        {
            throw std::logic_error("Result does not contain an error");
        }
        return std::get<EngineError>(storage_);
    }

    const EngineError& error() const
    {
        if (hasValue())
        {
            throw std::logic_error("Result does not contain an error");
        }
        return std::get<EngineError>(storage_);
    }

private:
    explicit Result(T value)
        : storage_(std::move(value))
    {
    }

    explicit Result(EngineError error)
        : storage_(std::move(error))
    {
    }

    std::variant<T, EngineError> storage_;
};

template<>
class Result<void>
{
public:
    static Result success()
    {
        return Result();
    }

    static Result failure(EngineError error)
    {
        return Result(std::move(error));
    }

    [[nodiscard]] bool hasValue() const noexcept
    {
        return !error_.has_value();
    }

    void value() const
    {
        if (!hasValue())
        {
            throw std::logic_error("Result does not contain a value");
        }
    }

    EngineError& error()
    {
        if (hasValue())
        {
            throw std::logic_error("Result does not contain an error");
        }
        return *error_;
    }

    const EngineError& error() const
    {
        if (hasValue())
        {
            throw std::logic_error("Result does not contain an error");
        }
        return *error_;
    }

private:
    Result() = default;

    explicit Result(EngineError error)
        : error_(std::move(error))
    {
    }

    std::optional<EngineError> error_;
};
}
