#pragma once
#include <Headers/kern_util.hpp>

constexpr UInt32 InvalidOffset = 0xFFFFFFFF;

template<typename T>
class ObjectField {
    UInt32 offset {InvalidOffset};

    public:
    void operator=(const UInt32 offset) {
        PANIC_COND(offset == InvalidOffset, "ObjField", "offset == InvalidOffset");
        this->offset = offset;
    }

    ObjectField<T> operator+(const UInt32 value) {
        PANIC_COND(this->offset == InvalidOffset, "ObjField", "value == InvalidOffset");
        ObjectField<T> ret {};
        ret.offset = this->offset + value;
        return ret;
    }

    T &get(void *that) {
        PANIC_COND(that == nullptr, "ObjField", "that == nullptr");
        PANIC_COND(this->offset == InvalidOffset, "ObjField", "this->offset == InvalidOffset");
        return getMember<T>(that, this->offset);
    }
};
