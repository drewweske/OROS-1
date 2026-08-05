#pragma once

#include <source_location>
#include <string_view>

namespace oros::foundation
{
    enum class ContractKind
    {
        assertion,
        precondition,
        postcondition
    };

    [[noreturn]] void report_contract_violation(
        ContractKind kind,
        std::string_view expression,
        std::string_view message,
        std::source_location location =
            std::source_location::current()) noexcept;
}

#define OROS_ASSERT(expression, message)                              \
    do                                                                \
    {                                                                 \
        if (!(expression))                                            \
        {                                                             \
            ::oros::foundation::report_contract_violation(            \
                ::oros::foundation::ContractKind::assertion,           \
                #expression,                                          \
                message,                                              \
                std::source_location::current());                      \
        }                                                             \
    } while (false)

#define OROS_EXPECTS(expression, message)                             \
    do                                                                \
    {                                                                 \
        if (!(expression))                                            \
        {                                                             \
            ::oros::foundation::report_contract_violation(            \
                ::oros::foundation::ContractKind::precondition,       \
                #expression,                                          \
                message,                                              \
                std::source_location::current());                      \
        }                                                             \
    } while (false)

#define OROS_ENSURES(expression, message)                             \
    do                                                                \
    {                                                                 \
        if (!(expression))                                            \
        {                                                             \
            ::oros::foundation::report_contract_violation(            \
                ::oros::foundation::ContractKind::postcondition,      \
                #expression,                                          \
                message,                                              \
                std::source_location::current());                      \
        }                                                             \
    } while (false)