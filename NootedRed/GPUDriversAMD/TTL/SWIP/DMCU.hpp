// AMD TTL SWIP Display Micro-Controller Unit
//
// Copyright © 2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <IOKit/IOTypes.h>

enum DMCUFirmwareType {
    kDMCUFirmwareTypeERAM = 0,
    kDMCUFirmwareTypeISR,
    kDMCUFirmwareTypeDMCUB,
    kDMCUFirmwareTypeCount,    // ????
};

struct DMCUFirmwareConstant {
    UInt64 loadAddress;
    UInt32 romSize;
    const void *rom;
};

#define DMCU_FW_CONSTANT(_LA, _R) \
    static const DMCUFirmwareConstant _R { .loadAddress = _LA, .romSize = sizeof(_##_R), .rom = _##_R }

struct DMCUFirmwareEntry {
    UInt64 loadAddress;
    UInt32 romSize;
    const void *rom;
    void *handle;
};

struct DMCUFirmwareInfo {
    UInt32 count;
    DMCUFirmwareEntry entry[0x4];
    UInt64 field88;
};
