#include "oros/foundation/log.hpp"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>
#include <system_error>
#include <utility>

namespace
{
    struct LoggerState final
    {
        std::mutex mutex{};
        std::atomic_bool initialized{false};
        oros::foundation::LogConfig config{};
        std::ofstream file{};
    };

    [[nodiscard]] LoggerState& logger_state() noexcept
    {
        static LoggerState state{};
        return state;
    }

    [[nodiscard]] bool get_local_time(
        const std::time_t time,
        std::tm& result) noexcept
    {
#if defined(_WIN32)
        return ::localtime_s(&result, &time) == 0;
#else
        return ::localtime_r(&time, &result) != nullptr;
#endif
    }

    [[nodiscard]] std::string make_timestamp()
    {
        using Clock = std::chrono::system_clock;

        const auto now = Clock::now();
        const std::time_t current_time =
            Clock::to_time_t(now);

        std::tm local_time{};

        if (!get_local_time(current_time, local_time))
        {
            return "0000-00-00 00:00:00.000";
        }

        const auto milliseconds =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()) %
            1000;

        std::ostringstream stream{};

        stream
            << std::put_time(
                   &local_time,
                   "%Y-%m-%d %H:%M:%S")
            << '.'
            << std::setfill('0')
            << std::setw(3)
            << milliseconds.count();

        return stream.str();
    }
}

namespace oros::foundation
{
    Status initialize_logging(LogConfig config)
    {
        LoggerState& state = logger_state();
        const std::scoped_lock lock{state.mutex};

        if (state.initialized.load())
        {
            return fail(
                ErrorCode::invalid_state,
                "OROS logging is already initialized.");
        }

        if (!config.write_to_console &&
            !config.write_to_file)
        {
            return fail(
                ErrorCode::invalid_argument,
                "Logging must have at least one output.");
        }

        if (config.write_to_file)
        {
            if (config.file_path.empty())
            {
                return fail(
                    ErrorCode::invalid_argument,
                    "The log file path must not be empty.");
            }

            const std::filesystem::path log_path{
                config.file_path
            };

            const std::filesystem::path parent_path =
                log_path.parent_path();

            if (!parent_path.empty())
            {
                std::error_code directory_error{};

                std::filesystem::create_directories(
                    parent_path,
                    directory_error);

                if (directory_error)
                {
                    return fail(
                        ErrorCode::input_output_failure,
                        "The log directory could not be created.");
                }
            }

            state.file.open(
                log_path,
                std::ios::out | std::ios::app);

            if (!state.file.is_open())
            {
                return fail(
                    ErrorCode::input_output_failure,
                    "The OROS log file could not be opened.");
            }
        }

        state.config = std::move(config);
        state.initialized.store(true);

        return {};
    }

    void shutdown_logging() noexcept
    {
        LoggerState& state = logger_state();

        try
        {
            const std::scoped_lock lock{state.mutex};

            if (state.file.is_open())
            {
                state.file.flush();
                state.file.close();
            }

            state.initialized.store(false);
        }
        catch (...)
        {
            state.initialized.store(false);
        }
    }

    bool is_logging_initialized() noexcept
    {
        return logger_state().initialized.load();
    }

    void write_log(
        const LogLevel level,
        const std::string_view category,
        const std::string_view message,
        const std::source_location location) noexcept
    {
        LoggerState& state = logger_state();

        if (!state.initialized.load())
        {
            return;
        }

        try
        {
            const std::scoped_lock lock{state.mutex};

            if (!state.initialized.load())
            {
                return;
            }

            std::ostringstream line{};

            line
                << make_timestamp()
                << " ["
                << to_string(level)
                << "] ["
                << category
                << "] "
                << message
                << " ("
                << location.file_name()
                << ':'
                << location.line()
                << ')';

            const std::string output = line.str();

            if (state.config.write_to_console)
            {
                std::FILE* console =
                    level >= LogLevel::error
                        ? stderr
                        : stdout;

                std::fwrite(
                    output.data(),
                    sizeof(char),
                    output.size(),
                    console);

                std::fputc('\n', console);
                std::fflush(console);
            }

            if (state.config.write_to_file &&
                state.file.is_open())
            {
                state.file << output << '\n';
                state.file.flush();
            }
        }
        catch (...)
        {
            std::fputs(
                "[critical] OROS logging failed.\n",
                stderr);

            std::fflush(stderr);
        }
    }
}