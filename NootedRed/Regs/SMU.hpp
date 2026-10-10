// Vega iGPU SMU Message Definitions
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

constexpr UInt32 MP0_SMN_C2PMSG_58                     = 0x7A;
constexpr UInt32 MP0_SMN_C2PMSG_91                     = 0x9B;
constexpr UInt32 MP0_SMN_C2PMSG_100                    = 0xA4;
constexpr UInt32 MP1_SMN_C2PMSG_66                     = 0x282;
constexpr UInt32 MP1_SMN_C2PMSG_82                     = 0x292;
constexpr UInt32 MP1_SMN_C2PMSG_90                     = 0x29A;
constexpr UInt32 MP1_SMN_FPS_CNT                       = 0x2C4;
constexpr UInt32 MP1_FIRMWARE_FLAGS                    = 0x3010024;
constexpr UInt32 MP1_FIRMWARE_FLAGS_INTERRUPTS_ENABLED = 0x1;
