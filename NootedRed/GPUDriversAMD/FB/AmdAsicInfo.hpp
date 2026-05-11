// AMD Framebuffer ASIC Info
//
// Copyright © 2026 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <IOKit/IOTypes.h>

struct AmdAsicBrandingTableEntry
{
    const UInt16      deviceId;
    const UInt16      pciRevNo;
    const char* const familyName;
    const char* const deviceName;

    constexpr AmdAsicBrandingTableEntry(const UInt16 deviceId, const UInt16 pciRevNo, const char* const familyName,
                                        const char* const deviceName) :
        deviceId(deviceId),
        pciRevNo(pciRevNo),
        familyName(familyName),
        deviceName(deviceName)
    { }

    constexpr AmdAsicBrandingTableEntry(const char* const familyName, const char* const deviceName) :
        AmdAsicBrandingTableEntry(0, 0, familyName, deviceName)
    { }

    constexpr AmdAsicBrandingTableEntry() :
        AmdAsicBrandingTableEntry("Radeon RX", "Graphics")
    { }
};
static_assert(sizeof(AmdAsicBrandingTableEntry) == 0x18);
