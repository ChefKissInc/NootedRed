// AMD ASIC DDI Capabilities
//
// Copyright (C) 2024-2026  ChefKiss.
//
// Licensed under the ChefKiss Open Knowledge License version 1.
//
// This work is licensed to you under the terms of the ChefKiss Open Knowledge
// License, either version 1 of the License, or (at your option) any later
// version published by ChefKiss.
//
// This work may not be used to train or develop artificial intelligence,
// and may not be used with artificial intelligence in any other way. See
// the ChefKiss Open Knowledge License for details.
//
// This work is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// ChefKiss Open Knowledge License for more details.
//
// You should have received a copy of the ChefKiss Open Knowledge License
// along with this work.  If not, see https://git.chefkiss.dev/ChefKiss/OpenKnowledge/raw/branch/master/LICENSE-v1.md.

#pragma once
#include <IOKit/IOTypes.h>

struct CAILAsicCapsEntry
{
    UInt32        familyId, deviceId;
    UInt32        revision, extRevision;
    UInt32        pciRevision;
    const UInt32* ddiCaps;
    const UInt32* skeleton;
};

struct CAILAsicCapsInitEntry
{
    UInt64        familyId;
    UInt64        deviceId;
    UInt64        revision;
    UInt64        extRevision;
    UInt64        pciRevision;
    const UInt32* ddiCaps;
    const void*   goldenSettings;
};

constexpr UInt32 DDI_CAP_APU                = 0x53;     // Device is an APU
constexpr UInt32 DDI_CAP_NO_DCE_SUPPORT     = 0x12C;    // Disable DCE support
constexpr UInt32 DDI_CAP_HW_VIRT            = 0x133;    // Device is virtualised
constexpr UInt32 DDI_CAP_HW_VIRT_VF         = 0x134;    // Device's virtualisation is VF
constexpr UInt32 DDI_CAP_ACTUAL_TEMPERATURE = 0x13A;    // Actual temperature feature
constexpr UInt32 DDI_CAP_LOAD_KICKER_SMC_FW = 0x13F;    // Load the Kicker SMC firmware
constexpr UInt32 DDI_CAP_EARLY_SAMU_INIT    = 0x160;    // Perform early SAMU initialisation
