// Copyright © 2024 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <IOKit/IOTypes.h>

//-------- ASIC DDI Capabilities --------//

struct CAILAsicCapsEntry {
    UInt32 familyId, deviceId;
    UInt32 revision, extRevision;
    UInt32 pciRevision;
    const UInt32 *ddiCaps;
    const UInt32 *skeleton;
};
static_assert(sizeof(CAILAsicCapsEntry) == 0x28);

struct CAILAsicCapsInitEntry {
    UInt64 familyId;
    UInt64 deviceId;
    UInt64 revision;
    UInt64 extRevision;
    UInt64 pciRevision;
    const UInt32 *ddiCaps;
    const void *goldenSettings;
};
static_assert(sizeof(CAILAsicCapsInitEntry) == 0x38);

constexpr UInt32 DDI_CAP_APU = 0x53;                    // Device is an APU
constexpr UInt32 DDI_CAP_NO_DCE_SUPPORT = 0x12C;        // Disable DCE support
constexpr UInt32 DDI_CAP_HW_VIRT = 0x133;               // Device is virtualised
constexpr UInt32 DDI_CAP_HW_VIRT_VF = 0x134;            // Device's virtualisation is VF
constexpr UInt32 DDI_CAP_ACTUAL_TEMPERATURE = 0x13A;    // Actual temperature feature
constexpr UInt32 DDI_CAP_LOAD_KICKER_SMC_FW = 0x13F;    // Load the Kicker SMC firmware
constexpr UInt32 DDI_CAP_EARLY_SAMU_INIT = 0x160;       // Perform early SAMU initialisation
