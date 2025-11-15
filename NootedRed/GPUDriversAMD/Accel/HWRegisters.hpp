#pragma once
#include <Headers/kern_util.hpp>
#include <IOKit/IOTypes.h>

class AMDRadeonX5000_AMDHWRegisters {
    public:
    auto readReg32(const UInt32 off) {
        auto vtable = getMember<void *>(this, 0);
        return getMember<UInt32 (*)(void *, UInt32)>(vtable, 0x118)(this, off);
    }

    void writeReg32(const UInt32 off, const UInt32 val) {
        auto vtable = getMember<void *>(this, 0);
        getMember<void (*)(void *, UInt32, UInt32)>(vtable, 0x120)(this, off, val);
    }
};
