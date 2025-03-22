// Copyright © 2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <IOKit/IOTypes.h>

struct SDMAFWConstant {
    const char *version;
    UInt32 romSize;
    const UInt8 *rom;
    UInt32 field18;
    UInt32 payloadOffDWords;
    UInt32 checksum;
};
