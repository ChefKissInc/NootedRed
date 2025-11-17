// AMD Device Capabilities
//
// Copyright © 2024-2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <GPUDriversAMD/CAIL/GoldenSettings.hpp>

constexpr UInt64 DEVICE_CAP_ENTRY_REV_DONT_CARE = 0xDEADCAFE;

struct AMDDeviceCapabilities {
    UInt64 familyId;
    UInt64 extRevision;
    UInt64 deviceId;
    UInt64 revision;
    UInt64 enumRevision;
    const void *swipInfo;
    const void *swipInfoMinimal;
    const UInt32 *devAttrFlags;
    CAILASICGoldenSettings *asicGoldenSettings;
    void *doorbellRange;
};
static_assert(sizeof(AMDDeviceCapabilities) == 0x50);
