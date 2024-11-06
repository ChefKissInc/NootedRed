// Copyright © 2022-2024 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <Headers/kern_util.hpp>

constexpr UInt32 InvalidOffset = 0xFFFFFFFF;

template<typename T>
class ObjectField {
    UInt32 offset {InvalidOffset};

    public:
    inline void operator=(const UInt32 other) { this->offset = other; }

    inline ObjectField<T> operator+(const UInt32 value) {
        PANIC_COND(this->offset == InvalidOffset, "ObjField", "value == InvalidOffset");
        ObjectField<T> ret {};
        ret.offset = this->offset + value;
        return ret;
    }

    inline T get(void *that) {
        PANIC_COND(that == nullptr, "ObjField", "that == nullptr");
        PANIC_COND(this->offset == InvalidOffset, "ObjField", "this->offset == InvalidOffset");
        return getMember<T>(that, this->offset);
    }

    inline T &getRef(void *that) {
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
