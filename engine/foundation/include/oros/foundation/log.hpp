#pragma once

#include "oros/foundation/result.hpp"

#include <cstdint>
#include <source_location>
#include <string>
#include <string_view>

namespace oros::foundation
{
    enum class LogLevel : std::uint8_t
    {
        trace,
        debug,
        info,
        warning,
        error,
        critical
    };

    struct LogConfig final
    {
        bool write_to_console{true};
        bool write_to_file{true};
        std::string file_path{"logs/oros.log"};
    };

    [[nodiscard]] constexpr std::string_view
    to_string(const LogLevel level) noexcept
    {
        switch (level)
        {
        case LogLevel::trace:
            return "trace";

        case LogLevel::debug:
            return "debug";

        case LogLevel::info:
            return "info";

        case LogLevel::warning:
            return "warning";

        case LogLevel::error:
            return "error";

        case LogLevel::critical:
            return "critical";
        }

        return "unknown";
    }

    [[nodiscard]] Status initialize_logging(
        LogConfig config = {});

    void shutdown_logging() noexcept;

    [[nodiscard]] bool is_logging_initialized() noexcept;

    void write_log(
        LogLevel level,
        std::string_view category,
        std::string_view message,
        std::source_location location =
            std::source_location::current()) noexcept;
}