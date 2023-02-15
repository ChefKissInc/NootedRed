//  Copyright © 2022 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.0. See LICENSE for
//  details.

#pragma once
#include <Headers/kern_util.hpp>

using t_createFirmware = void *(*)(const void *data, uint32_t size, uint32_t param3, const char *filename);
using t_putFirmware = bool (*)(void *that, uint32_t deviceType, void *fw);
using t_Vega10PowerTuneConstructor = void (*)(void *that, void *param1, void *param2);
using t_HWEngineConstructor = void (*)(void *that);
using t_HWEngineNew = void *(*)(size_t size);
using t_sendMsgToSmc = uint32_t (*)(void *smumData, uint32_t msgId);
using t_pspLoadExtended = uint32_t (*)(void *, uint64_t, uint64_t, const void *, size_t);

constexpr uint32_t AMDGPU_FAMILY_RAVEN = 0x8E;

constexpr uint32_t PPSMC_MSG_PowerUpVcn = 0xC;
constexpr uint32_t PPSMC_MSG_PowerUpSdma = 0xE;
constexpr uint32_t PPSMC_MSG_PowerGateMmHub = 0x35;

struct VFCT {
    char signature[4];
    uint32_t length;
    uint8_t revision, checksum;
    char oemId[6];
    char oemTableId[8];
    uint32_t oemRevision;
    char creatorId[4];
    uint32_t creatorRevision;
    char tableUUID[16];
    uint32_t vbiosImageOffset, lib1ImageOffset;
    uint32_t reserved[4];
} PACKED;

struct GOPVideoBIOSHeader {
    uint32_t pciBus, pciDevice, pciFunction;
    uint16_t vendorID, deviceID;
    uint16_t ssvId, ssId;
    uint32_t revision, imageLength;
} PACKED;

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
