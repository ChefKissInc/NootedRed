// CAIL DDI Capabilities per ASIC
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

static const UInt32 ddiCapsRaven[16]   = {0x800005,   0x500011FE, 0x80000,    0x11001000, 0x200,     0x68000001,
                                          0x20000000, 0x4002,     0x22420001, 0x9E20E10,  0x2000120, 0x0,
                                          0x0,        0x0,        0x0,        0x0};
static const UInt32 ddiCapsRenoir[16]  = {0x800005,   0x500011FE, 0x80000,    0x11001000, 0x200,     0x68000001,
                                          0x20000000, 0x4002,     0x22420001, 0x9E20E18,  0x2000120, 0x0,
                                          0x0,        0x0,        0x0,        0x0};
static const UInt32 ddiCapsRenoirE[16] = {0x800005,   0x500011FE, 0x80000,    0x11001000, 0x200,     0x68000001,
                                          0x20000000, 0x4002,     0x22420001, 0x9E60E18,  0x2000120, 0x0,
                                          0x0,        0x0,        0x0,        0x0};
