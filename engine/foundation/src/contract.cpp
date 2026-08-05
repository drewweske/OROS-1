#include "oros/foundation/contract.hpp"

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <string_view>

namespace
{
    [[nodiscard]] constexpr std::string_view
    to_string(const oros::foundation::ContractKind kind) noexcept
    {
        using oros::foundation::ContractKind;

        switch (kind)
        {
        case ContractKind::assertion:
            return "assertion";

        case ContractKind::precondition:
            return "precondition";

        case ContractKind::postcondition:
            return "postcondition";
        }

        return "unknown";
    }

    void write_text(const std::string_view text) noexcept
    {
        if (!text.empty())
        {
            std::fwrite(
                text.data(),
                sizeof(char),
                text.size(),
                stderr);
        }
    }
}

namespace oros::foundation
{
    [[noreturn]] void report_contract_violation(
        const ContractKind kind,
        const std::string_view expression,
        const std::string_view message,
        const std::source_location location) noexcept
    {
        std::fputs(
            "\n=== OROS CONTRACT VIOLATION ===\n",
            stderr);

        std::fputs("Kind:       ", stderr);
        write_text(to_string(kind));

        std::fputs("\nExpression: ", stderr);
        write_text(expression);

        std::fputs("\nMessage:    ", stderr);
        write_text(message);

        std::fputs("\nFile:       ", stderr);
        write_text(location.file_name());

        std::fprintf(
            stderr,
            "\nLine:       %zu",
            static_cast<std::size_t>(location.line()));

        std::fputs("\nFunction:   ", stderr);
        write_text(location.function_name());

        std::fputs(
            "\n===============================\n",
            stderr);

        std::fflush(stderr);
        std::abort();
    }
}