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
    const UInt8 *rom;
};

struct DMCUFirmwareEntry {
    UInt64 loadAddress;
    UInt32 romSize;
    const UInt8 *rom;
    void *handle;
};

struct DMCUFirmwareInfo {
    UInt32 count;
    DMCUFirmwareEntry entry[0x4];
    UInt64 field88;
};
