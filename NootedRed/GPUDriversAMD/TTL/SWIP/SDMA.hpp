// AMD TTL SWIP System Direct Memory Access
//
// Copyright © 2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <IOKit/IOTypes.h>

struct SDMAFWConstant {
    const char *version;
    UInt32 romSize;
    const void *rom;
    UInt32 field18;
    UInt32 payloadOffDWords;
    UInt32 checksum;
};

#define SDMA_FW_CONSTANT(_V, _R, _F18, _POD, _C) \
    static const SDMAFWConstant _R = {           \
        .version = _V,                           \
        .romSize = sizeof(_##_R),                \
        .rom = _##_R,                            \
        .field18 = _F18,                         \
        .payloadOffDWords = _POD,                \
        .checksum = _C,                          \
    }
