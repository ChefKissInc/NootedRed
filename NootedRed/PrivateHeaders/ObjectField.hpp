// Copyright © 2022-2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <Headers/kern_util.hpp>

template<typename T>
class ObjectField {
    static constexpr UInt32 InvalidOffset = 0xFFFFFFFF;

    UInt32 offset;

    constexpr ObjectField(UInt32 offset) : offset {offset} {}

    public:
    constexpr ObjectField() : ObjectField(InvalidOffset) {}

    inline void operator=(const UInt32 other) { this->offset = other; }

    inline ObjectField<T> operator+(const UInt32 value) {
        PANIC_COND(this->offset == InvalidOffset, "ObjField", "value == InvalidOffset");
        return ObjectField<T> {this->offset + value};
    }

    inline T &get(void *that) {
        PANIC_COND(that == nullptr, "ObjField", "that == nullptr");
        PANIC_COND(this->offset == InvalidOffset, "ObjField", "this->offset == InvalidOffset");
        return getMember<T>(that, this->offset);
    }

    inline void set(void *that, T value) {
        PANIC_COND(that == nullptr, "ObjField", "that == nullptr");
        PANIC_COND(this->offset == InvalidOffset, "ObjField", "this->offset == InvalidOffset");
        getMember<T>(that, this->offset) = value;
    }
};
