// AMD CAIL HW Block Golden Register Settings
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
#include <GPUDriversAMD/CAIL/HWBlock.hpp>
#include <IOKit/IOTypes.h>

struct CAILGoldenRegister
{
    const UInt32 regOffset;
    const UInt32 segment;
    const UInt32 andMask;
    const UInt32 orMask;
};

#define GOLDEN_REGISTER(reg, and, or) {.regOffset = (reg), .segment = reg##_BASE_IDX, .andMask = and, .orMask = or }
#define GOLDEN_REGISTER_TERMINATOR                                                                \
    {.regOffset = 0xFFFFFFFF, .segment = 0xFFFFFFFF, .andMask = 0xFFFFFFFF, .orMask = 0xFFFFFFFF}

struct CAILIPGoldenRegisters
{
    const CAILHWBlock         hwBlock;
    const CAILGoldenRegister* entries;
};

#define GOLDEN_REGISTERS(block, ents) {.hwBlock = kCAILHWBlock##block, .entries = (ents)}

#define GOLDEN_REGISTERS_TERMINATOR {.hwBlock = kCAILHWBlockUnknown, .entries = nullptr}

struct CAILASICGoldenSettings
{
    // Emulated GPUs through the Cadence Palladium Emulation platform.
    const CAILGoldenRegister*    palladiumGoldenSettings;
    const CAILIPGoldenRegisters* goldenSettings;
};
