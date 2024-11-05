// Copyright © 2022-2024 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <Headers/kern_util.hpp>

struct FWMetadata {
    const UInt8 *data;
    const UInt32 length;
};

struct FWDescriptor {
    const char *name;
    const FWMetadata metadata;
};

extern const struct FWDescriptor firmware[];
extern const size_t firmwareCount;

inline const FWMetadata &getFWByName(const char *name) {
    for (size_t i = 0; i < firmwareCount; i++) {
        if (strcmp(firmware[i].name, name)) { continue; }

        return firmware[i].metadata;
    }
    PANIC("FW", "'%s' not found", name);
}
