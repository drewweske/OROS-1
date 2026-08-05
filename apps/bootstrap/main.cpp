#include "oros/foundation/clock.hpp"
#include "oros/foundation/log.hpp"

#include <chrono>
#include <iostream>
#include <string>
#include <string_view>

namespace oros
{
    inline constexpr std::string_view engine_name{
        "OROS 1"
    };

    struct Version final
    {
        int major;
        int minor;
        int patch;
    };

    inline constexpr Version engine_version{
        OROS_VERSION_MAJOR,
        OROS_VERSION_MINOR,
        OROS_VERSION_PATCH
    };
}

int main()
{
    using namespace oros::foundation;

    Stopwatch startup_timer{};

    const Status logging_status =
        initialize_logging();

    if (!logging_status.has_value())
    {
        const Error& error =
            logging_status.error();

        std::cerr
            << "OROS logging initialization failed: ["
            << to_string(error.code)
            << "] "
            << error.message
            << '\n';

        return 1;
    }

    write_log(
        LogLevel::info,
        "bootstrap",
        "OROS logging initialized.");

    std::cout
        << oros::engine_name
        << " Engine Bootstrap\n";

    std::cout
        << "Version "
        << oros::engine_version.major
        << '.'
        << oros::engine_version.minor
        << '.'
        << oros::engine_version.patch
        << '\n';

    std::cout
        << "OROS-000 Genesis: PASS\n";

    std::cout
        << "OROS-001 Foundation: ACTIVE\n";

    const auto startup_microseconds =
        std::chrono::duration_cast<
            std::chrono::microseconds>(
                startup_timer.elapsed())
            .count();

    write_log(
        LogLevel::info,
        "bootstrap",
        "Foundation systems are active.");

    write_log(
        LogLevel::info,
        "bootstrap",
        "Bootstrap completed in " +
            std::to_string(startup_microseconds) +
            " microseconds.");

    shutdown_logging();

    return 0;
}