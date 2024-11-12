// Copyright © 2024 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <IOKit/IOTypes.h>

constexpr UInt32 PPSMC_MSG_PowerUpGfx = 0x6;
constexpr UInt32 PPSMC_MSG_PowerUpSdma = 0xE;
constexpr UInt32 PPSMC_MSG_DeviceDriverReset = 0x1E;
constexpr UInt32 PPSMC_MSG_SoftReset = 0x2E;
constexpr UInt32 PPSMC_MSG_PowerGateMmHub = 0x35;
constexpr UInt32 PPSMC_MSG_ForceGfxContentSave = 0x39;
