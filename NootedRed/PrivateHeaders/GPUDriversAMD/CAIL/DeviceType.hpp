// Copyright © 2024 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <IOKit/IOTypes.h>

enum AMDDeviceType {
    kAMDDeviceTypeVega10,
    kAMDDeviceTypeVega12,
    kAMDDeviceTypeVega20,
    kAMDDeviceTypeNavi10,
    kAMDDeviceTypeNavi12,
    kAMDDeviceTypeNavi14,
    kAMDDeviceTypeNavi21,
    kAMDDeviceTypeNavi22,
    kAMDDeviceTypeNavi23,
    kAMDDeviceTypeUnknown,
};
static_assert(sizeof(AMDDeviceType) == 0x4);

struct AMDDeviceTypeEntry {
    UInt32 deviceId;
    AMDDeviceType deviceType;
};
static_assert(sizeof(AMDDeviceTypeEntry) == 0x8);
