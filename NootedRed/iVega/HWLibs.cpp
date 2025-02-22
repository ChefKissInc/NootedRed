// Copyright © 2022-2024 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#include <Headers/kern_api.hpp>
#include <PrivateHeaders/Firmware.hpp>
#include <PrivateHeaders/GPUDriversAMD/CAIL/ASICCaps.hpp>
#include <PrivateHeaders/GPUDriversAMD/CAIL/DevCaps.hpp>
#include <PrivateHeaders/GPUDriversAMD/Driver.hpp>
#include <PrivateHeaders/GPUDriversAMD/Family.hpp>
#include <PrivateHeaders/GPUDriversAMD/PSP.hpp>
#include <PrivateHeaders/NRed.hpp>
#include <PrivateHeaders/PatcherPlus.hpp>
#include <PrivateHeaders/iVega/ASICCaps.hpp>
#include <PrivateHeaders/iVega/GoldenSettings.hpp>
#include <PrivateHeaders/iVega/HWLibs.hpp>
#include <PrivateHeaders/iVega/IPOffset.hpp>
#include <PrivateHeaders/iVega/Regs/SMU.hpp>
#include <PrivateHeaders/iVega/RenoirPPSMC.hpp>

//------ Target Kexts ------//

static const char *pathRadeonX5000HWLibs = "/System/Library/Extensions/AMDRadeonX5000HWServices.kext/Contents/PlugIns/"
                                           "AMDRadeonX5000HWLibs.kext/Contents/MacOS/AMDRadeonX5000HWLibs";

static KernelPatcher::KextInfo kextRadeonX5000HWLibs {
    "com.apple.kext.AMDRadeonX5000HWLibs",
    &pathRadeonX5000HWLibs,
    1,
    {true},
    {},
    KernelPatcher::KextInfo::Unloaded,
};

//------ Patterns ------//

static const UInt8 kDeviceTypeTablePattern[] = {0x60, 0x68, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x61, 0x68, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x62, 0x68, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x68, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x64, 0x68, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x67, 0x68, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const UInt8 kCreateFirmwarePattern[] = {0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x54, 0x53, 0x40,
    0x89, 0xC0, 0x41, 0x89, 0xD0, 0x41, 0x89, 0xF0, 0x40, 0x89, 0xF0, 0xBF, 0x20, 0x00, 0x00, 0x00, 0xE8};
static const UInt8 kCreateFirmwarePatternMask[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xF0, 0xFF, 0xF0, 0xFF, 0xFF, 0xF0, 0xFF, 0xFF, 0xF0, 0xF0, 0xFF, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

static const UInt8 kPutFirmwarePattern[] = {0x55, 0x48, 0x89, 0xE5, 0x83, 0xFE, 0x08, 0x7F};

static const UInt8 kCailAsicCapsTableHWLibsPattern[] = {0x6E, 0x00, 0x00, 0x00, 0x98, 0x67, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00};

static const UInt8 kPspCmdKmSubmitPattern[] = {0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x55, 0x41, 0x54,
    0x53, 0x50, 0x49, 0x89, 0xCD, 0x49, 0x89, 0xD7, 0x49, 0x89, 0xF4, 0x48, 0x89, 0xFB, 0x48, 0x8D, 0x75, 0xD0, 0xC7,
    0x06, 0x00, 0x00, 0x00, 0x00, 0xE8, 0x00, 0x00, 0x00, 0x00};
static const UInt8 kPspCmdKmSubmitPatternMask[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00};

static const UInt8 kPspCmdKmSubmitPattern1404[] = {0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x55, 0x41,
    0x54, 0x53, 0x48, 0x83, 0xEC, 0x18, 0x49, 0x89, 0xCD, 0x49, 0x89, 0xD7, 0x49, 0x89, 0xF4, 0x49, 0x89, 0xFE, 0x48,
    0x8D, 0x75, 0xD0, 0xC7, 0x06, 0x00, 0x00, 0x00, 0x00, 0xE8, 0x00, 0x00, 0x00, 0x00};
static const UInt8 kPspCmdKmSubmitPatternMask1404[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00};

static const UInt8 kCAILAsicCapsInitTablePattern[] = {0x6E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x98, 0x67, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00};

static const UInt8 kDeviceCapabilityTblPattern[] = {0x82, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3C, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x73, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE, 0xCA, 0xAD, 0xDE, 0x00, 0x00,
    0x00, 0x00, 0xFE, 0xCA, 0xAD, 0xDE, 0x00, 0x00, 0x00, 0x00};

static const UInt8 kPspReset31Pattern[] = {0x55, 0x48, 0x89, 0xE5, 0x53, 0x48, 0x83, 0xEC, 0x28, 0x31, 0xC0, 0x48, 0x89,
    0x45, 0xF0, 0x48, 0x89, 0x45, 0xE8, 0x48, 0x89, 0x45, 0xE0, 0x48, 0x89, 0x45, 0xD8, 0xB8, 0x01, 0x00, 0x00, 0x00,
    0x48, 0x85, 0xFF};
static const UInt8 kPspReset31Pattern1404[] = {0x55, 0x48, 0x89, 0xE5, 0x53, 0x48, 0x83, 0xEC, 0x28, 0x48, 0xC7, 0x45,
    0xE8, 0x00, 0x00, 0x00, 0x00, 0xB8, 0x01, 0x00, 0x00, 0x00, 0x48, 0x85, 0xFF};

static const UInt8 kPspBootloaderLoadSysdrv31Pattern[] = {0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x55,
    0x41, 0x54, 0x53, 0x48, 0x83, 0xEC, 0x28, 0x49, 0x89, 0xFC, 0x31, 0xDB, 0x48, 0x89, 0x5D, 0xD0, 0x48, 0x89, 0x5D,
    0xC8, 0x48, 0x89, 0x5D, 0xC0, 0x48, 0x89, 0x5D, 0xB8, 0x4C, 0x8B, 0xB7, 0x00, 0x00, 0x00, 0x00, 0x4C, 0x8B, 0xBF,
    0x00, 0x00, 0x00, 0x00, 0xBE, 0x91, 0x00, 0x00, 0x00};
static const UInt8 kPspBootloaderLoadSysdrv31PatternMask[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static const UInt8 kPspBootloaderLoadSysdrv31Pattern1404[] = {0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41,
    0x55, 0x41, 0x54, 0x53, 0x48, 0x83, 0xEC, 0x28, 0x48, 0x89, 0xFB, 0x48, 0xC7, 0x45, 0xC8, 0x00, 0x00, 0x00, 0x00,
    0x4C, 0x8B, 0xB7, 0x38, 0x0B, 0x00, 0x00, 0x4C, 0x8B, 0xBF, 0x48, 0x0B, 0x00, 0x00, 0x45, 0x31, 0xE4, 0xBE, 0x91,
    0x00, 0x00, 0x00};

static const UInt8 kPspBootloaderSetEccMode31Pattern[] = {0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x54,
    0x53, 0x48, 0x83, 0xEC, 0x20, 0x41, 0x89, 0xF7, 0x49, 0x89, 0xFE, 0x31, 0xDB, 0x48, 0x89, 0x5D, 0xD8, 0x48, 0x89,
    0x5D, 0xD0, 0x48, 0x89, 0x5D, 0xC8, 0x48, 0x89, 0x5D, 0xC0, 0xBE, 0xA4, 0x00, 0x00, 0x00, 0x31, 0xD2, 0xB9, 0x4B,
    0x00, 0x00, 0x00};
static const UInt8 kPspBootloaderSetEccMode31Pattern1404[] = {0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41,
    0x54, 0x53, 0x48, 0x83, 0xEC, 0x20, 0x41, 0x89, 0xF7, 0x48, 0x89, 0xFB, 0x48, 0xC7, 0x45, 0xD0, 0x00, 0x00, 0x00,
    0x00, 0x45, 0x31, 0xF6, 0xBE, 0xA4, 0x00, 0x00, 0x00, 0x31, 0xD2, 0xB9, 0x4B, 0x00, 0x00, 0x00, 0xE8, 0xA8, 0x5B,
    0xFF, 0xFF, 0x3D, 0x00, 0x0C, 0x0B, 0x00};

static const UInt8 kPspBootloaderIsSosRunning31Pattern[] = {0x55, 0x48, 0x89, 0xE5, 0xBE, 0x91, 0x00, 0x00, 0x00, 0x31,
    0xD2, 0xB9, 0x4B, 0x00, 0x00, 0x00};

static const UInt8 kPspBootloaderLoadSos31Pattern[] = {0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x54, 0x53,
    0x48, 0x83, 0xEC, 0x20, 0x49, 0x89, 0xFF, 0x31, 0xDB, 0x48, 0x89, 0x5D, 0xD8, 0x48, 0x89, 0x5D, 0xD0, 0x48, 0x89,
    0x5D, 0xC8, 0x48, 0x89, 0x5D, 0xC0, 0x4C, 0x8B, 0xB7, 0x00, 0x00, 0x00, 0x00, 0x4C, 0x8B, 0xA7, 0x00, 0x00, 0x00,
    0x00, 0xBE, 0x91, 0x00, 0x00, 0x00};
static const UInt8 kPspBootloaderLoadSos31PatternMask[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static const UInt8 kPspBootloaderLoadSos31Pattern1404[] = {0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x54,
    0x53, 0x48, 0x83, 0xEC, 0x20, 0x48, 0x89, 0xFB, 0x48, 0xC7, 0x45, 0xD0, 0x00, 0x00, 0x00, 0x00, 0x4C, 0x8B, 0xB7,
    0x38, 0x0B, 0x00, 0x00, 0x4C, 0x8B, 0xA7, 0x48, 0x0B, 0x00, 0x00, 0x45, 0x31, 0xFF, 0xBE, 0x91, 0x00, 0x00, 0x00};

static const UInt8 kPspSecurityFeatureCapsSet31Pattern[] = {0x55, 0x48, 0x89, 0xE5, 0x80, 0xA7, 0x20, 0x31, 0x00, 0x00};
static const UInt8 kPspSecurityFeatureCapsSet31Pattern13[] = {0x55, 0x48, 0x89, 0xE5, 0x8B, 0x87, 0x18, 0x39, 0x00,
    0x00};

static const UInt8 kSmu901CreateFunctionPointerListPattern[] = {0x55, 0x48, 0x89, 0xE5, 0x48, 0x8D, 0x05, 0x00, 0x00,
    0x00, 0x00, 0x48, 0x89, 0x87, 0x08, 0x00, 0x00, 0x00, 0x48, 0x8D, 0x05, 0x00, 0x00, 0x00, 0x00, 0x48, 0x89, 0x87,
    0x00, 0x00, 0x00, 0x00, 0x48, 0x8D, 0x05, 0x00, 0x00, 0x00, 0x00, 0x48, 0x89, 0x87, 0x08, 0x00, 0x00, 0x00, 0x48,
    0x8D, 0x05, 0x00, 0x00, 0x00, 0x00, 0x48, 0x89, 0x87, 0x00, 0x00, 0x00, 0x00, 0x48, 0x8D, 0x05, 0x00, 0x00, 0x00,
    0x00, 0x48, 0x89, 0x87, 0x08, 0x00, 0x00, 0x00, 0x48, 0x8D, 0x05, 0x00, 0x00, 0x00, 0x00, 0x48, 0x89, 0x87, 0x00,
    0x00, 0x00, 0x00, 0x48, 0x8D, 0x05, 0x00, 0x00, 0x00, 0x00, 0x48, 0x89, 0x87, 0x00, 0x00, 0x00, 0x00, 0x48, 0x8D,
    0x05, 0x00, 0x00, 0x00, 0x00, 0x48, 0x89, 0x87, 0x00, 0x00, 0x00, 0x00};
static const UInt8 kSmu901CreateFunctionPointerListPatternMask[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,
    0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x0F, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF,
    0xFF, 0x0F, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x0F, 0x00, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x0F, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00,
    0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x0F, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF,
    0x0F, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x0F, 0x00, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x0F, 0x00, 0xFF, 0xFF};
static const UInt8 kSmu901CreateFunctionPointerListPattern13[] = {0x55, 0x48, 0x89, 0xE5, 0x48, 0x8D, 0x05, 0x00, 0x00,
    0x00, 0x00, 0x48, 0x89, 0x87, 0x00, 0x00, 0x00, 0x00, 0x48, 0x8D, 0x05, 0x00, 0x00, 0x00, 0x00, 0x48, 0x89, 0x87,
    0x08, 0x00, 0x00, 0x00, 0x48, 0x8D, 0x05, 0x00, 0x00, 0x00, 0x00, 0x48, 0x89, 0x87, 0x00, 0x00, 0x00, 0x00, 0x48,
    0x8D, 0x05, 0x00, 0x00, 0x00, 0x00, 0x48, 0x89, 0x87, 0x08, 0x00, 0x00, 0x00, 0x48, 0x8D, 0x05, 0x00, 0x00, 0x00,
    0x00, 0x48, 0x89, 0x87, 0x00, 0x00, 0x00, 0x00, 0x48, 0x8D, 0x05, 0x00, 0x00, 0x00, 0x00, 0x48, 0x89, 0x87, 0x08,
    0x00, 0x00, 0x00, 0x48, 0x8D, 0x05, 0x00, 0x00, 0x00, 0x00, 0x48, 0x89, 0x87, 0x08, 0x00, 0x00, 0x00, 0x48, 0x8D,
    0x05, 0x00, 0x00, 0x00, 0x00, 0x48, 0x89, 0x87, 0x08, 0x00, 0x00, 0x00};

//------ Patches ------//

// Replace call to `_gc_get_hw_version` with constant (0x090001).
static const UInt8 kGcSwInitOriginal[] = {0x0C, 0xE8, 0x00, 0x00, 0x00, 0x00, 0x41, 0x89, 0xC7};
static const UInt8 kGcSwInitOriginalMask[] = {0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF};
static const UInt8 kGcSwInitPatched[] = {0x00, 0xB8, 0x01, 0x00, 0x09, 0x00, 0x00, 0x00, 0x00};
static const UInt8 kGcSwInitPatchedMask[] = {0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00};

// Replace call to `_gc_get_hw_version` with constant (0x090001).
static const UInt8 kGcSetFwEntryInfoOriginal[] = {0xE8, 0x00, 0x00, 0x00, 0x00, 0x31, 0x00, 0x41, 0x89, 0x00, 0x10,
    0x41, 0x89, 0x00, 0x00, 0x00};
static const UInt8 kGcSetFwEntryInfoOriginalMask[] = {0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0xFF,
    0xFF, 0xFF, 0x00, 0x00, 0x00};
static const UInt8 kGcSetFwEntryInfoPatched[] = {0xB8, 0x01, 0x00, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00};
static const UInt8 kGcSetFwEntryInfoPatchedMask[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00};
static const UInt8 kGcSetFwEntryInfoOriginal1404[] = {0xE8, 0x00, 0x00, 0x00, 0x00, 0x45, 0x31, 0xFF, 0x45, 0x89, 0x7E,
    0x10, 0x44};
static const UInt8 kGcSetFwEntryInfoOriginalMask1404[] = {0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF};
static const UInt8 kGcSetFwEntryInfoPatched1404[] = {0xB8, 0x01, 0x00, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00};
static const UInt8 kGcSetFwEntryInfoPatchedMask1404[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00};

// Force PSP 9.x switch case.
static const UInt8 kPspSwInit1Original[] = {0x8B, 0x43, 0x0C, 0x83, 0xC0, 0xF7, 0x83, 0xF8, 0x04};
static const UInt8 kPspSwInit1Patched[] = {0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x83, 0xF8, 0x04};

// Ditto. macOS 14.4+
static const UInt8 kPspSwInit1Original1404[] = {0x41, 0x8B, 0x46, 0x08, 0x83, 0xC0, 0xF7, 0x83, 0xF8, 0x04};
static const UInt8 kPspSwInit1Patched1404[] = {0x90, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x83, 0xF8, 0x04};

// Force PSP x.0.0.
static const UInt8 kPspSwInit2Original[] = {0x83, 0x7B, 0x10, 0x00, 0x0F, 0x85, 0x00, 0x00, 0x00, 0x00, 0x83, 0x7B,
    0x14, 0x01, 0x0F, 0x85, 0x00, 0x00, 0x00, 0x00};
static const UInt8 kPspSwInit2OriginalMask[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00};
static const UInt8 kPspSwInit2Patched[] = {0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x66,
    0x90, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90};

// Ditto. macOS 14.4+
static const UInt8 kPspSwInit2Original1404[] = {0x41, 0x83, 0x7E, 0x0C, 0x00, 0x0F, 0x85, 0x00, 0x00, 0x00, 0x00, 0x41,
    0x83, 0x7E, 0x10, 0x01, 0x0F, 0x85, 0x00, 0x00, 0x00, 0x00};
static const UInt8 kPspSwInit2OriginalMask1404[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00};
static const UInt8 kPspSwInit2Patched1404[] = {0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90,
    0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90};

// Catalina, for Renoir: Force PSP 11.x switch case. PSP 11.0.3 is actually PSP 12 in Catalina HWLibs.
static const UInt8 kPspSwInit1Original1015[] = {0x41, 0x8B, 0x44, 0x24, 0x0C, 0x83, 0xC0, 0xF7};
static const UInt8 kPspSwInit1Patched1015[] = {0xB8, 0x02, 0x00, 0x00, 0x00, 0x66, 0x90, 0x90};

// Ditto, part 2: Force PSP X.0.3.
static const UInt8 kPspSwInit2Original1015[] = {0x41, 0x83, 0x7C, 0x24, 0x10, 0x00, 0x0F, 0x85, 0x00, 0x00, 0x00, 0x00,
    0x41, 0x8B, 0x44, 0x24, 0x14, 0x83, 0xF8, 0x03, 0x0F, 0x85, 0x00, 0x00, 0x00, 0x00};
static const UInt8 kPspSwInit2OriginalMask1015[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00,
    0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00};
static const UInt8 kPspSwInit2Patched1015[] = {0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90,
    0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90};

// Replace call to `_smu_get_hw_version` with constant (0x1).
static const UInt8 kSmuInitFunctionPointerListOriginal[] = {0xE8, 0x00, 0x00, 0x00, 0x00, 0x89, 0xC3, 0x41, 0x89, 0x87,
    0x00, 0x00, 0x00, 0x00};
static const UInt8 kSmuInitFunctionPointerListOriginalMask[] = {0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0x00, 0x00, 0x00, 0x00};
static const UInt8 kSmuInitFunctionPointerListPatched[] = {0xB8, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00};
static const UInt8 kSmuInitFunctionPointerListPatchedMask[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00};

// Ditto. macOS 14.4+
static const UInt8 kSmuInitFunctionPointerListOriginal1404[] = {0xE8, 0x00, 0x00, 0x00, 0x00, 0x41, 0x89, 0xC7, 0x89,
    0x83, 0xC8, 0x02, 0x00, 0x00};
static const UInt8 kSmuInitFunctionPointerListOriginalMask1404[] = {0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static const UInt8 kSmuInitFunctionPointerListPatched1404[] = {0xB8, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00};
static const UInt8 kSmuInitFunctionPointerListPatchedMask1404[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00};

// Use correct PowerTuneServices by changing the switch statement case `0x8D` to `0x8E`.
static const UInt8 kCreatePowerTuneServices1Original[] = {0x41, 0x8B, 0x47, 0x18, 0x83, 0xC0, 0x88, 0x83, 0xF8, 0x17};
static const UInt8 kCreatePowerTuneServices1Patched[] = {0x41, 0x8B, 0x47, 0x18, 0x83, 0xC0, 0x87, 0x83, 0xF8, 0x17};

// Ditto. macOS 12.0+
static const UInt8 kCreatePowerTuneServices1Original12[] = {0xB8, 0x7E, 0xFF, 0xFF, 0xFF, 0x41, 0x03, 0x47, 0x18, 0x83,
    0xF8, 0x0F};
static const UInt8 kCreatePowerTuneServices1Patched12[] = {0xB8, 0x7D, 0xFF, 0xFF, 0xFF, 0x41, 0x03, 0x47, 0x18, 0x83,
    0xF8, 0x0F};

// Ditto. macOS 14.4+
static const UInt8 kCreatePowerTuneServices1Original1404[] = {0xB8, 0x7E, 0xFF, 0xFF, 0xFF, 0x03, 0x43, 0x18, 0x83,
    0xF8, 0x0F};
static const UInt8 kCreatePowerTuneServices1Patched1404[] = {0xB8, 0x7D, 0xFF, 0xFF, 0xFF, 0x03, 0x43, 0x18, 0x83, 0xF8,
    0x0F};

// Remove revision check to always use Vega 10 PowerTune.
static const UInt8 kCreatePowerTuneServices2Original[] = {0x41, 0x8B, 0x47, 0x1C, 0x83, 0xF8, 0x13, 0x77, 0x00};
static const UInt8 kCreatePowerTuneServices2Mask[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00};
static const UInt8 kCreatePowerTuneServices2Patched[] = {0x41, 0x8B, 0x47, 0x1C, 0x66, 0x90, 0x66, 0x90, 0x90};

// Ditto. macOS 14.4+
static const UInt8 kCreatePowerTuneServices2Original1404[] = {0x8B, 0x43, 0x1C, 0x83, 0xF8, 0x13, 0x77, 0x00};
static const UInt8 kCreatePowerTuneServices2Mask1404[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00};
static const UInt8 kCreatePowerTuneServices2Patched1404[] = {0x8B, 0x43, 0x1C, 0x66, 0x90, 0x66, 0x90, 0x90};

// Ventura added an explicit switch case for family ID.
// Now we have to make the switch case 0x8D be 0x8E.
static const UInt8 kCailQueryAdapterInfoOriginal[] = {0x83, 0xC0, 0x92, 0x83, 0xF8, 0x21};
static const UInt8 kCailQueryAdapterInfoPatched[] = {0x83, 0xC0, 0x91, 0x83, 0xF8, 0x21};

// Ventura removed the code for SDMA 4.1.x. Force use SDMA 4.0.
static const UInt8 kSDMAInitFunctionPointerListOriginal[] = {0x81, 0xFA, 0x00, 0x00, 0x04, 0x00, 0x0F, 0x84, 0x00, 0x00,
    0x00, 0x00};
static const UInt8 kSDMAInitFunctionPointerListOriginalMask[] = {0xFF, 0xFA, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,
    0x00, 0x00, 0x00};
static const UInt8 kSDMAInitFunctionPointerListPatched[] = {0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x90, 0xE9, 0x00, 0x00,
    0x00, 0x00};
static const UInt8 kSDMAInitFunctionPointerListPatchedMask[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,
    0x00, 0x00, 0x00};

//------ Module Logic ------//

static iVega::X5000HWLibs instance {};

iVega::X5000HWLibs &iVega::X5000HWLibs::singleton() { return instance; };

void iVega::X5000HWLibs::init() {
    PANIC_COND(this->initialised, "HWLibs", "Attempted to initialise module twice!");
    this->initialised = true;

    switch (getKernelVersion()) {
        case KernelVersion::Catalina:
            this->fwDirField = 0xB8;
            this->pspCommandDataField = 0xB00;
            this->smuInternalSWInitField = 0x378;
            this->smuInternalHWInitField = 0x380;
            this->smuInternalHWExitField = 0x388;
            this->smuInternalSWExitField = 0x390;
            this->smuFullAsicResetField = 0x3A0;
            this->smuNotifyEventField = 0x3A8;
            this->smuFullscreenEventField = 0x3B8;
            break;
        case KernelVersion::BigSur:
            this->fwDirField = 0xB8;
            this->smuSwInitialisedFieldBase = 0x280;
            this->smuInternalSWInitField = 0x638;
            this->smuInternalHWInitField = 0x640;
            this->smuInternalHWExitField = 0x648;
            this->smuInternalSWExitField = 0x650;
            this->smuFullAsicResetField = 0x660;
            this->smuNotifyEventField = 0x668;
            this->smuFullscreenEventField = 0x680;
            this->smuGetUCodeConstsField = 0x720;
            this->pspCommandDataField = 0xAF8;
            this->pspSecurityCapsField = 0x3120;
            this->pspLoadSOSField = 0x3124;
            this->pspTOSField = 0x3128;
            break;
        case KernelVersion::Monterey:
            this->fwDirField = 0xB0;
            this->smuSwInitialisedFieldBase = 0x280;
            this->smuInternalSWInitField = 0x648;
            this->smuInternalHWInitField = 0x650;
            this->smuInternalHWExitField = 0x658;
            this->smuInternalSWExitField = 0x660;
            this->smuFullAsicResetField = 0x670;
            this->smuNotifyEventField = 0x678;
            this->smuFullscreenEventField = 0x690;
            this->smuGetUCodeConstsField = 0x730;
            this->pspCommandDataField = 0xAF8;
            this->pspSecurityCapsField = 0x3120;
            this->pspLoadSOSField = 0x3124;
            this->pspTOSField = 0x3128;
            break;
        case KernelVersion::Ventura:
        case KernelVersion::Sonoma:
        case KernelVersion::Sequoia:
            this->fwDirField = 0xB0;
            this->smuSwInitialisedFieldBase = 0x2D0;
            this->pspCommandDataField = 0xB48;
            this->smuInternalSWInitField = 0x6C0;
            this->smuInternalHWInitField = 0x6C8;
            this->smuInternalHWExitField = 0x6D0;
            this->smuInternalSWExitField = 0x6D8;
            this->smuFullAsicResetField = 0x6E8;
            this->smuNotifyEventField = 0x6F0;
            this->smuFullscreenEventField = 0x708;
            this->smuGetUCodeConstsField = 0x7A8;
            this->pspSecurityCapsField = 0x3918;
            this->pspLoadSOSField = 0x391C;
            this->pspTOSField = 0x3920;
            break;
        default:
            PANIC("HWLibs", "Unsupported kernel version %d", getKernelVersion());
    }

    SYSLOG("HWLibs", "Module initialised.");

    lilu.onKextLoadForce(
        &kextRadeonX5000HWLibs, 1,
        [](void *user, KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size) {
            static_cast<iVega::X5000HWLibs *>(user)->processKext(patcher, id, slide, size);
        },
        this);
}

void iVega::X5000HWLibs::processKext(KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size) {
    if (kextRadeonX5000HWLibs.loadIndex != id) { return; }

    NRed::singleton().hwLateInit();

    CAILAsicCapsEntry *orgCapsTable;
    CAILAsicCapsInitEntry *orgCapsInitTable;
    AMDDeviceTypeEntry *orgDeviceTypeTable;
    AMDDeviceCapabilities *orgDevCapTable;

    if (NRed::singleton().getAttributes().isCatalina()) {
        orgDeviceTypeTable = nullptr;
    } else {
        SolveRequestPlus solveRequests[] = {
            {"__ZL15deviceTypeTable", orgDeviceTypeTable, kDeviceTypeTablePattern},
            {"__ZN11AMDFirmware14createFirmwareEPhjjPKc", this->orgCreateFirmware, kCreateFirmwarePattern,
                kCreateFirmwarePatternMask},
            {"__ZN20AMDFirmwareDirectory11putFirmwareE16_AMD_DEVICE_TYPEP11AMDFirmware", this->orgPutFirmware,
                kPutFirmwarePattern},
        };
        PANIC_COND(!SolveRequestPlus::solveAll(patcher, id, solveRequests, slide, size), "HWLibs",
            "Failed to resolve symbols");
    }

    SolveRequestPlus solveRequests[] = {
        {"__ZL20CAIL_ASIC_CAPS_TABLE", orgCapsTable, kCailAsicCapsTableHWLibsPattern},
        {"_CAILAsicCapsInitTable", orgCapsInitTable, kCAILAsicCapsInitTablePattern},
        {"_DeviceCapabilityTbl", orgDevCapTable, kDeviceCapabilityTblPattern},
    };
    PANIC_COND(!SolveRequestPlus::solveAll(patcher, id, solveRequests, slide, size), "HWLibs",
        "Failed to resolve symbols");

    if (NRed::singleton().getAttributes().isCatalina()) {
        RouteRequestPlus request {"__ZN16AmdTtlFwServices7getIpFwEjPKcP10_TtlFwInfo", wrapGetIpFw, this->orgGetIpFw};
        PANIC_COND(!request.route(patcher, id, slide, size), "HWLibs", "Failed to route getIpFw");
    } else {
        RouteRequestPlus requests[] = {
            {"__ZN35AMDRadeonX5000_AMDRadeonHWLibsX500025populateFirmwareDirectoryEv", wrapPopulateFirmwareDirectory,
                this->orgGetIpFw},
            {"_psp_bootloader_is_sos_running_3_1", hwLibsGeneralFailure, kPspBootloaderIsSosRunning31Pattern},
            {"_psp_security_feature_caps_set_3_1",
                NRed::singleton().getAttributes().isRenoir() ? pspSecurityFeatureCapsSet12 :
                                                               pspSecurityFeatureCapsSet10,
                NRed::singleton().getAttributes().isVenturaAndLater() ? kPspSecurityFeatureCapsSet31Pattern13 :
                                                                        kPspSecurityFeatureCapsSet31Pattern},
        };
        PANIC_COND(!RouteRequestPlus::routeAll(patcher, id, requests, slide, size), "HWLibs",
            "Failed to route symbols (>10.15)");
        if (NRed::singleton().getAttributes().isSonoma1404AndLater()) {
            RouteRequestPlus requests[] = {
                {"_psp_bootloader_load_sysdrv_3_1", hwLibsNoop, kPspBootloaderLoadSysdrv31Pattern1404},
                {"_psp_bootloader_set_ecc_mode_3_1", hwLibsNoop, kPspBootloaderSetEccMode31Pattern1404},
                {"_psp_bootloader_load_sos_3_1", pspBootloaderLoadSos10, kPspBootloaderLoadSos31Pattern1404},
                {"_psp_reset_3_1", hwLibsUnsupported, kPspReset31Pattern1404},
            };
            PANIC_COND(!RouteRequestPlus::routeAll(patcher, id, requests, slide, size), "HWLibs",
                "Failed to route symbols (>=14.4)");
        } else {
            RouteRequestPlus requests[] = {
                {"_psp_bootloader_load_sysdrv_3_1", hwLibsNoop, kPspBootloaderLoadSysdrv31Pattern,
                    kPspBootloaderLoadSysdrv31PatternMask},
                {"_psp_bootloader_set_ecc_mode_3_1", hwLibsNoop, kPspBootloaderSetEccMode31Pattern},
                {"_psp_bootloader_load_sos_3_1", pspBootloaderLoadSos10, kPspBootloaderLoadSos31Pattern,
                    kPspBootloaderLoadSos31PatternMask},
                {"_psp_reset_3_1", hwLibsUnsupported, kPspReset31Pattern},
            };
            PANIC_COND(!RouteRequestPlus::routeAll(patcher, id, requests, slide, size), "HWLibs",
                "Failed to route symbols (<14.4)");
        }
    }

    if (NRed::singleton().getAttributes().isSonoma1404AndLater()) {
        RouteRequestPlus request = {"_psp_cmd_km_submit", wrapPspCmdKmSubmit, this->orgPspCmdKmSubmit,
            kPspCmdKmSubmitPattern1404, kPspCmdKmSubmitPatternMask1404};
        PANIC_COND(!request.route(patcher, id, slide, size), "HWLibs", "Failed to route psp_cmd_km_submit (14.4+)");
    } else {
        RouteRequestPlus request = {"_psp_cmd_km_submit", wrapPspCmdKmSubmit, this->orgPspCmdKmSubmit,
            kPspCmdKmSubmitPattern, kPspCmdKmSubmitPatternMask};
        PANIC_COND(!request.route(patcher, id, slide, size), "HWLibs", "Failed to route psp_cmd_km_submit");
    }

    RouteRequestPlus request = {"_smu_9_0_1_create_function_pointer_list", wrapSmu901CreateFunctionPointerList,
        NRed::singleton().getAttributes().isVenturaAndLater() ? kSmu901CreateFunctionPointerListPattern13 :
                                                                kSmu901CreateFunctionPointerListPattern,
        kSmu901CreateFunctionPointerListPatternMask};
    PANIC_COND(!request.route(patcher, id, slide, size), "HWLibs",
        "Failed to route smu_9_0_1_create_function_pointer_list");

    PANIC_COND(MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) != KERN_SUCCESS, "HWLibs",
        "Failed to enable kernel writing");
    if (orgDeviceTypeTable != nullptr) {
        *orgDeviceTypeTable = {.deviceId = NRed::singleton().getDeviceID(), .deviceType = kAMDDeviceTypeNavi10};
    }

    auto targetDeviceId = NRed::singleton().getAttributes().isRenoir() ? 0x1636 : NRed::singleton().getDeviceID();
    for (; orgCapsInitTable->deviceId != 0xFFFFFFFF; orgCapsInitTable++) {
        if (orgCapsInitTable->familyId == AMD_FAMILY_RAVEN && orgCapsInitTable->deviceId == targetDeviceId) {
            orgCapsInitTable->deviceId = NRed::singleton().getDeviceID();
            orgCapsInitTable->revision = NRed::singleton().getDevRevision();
            orgCapsInitTable->extRevision =
                static_cast<UInt64>(NRed::singleton().getEnumRevision()) + NRed::singleton().getDevRevision();
            orgCapsInitTable->pciRevision = NRed::singleton().getPciRevision();
            orgCapsInitTable->ddiCaps = NRed::singleton().getAttributes().isRenoirE() ? ddiCapsRenoirE :
                                        NRed::singleton().getAttributes().isRenoir()  ? ddiCapsRenoir :
                                                                                        ddiCapsRaven;
            *orgCapsTable = {
                .familyId = AMD_FAMILY_RAVEN,
                .deviceId = NRed::singleton().getDeviceID(),
                .revision = NRed::singleton().getDevRevision(),
                .extRevision =
                    static_cast<UInt32>(NRed::singleton().getEnumRevision()) + NRed::singleton().getDevRevision(),
                .pciRevision = NRed::singleton().getPciRevision(),
                .ddiCaps = orgCapsInitTable->ddiCaps,
            };
            break;
        }
    }
    PANIC_COND(orgCapsInitTable->deviceId == 0xFFFFFFFF, "HWLibs", "Failed to find init caps table entry");
    for (; orgDevCapTable->familyId; orgDevCapTable++) {
        if (orgDevCapTable->familyId == AMD_FAMILY_RAVEN && orgDevCapTable->deviceId == targetDeviceId) {
            orgDevCapTable->deviceId = NRed::singleton().getDeviceID();
            orgDevCapTable->extRevision =
                static_cast<UInt64>(NRed::singleton().getEnumRevision()) + NRed::singleton().getDevRevision();
            orgDevCapTable->revision = DEVICE_CAP_ENTRY_REV_DONT_CARE;
            orgDevCapTable->enumRevision = DEVICE_CAP_ENTRY_REV_DONT_CARE;

            orgDevCapTable->asicGoldenSettings->goldenSettings =
                NRed::singleton().getAttributes().isRaven2() ? goldenSettingsRaven2 :
                NRed::singleton().getAttributes().isRenoir() ? goldenSettingsRenoir :
                                                               goldenSettingsRaven;

            break;
        }
    }
    PANIC_COND(orgDevCapTable->familyId == 0, "HWLibs", "Failed to find device capability table entry");
    MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);
    DBGLOG("HWLibs", "Applied DDI Caps patches");

    if (!NRed::singleton().getAttributes().isCatalina()) {
        const LookupPatchPlus patch {&kextRadeonX5000HWLibs, kGcSwInitOriginal, kGcSwInitOriginalMask, kGcSwInitPatched,
            kGcSwInitPatchedMask, 1};
        PANIC_COND(!patch.apply(patcher, slide, size), "HWLibs", "Failed to apply gc_sw_init spoof patch");
        if (NRed::singleton().getAttributes().isSonoma1404AndLater()) {
            const LookupPatchPlus patches[] = {
                {&kextRadeonX5000HWLibs, kPspSwInit1Original1404, kPspSwInit1Patched1404, 1},
                {&kextRadeonX5000HWLibs, kPspSwInit2Original1404, kPspSwInit2OriginalMask1404, kPspSwInit2Patched1404,
                    1},
                {&kextRadeonX5000HWLibs, kGcSetFwEntryInfoOriginal1404, kGcSetFwEntryInfoOriginalMask1404,
                    kGcSetFwEntryInfoPatched1404, kGcSetFwEntryInfoPatchedMask1404, 1},
            };
            PANIC_COND(!LookupPatchPlus::applyAll(patcher, patches, slide, size), "HWLibs",
                "Failed to apply spoof patches (>=14.4)");
        } else {
            const LookupPatchPlus patches[] = {
                {&kextRadeonX5000HWLibs, kPspSwInit1Original, kPspSwInit1Patched, 1},
                {&kextRadeonX5000HWLibs, kPspSwInit2Original, kPspSwInit2OriginalMask, kPspSwInit2Patched, 1},
                {&kextRadeonX5000HWLibs, kGcSetFwEntryInfoOriginal, kGcSetFwEntryInfoOriginalMask,
                    kGcSetFwEntryInfoPatched, kGcSetFwEntryInfoPatchedMask, 1},
            };
            PANIC_COND(!LookupPatchPlus::applyAll(patcher, patches, slide, size), "HWLibs",
                "Failed to apply spoof patches (<14.4)");
        }
    } else if (NRed::singleton().getAttributes().isRenoir()) {
        const LookupPatchPlus patches[] = {
            {&kextRadeonX5000HWLibs, kPspSwInit1Original1015, kPspSwInit1Patched1015, 1},
            {&kextRadeonX5000HWLibs, kPspSwInit2Original1015, kPspSwInit2OriginalMask1015, kPspSwInit2Patched1015, 1},
        };
        PANIC_COND(!LookupPatchPlus::applyAll(patcher, patches, slide, size), "HWLibs",
            "Failed to apply spoof patches");
    }

    if (NRed::singleton().getAttributes().isSonoma1404AndLater()) {
        const LookupPatchPlus patch {&kextRadeonX5000HWLibs, kCreatePowerTuneServices1Original1404,
            kCreatePowerTuneServices1Patched1404, 1};
        PANIC_COND(!patch.apply(patcher, slide, size), "HWLibs", "Failed to apply PowerTuneServices patch (>=14.4)");
    } else if (NRed::singleton().getAttributes().isMontereyAndLater()) {
        const LookupPatchPlus patch {&kextRadeonX5000HWLibs, kCreatePowerTuneServices1Original12,
            kCreatePowerTuneServices1Patched12, 1};
        PANIC_COND(!patch.apply(patcher, slide, size), "HWLibs", "Failed to apply PowerTuneServices patch (<14.4)");
    } else {
        const LookupPatchPlus patch {&kextRadeonX5000HWLibs, kCreatePowerTuneServices1Original,
            kCreatePowerTuneServices1Patched, 1};
        PANIC_COND(!patch.apply(patcher, slide, size), "HWLibs", "Failed to apply PowerTuneServices patch (<12.0)");
    }

    if (NRed::singleton().getAttributes().isSonoma1404AndLater()) {
        const LookupPatchPlus patches[] = {
            {&kextRadeonX5000HWLibs, kSmuInitFunctionPointerListOriginal1404,
                kSmuInitFunctionPointerListOriginalMask1404, kSmuInitFunctionPointerListPatched1404,
                kSmuInitFunctionPointerListPatchedMask1404, 1},
            {&kextRadeonX5000HWLibs, kCreatePowerTuneServices2Original1404, kCreatePowerTuneServices2Mask1404,
                kCreatePowerTuneServices2Patched1404, 1},
        };
        PANIC_COND(!LookupPatchPlus::applyAll(patcher, patches, slide, size), "HWLibs",
            "Failed to apply patches (>=14.4)");
    } else {
        const LookupPatchPlus patches[] = {
            {&kextRadeonX5000HWLibs, kSmuInitFunctionPointerListOriginal, kSmuInitFunctionPointerListOriginalMask,
                kSmuInitFunctionPointerListPatched, kSmuInitFunctionPointerListPatchedMask, 1},
            {&kextRadeonX5000HWLibs, kCreatePowerTuneServices2Original, kCreatePowerTuneServices2Mask,
                kCreatePowerTuneServices2Patched, 1},
        };
        PANIC_COND(!LookupPatchPlus::applyAll(patcher, patches, slide, size), "HWLibs",
            "Failed to apply patches (<14.4)");
    }

    if (NRed::singleton().getAttributes().isVenturaAndLater()) {
        const LookupPatchPlus patches[] = {
            {&kextRadeonX5000HWLibs, kCailQueryAdapterInfoOriginal, kCailQueryAdapterInfoPatched, 1},
            {&kextRadeonX5000HWLibs, kSDMAInitFunctionPointerListOriginal, kSDMAInitFunctionPointerListOriginalMask,
                kSDMAInitFunctionPointerListPatched, kSDMAInitFunctionPointerListPatchedMask, 1},
        };
        PANIC_COND(!LookupPatchPlus::applyAll(patcher, patches, slide, size), "HWLibs",
            "Failed to apply macOS 13.0+ patches");
    }
}

void iVega::X5000HWLibs::wrapPopulateFirmwareDirectory(void *that) {
    FunctionCast(wrapPopulateFirmwareDirectory, singleton().orgGetIpFw)(that);

    const auto &ravenFwData = getFWByName("ativvaxy_rv.dat");
    auto *ravenFw = singleton().orgCreateFirmware(ravenFwData.data, ravenFwData.length, 0x0100, "ativvaxy_rv.dat");
    PANIC_COND(ravenFw == nullptr, "HWLibs", "Failed to create Raven firmware");
    PANIC_COND(!singleton().orgPutFirmware(singleton().fwDirField.get(that), kAMDDeviceTypeNavi10, ravenFw), "HWLibs",
        "Failed to insert Raven firmware");

    const auto &renoirFwData = getFWByName("ativvaxy_nv.dat");
    auto *renoirFw = singleton().orgCreateFirmware(renoirFwData.data, renoirFwData.length, 0x0202, "ativvaxy_nv.dat");
    PANIC_COND(renoirFw == nullptr, "HWLibs", "Failed to create Renoir firmware");
    PANIC_COND(!singleton().orgPutFirmware(singleton().fwDirField.get(that), kAMDDeviceTypeNavi10, renoirFw), "HWLibs",
        "Failed to insert Renoir firmware");
}

bool iVega::X5000HWLibs::wrapGetIpFw(void *that, UInt32 ipVersion, char *name, void *out) {
    if (!strncmp(name, "ativvaxy_rv.dat", 16) || !strncmp(name, "ativvaxy_nv.dat", 16)) {
        const auto &fwDesc = getFWByName(name);
        getMember<const void *>(out, 0x0) = fwDesc.data;
        getMember<UInt32>(out, 0x8) = fwDesc.length;
        return true;
    }
    return FunctionCast(wrapGetIpFw, singleton().orgGetIpFw)(that, ipVersion, name, out);
}

CAILResult iVega::X5000HWLibs::hwLibsGeneralFailure() { return kCAILResultFailed; }
CAILResult iVega::X5000HWLibs::hwLibsUnsupported() { return kCAILResultUnsupported; }
CAILResult iVega::X5000HWLibs::hwLibsNoop() { return kCAILResultSuccess; }

CAILResult iVega::X5000HWLibs::pspBootloaderLoadSos10(void *ctx) {
    singleton().pspLoadSOSField.set(ctx, NRed::singleton().readReg32(MP_BASE + mmMP0_SMN_C2PMSG_59));
    (singleton().pspLoadSOSField + 0x4).set(ctx, NRed::singleton().readReg32(MP_BASE + mmMP0_SMN_C2PMSG_58));
    (singleton().pspLoadSOSField + 0x8).set(ctx, NRed::singleton().readReg32(MP_BASE + mmMP0_SMN_C2PMSG_58));
    return kCAILResultSuccess;
}

CAILResult iVega::X5000HWLibs::pspSecurityFeatureCapsSet10(void *ctx) {
    auto &securityCaps = singleton().pspSecurityCapsField.getRef(ctx);
    securityCaps &= ~static_cast<UInt8>(1);
    auto tOSVer = singleton().pspTOSField.get(ctx);
    if ((tOSVer & 0xFFFF0000) == 0x80000 && (tOSVer & 0xFF) > 0x50) {
        auto policyVer = NRed::singleton().readReg32(MP_BASE + mmMP0_SMN_C2PMSG_91);
        SYSLOG_COND((policyVer & 0xFF000000) != 0xA000000, "HWLibs", "Invalid security policy version: 0x%X",
            policyVer);
        if (policyVer == 0xA02031A || ((policyVer & 0xFFFFFF00) == 0xA020400 && (policyVer & 0xFC) > 0x23) ||
            ((policyVer & 0xFFFFFF00) == 0xA020300 && (policyVer & 0xFE) > 0x1D)) {
            securityCaps |= 1;
        }
    }

    return kCAILResultSuccess;
}

CAILResult iVega::X5000HWLibs::pspSecurityFeatureCapsSet12(void *ctx) {
    auto &securityCaps = singleton().pspSecurityCapsField.getRef(ctx);
    securityCaps &= ~static_cast<UInt8>(1);
    auto tOSVer = singleton().pspTOSField.get(ctx);
    if ((tOSVer & 0xFFFF0000) == 0x110000 && (tOSVer & 0xFF) > 0x2A) {
        auto policyVer = NRed::singleton().readReg32(MP_BASE + mmMP0_SMN_C2PMSG_91);
        SYSLOG_COND((policyVer & 0xFF000000) != 0xB000000, "HWLibs", "Invalid security policy version: 0x%X",
            policyVer);
        if ((policyVer & 0xFFFF0000) == 0xB090000 && (policyVer & 0xFE) > 0x35) { securityCaps |= 1; }
    }

    return kCAILResultSuccess;
}

CAILResult iVega::X5000HWLibs::wrapPspCmdKmSubmit(void *ctx, void *cmd, void *outData, void *outResponse) {
    char filename[64];
    bzero(filename, sizeof(filename));

    auto *data = singleton().pspCommandDataField.get(ctx);

    switch (getMember<AMDPSPCommand>(cmd, 0x0)) {
        case kPSPCommandLoadTA: {
            const char *name = reinterpret_cast<char *>(data + 0x8DB);
            if (!strncmp(name, "AMD DTM Application", 20)) {
                strncpy(filename, "psp_dtm.bin", 12);
                break;
            }
            if (!strncmp(name, "AMD HDCP Application", 21)) {
                strncpy(filename, "psp_hdcp.bin", 13);
                break;
            }
            if (!strncmp(name, "AMD AUC Application", 20)) {
                strncpy(filename, "psp_auc.bin", 12);
                break;
            }
            if (!strncmp(name, "AMD FP Application", 19)) {
                strncpy(filename, "psp_fp.bin", 11);
                break;
            }
            return FunctionCast(wrapPspCmdKmSubmit, singleton().orgPspCmdKmSubmit)(ctx, cmd, outData, outResponse);
        }
        case kPSPCommandLoadASD: {
            strncpy(filename, "psp_asd.bin", 12);
            break;
        }
        case kPSPCommandLoadIPFW: {
            auto *prefix = NRed::singleton().getAttributes().getGCPrefix();
            switch (getMember<AMDUCodeID>(cmd, 0x10)) {
                case kUCodeCE:
                    snprintf(filename, sizeof(filename), "%sce_ucode.bin", prefix);
                    break;
                case kUCodePFP:
                    snprintf(filename, sizeof(filename), "%spfp_ucode.bin", prefix);
                    break;
                case kUCodeME:
                    snprintf(filename, sizeof(filename), "%sme_ucode.bin", prefix);
                    break;
                case kUCodeMEC1JT:
                    snprintf(filename, sizeof(filename), "%smec_jt_ucode.bin", prefix);
                    break;
                case kUCodeMEC2JT:
                    if (NRed::singleton().getAttributes().isRenoir()) { return kCAILResultSuccess; }
                    snprintf(filename, sizeof(filename), "%smec_jt_ucode.bin", prefix);
                    break;
                case kUCodeMEC1:
                    snprintf(filename, sizeof(filename), "%smec_ucode.bin", prefix);
                    break;
                case kUCodeMEC2:
                    if (NRed::singleton().getAttributes().isRenoir()) { return kCAILResultSuccess; }
                    snprintf(filename, sizeof(filename), "%smec_ucode.bin", prefix);
                    break;
                case kUCodeRLC:
                    // Fake CGPG
                    if ((!NRed::singleton().getAttributes().isPicasso() &&
                            !NRed::singleton().getAttributes().isRaven2() &&
                            NRed::singleton().getAttributes().isRaven()) ||
                        (NRed::singleton().getAttributes().isPicasso() &&
                            ((NRed::singleton().getPciRevision() >= 0xC8 &&
                                 NRed::singleton().getPciRevision() <= 0xCC) ||
                                (NRed::singleton().getPciRevision() >= 0xD8 &&
                                    NRed::singleton().getPciRevision() <= 0xDD)))) {
                        snprintf(filename, sizeof(filename), "%srlc_fake_cgpg_ucode.bin", prefix);
                    } else {
                        snprintf(filename, sizeof(filename), "%srlc_ucode.bin", prefix);
                    }
                    break;
                case kUCodeSDMA0:
                    strncpy(filename, "sdma_4_1_ucode.bin", 19);
                    break;
                case kUCodeDMCUERAM:
                    if (NRed::singleton().getAttributes().isRenoir()) {
                        strncpy(filename, "dmcu_eram_dcn21.bin", 20);
                    } else {
                        strncpy(filename, "dmcu_eram_dcn10.bin", 20);
                    }
                    break;
                case kUCodeDMCUISR:
                    if (NRed::singleton().getAttributes().isRenoir()) {
                        strncpy(filename, "dmcu_intvectors_dcn21.bin", 26);
                    } else {
                        strncpy(filename, "dmcu_intvectors_dcn10.bin", 26);
                    }
                    break;
                case kUCodeRLCV:
                    // No RLC V on Renoir
                    if (NRed::singleton().getAttributes().isRenoir()) { return kCAILResultSuccess; }
                    snprintf(filename, sizeof(filename), "%srlcv_ucode.bin", prefix);
                    break;
                case kUCodeRLCSRListGPM:
                    snprintf(filename, sizeof(filename), "%srlc_srlist_gpm_mem.bin", prefix);
                    break;
                case kUCodeRLCSRListSRM:
                    snprintf(filename, sizeof(filename), "%srlc_srlist_srm_mem.bin", prefix);
                    break;
                case kUCodeRLCSRListCntl:
                    snprintf(filename, sizeof(filename), "%srlc_srlist_cntl.bin", prefix);
                    break;
                case kUCodeDMCUB:
                    // Just in case
                    if (NRed::singleton().getAttributes().isRenoir()) {
                        strncpy(filename, "atidmcub_rn.dat", 16);
                        break;
                    }
                    SYSLOG("HWLibs", "DMCU version B is not supposed to be loaded on this ASIC!");
                    return kCAILResultSuccess;
                default:
                    return FunctionCast(wrapPspCmdKmSubmit, singleton().orgPspCmdKmSubmit)(ctx, cmd, outData,
                        outResponse);
            }
            break;
        }
        default:
            return FunctionCast(wrapPspCmdKmSubmit, singleton().orgPspCmdKmSubmit)(ctx, cmd, outData, outResponse);
    }

    const auto &fw = getFWByName(filename);
    memcpy(data, fw.data, fw.length);
    getMember<UInt32>(cmd, 0xC) = fw.length;

    return FunctionCast(wrapPspCmdKmSubmit, singleton().orgPspCmdKmSubmit)(ctx, cmd, outData, outResponse);
}

CAILResult iVega::X5000HWLibs::smuReset() {
    // Linux has got no information on parameters of SoftReset.
    // There is no debug information nor is there debug prints in amdkmdag.sys related to this call.
    return NRed::singleton().sendMsgToSmc(PPSMC_MSG_SoftReset, 0x40);
}

CAILResult iVega::X5000HWLibs::smuPowerUp() {
    auto res = NRed::singleton().sendMsgToSmc(PPSMC_MSG_ForceGfxContentSave);
    if (res != kCAILResultSuccess && res != kCAILResultUnsupported) { return res; }

    res = NRed::singleton().sendMsgToSmc(PPSMC_MSG_PowerUpSdma);
    if (res != kCAILResultSuccess) { return res; }

    res = NRed::singleton().sendMsgToSmc(PPSMC_MSG_PowerUpGfx);
    if (res != kCAILResultSuccess) { return res; }

    res = NRed::singleton().sendMsgToSmc(PPSMC_MSG_PowerGateMmHub);
    if (res == kCAILResultUnsupported) { return kCAILResultSuccess; }

    return res;
}

CAILResult iVega::X5000HWLibs::smuInternalSwInit(void *ctx) {
    singleton().smuSwInitialisedFieldBase.set(ctx, true);
    return kCAILResultSuccess;
}

CAILResult iVega::X5000HWLibs::smu10InternalHwInit(void *) {
    auto res = smuReset();
    if (res != kCAILResultSuccess) { return res; }

    return smuPowerUp();
}

CAILResult iVega::X5000HWLibs::smu12InternalHwInit(void *) {
    UInt32 i = 0;
    for (; i < AMD_MAX_USEC_TIMEOUT; i++) {
        if (NRed::singleton().readReg32(MP1_Public | smnMP1_FIRMWARE_FLAGS) &
            smnMP1_FIRMWARE_FLAGS_INTERRUPTS_ENABLED) {
            break;
        }
        IOSleep(1);
    }
    if (i >= AMD_MAX_USEC_TIMEOUT) { return kCAILResultFailed; }

    auto res = smuReset();
    if (res != kCAILResultSuccess) { return res; }

    res = smuPowerUp();
    if (res != kCAILResultSuccess) { return res; }

    res = NRed::singleton().sendMsgToSmc(PPSMC_MSG_PowerGateAtHub);
    if (res == kCAILResultUnsupported) { return kCAILResultSuccess; }

    return res;
}

CAILResult iVega::X5000HWLibs::smuInternalHwExit(void *) { return smuReset(); }

CAILResult iVega::X5000HWLibs::smuFullAsicReset(void *, void *data) {
    return NRed::singleton().sendMsgToSmc(PPSMC_MSG_DeviceDriverReset, getMember<UInt32>(data, 4));
}

CAILResult iVega::X5000HWLibs::smu10NotifyEvent(void *, void *data) {
    switch (getMember<UInt32>(data, 4)) {
        case 0:
        case 4:
        case 10:    // Reinitialise
            return smuPowerUp();
        case 1:
        case 2:
        case 3:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:     // Reset
        case 11:    // Collect debug info
            return kCAILResultSuccess;
        default:
            SYSLOG("HWLibs", "Invalid input event to SMU notify event");
            return kCAILResultFailed;
    }
}

CAILResult iVega::X5000HWLibs::smu12NotifyEvent(void *, void *data) {
    switch (getMember<UInt32>(data, 4)) {
        case 0:
        case 4:
        case 8:
        case 10:    // Reinitialise
            return NRed::singleton().sendMsgToSmc(PPSMC_MSG_PowerUpSdma);
        case 1:
        case 2:
        case 3:
        case 5:
        case 6:
        case 7:
        case 9:     // Reset
        case 11:    // Collect debug info
            return kCAILResultSuccess;
        default:
            SYSLOG("HWLibs", "Invalid input event to SMU notify event");
            return kCAILResultFailed;
    }
}

CAILResult iVega::X5000HWLibs::smuFullScreenEvent(void *, UInt32 event) {
    switch (event) {
        case 1:
            NRed::singleton().writeReg32(MP_BASE + mmMP1_SMN_FPS_CNT,
                NRed::singleton().readReg32(MP_BASE + mmMP1_SMN_FPS_CNT) + 1);
            return kCAILResultSuccess;
        case 2:
            NRed::singleton().writeReg32(MP_BASE + mmMP1_SMN_FPS_CNT, 0);
            return kCAILResultSuccess;
        default:
            SYSLOG("HWLibs", "Invalid input event to SMU full screen event");
            return kCAILResultFailed;
    }
}

CAILResult iVega::X5000HWLibs::wrapSmu901CreateFunctionPointerList(void *ctx) {
    if (NRed::singleton().getAttributes().isCatalina()) {
        singleton().smuInternalSWInitField.set(ctx, reinterpret_cast<mach_vm_address_t>(hwLibsNoop));
    } else {
        singleton().smuInternalSWInitField.set(ctx, reinterpret_cast<mach_vm_address_t>(smuInternalSwInit));
        singleton().smuGetUCodeConstsField.set(ctx, reinterpret_cast<mach_vm_address_t>(hwLibsNoop));
    }
    singleton().smuFullscreenEventField.set(ctx, reinterpret_cast<mach_vm_address_t>(smuFullScreenEvent));
    if (NRed::singleton().getAttributes().isRenoir()) {
        singleton().smuInternalHWInitField.set(ctx, reinterpret_cast<mach_vm_address_t>(smu12InternalHwInit));
        singleton().smuNotifyEventField.set(ctx, reinterpret_cast<mach_vm_address_t>(smu12NotifyEvent));
    } else {
        singleton().smuInternalHWInitField.set(ctx, reinterpret_cast<mach_vm_address_t>(smu10InternalHwInit));
        singleton().smuNotifyEventField.set(ctx, reinterpret_cast<mach_vm_address_t>(smu10NotifyEvent));
    }
    singleton().smuInternalSWExitField.set(ctx, reinterpret_cast<mach_vm_address_t>(hwLibsNoop));
    singleton().smuInternalHWExitField.set(ctx, reinterpret_cast<mach_vm_address_t>(smuInternalHwExit));
    singleton().smuFullAsicResetField.set(ctx, reinterpret_cast<mach_vm_address_t>(smuFullAsicReset));
    return kCAILResultSuccess;
}
