//  Copyright © 2022 ChefKiss Inc. Licensed under the Non-Profit Open Software License version 3.0. See LICENSE for
//  details.

#pragma once
#include <Headers/kern_util.hpp>

using t_createFirmware = void *(*)(const void *data, uint32_t size, uint32_t param3, const char *filename);
using t_putFirmware = bool (*)(void *that, uint32_t deviceType, void *fw);
using t_Vega10PowerTuneConstructor = void (*)(void *that, void *param1, void *param2);
using t_HWEngineConstructor = void (*)(void *that);
using t_HWEngineNew = void *(*)(size_t size);
using t_sendMsgToSmcWithParam = uint32_t (*)(void *smumData, uint32_t msgId, uint32_t parameter);

struct CailAsicCapEntry {
    uint32_t familyId, deviceId;
    uint32_t revision, emulatedRev;
    uint32_t pciRev;
    void *caps, *skeleton;
} PACKED;

struct CailInitAsicCapEntry {
    uint64_t familyId, deviceId;
    uint64_t revision, emulatedRev;
    uint64_t pciRev;
    void *caps, *goldenCaps;
} PACKED;

struct GcFwConstant {
    const char *unknown1;
    uint32_t unknown2, size;
    uint32_t addr, unknown4;
    uint32_t unknown5, unknown6;
    uint8_t *data;
} PACKED;

struct SdmaFwConstant {
    const char *unknown1;
    uint32_t size, unknown2;
    uint8_t *data;
} PACKED;
