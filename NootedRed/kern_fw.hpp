//  Copyright © 2022-2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.0. See LICENSE for
//  details.

#pragma once
#include <Headers/kern_util.hpp>

struct FWDescriptor {
    const char *name;
    const uint8_t *data;
    const uint32_t size;
};

#define NRED_FW(name_, data_, size_) .name = name_, .data = data_, .size = size_

extern const struct FWDescriptor firmware[];
extern const size_t firmwareCount;

inline const FWDescriptor &getFWDescByName(const char *name) {
    for (size_t i = 0; i < firmwareCount; i++) {
        if (!strcmp(firmware[i].name, name)) { return firmware[i]; }
    }
    PANIC("nred", "getFWDescByName: '%s' not found", name);
}
