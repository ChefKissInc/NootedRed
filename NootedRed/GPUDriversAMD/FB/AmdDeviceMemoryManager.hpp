// AMD Framebuffer Device Memory Manager
//
// Copyright © 2024-2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once

enum struct AmdReservedMemorySelector {
    ReserveVRAM,
    Cursor1_32bpp,
    Cursor1_2bpp,
    Cursor2_32bpp,
    Cursor2_2bpp,
    Cursor3_32bpp,
    Cursor3_2bpp,
    Cursor4_32bpp,
    Cursor4_2bpp,
    Cursor5_32bpp,
    Cursor5_2bpp,
    Cursor6_32bpp,
    Cursor6_2bpp,
    PPLIBReserved,
    DMCUBReserved,
    MappedSurface1,
    MappedSurface2,
    MappedSurface3,
    MappedSurface4,
    MappedSurface5,
    MappedSurface6,
    ROMBase,
    Registers,
    Framebuffer,
};
static_assert(sizeof(AmdReservedMemorySelector) == 0x4);
