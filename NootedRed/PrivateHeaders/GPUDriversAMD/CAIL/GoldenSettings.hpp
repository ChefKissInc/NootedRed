// Copyright © 2024 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <IOKit/IOTypes.h>
#include <PrivateHeaders/GPUDriversAMD/CAIL/HWBlock.hpp>

//|-------- CAIL HW Block Golden Register Settings --------|//
//|        Golden Settings are known good defaults         |//
//|--------------------------------------------------------|//

struct CAILGoldenRegister {
    const UInt32 regOffset;
    const UInt32 segment;
    const UInt32 andMask;
    const UInt32 orMask;
};
static_assert(sizeof(CAILGoldenRegister) == 0x10);

#define GOLDEN_REGISTER(reg, and, or) \
    { .regOffset = reg, .segment = reg##_BASE_IDX, .andMask = and, .orMask = or }
#define GOLDEN_REGISTER_TERMINATOR \
    { .regOffset = 0xFFFFFFFF, .segment = 0xFFFFFFFF, .andMask = 0xFFFFFFFF, .orMask = 0xFFFFFFFF }

struct CAILIPGoldenRegisters {
    const CAILHWBlock hwBlock;
    const CAILGoldenRegister *entries;
};
static_assert(sizeof(CAILIPGoldenRegisters) == 0x10);

#define GOLDEN_REGISTERS(block, ents) \
    { .hwBlock = kCAILHWBlock##block, .entries = ents }

#define GOLDEN_REGISTERS_TERMINATOR \
    { .hwBlock = kCAILHWBlockUnknown, .entries = nullptr }

struct CAILASICGoldenSettings {
    // Golden settings for GPUs emulated using the Cadence Palladium Emulation platform. We don't care.
    const CAILGoldenRegister *palladiumGoldenSettings;
    const CAILIPGoldenRegisters *goldenSettings;
};
static_assert(sizeof(CAILASICGoldenSettings) == 0x10);
