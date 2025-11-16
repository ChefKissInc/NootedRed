// DCN2 Register Offsets
//
// Copyright © 2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <IOKit/IOTypes.h>

constexpr UInt32 HUBP_REG_STRIDE = 0xDC;
constexpr UInt32 HUBPRET_CONTROL = 0x66C;
constexpr UInt32 HUBP_SURFACE_CONFIG = 0x5E5;
constexpr UInt32 HUBP_ADDR_CONFIG = 0x5E6;
constexpr UInt32 HUBP_TILING_CONFIG = 0x5E7;
constexpr UInt32 HUBP_PRI_VIEWPORT_START = 0x5E9;
constexpr UInt32 HUBP_PRI_VIEWPORT_DIMENSION = 0x5EA;
constexpr UInt32 HUBPREQ_SURFACE_PITCH = 0x607;
constexpr UInt32 HUBPREQ_PRIMARY_SURFACE_ADDRESS = 0x60A;
constexpr UInt32 HUBPREQ_PRIMARY_SURFACE_ADDRESS_HIGH = 0x60B;
constexpr UInt32 HUBPREQ_FLIP_CONTROL = 0x61B;
constexpr UInt32 HUBPREQ_SURFACE_EARLIEST_INUSE = 0x625;
constexpr UInt32 HUBPREQ_SURFACE_EARLIEST_INUSE_HIGH = 0x626;

constexpr UInt32 OTG_REG_STRIDE = 0x80;
constexpr UInt32 OTG_CONTROL = 0x1B41;
constexpr UInt32 OTG_INTERLACE_CONTROL = 0x1B44;
