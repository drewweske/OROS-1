#include "oros/platform/window.hpp"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <windowsx.h>

#include <cstdint>
#include <limits>
#include <memory>
#include <new>
#include <string>
#include <string_view>
#include <utility>

namespace
{
    constexpr wchar_t window_class_name[] =
        L"OROS1.NativeWindow";

    [[nodiscard]] std::string make_win32_error_message(
        const std::string_view operation,
        const DWORD error_code)
    {
        return std::string{operation} +
            " Win32 error code: " +
            std::to_string(error_code) +
            '.';
    }

    [[nodiscard]] oros::foundation::Result<std::wstring>
    utf8_to_wide(const std::string_view text)
    {
        if (text.empty())
        {
            return oros::foundation::fail(
                oros::foundation::ErrorCode::invalid_argument,
                "Window text must not be empty.");
        }

        if (text.size() >
            static_cast<std::size_t>(
                std::numeric_limits<int>::max()))
        {
            return oros::foundation::fail(
                oros::foundation::ErrorCode::invalid_argument,
                "Window text is too large.");
        }

        const int text_size =
            static_cast<int>(text.size());

        const int required_size =
            ::MultiByteToWideChar(
                CP_UTF8,
                MB_ERR_INVALID_CHARS,
                text.data(),
                text_size,
                nullptr,
                0);

        if (required_size <= 0)
        {
            return oros::foundation::fail(
                oros::foundation::ErrorCode::invalid_argument,
                make_win32_error_message(
                    "The UTF-8 window title could not be converted.",
                    ::GetLastError()));
        }

        std::wstring converted(
            static_cast<std::size_t>(required_size),
            L'\0');

        const int converted_size =
            ::MultiByteToWideChar(
                CP_UTF8,
                MB_ERR_INVALID_CHARS,
                text.data(),
                text_size,
                converted.data(),
                required_size);

        if (converted_size != required_size)
        {
            return oros::foundation::fail(
                oros::foundation::ErrorCode::internal_failure,
                make_win32_error_message(
                    "The UTF-8 window title conversion failed.",
                    ::GetLastError()));
        }

        return converted;
    }

    [[nodiscard]] bool is_extended_key(
        const LPARAM long_parameter) noexcept
    {
        constexpr std::uintptr_t extended_key_mask =
            static_cast<std::uintptr_t>(1U) << 24U;

        return (
            static_cast<std::uintptr_t>(long_parameter) &
            extended_key_mask) != 0U;
    }

    [[nodiscard]] UINT normalize_virtual_key(
        const WPARAM word_parameter,
        const LPARAM long_parameter) noexcept
    {
        const UINT virtual_key =
            static_cast<UINT>(word_parameter);

        if (virtual_key == VK_SHIFT)
        {
            const std::uintptr_t parameter_bits =
                static_cast<std::uintptr_t>(
                    long_parameter);

            const UINT scan_code =
                static_cast<UINT>(
                    (parameter_bits >> 16U) &
                    0xFFU);

            const UINT mapped_key =
                ::MapVirtualKeyW(
                    scan_code,
                    MAPVK_VSC_TO_VK_EX);

            return mapped_key != 0U
                ? mapped_key
                : virtual_key;
        }

        if (virtual_key == VK_CONTROL)
        {
            return is_extended_key(long_parameter)
                ? VK_RCONTROL
                : VK_LCONTROL;
        }

        if (virtual_key == VK_MENU)
        {
            return is_extended_key(long_parameter)
                ? VK_RMENU
                : VK_LMENU;
        }

        return virtual_key;
    }

    [[nodiscard]] oros::platform::Key translate_key(
        const WPARAM word_parameter,
        const LPARAM long_parameter) noexcept
    {
        using oros::platform::Key;

        const UINT virtual_key =
            normalize_virtual_key(
                word_parameter,
                long_parameter);

        if (virtual_key >=
                static_cast<UINT>('A') &&
            virtual_key <=
                static_cast<UINT>('Z'))
        {
            const auto offset =
                static_cast<std::uint16_t>(
                    virtual_key -
                    static_cast<UINT>('A'));

            const auto first_key =
                static_cast<std::uint16_t>(
                    Key::a);

            return static_cast<Key>(
                static_cast<std::uint16_t>(
                    first_key + offset));
        }

        if (virtual_key >=
                static_cast<UINT>('0') &&
            virtual_key <=
                static_cast<UINT>('9'))
        {
            const auto offset =
                static_cast<std::uint16_t>(
                    virtual_key -
                    static_cast<UINT>('0'));

            const auto first_key =
                static_cast<std::uint16_t>(
                    Key::number_0);

            return static_cast<Key>(
                static_cast<std::uint16_t>(
                    first_key + offset));
        }

        switch (virtual_key)
        {
        case VK_ESCAPE:
            return Key::escape;

        case VK_SPACE:
            return Key::space;

        case VK_RETURN:
            return Key::enter;

        case VK_TAB:
            return Key::tab;

        case VK_BACK:
            return Key::backspace;

        case VK_LEFT:
            return Key::left;

        case VK_RIGHT:
            return Key::right;

        case VK_UP:
            return Key::up;

        case VK_DOWN:
            return Key::down;

        case VK_LSHIFT:
            return Key::left_shift;

        case VK_RSHIFT:
            return Key::right_shift;

        case VK_LCONTROL:
            return Key::left_control;

        case VK_RCONTROL:
            return Key::right_control;

        case VK_LMENU:
            return Key::left_alt;

        case VK_RMENU:
            return Key::right_alt;

        case VK_INSERT:
            return Key::insert;

        case VK_DELETE:
            return Key::delete_key;

        case VK_HOME:
            return Key::home;

        case VK_END:
            return Key::end;

        case VK_PRIOR:
            return Key::page_up;

        case VK_NEXT:
            return Key::page_down;

        case VK_F1:
            return Key::f1;

        case VK_F2:
            return Key::f2;

        case VK_F3:
            return Key::f3;

        case VK_F4:
            return Key::f4;

        case VK_F5:
            return Key::f5;

        case VK_F6:
            return Key::f6;

        case VK_F7:
            return Key::f7;

        case VK_F8:
            return Key::f8;

        case VK_F9:
            return Key::f9;

        case VK_F10:
            return Key::f10;

        case VK_F11:
            return Key::f11;

        case VK_F12:
            return Key::f12;

        default:
            return Key::unknown;
        }
    }

    [[nodiscard]] oros::platform::MousePosition
    mouse_position_from_message(
        const LPARAM long_parameter) noexcept
    {
        return oros::platform::MousePosition{
            static_cast<std::int32_t>(
                GET_X_LPARAM(long_parameter)),
            static_cast<std::int32_t>(
                GET_Y_LPARAM(long_parameter))
        };
    }

    [[nodiscard]] bool any_mouse_button_down(
        const oros::platform::InputState& input) noexcept
    {
        using oros::platform::MouseButton;

        return
            input.mouse_button_down(
                MouseButton::left) ||
            input.mouse_button_down(
                MouseButton::right) ||
            input.mouse_button_down(
                MouseButton::middle) ||
            input.mouse_button_down(
                MouseButton::extra_1) ||
            input.mouse_button_down(
                MouseButton::extra_2);
    }
}

namespace oros::platform
{
    struct Window::Implementation final
    {
        HINSTANCE instance{};
        HWND handle{};

        InputState input{};

        bool open{};
        bool close_requested{};
        bool mouse_position_initialized{};

        ~Implementation()
        {
            if (handle != nullptr &&
                ::IsWindow(handle) != FALSE)
            {
                ::DestroyWindow(handle);
            }
        }

        [[nodiscard]] static foundation::Status
        register_window_class(
            const HINSTANCE application_instance)
        {
            WNDCLASSEXW window_class{};

            window_class.cbSize =
                sizeof(WNDCLASSEXW);

            window_class.style =
                CS_HREDRAW |
                CS_VREDRAW;

            window_class.lpfnWndProc =
                &Implementation::window_procedure;

            window_class.hInstance =
                application_instance;

            window_class.hCursor =
                ::LoadCursorW(
                    nullptr,
                    IDC_ARROW);

            window_class.hbrBackground =
                ::GetSysColorBrush(
                    COLOR_WINDOW);

            window_class.lpszClassName =
                window_class_name;

            const ATOM registered_class =
                ::RegisterClassExW(
                    &window_class);

            if (registered_class != 0)
            {
                return {};
            }

            const DWORD error_code =
                ::GetLastError();

            if (error_code ==
                ERROR_CLASS_ALREADY_EXISTS)
            {
                return {};
            }

            return foundation::fail(
                foundation::ErrorCode::internal_failure,
                make_win32_error_message(
                    "The OROS window class could not be registered.",
                    error_code));
        }

        void set_key_from_message(
            const WPARAM word_parameter,
            const LPARAM long_parameter,
            const bool is_down) noexcept
        {
            const Key key =
                translate_key(
                    word_parameter,
                    long_parameter);

            if (key != Key::unknown)
            {
                input.set_key(
                    key,
                    is_down);
            }
        }

        void set_mouse_button(
            const HWND window_handle,
            const MouseButton button,
            const bool is_down) noexcept
        {
            input.set_mouse_button(
                button,
                is_down);

            if (is_down)
            {
                ::SetCapture(window_handle);
                return;
            }

            if (!any_mouse_button_down(input) &&
                ::GetCapture() == window_handle)
            {
                ::ReleaseCapture();
            }
        }

        static LRESULT CALLBACK window_procedure(
            const HWND window_handle,
            const UINT message,
            const WPARAM word_parameter,
            const LPARAM long_parameter) noexcept
        {
            Implementation* implementation =
                reinterpret_cast<Implementation*>(
                    ::GetWindowLongPtrW(
                        window_handle,
                        GWLP_USERDATA));

            if (message == WM_NCCREATE)
            {
                const auto* creation_data =
                    reinterpret_cast<
                        const CREATESTRUCTW*>(
                            long_parameter);

                implementation =
                    static_cast<Implementation*>(
                        creation_data->lpCreateParams);

                if (implementation != nullptr)
                {
                    implementation->handle =
                        window_handle;

                    implementation->open =
                        true;

                    ::SetWindowLongPtrW(
                        window_handle,
                        GWLP_USERDATA,
                        reinterpret_cast<LONG_PTR>(
                            implementation));
                }
            }

            if (implementation != nullptr)
            {
                switch (message)
                {
                case WM_SETFOCUS:
                    implementation->input.set_focused(
                        true);

                    implementation
                        ->mouse_position_initialized =
                            false;

                    return 0;

                case WM_KILLFOCUS:
                    implementation->input.set_focused(
                        false);

                    implementation
                        ->mouse_position_initialized =
                            false;

                    if (::GetCapture() ==
                        window_handle)
                    {
                        ::ReleaseCapture();
                    }

                    return 0;

                case WM_KEYDOWN:
                case WM_SYSKEYDOWN:
                    implementation
                        ->set_key_from_message(
                            word_parameter,
                            long_parameter,
                            true);

                    return 0;

                case WM_KEYUP:
                case WM_SYSKEYUP:
                    implementation
                        ->set_key_from_message(
                            word_parameter,
                            long_parameter,
                            false);

                    return 0;

                case WM_MOUSEMOVE:
                {
                    const MousePosition position =
                        mouse_position_from_message(
                            long_parameter);

                    if (!implementation
                             ->mouse_position_initialized)
                    {
                        implementation->input
                            .synchronize_mouse_position(
                                position);

                        implementation
                            ->mouse_position_initialized =
                                true;
                    }
                    else
                    {
                        implementation->input
                            .set_mouse_position(
                                position);
                    }

                    return 0;
                }

                case WM_LBUTTONDOWN:
                    implementation->set_mouse_button(
                        window_handle,
                        MouseButton::left,
                        true);

                    return 0;

                case WM_LBUTTONUP:
                    implementation->set_mouse_button(
                        window_handle,
                        MouseButton::left,
                        false);

                    return 0;

                case WM_RBUTTONDOWN:
                    implementation->set_mouse_button(
                        window_handle,
                        MouseButton::right,
                        true);

                    return 0;

                case WM_RBUTTONUP:
                    implementation->set_mouse_button(
                        window_handle,
                        MouseButton::right,
                        false);

                    return 0;

                case WM_MBUTTONDOWN:
                    implementation->set_mouse_button(
                        window_handle,
                        MouseButton::middle,
                        true);

                    return 0;

                case WM_MBUTTONUP:
                    implementation->set_mouse_button(
                        window_handle,
                        MouseButton::middle,
                        false);

                    return 0;

                case WM_XBUTTONDOWN:
                {
                    const WORD native_button =
                        GET_XBUTTON_WPARAM(
                            word_parameter);

                    const MouseButton button =
                        native_button == XBUTTON1
                            ? MouseButton::extra_1
                            : MouseButton::extra_2;

                    implementation->set_mouse_button(
                        window_handle,
                        button,
                        true);

                    return TRUE;
                }

                case WM_XBUTTONUP:
                {
                    const WORD native_button =
                        GET_XBUTTON_WPARAM(
                            word_parameter);

                    const MouseButton button =
                        native_button == XBUTTON1
                            ? MouseButton::extra_1
                            : MouseButton::extra_2;

                    implementation->set_mouse_button(
                        window_handle,
                        button,
                        false);

                    return TRUE;
                }

                case WM_MOUSEWHEEL:
                {
                    const short wheel_delta =
                        GET_WHEEL_DELTA_WPARAM(
                            word_parameter);

                    implementation->input
                        .add_mouse_wheel(
                            static_cast<float>(
                                wheel_delta) /
                            static_cast<float>(
                                WHEEL_DELTA));

                    return 0;
                }

                case WM_CLOSE:
                    implementation->close_requested =
                        true;

                    ::DestroyWindow(
                        window_handle);

                    return 0;

                case WM_DESTROY:
                    implementation->open =
                        false;

                    return 0;

                case WM_NCDESTROY:
                    implementation->open =
                        false;

                    implementation->handle =
                        nullptr;

                    ::SetWindowLongPtrW(
                        window_handle,
                        GWLP_USERDATA,
                        0);

                    break;

                default:
                    break;
                }
            }

            return ::DefWindowProcW(
                window_handle,
                message,
                word_parameter,
                long_parameter);
        }
    };

    foundation::Result<Window>
    Window::create(WindowConfig config)
    {
        if (config.title.empty())
        {
            return foundation::fail(
                foundation::ErrorCode::invalid_argument,
                "The window title must not be empty.");
        }

        if (config.client_width == 0 ||
            config.client_height == 0)
        {
            return foundation::fail(
                foundation::ErrorCode::invalid_argument,
                "Window dimensions must be greater than zero.");
        }

        constexpr auto maximum_dimension =
            static_cast<std::uint32_t>(
                std::numeric_limits<int>::max());

        if (config.client_width >
                maximum_dimension ||
            config.client_height >
                maximum_dimension)
        {
            return foundation::fail(
                foundation::ErrorCode::invalid_argument,
                "Window dimensions exceed Win32 limits.");
        }

        const foundation::Result<std::wstring>
            converted_title =
                utf8_to_wide(config.title);

        if (!converted_title.has_value())
        {
            return foundation::fail(
                converted_title.error().code,
                converted_title.error().message);
        }

        std::unique_ptr<Implementation>
            implementation{};

        try
        {
            implementation =
                std::make_unique<
                    Implementation>();
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::out_of_memory,
                "The native window state could not be allocated.");
        }

        implementation->instance =
            ::GetModuleHandleW(
                nullptr);

        if (implementation->instance ==
            nullptr)
        {
            return foundation::fail(
                foundation::ErrorCode::internal_failure,
                make_win32_error_message(
                    "The application module handle could not be acquired.",
                    ::GetLastError()));
        }

        const foundation::Status class_status =
            Implementation::register_window_class(
                implementation->instance);

        if (!class_status.has_value())
        {
            return foundation::fail(
                class_status.error().code,
                class_status.error().message);
        }

        DWORD window_style =
            WS_OVERLAPPED |
            WS_CAPTION |
            WS_SYSMENU |
            WS_MINIMIZEBOX;

        if (config.resizable)
        {
            window_style |=
                WS_THICKFRAME |
                WS_MAXIMIZEBOX;
        }

        constexpr DWORD extended_style =
            WS_EX_APPWINDOW;

        RECT window_rectangle{
            0,
            0,
            static_cast<LONG>(
                config.client_width),
            static_cast<LONG>(
                config.client_height)
        };

        if (::AdjustWindowRectEx(
                &window_rectangle,
                window_style,
                FALSE,
                extended_style) == FALSE)
        {
            return foundation::fail(
                foundation::ErrorCode::internal_failure,
                make_win32_error_message(
                    "The native window size could not be calculated.",
                    ::GetLastError()));
        }

        const int window_width =
            static_cast<int>(
                window_rectangle.right -
                window_rectangle.left);

        const int window_height =
            static_cast<int>(
                window_rectangle.bottom -
                window_rectangle.top);

        const HWND window_handle =
            ::CreateWindowExW(
                extended_style,
                window_class_name,
                converted_title->c_str(),
                window_style,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                window_width,
                window_height,
                nullptr,
                nullptr,
                implementation->instance,
                implementation.get());

        if (window_handle == nullptr)
        {
            return foundation::fail(
                foundation::ErrorCode::internal_failure,
                make_win32_error_message(
                    "The native OROS window could not be created.",
                    ::GetLastError()));
        }

        implementation->handle =
            window_handle;

        implementation->open =
            true;

        if (config.start_visible)
        {
            ::ShowWindow(
                implementation->handle,
                SW_SHOW);

            ::UpdateWindow(
                implementation->handle);
        }

        return Window{
            std::move(implementation)
        };
    }

    Window::Window(
        std::unique_ptr<Implementation>
            implementation) noexcept
        : implementation_{
              std::move(implementation)}
    {
    }

    Window::~Window() = default;

    Window::Window(
        Window&& other) noexcept =
            default;

    Window& Window::operator=(
        Window&& other) noexcept =
            default;

    void Window::pump_events() noexcept
    {
        if (implementation_ == nullptr)
        {
            return;
        }

        implementation_->input.begin_frame();

        MSG message{};

        while (::PeekMessageW(
                   &message,
                   nullptr,
                   0,
                   0,
                   PM_REMOVE) != FALSE)
        {
            if (message.message ==
                WM_QUIT)
            {
                implementation_
                    ->close_requested =
                        true;

                implementation_->open =
                    false;

                break;
            }

            ::TranslateMessage(
                &message);

            ::DispatchMessageW(
                &message);
        }
    }

    void Window::request_close() noexcept
    {
        if (implementation_ == nullptr)
        {
            return;
        }

        implementation_->close_requested =
            true;

        if (implementation_->handle !=
            nullptr)
        {
            ::PostMessageW(
                implementation_->handle,
                WM_CLOSE,
                0,
                0);
        }
    }

    bool Window::is_open() const noexcept
    {
        return implementation_ != nullptr &&
            implementation_->open &&
            implementation_->handle != nullptr;
    }

    bool Window::should_close() const noexcept
    {
        return implementation_ == nullptr ||
            implementation_->close_requested ||
            !implementation_->open;
    }

    WindowExtent
    Window::client_extent() const noexcept
    {
        if (implementation_ == nullptr ||
            implementation_->handle ==
                nullptr)
        {
            return {};
        }

        RECT client_rectangle{};

        if (::GetClientRect(
                implementation_->handle,
                &client_rectangle) == FALSE)
        {
            return {};
        }

        const LONG width =
            client_rectangle.right -
            client_rectangle.left;

        const LONG height =
            client_rectangle.bottom -
            client_rectangle.top;

        return WindowExtent{
            width > 0
                ? static_cast<std::uint32_t>(
                      width)
                : 0U,
            height > 0
                ? static_cast<std::uint32_t>(
                      height)
                : 0U
        };
    }

    const InputState&
    Window::input_state() const noexcept
    {
        static const InputState empty_input{};

        if (implementation_ == nullptr)
        {
            return empty_input;
        }

        return implementation_->input;
    }

    void* Window::native_handle() const noexcept
    {
        if (implementation_ == nullptr)
        {
            return nullptr;
        }

        return reinterpret_cast<void*>(
            implementation_->handle);
    }
}