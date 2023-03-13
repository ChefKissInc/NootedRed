//  Copyright © 2022-2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.0. See LICENSE for
//  details.

#pragma once
#include <Headers/kern_util.hpp>

struct AtomCommonTableHeader {
    uint16_t structureSize;
    uint8_t formatRev;
    uint8_t contentRev;
} PACKED;

constexpr uint32_t ATOM_ROM_TABLE_PTR = 0x48;
constexpr uint32_t ATOM_ROM_DATA_PTR = 0x20;

struct IgpSystemInfoV11 : public AtomCommonTableHeader {
    uint32_t vbiosMisc;
    uint32_t gpuCapInfo;
    uint32_t systemConfig;
    uint32_t cpuCapInfo;
    uint16_t gpuclkSsPercentage;
    uint16_t gpuclkSsType;
    uint16_t lvdsSsPercentage;
    uint16_t lvdsSsRate10hz;
    uint16_t hdmiSsPercentage;
    uint16_t hdmiSsRate10hz;
    uint16_t dviSsPercentage;
    uint16_t dviSsRate10hz;
    uint16_t dpPhyOverride;
    uint16_t lvdsMisc;
    uint16_t backlightPwmHz;
    uint8_t memoryType;
    uint8_t umaChannelCount;
} PACKED;

struct IgpSystemInfoV2 : public AtomCommonTableHeader {
    uint32_t vbiosMisc;
    uint32_t gpuCapInfo;
    uint32_t systemConfig;
    uint32_t cpuCapInfo;
    uint16_t gpuclkSsPercentage;
    uint16_t gpuclkSsType;
    uint16_t dpPhyOverride;
    uint8_t memoryType;
    uint8_t umaChannelCount;
} PACKED;

union IgpSystemInfo {
    AtomCommonTableHeader header;
    IgpSystemInfoV11 infoV11;
    IgpSystemInfoV2 infoV2;
};
