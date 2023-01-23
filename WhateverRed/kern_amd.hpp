//  Copyright © 2022 ChefKiss Inc. Licensed under the Non-Profit Open Software License version 3.0. See LICENSE for
//  details.
// SPDX-License-Identifier: NPOSL-3.0

#pragma once
#include <Headers/kern_util.hpp>

struct CailAsicCapEntry {
    uint32_t familyId, deviceId;
    uint32_t revision, emulatedRev;
    uint32_t pciRev;
    void *caps, *skeleton;
};

struct CailInitAsicCapEntry {
    uint64_t familyId, deviceId;
    uint64_t revision, emulatedRev;
    uint64_t pciRev;
    void *caps, *goldenCaps;
};

struct GcFwConstant {
    const char *unknown1;
    uint32_t unknown2, size;
    uint32_t addr, unknown4;
    uint32_t unknown5, unknown6;
    uint8_t *data;
};

struct SdmaFwConstant {
    const char *unknown1;
    uint32_t size, unknown2;
    uint8_t *data;
};
