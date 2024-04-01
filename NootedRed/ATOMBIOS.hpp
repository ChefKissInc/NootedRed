//! Copyright © 2022-2024 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5.
//! See LICENSE for details.

#pragma once
#include <Headers/kern_util.hpp>

struct VFCT {
    char signature[4];
    UInt32 length;
    UInt8 revision, checksum;
    char oemId[6];
    char oemTableId[8];
    UInt32 oemRevision;
    char creatorId[4];
    UInt32 creatorRevision;
    char tableUUID[16];
    UInt32 vbiosImageOffset, lib1ImageOffset;
    UInt32 reserved[4];
} PACKED;

struct GOPVideoBIOSHeader {
    UInt32 pciBus, pciDevice, pciFunction;
    UInt16 vendorID, deviceID;
    UInt16 ssvId, ssId;
    UInt32 revision, imageLength;
} PACKED;

struct ATOMCommonTableHeader {
    UInt16 structureSize;
    UInt8 formatRev;
    UInt8 contentRev;
} PACKED;

constexpr UInt32 ATOM_ROM_TABLE_PTR = 0x48;
constexpr UInt32 ATOM_ROM_DATA_PTR = 0x20;

struct AtomFirmwareInfo : public ATOMCommonTableHeader {
    UInt32 firmwareRevision;
    UInt32 bootupSclkIn10Khz;
    UInt32 bootupMclkIn10Khz;
    UInt32 firmwareCapability;
    UInt32 mainCallParserEntry;
    UInt32 biosScratchRegStartAddr;
};

struct IGPSystemInfoV11 : public ATOMCommonTableHeader {
    UInt32 vbiosMisc;
    UInt32 gpuCapInfo;
    UInt32 systemConfig;
    UInt32 cpuCapInfo;
    UInt16 gpuclkSsPercentage;
    UInt16 gpuclkSsType;
    UInt16 lvdsSsPercentage;
    UInt16 lvdsSsRate10hz;
    UInt16 hdmiSsPercentage;
    UInt16 hdmiSsRate10hz;
    UInt16 dviSsPercentage;
    UInt16 dviSsRate10hz;
    UInt16 dpPhyOverride;
    UInt16 lvdsMisc;
    UInt16 backlightPwmHz;
    UInt8 memoryType;
    UInt8 umaChannelCount;
} PACKED;

enum DMIT17MemType : UInt8 {
    kDDR2MemType = 0x13,
    kDDR2FBDIMMMemType,
    kDDR3MemType = 0x18,
    kDDR4MemType = 0x1A,
    kLPDDR2MemType = 0x1C,
    kLPDDR3MemType,
    kLPDDR4MemType,
    kDDR5MemType = 0x22,
    kLPDDR5MemType,
};

struct IGPSystemInfoV2 : public ATOMCommonTableHeader {
    UInt32 vbiosMisc;
    UInt32 gpuCapInfo;
    UInt32 systemConfig;
    UInt32 cpuCapInfo;
    UInt16 gpuclkSsPercentage;
    UInt16 gpuclkSsType;
    UInt16 dpPhyOverride;
    UInt8 memoryType;
    UInt8 umaChannelCount;
} PACKED;

union IGPSystemInfo {
    ATOMCommonTableHeader header;
    IGPSystemInfoV11 infoV11;
    IGPSystemInfoV2 infoV2;
};

struct ATOMDispObjPathV2 {
    UInt16 dispObjId;
    UInt16 dispRecordOff;
    UInt16 encoderObjId;
    UInt16 extEncoderObjId;
    UInt16 encoderRecordOff;
    UInt16 extEncoderRecordOff;
    UInt16 devTag;
    UInt8 priorityId;
    UInt8 _reserved;
} PACKED;

struct DispObjInfoTableV1_4 : public ATOMCommonTableHeader {
    UInt16 supportedDevices;
    UInt8 pathCount;
    UInt8 _reserved;
    ATOMDispObjPathV2 paths[];
} PACKED;
