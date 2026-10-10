// AMD TTL SWIP System Direct Memory Access
//
// Copyright (C) 2025-2026  ChefKiss.
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

struct SDMAFWConstant
{
    const char* version;
    UInt32      romSize;
    const void* rom;
    UInt32      field18;
    UInt32      payloadOffDWords;
    UInt32      checksum;
};

#define SDMA_FW_CONSTANT(_V, _R, _F18, _POD, _C) \
    static const SDMAFWConstant _R = {           \
        .version          = (_V),                \
        .romSize          = sizeof(_##_R),       \
        .rom              = _##_R,               \
        .field18          = (_F18),              \
        .payloadOffDWords = (_POD),              \
        .checksum         = (_C),                \
    }
