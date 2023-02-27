//  Copyright © 2022 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.0. See LICENSE for
//  details.

#pragma once
#include <Headers/kern_util.hpp>

using t_createFirmware = void *(*)(const void *data, uint32_t size, uint32_t param3, const char *filename);
using t_putFirmware = bool (*)(void *that, uint32_t deviceType, void *fw);
using t_Vega10PowerTuneConstructor = void (*)(void *that, void *param1, void *param2);
using t_HWEngineConstructor = void (*)(void *that);
using t_HWEngineNew = void *(*)(size_t size);
using t_sendMsgToSmc = uint32_t (*)(void *smum, uint32_t msgId);
using t_pspLoadExtended = uint32_t (*)(void *, uint64_t, uint64_t, const void *, size_t);

constexpr uint32_t AMDGPU_FAMILY_RV = 0x8E;

constexpr uint32_t PPSMC_MSG_PowerUpSdma = 0xE;
constexpr uint32_t PPSMC_MSG_PowerGateMmHub = 0x35;

constexpr uint32_t mmHUBP0_DCSURF_ADDR_CONFIG = 0x055A;
constexpr uint32_t mmHUBP0_DCSURF_PRI_VIEWPORT_DIMENSION = 0x055D;
constexpr uint32_t mmHUBP0_DCSURF_PRI_VIEWPORT_START = 0x055C;
constexpr uint32_t mmHUBP0_DCSURF_SURFACE_CONFIG = 0x0559;
constexpr uint32_t mmHUBP0_DCSURF_TILING_CONFIG = 0x055B;
constexpr uint32_t mmHUBP1_DCSURF_ADDR_CONFIG = 0x061E;
constexpr uint32_t mmHUBP1_DCSURF_PRI_VIEWPORT_DIMENSION = 0x0621;
constexpr uint32_t mmHUBP1_DCSURF_PRI_VIEWPORT_START = 0x0620;
constexpr uint32_t mmHUBP1_DCSURF_SURFACE_CONFIG = 0x061D;
constexpr uint32_t mmHUBP1_DCSURF_TILING_CONFIG = 0x061F;
constexpr uint32_t mmHUBP2_DCSURF_ADDR_CONFIG = 0x06E2;
constexpr uint32_t mmHUBP2_DCSURF_PRI_VIEWPORT_DIMENSION = 0x06E5;
constexpr uint32_t mmHUBP2_DCSURF_PRI_VIEWPORT_START = 0x06E4;
constexpr uint32_t mmHUBP2_DCSURF_SURFACE_CONFIG = 0x06E1;
constexpr uint32_t mmHUBP2_DCSURF_TILING_CONFIG = 0x06E3;
constexpr uint32_t mmHUBP3_DCSURF_ADDR_CONFIG = 0x07A6;
constexpr uint32_t mmHUBP3_DCSURF_PRI_VIEWPORT_DIMENSION = 0x07A9;
constexpr uint32_t mmHUBP3_DCSURF_PRI_VIEWPORT_START = 0x07A8;
constexpr uint32_t mmHUBP3_DCSURF_SURFACE_CONFIG = 0x07A5;
constexpr uint32_t mmHUBP3_DCSURF_TILING_CONFIG = 0x07A7;
constexpr uint32_t mmHUBPREQ0_DCSURF_FLIP_CONTROL = 0x058E;
constexpr uint32_t mmHUBPREQ0_DCSURF_PRIMARY_SURFACE_ADDRESS = 0x057D;
constexpr uint32_t mmHUBPREQ0_DCSURF_PRIMARY_SURFACE_ADDRESS_HIGH = 0x057E;
constexpr uint32_t mmHUBPREQ0_DCSURF_SURFACE_EARLIEST_INUSE = 0x0597;
constexpr uint32_t mmHUBPREQ0_DCSURF_SURFACE_EARLIEST_INUSE_HIGH = 0x0598;
constexpr uint32_t mmHUBPREQ0_DCSURF_SURFACE_PITCH = 0x057B;
constexpr uint32_t mmHUBPREQ1_DCSURF_FLIP_CONTROL = 0x0652;
constexpr uint32_t mmHUBPREQ1_DCSURF_PRIMARY_SURFACE_ADDRESS = 0x0641;
constexpr uint32_t mmHUBPREQ1_DCSURF_PRIMARY_SURFACE_ADDRESS_HIGH = 0x0642;
constexpr uint32_t mmHUBPREQ1_DCSURF_SURFACE_EARLIEST_INUSE = 0x065B;
constexpr uint32_t mmHUBPREQ1_DCSURF_SURFACE_EARLIEST_INUSE_HIGH = 0x065C;
constexpr uint32_t mmHUBPREQ1_DCSURF_SURFACE_PITCH = 0x063F;
constexpr uint32_t mmHUBPREQ2_DCSURF_FLIP_CONTROL = 0x0716;
constexpr uint32_t mmHUBPREQ2_DCSURF_PRIMARY_SURFACE_ADDRESS = 0x0705;
constexpr uint32_t mmHUBPREQ2_DCSURF_PRIMARY_SURFACE_ADDRESS_HIGH = 0x0706;
constexpr uint32_t mmHUBPREQ2_DCSURF_SURFACE_EARLIEST_INUSE = 0x071F;
constexpr uint32_t mmHUBPREQ2_DCSURF_SURFACE_EARLIEST_INUSE_HIGH = 0x0720;
constexpr uint32_t mmHUBPREQ2_DCSURF_SURFACE_PITCH = 0x0703;
constexpr uint32_t mmHUBPREQ3_DCSURF_FLIP_CONTROL = 0x07DA;
constexpr uint32_t mmHUBPREQ3_DCSURF_PRIMARY_SURFACE_ADDRESS = 0x07C9;
constexpr uint32_t mmHUBPREQ3_DCSURF_PRIMARY_SURFACE_ADDRESS_HIGH = 0x07CA;
constexpr uint32_t mmHUBPREQ3_DCSURF_SURFACE_EARLIEST_INUSE = 0x07E3;
constexpr uint32_t mmHUBPREQ3_DCSURF_SURFACE_EARLIEST_INUSE_HIGH = 0x07E4;
constexpr uint32_t mmHUBPREQ3_DCSURF_SURFACE_PITCH = 0x07C7;
constexpr uint32_t mmHUBPRET0_HUBPRET_CONTROL = 0x05E0;
constexpr uint32_t mmHUBPRET1_HUBPRET_CONTROL = 0x06A4;
constexpr uint32_t mmHUBPRET2_HUBPRET_CONTROL = 0x0768;
constexpr uint32_t mmHUBPRET3_HUBPRET_CONTROL = 0x082C;
constexpr uint32_t mmOTG0_OTG_CONTROL = 0x1B41;
constexpr uint32_t mmOTG0_OTG_INTERLACE_CONTROL = 0x1B44;
constexpr uint32_t mmOTG1_OTG_CONTROL = 0x1BC1;
constexpr uint32_t mmOTG1_OTG_INTERLACE_CONTROL = 0x1BC4;
constexpr uint32_t mmOTG2_OTG_CONTROL = 0x1C41;
constexpr uint32_t mmOTG2_OTG_INTERLACE_CONTROL = 0x1C44;
constexpr uint32_t mmOTG3_OTG_CONTROL = 0x1CC1;
constexpr uint32_t mmOTG3_OTG_INTERLACE_CONTROL = 0x1CC4;
constexpr uint32_t mmOTG4_OTG_CONTROL = 0x1D41;
constexpr uint32_t mmOTG4_OTG_INTERLACE_CONTROL = 0x1D44;
constexpr uint32_t mmOTG5_OTG_CONTROL = 0x1DC1;
constexpr uint32_t mmOTG5_OTG_INTERLACE_CONTROL = 0x1DC4;

constexpr uint32_t GFX_FW_TYPE_CP_MEC = 5;
constexpr uint32_t GFX_FW_TYPE_CP_MEC_ME2 = 6;

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

struct CommonFirmwareHeader {
    uint32_t size;
    uint32_t headerSize;
    uint16_t headerMajor;
    uint16_t headerMinor;
    uint16_t ipMajor;
    uint16_t ipMinor;
    uint32_t ucodeVer;
    uint32_t ucodeSize;
    uint32_t ucodeOff;
    uint32_t crc32;
} PACKED;

struct GfxFwHeaderV1 : public CommonFirmwareHeader {
    uint32_t ucodeFeatureVer;
    uint32_t jtOff;
    uint32_t jtSize;
} PACKED;

struct SdmaFwHeaderV1 : public CommonFirmwareHeader {
    uint32_t ucodeFeatureVer;
    uint32_t ucodeChangeVer;
    uint32_t jtOff;
    uint32_t jtSize;
} PACKED;

struct RlcFwHeaderV2_1 : public CommonFirmwareHeader {
    uint32_t ucodeFeatureVer;
    uint32_t jtOff;
    uint32_t jtSize;
    uint32_t saveAndRestoreOff;
    uint32_t clearStateDescOff;
    uint32_t availScratchRamLocations;
    uint32_t regRestoreListSize;
    uint32_t regListFmtStart;
    uint32_t regListFmtSeparateStart;
    uint32_t startingOffsetsStart;
    uint32_t regListFmtSize;
    uint32_t regListFmtArrayOff;
    uint32_t regListSize;
    uint32_t regListArrayOff;
    uint32_t regListFmtSeparateSize;
    uint32_t regListFmtSeparateArrayOff;
    uint32_t regListSeparateSize;
    uint32_t regListSeparateArrayOff;
    uint32_t regListFmtDirectRegListLen;
    uint32_t saveRestoreListCntlUcodeVer;
    uint32_t saveRestoreListCntlFeatureVer;
    uint32_t saveRestoreListCntlSize;
    uint32_t saveRestoreListCntlOff;
    uint32_t saveRestoreListGpmUcodeVer;
    uint32_t saveRestoreListGpmFeatureVer;
    uint32_t saveRestoreListGpmSize;
    uint32_t saveRestoreListGpmOff;
    uint32_t saveRestoreListSrmUcodeVer;
    uint32_t saveRestoreListSrmFeatureVer;
    uint32_t saveRestoreListSrmSize;
    uint32_t saveRestoreListSrmOff;
} PACKED;

struct PspFwLegacyBinDesc {
    uint32_t fwVer;
    uint32_t off;
    uint32_t size;
} PACKED;

struct PspTaFwHeaderV1 : public CommonFirmwareHeader {
    PspFwLegacyBinDesc xgmi, ras, hdcp, dtm, securedisplay;
} PACKED;

struct CailAsicCapEntry {
    uint32_t familyId, deviceId;
    uint32_t revision, emulatedRev;
    uint32_t pciRev;
    uint32_t _reserved;
    uint32_t *caps;
    uint32_t *skeleton;
} PACKED;

struct CailInitAsicCapEntry {
    uint64_t familyId, deviceId;
    uint64_t revision, emulatedRev;
    uint64_t pciRev;
    uint32_t *caps;
    void *goldenCaps;
} PACKED;

struct GcFwConstant {
    const char *firmwareVer;
    uint32_t featureVer, size;
    uint32_t addr, unknown4;
    uint32_t unknown5, unknown6;
    const uint8_t *data;
} PACKED;

struct SdmaFwConstant {
    const char *unknown1;
    uint32_t size, unknown2;
    const uint8_t *data;
} PACKED;
