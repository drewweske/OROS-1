#include "oros/foundation/clock.hpp"
#include "oros/foundation/contract.hpp"
#include "oros/foundation/error.hpp"
#include "oros/foundation/log.hpp"
#include "oros/foundation/result.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>

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

    TestState state{};

    check(
        state,
        to_string(ErrorCode::none) == "none",
        "ErrorCode::none converts to text");

    check(
        state,
        to_string(ErrorCode::not_found) == "not_found",
        "ErrorCode::not_found converts to text");

    const Error error{
        ErrorCode::invalid_argument,
        "The argument was invalid."
    };

    check(
        state,
        error.has_error(),
        "Error reports that it contains an error");

    check(
        state,
        static_cast<bool>(error),
        "Error converts to true when an error exists");

    const Result<int> successful_result{42};

    check(
        state,
        successful_result.has_value(),
        "Result can contain a successful value");

    check(
        state,
        successful_result.value() == 42,
        "Result preserves its successful value");

    const Result<int> failed_result = fail(
        ErrorCode::not_found,
        "The requested value was not found.");

    check(
        state,
        !failed_result.has_value(),
        "Result can contain an error");

    check(
        state,
        failed_result.error().code ==
            ErrorCode::not_found,
        "Result preserves its error code");

    check(
        state,
        failed_result.error().message ==
            "The requested value was not found.",
        "Result preserves its error message");

    const TimePoint start{};
    const TimePoint end =
        start + std::chrono::milliseconds{5};

    check(
        state,
        elapsed_time(start, end) ==
            std::chrono::milliseconds{5},
        "Monotonic elapsed time is calculated correctly");

    Stopwatch stopwatch{};

    check(
        state,
        stopwatch.elapsed().count() >= 0,
        "Stopwatch reports a non-negative duration");

    check(
        state,
        to_string(LogLevel::warning) == "warning",
        "LogLevel converts to text");

    LogConfig invalid_config{};
    invalid_config.write_to_console = false;
    invalid_config.write_to_file = false;

    const Status invalid_logging =
        initialize_logging(invalid_config);

    check(
        state,
        !invalid_logging.has_value(),
        "Logger rejects a configuration with no outputs");

    check(
        state,
        invalid_logging.error().code ==
            ErrorCode::invalid_argument,
        "Logger reports the correct configuration error");

    check(
        state,
        !is_logging_initialized(),
        "Logger remains uninitialized after rejection");

    const std::filesystem::path log_path{
        "logs/tests/oros-foundation-tests.log"
    };

    std::error_code remove_error{};
    std::filesystem::remove(
        log_path,
        remove_error);

    LogConfig valid_config{};
    valid_config.write_to_console = false;
    valid_config.write_to_file = true;
    valid_config.file_path = log_path.string();

    const Status logging_status =
        initialize_logging(valid_config);

    check(
        state,
        logging_status.has_value(),
        "Logger initializes with a valid file output");

    check(
        state,
        is_logging_initialized(),
        "Logger reports its initialized state");

    if (logging_status.has_value())
    {
        write_log(
            LogLevel::info,
            "foundation-test",
            "Logger wrote a test message.");

        shutdown_logging();
    }

    check(
        state,
        !is_logging_initialized(),
        "Logger shuts down cleanly");

    std::ifstream log_file{log_path};

    check(
        state,
        log_file.is_open(),
        "Logger creates the requested log file");

    std::ostringstream log_contents_stream{};
    log_contents_stream << log_file.rdbuf();

    const std::string log_contents =
        log_contents_stream.str();

    check(
        state,
        log_contents.find(
            "[info] [foundation-test] "
            "Logger wrote a test message.") !=
            std::string::npos,
        "Logger writes the expected message");

    const ContractKind contract_kind =
        ContractKind::assertion;

    check(
        state,
        contract_kind == ContractKind::assertion,
        "Contract declarations are available");

    std::cout
        << "\nFoundation test summary: "
        << state.checks - state.failures
        << '/'
        << state.checks
        << " passed.\n";

    return state.failures == 0 ? 0 : 1;
}