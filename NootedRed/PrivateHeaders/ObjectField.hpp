// Copyright © 2022-2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <Headers/kern_util.hpp>

template<typename T>
class ObjectField {
    static constexpr UInt32 InvalidOffset = 0xFFFFFFFF;

    UInt32 offset;

    constexpr ObjectField(const UInt32 offset) : offset {offset} {}

    public:
    constexpr ObjectField() : ObjectField(InvalidOffset) {}

    inline void operator=(const UInt32 other) {
        PANIC_COND(this->offset != InvalidOffset, "ObjField", "Offset reassigned");
        this->offset = other;
    }

    inline ObjectField<T> operator+(const UInt32 value) {
        PANIC_COND(this->offset == InvalidOffset, "ObjField", "Uninitialised");
        return ObjectField<T> {this->offset + value};
    }

    inline T &get(void *const obj) {
        PANIC_COND(obj == nullptr, "ObjField", "Object parameter is null");
        PANIC_COND(this->offset == InvalidOffset, "ObjField", "Uninitialised");
        return getMember<T>(obj, this->offset);
    }

    inline void set(void *const obj, const T value) {
        PANIC_COND(obj == nullptr, "ObjField", "Object parameter is null");
        PANIC_COND(this->offset == InvalidOffset, "ObjField", "Uninitialised");
        getMember<T>(obj, this->offset) = value;
    }
};
