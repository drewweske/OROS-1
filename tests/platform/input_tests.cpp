#include "oros/platform/input.hpp"

#include <iostream>
#include <string_view>

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
    using namespace oros::platform;

    TestState state{};

    InputState keyboard{};

    check(
        state,
        keyboard.focused(),
        "Input begins focused");

    check(
        state,
        !keyboard.key_down(Key::a),
        "Keys begin released");

    keyboard.set_key(
        Key::a,
        true);

    check(
        state,
        keyboard.key_down(Key::a),
        "A key can become held");

    check(
        state,
        keyboard.key_pressed(Key::a),
        "A new key-down transition is pressed");

    check(
        state,
        !keyboard.key_released(Key::a),
        "A held key is not released");

    keyboard.begin_frame();

    check(
        state,
        keyboard.key_down(Key::a),
        "A held key remains down next frame");

    check(
        state,
        !keyboard.key_pressed(Key::a),
        "A held key is not repeatedly pressed");

    check(
        state,
        !keyboard.key_released(Key::a),
        "A held key remains unreleased");

    keyboard.set_key(
        Key::a,
        false);

    check(
        state,
        !keyboard.key_down(Key::a),
        "A key can become released");

    check(
        state,
        !keyboard.key_pressed(Key::a),
        "A released key is not pressed");

    check(
        state,
        keyboard.key_released(Key::a),
        "A key-up transition is released");

    keyboard.begin_frame();

    check(
        state,
        !keyboard.key_released(Key::a),
        "A release transition lasts one frame");

    InputState mouse_buttons{};

    mouse_buttons.set_mouse_button(
        MouseButton::left,
        true);

    check(
        state,
        mouse_buttons.mouse_button_down(
            MouseButton::left),
        "A mouse button can become held");

    check(
        state,
        mouse_buttons.mouse_button_pressed(
            MouseButton::left),
        "A new mouse-button transition is pressed");

    mouse_buttons.begin_frame();

    check(
        state,
        mouse_buttons.mouse_button_down(
            MouseButton::left),
        "A mouse button remains held next frame");

    check(
        state,
        !mouse_buttons.mouse_button_pressed(
            MouseButton::left),
        "A held mouse button is not repeatedly pressed");

    mouse_buttons.set_mouse_button(
        MouseButton::left,
        false);

    check(
        state,
        mouse_buttons.mouse_button_released(
            MouseButton::left),
        "A mouse-button release is detected");

    mouse_buttons.begin_frame();

    check(
        state,
        !mouse_buttons.mouse_button_released(
            MouseButton::left),
        "A mouse-button release lasts one frame");

    InputState mouse_motion{};

    mouse_motion.set_mouse_position(
        MousePosition{
            10,
            20
        });

    check(
        state,
        mouse_motion.mouse_position().x == 10 &&
            mouse_motion.mouse_position().y == 20,
        "Mouse position is stored");

    check(
        state,
        mouse_motion.mouse_delta().x == 10 &&
            mouse_motion.mouse_delta().y == 20,
        "Initial mouse movement produces a delta");

    mouse_motion.set_mouse_position(
        MousePosition{
            14,
            17
        });

    check(
        state,
        mouse_motion.mouse_position().x == 14 &&
            mouse_motion.mouse_position().y == 17,
        "Mouse position updates");

    check(
        state,
        mouse_motion.mouse_delta().x == 14 &&
            mouse_motion.mouse_delta().y == 17,
        "Mouse movement accumulates during a frame");

    mouse_motion.begin_frame();

    check(
        state,
        mouse_motion.mouse_delta().x == 0 &&
            mouse_motion.mouse_delta().y == 0,
        "Mouse delta clears at the next frame");

    check(
        state,
        mouse_motion.mouse_position().x == 14 &&
            mouse_motion.mouse_position().y == 17,
        "Mouse position persists between frames");

    mouse_motion.add_mouse_wheel(1.0F);
    mouse_motion.add_mouse_wheel(-0.25F);

    check(
        state,
        mouse_motion.mouse_wheel_delta() == 0.75F,
        "Mouse-wheel movement accumulates");

    mouse_motion.begin_frame();

    check(
        state,
        mouse_motion.mouse_wheel_delta() == 0.0F,
        "Mouse-wheel movement clears next frame");

    InputState focus_state{};

    focus_state.set_key(
        Key::w,
        true);

    focus_state.set_mouse_button(
        MouseButton::right,
        true);

    focus_state.begin_frame();
    focus_state.set_focused(false);

    check(
        state,
        !focus_state.focused(),
        "Input can lose focus");

    check(
        state,
        !focus_state.key_down(Key::w),
        "Focus loss clears held keys");

    check(
        state,
        focus_state.key_released(Key::w),
        "Focus loss creates a key-release transition");

    check(
        state,
        !focus_state.mouse_button_down(
            MouseButton::right),
        "Focus loss clears held mouse buttons");

    check(
        state,
        focus_state.mouse_button_released(
            MouseButton::right),
        "Focus loss creates a mouse release transition");

    focus_state.set_focused(true);

    check(
        state,
        focus_state.focused(),
        "Input can regain focus");

    InputState bounds_state{};

    bounds_state.set_key(
        static_cast<Key>(9999),
        true);

    bounds_state.set_mouse_button(
        static_cast<MouseButton>(255),
        true);

    check(
        state,
        !bounds_state.key_down(Key::a),
        "An invalid key value is ignored");

    check(
        state,
        !bounds_state.mouse_button_down(
            MouseButton::left),
        "An invalid mouse-button value is ignored");

    std::cout
        << "\nInput test summary: "
        << state.checks - state.failures
        << '/'
        << state.checks
        << " passed.\n";

    return state.failures == 0 ? 0 : 1;
}