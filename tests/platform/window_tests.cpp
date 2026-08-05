#include "oros/platform/window.hpp"

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

    TestState state{};

    WindowConfig empty_title_config{};
    empty_title_config.title.clear();
    empty_title_config.start_visible = false;

    const Result<Window> empty_title_result =
        Window::create(empty_title_config);

    check(
        state,
        !empty_title_result.has_value(),
        "Window rejects an empty title");

    check(
        state,
        !empty_title_result.has_value() &&
            empty_title_result.error().code ==
                ErrorCode::invalid_argument,
        "Empty title reports invalid_argument");

    WindowConfig zero_width_config{};
    zero_width_config.client_width = 0;
    zero_width_config.start_visible = false;

    const Result<Window> zero_width_result =
        Window::create(zero_width_config);

    check(
        state,
        !zero_width_result.has_value(),
        "Window rejects a zero width");

    check(
        state,
        !zero_width_result.has_value() &&
            zero_width_result.error().code ==
                ErrorCode::invalid_argument,
        "Zero width reports invalid_argument");

    WindowConfig valid_config{};
    valid_config.title = "OROS Platform Window Test";
    valid_config.client_width = 640;
    valid_config.client_height = 360;
    valid_config.resizable = false;
    valid_config.start_visible = false;

    Result<Window> window_result =
        Window::create(valid_config);

    check(
        state,
        window_result.has_value(),
        "A valid native window can be created");

    if (!window_result.has_value())
    {
        std::cerr
            << "Native window creation failed: ["
            << to_string(window_result.error().code)
            << "] "
            << window_result.error().message
            << '\n';

        std::cout
            << "\nWindow test summary: "
            << state.checks - state.failures
            << '/'
            << state.checks
            << " passed.\n";

        return 1;
    }

    Window window{
        std::move(window_result.value())
    };

    check(
        state,
        window.is_open(),
        "A newly created window is open");

    check(
        state,
        !window.should_close(),
        "A newly created window does not request closure");

    check(
        state,
        window.native_handle() != nullptr,
        "A native window handle is available");

    const WindowExtent initial_extent =
        window.client_extent();

    check(
        state,
        initial_extent.width == 640,
        "Window preserves the requested client width");

    check(
        state,
        initial_extent.height == 360,
        "Window preserves the requested client height");

    check(
        state,
        !window.input_state().key_down(Key::escape),
        "Window input begins with Escape released");

    window.pump_events();

    check(
        state,
        window.is_open(),
        "Pumping ordinary events keeps the window open");

    window.request_close();

    check(
        state,
        window.should_close(),
        "A programmatic close request is recorded");

    for (int attempt = 0;
         attempt < 8 && window.is_open();
         ++attempt)
    {
        window.pump_events();
    }

    check(
        state,
        !window.is_open(),
        "The close message destroys the native window");

    check(
        state,
        window.should_close(),
        "A destroyed window remains marked for closure");

    check(
        state,
        window.native_handle() == nullptr,
        "A destroyed window clears its native handle");

    const WindowExtent closed_extent =
        window.client_extent();

    check(
        state,
        closed_extent.width == 0 &&
            closed_extent.height == 0,
        "A destroyed window reports an empty extent");

    std::cout
        << "\nWindow test summary: "
        << state.checks - state.failures
        << '/'
        << state.checks
        << " passed.\n";

    return state.failures == 0 ? 0 : 1;
}