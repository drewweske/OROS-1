#include "oros/foundation/contract.hpp"

int main()
{
    volatile int item_count{0};

    OROS_EXPECTS(
        item_count > 0,
        "The item count must be greater than zero.");

    return 0;
}