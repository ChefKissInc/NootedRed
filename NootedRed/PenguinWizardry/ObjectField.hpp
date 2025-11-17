// Dynamic object field
//
// Copyright © 2022-2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <Headers/kern_util.hpp>

template<typename T>
class ObjectField {
    static constexpr UInt32 InvalidOffset = 0xFFFFFFFF;

    UInt32 offset {InvalidOffset};

    constexpr ObjectField(const UInt32 offset) : offset {offset} {}

    public:
    constexpr ObjectField() {}

    void operator=(const UInt32 other) {
        assert(this->offset == InvalidOffset);
        this->offset = other;
    }

    ObjectField<T> operator+(const UInt32 value) {
        assert(this->offset != InvalidOffset);
        return ObjectField<T> {this->offset + value};
    }

    T &operator()(void *const obj) {
        assert(obj != nullptr);
        assert(this->offset != InvalidOffset);
        return getMember<T>(obj, this->offset);
    }
};
