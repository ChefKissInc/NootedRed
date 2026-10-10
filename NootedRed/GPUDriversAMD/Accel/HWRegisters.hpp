// Version-independent interface to the `AMDRadeonX5000_AMDHWRegisters` class
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
#include <Headers/kern_util.hpp>
#include <IOKit/IOTypes.h>

class AMDRadeonX5000_AMDHWRegisters
{
public:
    auto read(const UInt32 off)
    {
        auto vtable = getMember<void*>(this, 0);
        return getMember<UInt32 (*)(void*, UInt32)>(vtable, 0x118)(this, off);
    }

    void write(const UInt32 off, const UInt32 val)
    {
        auto vtable = getMember<void*>(this, 0);
        getMember<void (*)(void*, UInt32, UInt32)>(vtable, 0x120)(this, off, val);
    }
};
