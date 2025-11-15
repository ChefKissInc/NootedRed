// Copyright © 2024-2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <IOKit/IOTypes.h>

constexpr UInt32 HUBP_REG_STRIDE = 0xC4;
constexpr UInt32 HUBPRET_CONTROL = 0x5E0;
constexpr UInt32 HUBP_SURFACE_CONFIG = 0x559;
constexpr UInt32 HUBP_ADDR_CONFIG = 0x55A;
constexpr UInt32 HUBP_TILING_CONFIG = 0x55B;
constexpr UInt32 HUBP_PRI_VIEWPORT_START = 0x55C;
constexpr UInt32 HUBP_PRI_VIEWPORT_DIMENSION = 0x55D;
constexpr UInt32 HUBPREQ_SURFACE_PITCH = 0x57B;
constexpr UInt32 HUBPREQ_PRIMARY_SURFACE_ADDRESS = 0x57D;
constexpr UInt32 HUBPREQ_PRIMARY_SURFACE_ADDRESS_HIGH = 0x57E;
constexpr UInt32 HUBPREQ_FLIP_CONTROL = 0x58E;
constexpr UInt32 HUBPREQ_SURFACE_EARLIEST_INUSE = 0x597;
constexpr UInt32 HUBPREQ_SURFACE_EARLIEST_INUSE_HIGH = 0x598;

constexpr UInt32 OTG_REG_STRIDE = 0x80;
constexpr UInt32 OTG_CONTROL = 0x1B41;
constexpr UInt32 OTG_INTERLACE_CONTROL = 0x1B44;
