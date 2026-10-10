// AMD Device Capabilities
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
#include <GPUDriversAMD/CAIL/GoldenSettings.hpp>

constexpr UInt64 DEVICE_CAP_ENTRY_REV_DONT_CARE = 0xDEADCAFE;

struct AMDDeviceCapabilities
{
    UInt64                  familyId;
    UInt64                  extRevision;
    UInt64                  deviceId;
    UInt64                  revision;
    UInt64                  enumRevision;
    const void*             swipInfo;
    const void*             swipInfoMinimal;
    const UInt32*           devAttrFlags;
    CAILASICGoldenSettings* asicGoldenSettings;
    void*                   doorbellRange;
};
