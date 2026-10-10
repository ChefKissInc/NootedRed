// AMD Framebuffer Device Memory Manager
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

enum struct AmdReservedMemorySelector
{
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
