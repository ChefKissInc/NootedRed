// AMD TTL SoftwareIP Block Version
//
// Copyright © 2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <IOKit/IOTypes.h>

struct SWIPIPVersion
{
    UInt32 major;
    UInt32 minor;
    UInt32 patch;

    constexpr SWIPIPVersion(const UInt32 major, const UInt32 minor, const UInt32 patch) :
        major(major),
        minor(minor),
        patch(patch)
    { }

    constexpr auto toHW() const { return (this->minor << 8) | (this->major << 16) | this->patch; }

    constexpr auto operator==(const SWIPIPVersion& other) const { return this->toHW() == other.toHW(); }
};
