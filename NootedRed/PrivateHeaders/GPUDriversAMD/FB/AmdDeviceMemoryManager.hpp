// Copyright © 2024 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once

enum AmdReservedMemorySelector {
    kAmdReservedMemorySelectorReserveVRAM,
    kAmdReservedMemorySelectorCursor1_32bpp,
    kAmdReservedMemorySelectorCursor1_2bpp,
    kAmdReservedMemorySelectorCursor2_32bpp,
    kAmdReservedMemorySelectorCursor2_2bpp,
    kAmdReservedMemorySelectorCursor3_32bpp,
    kAmdReservedMemorySelectorCursor3_2bpp,
    kAmdReservedMemorySelectorCursor4_32bpp,
    kAmdReservedMemorySelectorCursor4_2bpp,
    kAmdReservedMemorySelectorCursor5_32bpp,
    kAmdReservedMemorySelectorCursor5_2bpp,
    kAmdReservedMemorySelectorCursor6_32bpp,
    kAmdReservedMemorySelectorCursor6_2bpp,
    kAmdReservedMemorySelectorPPLIBReserved,
    kAmdReservedMemorySelectorDMCUBReserved,
    kAmdReservedMemorySelectorMappedSurface1,
    kAmdReservedMemorySelectorMappedSurface2,
    kAmdReservedMemorySelectorMappedSurface3,
    kAmdReservedMemorySelectorMappedSurface4,
    kAmdReservedMemorySelectorMappedSurface5,
    kAmdReservedMemorySelectorMappedSurface6,
    kAmdReservedMemorySelectorROMBase,
    kAmdReservedMemorySelectorRegisters,
    kAmdReservedMemorySelectorFramebuffer,
};
