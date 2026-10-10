// AMD TTL SWIP Display Micro-Controller Unit
//
// Copyright (C) 2025-2026  ChefKiss.
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

enum DMCUFirmwareType
{
    kDMCUFirmwareTypeERAM = 0,
    kDMCUFirmwareTypeISR,
    kDMCUFirmwareTypeDMCUB,
    kDMCUFirmwareTypeCount,    // ????
};

struct DMCUFirmwareConstant
{
    UInt64      loadAddress;
    UInt32      romSize;
    const void* rom;
};

#define DMCU_FW_CONSTANT(_LA, _R)                                                                         \
    static const DMCUFirmwareConstant _R { .loadAddress = (_LA), .romSize = sizeof(_##_R), .rom = _##_R }

struct DMCUFirmwareEntry
{
    UInt64      loadAddress;
    UInt32      romSize;
    const void* rom;
    void*       handle;
};

struct DMCUFirmwareInfo
{
    UInt32            count;
    DMCUFirmwareEntry entry[0x4];
    UInt64            field88;
};
