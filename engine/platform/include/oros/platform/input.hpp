#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace oros::platform
{
    enum class Key : std::uint16_t
    {
        unknown = 0,

        a,
        b,
        c,
        d,
        e,
        f,
        g,
        h,
        i,
        j,
        k,
        l,
        m,
        n,
        o,
        p,
        q,
        r,
        s,
        t,
        u,
        v,
        w,
        x,
        y,
        z,

        number_0,
        number_1,
        number_2,
        number_3,
        number_4,
        number_5,
        number_6,
        number_7,
        number_8,
        number_9,

        escape,
        space,
        enter,
        tab,
        backspace,

        left,
        right,
        up,
        down,

        left_shift,
        right_shift,
        left_control,
        right_control,
        left_alt,
        right_alt,

        insert,
        delete_key,
        home,
        end,
        page_up,
        page_down,

        f1,
        f2,
        f3,
        f4,
        f5,
        f6,
        f7,
        f8,
        f9,
        f10,
        f11,
        f12,

        count
    };

    enum class MouseButton : std::uint8_t
    {
        left = 0,
        right,
        middle,
        extra_1,
        extra_2,

        count
    };

    struct MousePosition final
    {
        std::int32_t x{};
        std::int32_t y{};
    };

    struct MouseDelta final
    {
        std::int32_t x{};
        std::int32_t y{};
    };

    class InputState final
    {
    public:
        void begin_frame() noexcept
        {
            previous_keys_ = current_keys_;
            previous_mouse_buttons_ =
                current_mouse_buttons_;

            mouse_delta_ = {};
            mouse_wheel_delta_ = 0.0F;
        }

        void set_key(
            const Key key,
            const bool is_down) noexcept
        {
            const std::size_t index =
                key_index(key);

            if (index >= current_keys_.size())
            {
                return;
            }

            current_keys_[index] = is_down;
        }

        void set_mouse_button(
            const MouseButton button,
            const bool is_down) noexcept
        {
            const std::size_t index =
                mouse_button_index(button);

            if (index >= current_mouse_buttons_.size())
            {
                return;
            }

            current_mouse_buttons_[index] =
                is_down;
        }

        void synchronize_mouse_position(
            const MousePosition position) noexcept
        {
            mouse_position_ = position;
            mouse_delta_ = {};
        }

        void set_mouse_position(
            const MousePosition position) noexcept
        {
            mouse_delta_.x +=
                position.x - mouse_position_.x;

            mouse_delta_.y +=
                position.y - mouse_position_.y;

            mouse_position_ = position;
        }

        void add_mouse_wheel(
            const float amount) noexcept
        {
            mouse_wheel_delta_ += amount;
        }

        void set_focused(
            const bool focused) noexcept
        {
            focused_ = focused;

            if (!focused_)
            {
                current_keys_.fill(false);
                current_mouse_buttons_.fill(false);
            }
        }

        [[nodiscard]] bool key_down(
            const Key key) const noexcept
        {
            return read_key(
                current_keys_,
                key);
        }

        [[nodiscard]] bool key_pressed(
            const Key key) const noexcept
        {
            return key_down(key) &&
                !read_key(previous_keys_, key);
        }

        [[nodiscard]] bool key_released(
            const Key key) const noexcept
        {
            return !key_down(key) &&
                read_key(previous_keys_, key);
        }

        [[nodiscard]] bool mouse_button_down(
            const MouseButton button) const noexcept
        {
            return read_mouse_button(
                current_mouse_buttons_,
                button);
        }

        [[nodiscard]] bool mouse_button_pressed(
            const MouseButton button) const noexcept
        {
            return mouse_button_down(button) &&
                !read_mouse_button(
                    previous_mouse_buttons_,
                    button);
        }

        [[nodiscard]] bool mouse_button_released(
            const MouseButton button) const noexcept
        {
            return !mouse_button_down(button) &&
                read_mouse_button(
                    previous_mouse_buttons_,
                    button);
        }

        [[nodiscard]] MousePosition
        mouse_position() const noexcept
        {
            return mouse_position_;
        }

        [[nodiscard]] MouseDelta
        mouse_delta() const noexcept
        {
            return mouse_delta_;
        }

        [[nodiscard]] float
        mouse_wheel_delta() const noexcept
        {
            return mouse_wheel_delta_;
        }

        [[nodiscard]] bool
        focused() const noexcept
        {
            return focused_;
        }

    private:
        static constexpr std::size_t key_count =
            static_cast<std::size_t>(Key::count);

        static constexpr std::size_t
            mouse_button_count =
                static_cast<std::size_t>(
                    MouseButton::count);

        using KeyStates =
            std::array<bool, key_count>;

        using MouseButtonStates =
            std::array<bool, mouse_button_count>;

        [[nodiscard]] static constexpr std::size_t
        key_index(const Key key) noexcept
        {
            return static_cast<std::size_t>(key);
        }

        [[nodiscard]] static constexpr std::size_t
        mouse_button_index(
            const MouseButton button) noexcept
        {
            return static_cast<std::size_t>(button);
        }

        [[nodiscard]] static bool read_key(
            const KeyStates& states,
            const Key key) noexcept
        {
            const std::size_t index =
                key_index(key);

            return index < states.size() &&
                states[index];
        }

        [[nodiscard]] static bool read_mouse_button(
            const MouseButtonStates& states,
            const MouseButton button) noexcept
        {
            const std::size_t index =
                mouse_button_index(button);

            return index < states.size() &&
                states[index];
        }

        KeyStates current_keys_{};
        KeyStates previous_keys_{};

        MouseButtonStates current_mouse_buttons_{};
        MouseButtonStates previous_mouse_buttons_{};

        MousePosition mouse_position_{};
        MouseDelta mouse_delta_{};

        float mouse_wheel_delta_{};
        bool focused_{true};
    };
}