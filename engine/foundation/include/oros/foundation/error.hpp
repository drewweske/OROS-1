#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace oros::foundation
{
    enum class ErrorCode : std::uint16_t
    {
        none = 0,
        invalid_argument,
        invalid_state,
        not_found,
        input_output_failure,
        unsupported_operation,
        out_of_memory,
        timeout,
        internal_failure
    };

    struct Error final
    {
        ErrorCode code{ErrorCode::none};
        std::string message{};

        [[nodiscard]] bool has_error() const noexcept
        {
            return code != ErrorCode::none;
        }

        [[nodiscard]] explicit operator bool() const noexcept
        {
            return has_error();
        }
    };

    [[nodiscard]] constexpr std::string_view
    to_string(const ErrorCode code) noexcept
    {
        switch (code)
        {
        case ErrorCode::none:
            return "none";

        case ErrorCode::invalid_argument:
            return "invalid_argument";

        case ErrorCode::invalid_state:
            return "invalid_state";

        case ErrorCode::not_found:
            return "not_found";

        case ErrorCode::input_output_failure:
            return "input_output_failure";

        case ErrorCode::unsupported_operation:
            return "unsupported_operation";

        case ErrorCode::out_of_memory:
            return "out_of_memory";

        case ErrorCode::timeout:
            return "timeout";

        case ErrorCode::internal_failure:
            return "internal_failure";
        }

        return "unknown";
    }
}