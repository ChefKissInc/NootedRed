//! Copyright © 2022-2024 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5.
//! See LICENSE for details.

#pragma once
#include <Headers/kern_util.hpp>

struct FWDescriptor {
    const char *name;
    const UInt8 *data;
    const UInt32 length;
};

#define FIRMWARE(name_, data_, length_) .name = name_, .data = data_, .length = length_
extern const struct FWDescriptor firmware[];
extern const size_t firmwareCount;

inline const FWDescriptor &getFWByName(const char *name) {
    for (size_t i = 0; i < firmwareCount; i++) {
        if (strcmp(firmware[i].name, name)) { continue; }

        return firmware[i];
    }
    PANIC("FW", "'%s' not found", name);
}
