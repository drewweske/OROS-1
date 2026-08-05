#include "oros/foundation/clock.hpp"
#include "oros/foundation/log.hpp"
#include "oros/platform/window.hpp"

#include <chrono>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

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
    using namespace oros::platform;

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
        << "OROS-001 Foundation: PASS\n";

    WindowConfig window_config{};
    window_config.title =
        "OROS 1 - OROS-002 Platform";
    window_config.client_width = 1280;
    window_config.client_height = 720;
    window_config.resizable = true;
    window_config.start_visible = true;

    Result<Window> window_result =
        Window::create(
            std::move(window_config));

    if (!window_result.has_value())
    {
        const Error& error =
            window_result.error();

        write_log(
            LogLevel::critical,
            "platform",
            "Native window creation failed: [" +
                std::string{to_string(error.code)} +
                "] " +
                error.message);

        std::cerr
            << "OROS native window creation failed: ["
            << to_string(error.code)
            << "] "
            << error.message
            << '\n';

        shutdown_logging();
        return 1;
    }

    {
        Window window{
            std::move(window_result.value())
        };

        const WindowExtent extent =
            window.client_extent();

        write_log(
            LogLevel::info,
            "platform",
            std::string{
                "Native window created with client extent "
            } +
                std::to_string(extent.width) +
                "x" +
                std::to_string(extent.height) +
                ".");

        std::cout
            << "OROS-002 Platform: ACTIVE\n";

        std::cout
            << "Native window: "
            << extent.width
            << 'x'
            << extent.height
            << '\n';

        std::cout
            << "Close the window or press Escape to exit.\n";

        const auto startup_microseconds =
            std::chrono::duration_cast<
                std::chrono::microseconds>(
                    startup_timer.elapsed())
                .count();

        write_log(
            LogLevel::info,
            "bootstrap",
            "Bootstrap completed in " +
                std::to_string(
                    startup_microseconds) +
                " microseconds.");

        while (!window.should_close())
        {
            window.pump_events();

            const InputState& input =
                window.input_state();

            if (input.key_pressed(Key::escape))
            {
                write_log(
                    LogLevel::info,
                    "input",
                    "Escape requested window closure.");

                window.request_close();
            }

            std::this_thread::sleep_for(
                std::chrono::milliseconds{1});
        }
    }

    write_log(
        LogLevel::info,
        "platform",
        "Native window destroyed cleanly.");

    shutdown_logging();

    return 0;
}