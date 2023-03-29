//  Copyright © 2022-2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.0. See LICENSE for
//  details.

#ifndef kern_vbios_hpp
#define kern_vbios_hpp
#include <Headers/kern_util.hpp>

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

struct AtomCommonTableHeader {
    uint16_t structureSize;
    uint8_t formatRev;
    uint8_t contentRev;
} PACKED;

constexpr uint32_t ATOM_ROM_TABLE_PTR = 0x48;
constexpr uint32_t ATOM_ROM_DATA_PTR = 0x20;

constexpr uint32_t ATOM_ROM_SIZE_OFFSET = 0x2;
constexpr uint32_t ATOM_ROM_CHECKSUM_OFFSET = 0x21;

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

struct AtomDispObjPathV2 {
    uint16_t dispObjId;
    uint16_t dispRecordOff;
    uint16_t encoderObjId;
    uint16_t extEncoderObjId;
    uint16_t encoderRecordOff;
    uint16_t extEncoderRecordOff;
    uint16_t devTag;
    uint8_t priorityId;
    uint8_t _reserved;
} PACKED;

struct DispObjInfoTableV1_4 : public AtomCommonTableHeader {
    uint16_t supportedDevices;
    uint8_t pathCount;
    uint8_t _reserved;
    AtomDispObjPathV2 dispPaths[];
} PACKED;

#endif /* kern_vbios_hpp */
