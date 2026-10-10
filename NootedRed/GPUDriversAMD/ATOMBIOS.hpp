// AMD GPU BIOS
//
// Copyright (C) 2022-2026  ChefKiss.
//
// Licensed under the ChefKiss Open Knowledge License version 1.
//
// This work is licensed to you under the terms of the ChefKiss Open Knowledge
// License, either version 1 of the License, or (at your option) any later
// version published by ChefKiss.
//
// This work may not be used to train or develop artificial intelligence,
// and may not be used with artificial intelligence in any other way. See
// the ChefKiss Open Knowledge License for details.
//
// This work is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// ChefKiss Open Knowledge License for more details.
//
// You should have received a copy of the ChefKiss Open Knowledge License
// along with this work.  If not, see https://git.chefkiss.dev/ChefKiss/OpenKnowledge/raw/branch/master/LICENSE-v1.md.

#pragma once
#include <IOKit/IOTypes.h>

constexpr UInt32 ATOMBIOS_IMAGE_SIZE = 0x10000;

struct VFCT
{
    char   signature[4];
    UInt32 length;
    UInt8  revision, checksum;
    char   oemId[6];
    char   oemTableId[8];
    UInt32 oemRevision;
    char   creatorId[4];
    UInt32 creatorRevision;
    char   tableUUID[16];
    UInt32 vbiosImageOffset, lib1ImageOffset;
    UInt32 reserved[4];
};

struct GOPVideoBIOSHeader
{
    UInt32 pciBus, pciDevice, pciFunction;
    UInt16 vendorID, deviceID;
    UInt16 ssvId, ssId;
    UInt32 revision, imageLength;
};

struct ATOMCommonTableHeader
{
    UInt16 structureSize;
    UInt8  formatRev;
    UInt8  contentRev;
};

constexpr UInt32 ATOM_ROM_TABLE_PTR = 0x48;
constexpr UInt32 ATOM_ROM_DATA_PTR  = 0x20;

enum ATOMDMIT17MemType : UInt8
{
    kATOMDMIT17MemTypeOther = 0x01,
    kATOMDMIT17MemTypeUnknown,
    kATOMDMIT17MemTypeDRAM,
    kATOMDMIT17MemTypeEDRAM,
    kATOMDMIT17MemTypeVRAM,
    kATOMDMIT17MemTypeSRAM,
    kATOMDMIT17MemTypeRAM,
    kATOMDMIT17MemTypeROM,
    kATOMDMIT17MemTypeFlash,
    kATOMDMIT17MemTypeEEPROM,
    kATOMDMIT17MemTypeFEPROM,
    kATOMDMIT17MemTypeEPROM,
    kATOMDMIT17MemTypeCDRAM,
    kATOMDMIT17MemTypeThreeDRAM,
    kATOMDMIT17MemTypeSDRAM,
    kATOMDMIT17MemTypeSGRAM,
    kATOMDMIT17MemTypeRDRAM,
    kATOMDMIT17MemTypeDDR,
    kATOMDMIT17MemTypeDDR2,
    kATOMDMIT17MemTypeDDR2FBDIMM,
    kATOMDMIT17MemTypeDDR3 = 0x18,
    kATOMDMIT17MemTypeFBD2,
    kATOMDMIT17MemTypeDDR4,
    kATOMDMIT17MemTypeLPDDR,
    kATOMDMIT17MemTypeLPDDR2,
    kATOMDMIT17MemTypeLPDDR3,
    kATOMDMIT17MemTypeLPDDR4,
    kATOMDMIT17MemTypeGDDR6,
    kATOMDMIT17MemTypeHBM,
    kATOMDMIT17MemTypeHBM2,
    kATOMDMIT17MemTypeDDR5,
    kATOMDMIT17MemTypeLPDDR5,
    kATOMDMIT17MemTypeLPDDR5x,
};

struct ATOMIGPSystemInfoV1
{
    ATOMCommonTableHeader header;
    UInt32                vbiosMisc;
    UInt32                gpuCapInfo;
    UInt32                systemConfig;
    UInt32                cpuCapInfo;
    UInt16                gpuclkSsPercentage;
    UInt16                gpuclkSsType;
    UInt16                lvdsSsPercentage;
    UInt16                lvdsSsRate10hz;
    UInt16                hdmiSsPercentage;
    UInt16                hdmiSsRate10hz;
    UInt16                dviSsPercentage;
    UInt16                dviSsRate10hz;
    UInt16                dpPhyOverride;
    UInt16                lvdsMisc;
    UInt16                backlightPwmHz;
    ATOMDMIT17MemType     memoryType;
    UInt8                 umaChannelCount;
    /* rest differ between minor versions, so omitted for brevity */
};

struct ATOMIGPSystemInfoV2 : public ATOMCommonTableHeader
{
    UInt32            vbiosMisc;
    UInt32            gpuCapInfo;
    UInt32            systemConfig;
    UInt32            cpuCapInfo;
    UInt16            gpuclkSsPercentage;
    UInt16            gpuclkSsType;
    UInt16            dpPhyOverride;
    ATOMDMIT17MemType memoryType;
    UInt8             umaChannelCount;
    /* rest differ between minor versions, so omitted for brevity */
};

enum ATOMVRAMType : UInt8
{
    kATOMVRAMTypeUnknown = 0x00,
    kATOMVRAMTypeGDDR5   = 0x50,    // start of atom_dgpu_vram_type
    kATOMVRAMTypeHBM2    = 0x60,
    kATOMVRAMTypeHBM2E   = 0x61,
    kATOMVRAMTypeGDDR6   = 0x70,
    kATOMVRAMTypeHBM3    = 0x80,
    kATOMVRAMTypeHBM3E   = 0x81,
    kATOMVRAMTypeDDR3    = 0xF0,    // Now extensions for `AmdAtomVramInfoIGP`.
    kATOMVRAMTypeDDR4    = 0xF1,
};

struct ATOMDispObjPath
{
    UInt16 dispObjId;
    UInt16 dispRecordOff;
    UInt16 encoderObjId;
    UInt16 extEncoderObjId;
    UInt16 encoderRecordOff;
    UInt16 extEncoderRecordOff;
    UInt16 devTag;
    UInt8  priorityId;
    UInt8  _reserved;
};

struct DispObjInfoTableV1 : public ATOMCommonTableHeader
{
    UInt16            supportedDevices;
    UInt8             pathCount;
    UInt8             _reserved;
    ATOMDispObjPath paths[];
};
