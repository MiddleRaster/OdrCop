#pragma once

namespace Odr
{
    struct AnonInfo
    {
        const bool excludeFromComparison;
        explicit AnonInfo(const bool b) noexcept : excludeFromComparison(b) {}
    };
}
