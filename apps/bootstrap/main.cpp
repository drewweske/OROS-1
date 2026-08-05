#include "oros/foundation/clock.hpp"
#include "oros/foundation/log.hpp"
#include "oros/platform/window.hpp"
#include "oros/rendering/renderer.hpp"
#include "oros/runtime/engine_runtime.hpp"

#include <chrono>
#include <cstdint>
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
    using namespace oros::rendering;
    using namespace oros::runtime;

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

    std::cout
        << "OROS-002 Platform: PASS\n";

    WindowConfig window_config{};
    window_config.title =
        "OROS 1 - OROS-004 Runtime";
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

    int exit_code = 0;

    {
        Window window{
            std::move(window_result.value())
        };

        WindowExtent current_extent =
            window.client_extent();

        write_log(
            LogLevel::info,
            "platform",
            std::string{
                "Native window created with client extent "
            } +
                std::to_string(current_extent.width) +
                "x" +
                std::to_string(current_extent.height) +
                ".");

        RendererConfig renderer_config{};
        renderer_config.frame_buffer_count = 3U;
        renderer_config.enable_debug_layer = false;
        renderer_config.enable_gpu_validation = false;
        renderer_config.prefer_high_performance_adapter =
            true;
        renderer_config.vertical_synchronization = true;
        renderer_config.allow_tearing = true;

        Result<Renderer> renderer_result =
            Renderer::create(
                window.native_handle(),
                renderer_config);

        if (!renderer_result.has_value())
        {
            const Error& error =
                renderer_result.error();

            write_log(
                LogLevel::critical,
                "rendering",
                "Direct3D 12 renderer creation failed: [" +
                    std::string{
                        to_string(error.code)} +
                    "] " +
                    error.message);

            std::cerr
                << "OROS renderer creation failed: ["
                << to_string(error.code)
                << "] "
                << error.message
                << '\n';

            exit_code = 1;
        }
        else
        {
            {
                Renderer renderer{
                    std::move(renderer_result.value())
                };

                const RendererInfo& renderer_info =
                    renderer.info();

                const std::uint64_t
                    dedicated_memory_mib =
                        renderer_info.
                            dedicated_video_memory_bytes /
                        (1024ULL * 1024ULL);

                write_log(
                    LogLevel::info,
                    "rendering",
                    "Direct3D 12 renderer initialized on " +
                        renderer_info.adapter_name +
                        ".");

                write_log(
                    LogLevel::info,
                    "rendering",
                    "Dedicated video memory: " +
                        std::to_string(
                            dedicated_memory_mib) +
                        " MiB.");

                EngineRuntimeConfig runtime_config{};

                Result<EngineRuntime> runtime_result =
                    EngineRuntime::create(
                        runtime_config);

                if (!runtime_result.has_value())
                {
                    const Error& error =
                        runtime_result.error();

                    write_log(
                        LogLevel::critical,
                        "runtime",
                        "Engine runtime creation failed: [" +
                            std::string{
                                to_string(error.code)} +
                            "] " +
                            error.message);

                    std::cerr
                        << "OROS runtime creation failed: ["
                        << to_string(error.code)
                        << "] "
                        << error.message
                        << '\n';

                    exit_code = 1;
                }
                else
                {
                    EngineRuntime runtime{
                        std::move(runtime_result.value())
                    };

                    const Status runtime_start_status =
                        runtime.start(
                            monotonic_now());

                    if (!runtime_start_status.has_value())
                    {
                        const Error& error =
                            runtime_start_status.error();

                        write_log(
                            LogLevel::critical,
                            "runtime",
                            "Engine runtime startup failed: [" +
                                std::string{
                                    to_string(
                                        error.code)} +
                                "] " +
                                error.message);

                        std::cerr
                            << "OROS runtime startup failed: ["
                            << to_string(error.code)
                            << "] "
                            << error.message
                            << '\n';

                        exit_code = 1;
                    }
                    else
                    {
                        write_log(
                            LogLevel::info,
                            "runtime",
                            "Engine runtime entered the " +
                                std::string{
                                    to_string(
                                        runtime.state())} +
                                " state.");

                        std::cout
                            << "OROS-003 First Light: PASS\n";

                        std::cout
                            << "OROS-004 Runtime: ACTIVE\n";

                        std::cout
                            << "Renderer: Direct3D 12\n";

                        std::cout
                            << "Adapter: "
                            << renderer_info.adapter_name
                            << '\n';

                        std::cout
                            << "Dedicated video memory: "
                            << dedicated_memory_mib
                            << " MiB\n";

                        std::cout
                            << "Frame buffers: "
                            << renderer_info.
                                frame_buffer_count
                            << '\n';

                        std::cout
                            << "Native window: "
                            << current_extent.width
                            << 'x'
                            << current_extent.height
                            << '\n';

                        std::cout
                            << "Fixed simulation step: "
                            << runtime.config().
                                fixed_step.
                                    simulation_step.count()
                            << " nanoseconds\n";

                        std::cout
                            << "Maximum updates per frame: "
                            << runtime.config().
                                fixed_step.
                                    maximum_updates_per_frame
                            << '\n';

                        std::cout
                            << "Close the window or press "
                            << "Escape to exit.\n";

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

                        const ClearColor first_light_color{
                            0.015F,
                            0.075F,
                            0.140F,
                            1.0F
                        };

                        const auto request_runtime_stop =
                            [&runtime, &exit_code](
                                const std::string_view reason)
                            {
                                const Status request_status =
                                    runtime.request_stop();

                                if (!request_status.has_value())
                                {
                                    const Error& error =
                                        request_status.error();

                                    write_log(
                                        LogLevel::critical,
                                        "runtime",
                                        "Runtime stop request "
                                        "failed: [" +
                                            std::string{
                                                to_string(
                                                    error.code)} +
                                            "] " +
                                            error.message);

                                    std::cerr
                                        << "OROS runtime stop "
                                        << "request failed: ["
                                        << to_string(
                                            error.code)
                                        << "] "
                                        << error.message
                                        << '\n';

                                    exit_code = 1;
                                    return false;
                                }

                                write_log(
                                    LogLevel::info,
                                    "runtime",
                                    "Runtime stop requested: " +
                                        std::string{reason} +
                                        ".");

                                return true;
                            };

                        while (runtime.is_running())
                        {
                            window.pump_events();

                            if (window.should_close())
                            {
                                if (!request_runtime_stop(
                                        "native window closed"))
                                {
                                    break;
                                }

                                continue;
                            }

                            const InputState& input =
                                window.input_state();

                            if (input.key_pressed(Key::escape))
                            {
                                window.request_close();

                                if (!request_runtime_stop(
                                        "Escape key pressed"))
                                {
                                    break;
                                }

                                continue;
                            }

                            Result<RuntimeFrame> frame_result =
                                runtime.begin_frame(
                                    monotonic_now());

                            if (!frame_result.has_value())
                            {
                                const Error& error =
                                    frame_result.error();

                                write_log(
                                    LogLevel::critical,
                                    "runtime",
                                    "Runtime frame failed: [" +
                                        std::string{
                                            to_string(
                                                error.code)} +
                                        "] " +
                                        error.message);

                                std::cerr
                                    << "OROS runtime frame "
                                    << "failed: ["
                                    << to_string(error.code)
                                    << "] "
                                    << error.message
                                    << '\n';

                                exit_code = 1;

                                if (!request_runtime_stop(
                                        "runtime frame failure"))
                                {
                                    break;
                                }

                                continue;
                            }

                            const WindowExtent new_extent =
                                window.client_extent();

                            if (new_extent.width !=
                                    current_extent.width ||
                                new_extent.height !=
                                    current_extent.height)
                            {
                                const Status resize_status =
                                    renderer.resize(
                                        new_extent.width,
                                        new_extent.height);

                                if (!resize_status.has_value())
                                {
                                    const Error& error =
                                        resize_status.error();

                                    write_log(
                                        LogLevel::critical,
                                        "rendering",
                                        "Renderer resize "
                                        "failed: [" +
                                            std::string{
                                                to_string(
                                                    error.code)} +
                                            "] " +
                                            error.message);

                                    std::cerr
                                        << "OROS renderer "
                                        << "resize failed: ["
                                        << to_string(
                                            error.code)
                                        << "] "
                                        << error.message
                                        << '\n';

                                    exit_code = 1;

                                    if (!request_runtime_stop(
                                            "renderer resize "
                                            "failure"))
                                    {
                                        break;
                                    }

                                    continue;
                                }

                                current_extent = new_extent;
                            }

                            if (current_extent.width == 0U ||
                                current_extent.height == 0U)
                            {
                                std::this_thread::sleep_for(
                                    std::chrono::
                                        milliseconds{16});

                                continue;
                            }

                            const Status frame_status =
                                renderer.draw_frame(
                                    first_light_color);

                            if (!frame_status.has_value())
                            {
                                const Error& error =
                                    frame_status.error();

                                write_log(
                                    LogLevel::critical,
                                    "rendering",
                                    "Frame rendering failed: [" +
                                        std::string{
                                            to_string(
                                                error.code)} +
                                        "] " +
                                        error.message);

                                std::cerr
                                    << "OROS frame rendering "
                                    << "failed: ["
                                    << to_string(error.code)
                                    << "] "
                                    << error.message
                                    << '\n';

                                exit_code = 1;

                                if (!request_runtime_stop(
                                        "frame rendering "
                                        "failure"))
                                {
                                    break;
                                }
                            }
                        }

                        const Status idle_status =
                            renderer.wait_idle();

                        if (!idle_status.has_value())
                        {
                            const Error& error =
                                idle_status.error();

                            write_log(
                                LogLevel::error,
                                "rendering",
                                "Final GPU wait failed: [" +
                                    std::string{
                                        to_string(
                                            error.code)} +
                                    "] " +
                                    error.message);

                            std::cerr
                                << "OROS final GPU wait "
                                << "failed: ["
                                << to_string(error.code)
                                << "] "
                                << error.message
                                << '\n';

                            exit_code = 1;
                        }

                        if (runtime.is_stop_requested())
                        {
                            const Status runtime_stop_status =
                                runtime.stop();

                            if (!runtime_stop_status.
                                    has_value())
                            {
                                const Error& error =
                                    runtime_stop_status.error();

                                write_log(
                                    LogLevel::critical,
                                    "runtime",
                                    "Engine runtime shutdown "
                                    "failed: [" +
                                        std::string{
                                            to_string(
                                                error.code)} +
                                        "] " +
                                        error.message);

                                std::cerr
                                    << "OROS runtime shutdown "
                                    << "failed: ["
                                    << to_string(error.code)
                                    << "] "
                                    << error.message
                                    << '\n';

                                exit_code = 1;
                            }
                        }
                        else if (runtime.is_running())
                        {
                            write_log(
                                LogLevel::critical,
                                "runtime",
                                "The engine loop ended without "
                                "a legal runtime stop request.");

                            std::cerr
                                << "OROS engine loop ended "
                                << "without a legal runtime "
                                << "stop request.\n";

                            exit_code = 1;
                        }

                        if (runtime.is_stopped())
                        {
                            const std::uint64_t
                                completed_frames =
                                    runtime.scheduler().
                                        completed_frame_count();

                            const std::uint64_t
                                completed_ticks =
                                    runtime.scheduler().
                                        completed_simulation_tick_count();

                            const auto dropped_microseconds =
                                std::chrono::duration_cast<
                                    std::chrono::microseconds>(
                                        runtime.scheduler().
                                            dropped_time())
                                    .count();

                            write_log(
                                LogLevel::info,
                                "runtime",
                                "Engine runtime stopped after " +
                                    std::to_string(
                                        completed_frames) +
                                    " frames and " +
                                    std::to_string(
                                        completed_ticks) +
                                    " simulation ticks.");

                            write_log(
                                LogLevel::info,
                                "runtime",
                                "Runtime dropped " +
                                    std::to_string(
                                        dropped_microseconds) +
                                    " microseconds.");

                            std::cout
                                << "OROS-004 Runtime: STOPPED\n";

                            std::cout
                                << "Completed runtime frames: "
                                << completed_frames
                                << '\n';

                            std::cout
                                << "Completed simulation ticks: "
                                << completed_ticks
                                << '\n';
                        }
                    }
                }
            }

            write_log(
                LogLevel::info,
                "rendering",
                "Direct3D 12 renderer destroyed cleanly.");
        }
    }

    write_log(
        LogLevel::info,
        "platform",
        "Native window destroyed cleanly.");

    shutdown_logging();

    return exit_code;
}