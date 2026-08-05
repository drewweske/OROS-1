#pragma once

#include "oros/foundation/result.hpp"
#include "oros/platform/input.hpp"

#include <cstdint>
#include <memory>
#include <string>

namespace oros::platform
{
    struct WindowExtent final
    {
        std::uint32_t width{};
        std::uint32_t height{};
    };

    struct WindowConfig final
    {
        std::string title{"OROS 1"};
        std::uint32_t client_width{1280};
        std::uint32_t client_height{720};
        bool resizable{true};
        bool start_visible{true};
    };

    class Window final
    {
    public:
        [[nodiscard]] static foundation::Result<Window>
        create(WindowConfig config);

        ~Window();

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;

        Window(Window&& other) noexcept;
        Window& operator=(Window&& other) noexcept;

        void pump_events() noexcept;

        void request_close() noexcept;

        [[nodiscard]] bool is_open() const noexcept;

        [[nodiscard]] bool should_close() const noexcept;

        [[nodiscard]] WindowExtent
        client_extent() const noexcept;

        [[nodiscard]] const InputState&
        input_state() const noexcept;

        [[nodiscard]] void*
        native_handle() const noexcept;

    private:
        struct Implementation;

        explicit Window(
            std::unique_ptr<Implementation> implementation) noexcept;

        std::unique_ptr<Implementation> implementation_;
    };
}