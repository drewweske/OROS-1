#include <iostream>
#include <string_view>

namespace oros {

    inline constexpr std::string_view engine_name{ "OROS 1" };

    struct Version final {
        int major;
        int minor;
        int patch;
    };

    inline constexpr Version engine_version{
        OROS_VERSION_MAJOR,
        OROS_VERSION_MINOR,
        OROS_VERSION_PATCH,
    };

} // namespace oros

int main() {
    std::cout << oros::engine_name << " Engine Bootstrap\n";
    std::cout << "Version "
        << oros::engine_version.major << '.'
        << oros::engine_version.minor << '.'
        << oros::engine_version.patch << '\n';
    std::cout << "OROS-000 Genesis: PASS\n";

    return 0;
}