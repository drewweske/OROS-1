#pragma once

#include "oros/foundation/error.hpp"

#include <expected>
#include <string>
#include <utility>

namespace oros::foundation
{
    template <typename T>
    using Result = std::expected<T, Error>;

    using Status = Result<void>;

    [[nodiscard]] inline std::unexpected<Error>
    fail(const ErrorCode code, std::string message)
    {
        return std::unexpected<Error>{
            Error{
                code,
                std::move(message)
            }
        };
    }
}