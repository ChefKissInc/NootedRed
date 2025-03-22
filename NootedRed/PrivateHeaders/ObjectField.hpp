// Copyright © 2022-2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <Headers/kern_util.hpp>

template<typename T>
class ObjectField {
    static constexpr UInt32 InvalidOffset = 0xFFFFFFFF;

#ifdef DEBUG
    const char *descr;
    UInt32 derivativeOffset;
#endif
    UInt32 offset;

    public:
#ifdef DEBUG
    constexpr ObjectField(const char *descr) : descr {descr}, derivativeOffset {0}, offset {InvalidOffset} {}
#else
    constexpr ObjectField(const char *) : offset {InvalidOffset} {}
#endif

    inline void operator=(const UInt32 other) { this->offset = other; }

    inline ObjectField<T> operator+(const UInt32 value) {
        PANIC_COND(this->offset == InvalidOffset, "ObjField", "value == InvalidOffset");
#ifdef DEBUG
        ObjectField<T> ret {this->descr};
        ret.derivativeOffset = value;
#else
        ObjectField<T> ret {nullptr};
#endif
        ret.offset = this->offset + value;
        return ret;
    }

    inline T &get(void *that) {
        PANIC_COND(that == nullptr, "ObjField", "that == nullptr");
        PANIC_COND(this->offset == InvalidOffset, "ObjField", "this->offset == InvalidOffset");
#ifdef DEBUG
        if (this->derivativeOffset == 0) {
            DBGLOG("ObjField", "Reading field `%s` (0x%X) in %p", this->descr, this->offset, that);
        } else {
            DBGLOG("ObjField", "Reading field derived from `%s` (0x%X+0x%X) in %p", this->descr, this->offset,
                this->derivativeOffset, that);
        }
#endif
        return getMember<T>(that, this->offset);
    }

    inline void set(void *that, T value) {
        PANIC_COND(that == nullptr, "ObjField", "that == nullptr");
        PANIC_COND(this->offset == InvalidOffset, "ObjField", "this->offset == InvalidOffset");
#ifdef DEBUG
        if (this->derivativeOffset == 0) {
            DBGLOG("ObjField", "Writing to field `%s` (0x%X) in %p", this->descr, this->offset, that);
        } else {
            DBGLOG("ObjField", "Writing to field derived from `%s` (0x%X+0x%X) in %p", this->descr, this->offset,
                this->derivativeOffset, that);
        }
#endif
        getMember<T>(that, this->offset) = value;
    }
};
