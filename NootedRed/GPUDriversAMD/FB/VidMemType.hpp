// AMDRadeonX6000Framebuffer Video Memory Type
//
// Copyright © 2024-2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <IOKit/IOTypes.h>

enum struct VideoMemoryType : UInt32
{
    Unknown = 0,
    DDR2,
    GDDR5,
    DDR3,
    DDR4,
    HBM,
    GDDR6,
};
static_assert(sizeof(VideoMemoryType) == 0x4);
