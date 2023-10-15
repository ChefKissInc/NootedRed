//! Copyright © 2022-2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5.
//! See LICENSE for details.

#pragma once
#include <Headers/kern_util.hpp>

using t_GenericConstructor = void (*)(void *that);

constexpr UInt32 AMDGPU_FAMILY_RAVEN = 0x8E;

constexpr UInt32 PPSMC_MSG_PowerDownSdma = 0xD;
constexpr UInt32 PPSMC_MSG_PowerUpSdma = 0xE;

constexpr UInt32 mmHUBP0_DCSURF_ADDR_CONFIG = 0x55A;
constexpr UInt32 mmHUBP1_DCSURF_ADDR_CONFIG = 0x61E;
constexpr UInt32 mmHUBP0_DCSURF_SURFACE_CONFIG = 0x559;
constexpr UInt32 mmHUBP0_DCSURF_TILING_CONFIG = 0x55B;
constexpr UInt32 mmHUBP0_DCSURF_PRI_VIEWPORT_START = 0x55C;
constexpr UInt32 mmHUBP0_DCSURF_PRI_VIEWPORT_DIMENSION = 0x55D;
constexpr UInt32 mmHUBP2_DCSURF_ADDR_CONFIG = 0x6E2;
constexpr UInt32 mmHUBP1_DCSURF_SURFACE_CONFIG = 0x61D;
constexpr UInt32 mmHUBP1_DCSURF_TILING_CONFIG = 0x61F;
constexpr UInt32 mmHUBP1_DCSURF_PRI_VIEWPORT_START = 0x620;
constexpr UInt32 mmHUBP1_DCSURF_PRI_VIEWPORT_DIMENSION = 0x621;
constexpr UInt32 mmHUBP3_DCSURF_ADDR_CONFIG = 0x7A6;
constexpr UInt32 mmHUBP2_DCSURF_SURFACE_CONFIG = 0x6E1;
constexpr UInt32 mmHUBP2_DCSURF_TILING_CONFIG = 0x6E3;
constexpr UInt32 mmHUBP2_DCSURF_PRI_VIEWPORT_START = 0x6E4;
constexpr UInt32 mmHUBP2_DCSURF_PRI_VIEWPORT_DIMENSION = 0x6E5;
constexpr UInt32 mmHUBP3_DCSURF_SURFACE_CONFIG = 0x7A5;
constexpr UInt32 mmHUBP3_DCSURF_TILING_CONFIG = 0x7A7;
constexpr UInt32 mmHUBP3_DCSURF_PRI_VIEWPORT_START = 0x7A8;
constexpr UInt32 mmHUBP3_DCSURF_PRI_VIEWPORT_DIMENSION = 0x7A9;

constexpr UInt32 mmHUBPREQ0_DCSURF_SURFACE_PITCH = 0x57B;
constexpr UInt32 mmHUBPREQ0_DCSURF_PRIMARY_SURFACE_ADDRESS = 0x57D;
constexpr UInt32 mmHUBPREQ0_DCSURF_PRIMARY_SURFACE_ADDRESS_HIGH = 0x57E;
constexpr UInt32 mmHUBPREQ0_DCSURF_FLIP_CONTROL = 0x58E;
constexpr UInt32 mmHUBPREQ0_DCSURF_SURFACE_EARLIEST_INUSE = 0x597;
constexpr UInt32 mmHUBPREQ0_DCSURF_SURFACE_EARLIEST_INUSE_HIGH = 0x598;
constexpr UInt32 mmHUBPREQ1_DCSURF_SURFACE_PITCH = 0x63F;
constexpr UInt32 mmHUBPREQ1_DCSURF_PRIMARY_SURFACE_ADDRESS = 0x641;
constexpr UInt32 mmHUBPREQ1_DCSURF_PRIMARY_SURFACE_ADDRESS_HIGH = 0x642;
constexpr UInt32 mmHUBPREQ1_DCSURF_FLIP_CONTROL = 0x652;
constexpr UInt32 mmHUBPREQ1_DCSURF_SURFACE_EARLIEST_INUSE = 0x65B;
constexpr UInt32 mmHUBPREQ1_DCSURF_SURFACE_EARLIEST_INUSE_HIGH = 0x65C;
constexpr UInt32 mmHUBPREQ2_DCSURF_SURFACE_PITCH = 0x703;
constexpr UInt32 mmHUBPREQ2_DCSURF_PRIMARY_SURFACE_ADDRESS = 0x705;
constexpr UInt32 mmHUBPREQ2_DCSURF_PRIMARY_SURFACE_ADDRESS_HIGH = 0x706;
constexpr UInt32 mmHUBPREQ2_DCSURF_FLIP_CONTROL = 0x716;
constexpr UInt32 mmHUBPREQ2_DCSURF_SURFACE_EARLIEST_INUSE = 0x71F;
constexpr UInt32 mmHUBPREQ2_DCSURF_SURFACE_EARLIEST_INUSE_HIGH = 0x720;
constexpr UInt32 mmHUBPREQ3_DCSURF_SURFACE_PITCH = 0x7C7;
constexpr UInt32 mmHUBPREQ3_DCSURF_PRIMARY_SURFACE_ADDRESS = 0x7C9;
constexpr UInt32 mmHUBPREQ3_DCSURF_PRIMARY_SURFACE_ADDRESS_HIGH = 0x7CA;
constexpr UInt32 mmHUBPREQ3_DCSURF_FLIP_CONTROL = 0x7DA;
constexpr UInt32 mmHUBPREQ3_DCSURF_SURFACE_EARLIEST_INUSE = 0x7E3;
constexpr UInt32 mmHUBPREQ3_DCSURF_SURFACE_EARLIEST_INUSE_HIGH = 0x7E4;

constexpr UInt32 mmHUBPRET0_HUBPRET_CONTROL = 0x5E0;
constexpr UInt32 mmHUBPRET1_HUBPRET_CONTROL = 0x6A4;
constexpr UInt32 mmHUBPRET2_HUBPRET_CONTROL = 0x768;
constexpr UInt32 mmHUBPRET3_HUBPRET_CONTROL = 0x82C;

constexpr UInt32 mmOTG0_OTG_CONTROL = 0x1B41;
constexpr UInt32 mmOTG0_OTG_INTERLACE_CONTROL = 0x1B44;
constexpr UInt32 mmOTG1_OTG_CONTROL = 0x1BC1;
constexpr UInt32 mmOTG1_OTG_INTERLACE_CONTROL = 0x1BC4;
constexpr UInt32 mmOTG2_OTG_CONTROL = 0x1C41;
constexpr UInt32 mmOTG2_OTG_INTERLACE_CONTROL = 0x1C44;
constexpr UInt32 mmOTG3_OTG_CONTROL = 0x1CC1;
constexpr UInt32 mmOTG3_OTG_INTERLACE_CONTROL = 0x1CC4;
constexpr UInt32 mmOTG4_OTG_CONTROL = 0x1D41;
constexpr UInt32 mmOTG4_OTG_INTERLACE_CONTROL = 0x1D44;
constexpr UInt32 mmOTG5_OTG_CONTROL = 0x1DC1;
constexpr UInt32 mmOTG5_OTG_INTERLACE_CONTROL = 0x1DC4;

constexpr UInt32 mmPCIE_INDEX2 = 0xE;
constexpr UInt32 mmPCIE_DATA2 = 0xF;

constexpr UInt32 mmIH_CHICKEN = 0x122C;
constexpr UInt32 mmIH_MC_SPACE_GPA_ENABLE = 0x10;
constexpr UInt32 mmIH_CLK_CTRL = 0x117B;
constexpr UInt32 mmIH_IH_BUFFER_MEM_CLK_SOFT_OVERRIDE_SHIFT = 0x1A;
constexpr UInt32 mmIH_DBUS_MUX_CLK_SOFT_OVERRIDE_SHIFT = 0x1B;

constexpr UInt32 MP_BASE = 0x16000;

constexpr UInt32 AMDGPU_MAX_USEC_TIMEOUT = 100000;

constexpr UInt32 mmMP1_SMN_C2PMSG_90 = 0x29A;
constexpr UInt32 mmMP1_SMN_C2PMSG_82 = 0x292;
constexpr UInt32 mmMP1_SMN_C2PMSG_66 = 0x282;

struct CommonFirmwareHeader {
    UInt32 size;
    UInt32 headerSize;
    UInt16 headerMajor;
    UInt16 headerMinor;
    UInt16 ipMajor;
    UInt16 ipMinor;
    UInt32 ucodeVer;
    UInt32 ucodeSize;
    UInt32 ucodeOff;
    UInt32 crc32;
} PACKED;

struct GPUInfoFirmware {
    UInt32 gcNumSe;
    UInt32 gcNumCuPerSh;
    UInt32 gcNumShPerSe;
    UInt32 gcNumRbPerSe;
    UInt32 gcNumTccs;
    UInt32 gcNumGprs;
    UInt32 gcNumMaxGsThds;
    UInt32 gcGsTableDepth;
    UInt32 gcGsPrimBuffDepth;
    UInt32 gcParameterCacheDepth;
    UInt32 gcDoubleOffchipLdsBuffer;
    UInt32 gcWaveSize;
    UInt32 gcMaxWavesPerSimd;
    UInt32 gcMaxScratchSlotsPerCu;
    UInt32 gcLdsSize;
} PACKED;

struct CAILAsicCapsEntry {
    UInt32 familyId, deviceId;
    UInt32 revision, extRevision;
    UInt32 pciRevision;
    UInt32 _reserved;
    const UInt32 *caps;
    const UInt32 *skeleton;
} PACKED;

struct CAILAsicCapsInitEntry {
    UInt64 familyId, deviceId;
    UInt64 revision, extRevision;
    UInt64 pciRevision;
    const UInt32 *caps;
    const void *goldenCaps;
} PACKED;

static const UInt32 ddiCapsRaven[16] = {0x800005, 0x500011FE, 0x80000, 0x11001000, 0x200, 0x68000001, 0x20000000,
    0x4002, 0x22420001, 0x9E20E10, 0x2000120, 0x0, 0x0, 0x0, 0x0, 0x0};
static const UInt32 ddiCapsRenoir[16] = {0x800005, 0x500011FE, 0x80000, 0x11001000, 0x200, 0x68000001, 0x20000000,
    0x4002, 0x22420001, 0x9E20E18, 0x2000120, 0x0, 0x0, 0x0, 0x0, 0x0};

enum CAILResult : UInt32 {
    kCAILResultSuccess = 0,
    kCAILResultInvalidArgument,
    kCAILResultGeneralFailure,
    kCAILResultResourcesExhausted,
    kCAILResultUnsupported,
};

struct CAILDeviceTypeEntry {
    UInt32 deviceId;
    UInt32 deviceType;
} PACKED;

static const UInt32 ravenDevAttrFlags = 0x49;

struct DeviceCapabilityEntry {
    UInt64 familyId, extRevision;
    UInt64 deviceId, revision, enumRevision;
    const void *swipInfo, *swipInfoMinimal;
    const UInt32 *devAttrFlags;
    const void *goldenRegisterSetings, *doorbellRange;
} PACKED;

constexpr UInt64 DEVICE_CAP_ENTRY_REV_DONT_CARE = 0xDEADCAFEU;

enum VideoMemoryType : UInt32 {
    kVideoMemoryTypeUnknown,
    kVideoMemoryTypeDDR2,
    kVideoMemoryTypeDDR3 = 3,
    kVideoMemoryTypeDDR4,
};

constexpr UInt32 PP_RESULT_OK = 1;

constexpr UInt32 ADDR_CHIP_FAMILY_AI = 8;

constexpr UInt32 Dcn1NonBpp64SwModeMask = 0x2220221;
constexpr UInt32 Dcn1Bpp64SwModeMask = 0x6660661;
constexpr UInt32 Dcn2NonBpp64SwModeMask = 0x2020201;
constexpr UInt32 Dcn2Bpp64SwModeMask = 0x6060601;

struct Gfx9ChipSettings {
    UInt32 isArcticIsland : 1;
    UInt32 isVega10 : 1;
    UInt32 isRaven : 1;
    UInt32 isVega12 : 1;
    UInt32 isVega20 : 1;
    UInt32 reserved0 : 27;
    UInt32 isDce12 : 1;
    UInt32 isDcn1 : 1;
    UInt32 reserved1 : 30;
    UInt32 metaBaseAlignFix : 1;
    UInt32 depthPipeXorDisable : 1;
    UInt32 htileAlignFix : 1;
    UInt32 applyAliasFix : 1;
    UInt32 htileCacheRbConflict : 1;
    UInt32 reserved2 : 27;
};

enum AMDPSPCommand : UInt32 {
    kPSPCommandLoadTA = 1,
    kPSPCommandLoadASD = 4,
    kPSPCommandLoadIPFW = 6,
};

enum AMDUCodeID : UInt32 {
    kUCodeCE = 2,
    kUCodePFP,
    kUCodeME,
    kUCodeMEC1JT,
    kUCodeMEC2JT,
    kUCodeMEC1,
    kUCodeMEC2,
    kUCodeRLC = 11,
    kUCodeSDMA0,
    kUCodeDMCUERAM = 19,
    kUCodeDMCUISR,
    kUCodeRLCV = 21,
    kUCodeRLCSRListGPM = 23,
    kUCodeRLCSRListSRM,
    kUCodeRLCSRListCntl,
    kUCodeDMCUB = 35,
};
