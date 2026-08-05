#include "oros/platform/window.hpp"
#include "oros/rendering/renderer.hpp"

#include <iostream>
#include <string_view>
#include <utility>

namespace
{
    struct TestState final
    {
        int checks{};
        int failures{};
    };

    void check(
        TestState& state,
        const bool condition,
        const std::string_view name)
    {
        ++state.checks;

        if (condition)
        {
            std::cout
                << "[pass] "
                << name
                << '\n';

            return;
        }

        ++state.failures;

        std::cerr
            << "[fail] "
            << name
            << '\n';
    }
}

int main()
{
    using namespace oros::foundation;
    using namespace oros::platform;
    using namespace oros::rendering;

    TestState state{};

    const Result<Renderer> null_window_result =
        Renderer::create(nullptr);

    check(
        state,
        !null_window_result.has_value(),
        "Renderer rejects a null native window handle");

    check(
        state,
        !null_window_result.has_value() &&
            null_window_result.error().code ==
                ErrorCode::invalid_argument,
        "Null window reports invalid_argument");

    WindowConfig window_config{};
    window_config.title =
        "OROS Direct3D 12 Renderer Test";
    window_config.client_width = 640;
    window_config.client_height = 360;
    window_config.resizable = false;
    window_config.start_visible = false;

    Result<Window> window_result =
        Window::create(window_config);

    check(
        state,
        window_result.has_value(),
        "Renderer test creates a native window");

    if (!window_result.has_value())
    {
        std::cerr
            << "Native window creation failed: ["
            << to_string(window_result.error().code)
            << "] "
            << window_result.error().message
            << '\n';

        std::cout
            << "\nRenderer test summary: "
            << state.checks - state.failures
            << '/'
            << state.checks
            << " passed.\n";

        return 1;
    }

    Window window{
        std::move(window_result.value())
    };

    RendererConfig too_few_buffers_config{};
    too_few_buffers_config.frame_buffer_count = 1U;

    const Result<Renderer> too_few_buffers_result =
        Renderer::create(
            window.native_handle(),
            too_few_buffers_config);

    check(
        state,
        !too_few_buffers_result.has_value(),
        "Renderer rejects fewer than two frame buffers");

    check(
        state,
        !too_few_buffers_result.has_value() &&
            too_few_buffers_result.error().code ==
                ErrorCode::invalid_argument,
        "Too few frame buffers report invalid_argument");

    RendererConfig too_many_buffers_config{};
    too_many_buffers_config.frame_buffer_count = 9U;

    const Result<Renderer> too_many_buffers_result =
        Renderer::create(
            window.native_handle(),
            too_many_buffers_config);

    check(
        state,
        !too_many_buffers_result.has_value(),
        "Renderer rejects more than eight frame buffers");

    check(
        state,
        !too_many_buffers_result.has_value() &&
            too_many_buffers_result.error().code ==
                ErrorCode::invalid_argument,
        "Too many frame buffers report invalid_argument");

    RendererConfig invalid_validation_config{};
    invalid_validation_config.enable_debug_layer =
        false;
    invalid_validation_config.enable_gpu_validation =
        true;

    const Result<Renderer> invalid_validation_result =
        Renderer::create(
            window.native_handle(),
            invalid_validation_config);

    check(
        state,
        !invalid_validation_result.has_value(),
        "GPU validation requires the debug layer");

    check(
        state,
        !invalid_validation_result.has_value() &&
            invalid_validation_result.error().code ==
                ErrorCode::invalid_argument,
        "Invalid validation configuration reports "
        "invalid_argument");

    RendererConfig renderer_config{};
    renderer_config.frame_buffer_count = 3U;
    renderer_config.enable_debug_layer = false;
    renderer_config.enable_gpu_validation = false;
    renderer_config.prefer_high_performance_adapter =
        true;
    renderer_config.vertical_synchronization = true;
    renderer_config.allow_tearing = false;

    Result<Renderer> renderer_result =
        Renderer::create(
            window.native_handle(),
            renderer_config);

    check(
        state,
        renderer_result.has_value(),
        "A valid Direct3D 12 renderer can be created");

    if (!renderer_result.has_value())
    {
        std::cerr
            << "Renderer creation failed: ["
            << to_string(renderer_result.error().code)
            << "] "
            << renderer_result.error().message
            << '\n';

        window.request_close();

        for (int attempt = 0;
             attempt < 8 && window.is_open();
             ++attempt)
        {
            window.pump_events();
        }

        std::cout
            << "\nRenderer test summary: "
            << state.checks - state.failures
            << '/'
            << state.checks
            << " passed.\n";

        return 1;
    }

    {
        Renderer renderer{
            std::move(renderer_result.value())
        };

        check(
            state,
            renderer.is_ready(),
            "A newly created renderer is ready");

        const RendererInfo& information =
            renderer.info();

        check(
            state,
            !information.adapter_name.empty(),
            "Renderer reports its selected adapter");

        check(
            state,
            information.frame_buffer_count == 3U,
            "Renderer preserves the frame-buffer count");

        check(
            state,
            !information.debug_layer_enabled,
            "Renderer reports the disabled debug layer");

        check(
            state,
            !information.gpu_validation_enabled,
            "Renderer reports disabled GPU validation");

        const ClearColor first_color{
            0.015F,
            0.025F,
            0.060F,
            1.0F
        };

        const Status first_frame_status =
            renderer.draw_frame(first_color);

        check(
            state,
            first_frame_status.has_value(),
            "Renderer clears and presents its first frame");

        const ClearColor second_color{
            0.060F,
            0.020F,
            0.015F,
            1.0F
        };

        const Status second_frame_status =
            renderer.draw_frame(second_color);

        check(
            state,
            second_frame_status.has_value(),
            "Renderer clears and presents another frame");

        const Status first_idle_status =
            renderer.wait_idle();

        check(
            state,
            first_idle_status.has_value(),
            "Renderer waits for the GPU to become idle");

        const Status resize_status =
            renderer.resize(800U, 450U);

        check(
            state,
            resize_status.has_value(),
            "Renderer resizes its swap-chain buffers");

        const ClearColor resized_color{
            0.010F,
            0.055F,
            0.035F,
            1.0F
        };

        const Status resized_frame_status =
            renderer.draw_frame(resized_color);

        check(
            state,
            resized_frame_status.has_value(),
            "Renderer draws after a buffer resize");

        const Status minimized_resize_status =
            renderer.resize(0U, 0U);

        check(
            state,
            minimized_resize_status.has_value(),
            "Renderer safely ignores a zero-sized resize");

        const Status final_idle_status =
            renderer.wait_idle();

        check(
            state,
            final_idle_status.has_value(),
            "Renderer completes its final GPU work");
    }

    check(
        state,
        window.is_open(),
        "Renderer destruction preserves the window");

    window.request_close();

    for (int attempt = 0;
         attempt < 8 && window.is_open();
         ++attempt)
    {
        window.pump_events();
    }

    check(
        state,
        !window.is_open(),
        "Renderer test window closes cleanly");

    std::cout
        << "\nRenderer test summary: "
        << state.checks - state.failures
        << '/'
        << state.checks
        << " passed.\n";

    return state.failures == 0 ? 0 : 1;
}