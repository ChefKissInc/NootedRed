// Dynamic object field
//
// Copyright (C) 2022-2026  ChefKiss.
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

template<typename T>
class ObjectField
{
    static constexpr UInt32 InvalidOffset = 0xFFFFFFFF;

    UInt32 offset{InvalidOffset};

public:
    constexpr ObjectField() { }

    constexpr ObjectField(const UInt32 offset) :
        offset{offset}
    { }

    void operator=(const UInt32 other)
    {
        assert(this->offset == InvalidOffset);
        this->offset = other;
    }

    ObjectField<T> operator+(const UInt32 value) const
    {
        assert(this->offset != InvalidOffset);
        return ObjectField<T>{this->offset + value};
    }

    T& operator()(void* const obj) const
    {
        assert(obj != nullptr);
        assert(this->offset != InvalidOffset);
        return getMember<T>(obj, this->offset);
    }
};
