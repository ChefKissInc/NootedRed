// Copyright © 2022-2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#include <Headers/kern_api.hpp>
#include <PrivateHeaders/Firmware.hpp>
#include <PrivateHeaders/GPUDriversAMD/CAIL/ASICCaps.hpp>
#include <PrivateHeaders/GPUDriversAMD/CAIL/DevCaps.hpp>
#include <PrivateHeaders/GPUDriversAMD/CAIL/SWIP/SDMA.hpp>
#include <PrivateHeaders/GPUDriversAMD/Driver.hpp>
#include <PrivateHeaders/GPUDriversAMD/Family.hpp>
#include <PrivateHeaders/GPUDriversAMD/PSP.hpp>
#include <PrivateHeaders/NRed.hpp>
#include <PrivateHeaders/PatcherPlus.hpp>
#include <PrivateHeaders/iVega/ASICCaps.hpp>
#include <PrivateHeaders/iVega/GoldenSettings.hpp>
#include <PrivateHeaders/iVega/HWLibs.hpp>
#include <PrivateHeaders/iVega/IPOffset.hpp>
#include <PrivateHeaders/iVega/Regs/SDMA0.hpp>
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

static const UInt8 kGcSetFwEntryInfoCallPattern[] = {0x14, 0x4C, 0x89, 0xF1, 0xE8, 0x00, 0x00, 0x00, 0x00, 0x80, 0xC0};
static const UInt8 kGcSetFwEntryInfoCallPatternMask[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xF0,
    0xF0};
static constexpr size_t kGcSetFwEntryInfoCallPatternJumpInstOff = 0x4;

static const UInt8 kSdmaInitFuncPtrListCallPattern[] = {0x44, 0x89, 0x00, 0x44, 0x89, 0x00, 0xE8, 0x00, 0x00, 0x00,
    0x00, 0x48, 0x8B, 0x00, 0x00, 0x85};
static const UInt8 kSdmaInitFuncPtrListCallPatternMask[] = {0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0x00,
    0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF};
static constexpr size_t kSdmaInitFuncPtrListCallPatternJumpInstOff = 0x6;

static const UInt8 kDmcuBackdoorLoadFwBranchPattern[] = {0x8D, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x83, 0xF8, 0x02, 0x0F,
    0x82, 0x00, 0x00, 0x00, 0x00};
static const UInt8 kDmcuBackdoorLoadFwBranchPatternMask[] = {0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0x00, 0x00, 0x00, 0x00};
static constexpr size_t kDmcuBackdoorLoadFwDcn1ConstantsBranchOff = 0x9;
static constexpr size_t kDmcuGetDcn1FwConstantsCallOff = 0x24;
static constexpr size_t kDmcuGetDcn21FwConstantsCallOff = 0x4C;

static const UInt8 kSmuInitFunctionPointerListCallPattern[] = {0x49, 0x8B, 0x00, 0x0C, 0x40, 0x8B, 0x00, 0x14, 0xE8,
    0x00, 0x00, 0x00, 0x00};
static const UInt8 kSmuInitFunctionPointerListCallPatternMask[] = {0xFF, 0xFF, 0x00, 0xFF, 0xF0, 0xFF, 0x00, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00};
static constexpr size_t kSmuInitFunctionPointerListCallPatternJumpInstOff = 0x8;

//------ Patches ------//

// Replace call to `_gc_get_hw_version` with constant (0x090001).
static const UInt8 kGcSwInitOriginal[] = {0x0C, 0xE8, 0x00, 0x00, 0x00, 0x00, 0x41, 0x89, 0xC7};
static const UInt8 kGcSwInitOriginalMask[] = {0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF};
static const UInt8 kGcSwInitPatched[] = {0x00, 0xB8, 0x01, 0x00, 0x09, 0x00, 0x00, 0x00, 0x00};
static const UInt8 kGcSwInitPatchedMask[] = {0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00};

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

// Use "correct" PowerTuneServices by changing the switch statement case `0x8D` to `0x8E`.
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

//------ Module Logic ------//

static iVega::X5000HWLibs moduleInstance {};

iVega::X5000HWLibs &iVega::X5000HWLibs::singleton() { return moduleInstance; };

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
            this->gcSwFirmwareField = 0x2E8;
            this->dmcuEnablePSPFWLoadField = 0x248;
            this->dmcuABMLevelField = 0x24C;
            this->sdmaGetFwConstantsField = 0x270;
            this->sdmaStartEngineField = 0x298;
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
            this->pspSOSField = 0x3124;
            this->pspTOSVerField = 0x3128;
            this->gcSwFirmwareField = 0x2F0;
            this->dmcuEnablePSPFWLoadField = 0x248;
            this->dmcuABMLevelField = 0x24C;
            this->sdmaGetFwConstantsField = 0x270;
            this->sdmaStartEngineField = 0x298;
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
            this->pspSOSField = 0x3124;
            this->pspTOSVerField = 0x3128;
            this->gcSwFirmwareField = 0x2F0;
            this->dmcuEnablePSPFWLoadField = 0x248;
            this->dmcuABMLevelField = 0x24C;
            this->sdmaGetFwConstantsField = 0x270;
            this->sdmaStartEngineField = 0x298;
            break;
        case KernelVersion::Ventura:
            if (!NRed::singleton().getAttributes().isVentura1304Based()) {
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
                this->pspSOSField = 0x391C;
                this->pspTOSVerField = 0x3920;
                this->gcSwFirmwareField = 0x340;
                this->dmcuEnablePSPFWLoadField = 0x248;
                this->dmcuABMLevelField = 0x24C;
                this->sdmaGetFwConstantsField = 0x2C8;
                this->sdmaStartEngineField = 0x2F0;
                break;
            }
            [[fallthrough]];
        case KernelVersion::Sonoma:
        case KernelVersion::Sequoia:
        case KernelVersion::Tahoe:
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
            this->pspSOSField = 0x391C;
            this->pspTOSVerField = 0x3920;
            this->gcSwFirmwareField = 0x340;
            this->dmcuEnablePSPFWLoadField = 0x298;
            this->dmcuABMLevelField = 0x29C;
            this->sdmaGetFwConstantsField = 0x2C8;
            this->sdmaStartEngineField = 0x2F0;
            break;
        default:
            PANIC("HWLibs", "Unsupported kernel version: %d", getKernelVersion());
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

    CAILAsicCapsEntry *orgCapsTable = nullptr;
    CAILAsicCapsInitEntry *orgCapsInitTable = nullptr;
    AMDDeviceTypeEntry *orgDeviceTypeTable = nullptr;
    AMDDeviceCapabilities *orgDevCapTable = nullptr;

    if (!NRed::singleton().getAttributes().isCatalina()) {
        PatcherPlus::PatternSolveRequest solveRequests[] = {
            {"__ZL15deviceTypeTable", orgDeviceTypeTable, kDeviceTypeTablePattern},
            {"__ZN11AMDFirmware14createFirmwareEPhjjPKc", this->orgCreateFirmware, kCreateFirmwarePattern,
                kCreateFirmwarePatternMask},
            {"__ZN20AMDFirmwareDirectory11putFirmwareE16_AMD_DEVICE_TYPEP11AMDFirmware", this->orgPutFirmware,
                kPutFirmwarePattern},
        };
        PANIC_COND(!PatcherPlus::PatternSolveRequest::solveAll(patcher, id, solveRequests, slide, size), "HWLibs",
            "Failed to resolve symbols");
    }

    PatcherPlus::PatternSolveRequest solveRequests[] = {
        {"__ZL20CAIL_ASIC_CAPS_TABLE", orgCapsTable, kCailAsicCapsTableHWLibsPattern},
        {"_CAILAsicCapsInitTable", orgCapsInitTable, kCAILAsicCapsInitTablePattern},
        {"_DeviceCapabilityTbl", orgDevCapTable, kDeviceCapabilityTblPattern},
    };
    PANIC_COND(!PatcherPlus::PatternSolveRequest::solveAll(patcher, id, solveRequests, slide, size), "HWLibs",
        "Failed to resolve symbols");

    if (NRed::singleton().getAttributes().isCatalina()) {
        PatcherPlus::PatternRouteRequest request {"__ZN16AmdTtlFwServices7getIpFwEjPKcP10_TtlFwInfo", wrapGetIpFw,
            this->orgGetIpFw};
        PANIC_COND(!request.route(patcher, id, slide, size), "HWLibs", "Failed to route getIpFw");
    } else {
        // TODO: Not override the PSP 9 code.
        PatcherPlus::PatternRouteRequest requests[] = {
            {"__ZN35AMDRadeonX5000_AMDRadeonHWLibsX500025populateFirmwareDirectoryEv", wrapPopulateFirmwareDirectory,
                this->orgGetIpFw},
            {"_psp_bootloader_is_sos_running_3_1", cailGeneralFailure, kPspBootloaderIsSosRunning31Pattern},
            {"_psp_security_feature_caps_set_3_1",
                NRed::singleton().getAttributes().isRenoir() ? pspSecurityFeatureCapsSet12 :
                                                               pspSecurityFeatureCapsSet10,
                NRed::singleton().getAttributes().isVenturaAndLater() ? kPspSecurityFeatureCapsSet31Pattern13 :
                                                                        kPspSecurityFeatureCapsSet31Pattern},
        };
        PANIC_COND(!PatcherPlus::PatternRouteRequest::routeAll(patcher, id, requests, slide, size), "HWLibs",
            "Failed to route symbols (>10.15)");
        if (NRed::singleton().getAttributes().isSonoma1404AndLater()) {
            PatcherPlus::PatternRouteRequest pspRequests[] = {
                {"_psp_bootloader_load_sysdrv_3_1", cailNoop, kPspBootloaderLoadSysdrv31Pattern1404},
                {"_psp_bootloader_set_ecc_mode_3_1", cailNoop, kPspBootloaderSetEccMode31Pattern1404},
                {"_psp_bootloader_load_sos_3_1", pspBootloaderLoadSos10, kPspBootloaderLoadSos31Pattern1404},
                {"_psp_reset_3_1", cailUnsupported, kPspReset31Pattern1404},
            };
            PANIC_COND(!PatcherPlus::PatternRouteRequest::routeAll(patcher, id, pspRequests, slide, size), "HWLibs",
                "Failed to route symbols (>=14.4)");
        } else {
            PatcherPlus::PatternRouteRequest pspRequests[] = {
                {"_psp_bootloader_load_sysdrv_3_1", cailNoop, kPspBootloaderLoadSysdrv31Pattern,
                    kPspBootloaderLoadSysdrv31PatternMask},
                {"_psp_bootloader_set_ecc_mode_3_1", cailNoop, kPspBootloaderSetEccMode31Pattern},
                {"_psp_bootloader_load_sos_3_1", pspBootloaderLoadSos10, kPspBootloaderLoadSos31Pattern,
                    kPspBootloaderLoadSos31PatternMask},
                {"_psp_reset_3_1", cailUnsupported, kPspReset31Pattern},
            };
            PANIC_COND(!PatcherPlus::PatternRouteRequest::routeAll(patcher, id, pspRequests, slide, size), "HWLibs",
                "Failed to route symbols (<14.4)");
        }
    }

    if (NRed::singleton().getAttributes().isSonoma1404AndLater()) {
        PatcherPlus::PatternRouteRequest request {"_psp_cmd_km_submit", wrapPspCmdKmSubmit, this->orgPspCmdKmSubmit,
            kPspCmdKmSubmitPattern1404, kPspCmdKmSubmitPatternMask1404};
        PANIC_COND(!request.route(patcher, id, slide, size), "HWLibs", "Failed to route psp_cmd_km_submit (14.4+)");
    } else {
        PatcherPlus::PatternRouteRequest request {"_psp_cmd_km_submit", wrapPspCmdKmSubmit, this->orgPspCmdKmSubmit,
            kPspCmdKmSubmitPattern, kPspCmdKmSubmitPatternMask};
        PANIC_COND(!request.route(patcher, id, slide, size), "HWLibs", "Failed to route psp_cmd_km_submit");
    }

    PatcherPlus::JumpPatternRouteRequest fwRequests[] = {
        {"_gc_set_fw_entry_info", wrapGcSetFwEntryInfo, this->orgGcSetFwEntryInfo, kGcSetFwEntryInfoCallPattern,
            kGcSetFwEntryInfoCallPatternMask, kGcSetFwEntryInfoCallPatternJumpInstOff},
        {"_sdma_init_function_pointer_list", wrapSdmaInitFunctionPointerList, this->orgSdmaInitFunctionPointerList,
            kSdmaInitFuncPtrListCallPattern, kSdmaInitFuncPtrListCallPatternMask,
            kSdmaInitFuncPtrListCallPatternJumpInstOff},
    };
    PANIC_COND(!PatcherPlus::JumpPatternRouteRequest::routeAll(patcher, id, fwRequests, slide, size), "HWLibs",
        "Failed to route FW-related functions");

    KernelPatcher::RouteRequest dmcuFwRequests[] = {
        {nullptr, getDcn1FwConstants},
        {nullptr, getDcn21FwConstants},
    };

    dmcuFwRequests[0].from = patcher.solveSymbol(id, "_dmcu_get_dcn1_fw_constants", slide, size, true);
    dmcuFwRequests[1].from = patcher.solveSymbol(id, "_dmcu_get_dcn21_fw_constants", slide, size, true);
    if (dmcuFwRequests[0].from == 0 || dmcuFwRequests[1].from == 0) {
        auto trySolveCall = [](KernelPatcher::RouteRequest &req, const mach_vm_address_t addr,
                                const mach_vm_address_t end) {
            for (size_t off = 0; off < 2; off += 1) {
                // call ...
                // test al, al
                // je ...
                auto *data = reinterpret_cast<const UInt8 *>(addr + off);
                if (addr + off + 13 > end) { return false; }
                if (data[0] == 0xE8 && data[5] == 0x84 && data[6] == 0xC0 && data[7] == 0x0F && data[8] == 0x84) {
                    req.from = PatcherPlus::jumpInstDestination(addr + off, end);
                    return true;
                }
            }
            return false;
        };

        size_t offset = 0;
        PANIC_COND(!KernelPatcher::findPattern(kDmcuBackdoorLoadFwBranchPattern, kDmcuBackdoorLoadFwBranchPatternMask,
                       arrsize(kDmcuBackdoorLoadFwBranchPattern), reinterpret_cast<const void *>(slide), size, &offset),
            "HWLibs", "Failed to find `dmcu_backdoor_load_fw` branch pattern");
        auto branch =
            PatcherPlus::jumpInstDestination(slide + offset + kDmcuBackdoorLoadFwDcn1ConstantsBranchOff, slide + size);
        PANIC_COND(branch == 0, "HWLibs", "Failed to get `dmcu_backdoor_load_fw` branch destination");

        PANIC_COND(!trySolveCall(dmcuFwRequests[0], branch + kDmcuGetDcn1FwConstantsCallOff, slide + size), "HWLibs",
            "Failed to solve `dmcu_get_dcn1_fw_constants` via call pattern");
        PANIC_COND(!trySolveCall(dmcuFwRequests[1], slide + offset + kDmcuGetDcn21FwConstantsCallOff, slide + size),
            "HWLibs", "Failed to solve `dmcu_get_dcn21_fw_constants` via call pattern");
    }

    PANIC_COND(!patcher.routeMultiple(id, dmcuFwRequests, slide, size), "HWLibs",
        "Failed to route DMCU FW-related functions");

    PatcherPlus::JumpPatternRouteRequest smuRequest {"_smu_init_function_pointer_list", wrapSmuInitFunctionPointerList,
        this->orgSmuInitFunctionPointerList, kSmuInitFunctionPointerListCallPattern,
        kSmuInitFunctionPointerListCallPatternMask, kSmuInitFunctionPointerListCallPatternJumpInstOff};
    PANIC_COND(!smuRequest.route(patcher, id, slide, size), "HWLibs",
        "Failed to route `smu_init_function_pointer_list`");

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
    for (; orgDevCapTable->familyId != 0; orgDevCapTable++) {
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

    // TODO: Replace this hack with a simple hook.
    if (NRed::singleton().getAttributes().isCatalina()) {
        if (NRed::singleton().getAttributes().isRenoir()) {
            const PatcherPlus::MaskedLookupPatch patches[] = {
                {&kextRadeonX5000HWLibs, kPspSwInit1Original1015, kPspSwInit1Patched1015, 1},
                {&kextRadeonX5000HWLibs, kPspSwInit2Original1015, kPspSwInit2OriginalMask1015, kPspSwInit2Patched1015,
                    1},
            };
            PANIC_COND(!PatcherPlus::MaskedLookupPatch::applyAll(patcher, patches, slide, size), "HWLibs",
                "Failed to apply spoof patches");
        }
    } else {
        const PatcherPlus::MaskedLookupPatch patch {&kextRadeonX5000HWLibs, kGcSwInitOriginal, kGcSwInitOriginalMask,
            kGcSwInitPatched, kGcSwInitPatchedMask, 1};
        PANIC_COND(!patch.apply(patcher, slide, size), "HWLibs", "Failed to apply gc_sw_init spoof patch");
        if (NRed::singleton().getAttributes().isSonoma1404AndLater()) {
            const PatcherPlus::MaskedLookupPatch patches[] = {
                {&kextRadeonX5000HWLibs, kPspSwInit1Original1404, kPspSwInit1Patched1404, 1},
                {&kextRadeonX5000HWLibs, kPspSwInit2Original1404, kPspSwInit2OriginalMask1404, kPspSwInit2Patched1404,
                    1},
            };
            PANIC_COND(!PatcherPlus::MaskedLookupPatch::applyAll(patcher, patches, slide, size), "HWLibs",
                "Failed to apply spoof patches (>=14.4)");
        } else {
            const PatcherPlus::MaskedLookupPatch patches[] = {
                {&kextRadeonX5000HWLibs, kPspSwInit1Original, kPspSwInit1Patched, 1},
                {&kextRadeonX5000HWLibs, kPspSwInit2Original, kPspSwInit2OriginalMask, kPspSwInit2Patched, 1},
            };
            PANIC_COND(!PatcherPlus::MaskedLookupPatch::applyAll(patcher, patches, slide, size), "HWLibs",
                "Failed to apply spoof patches (<14.4)");
        }
    }

    // TODO: Replace this hack with a simple hook.
    if (NRed::singleton().getAttributes().isSonoma1404AndLater()) {
        const PatcherPlus::MaskedLookupPatch patches[] = {
            {&kextRadeonX5000HWLibs, kCreatePowerTuneServices1Original1404, kCreatePowerTuneServices1Patched1404, 1},
            {&kextRadeonX5000HWLibs, kCreatePowerTuneServices2Original1404, kCreatePowerTuneServices2Mask1404,
                kCreatePowerTuneServices2Patched1404, 1},
        };
        PANIC_COND(!PatcherPlus::MaskedLookupPatch::applyAll(patcher, patches, slide, size), "HWLibs",
            "Failed to apply PowerTuneServices patches (>=14.4)");
    } else {
        if (NRed::singleton().getAttributes().isMontereyAndLater()) {
            const PatcherPlus::MaskedLookupPatch patch {&kextRadeonX5000HWLibs, kCreatePowerTuneServices1Original12,
                kCreatePowerTuneServices1Patched12, 1};
            PANIC_COND(!patch.apply(patcher, slide, size), "HWLibs", "Failed to apply PowerTuneServices patch (<14.4)");
        } else {
            const PatcherPlus::MaskedLookupPatch patch {&kextRadeonX5000HWLibs, kCreatePowerTuneServices1Original,
                kCreatePowerTuneServices1Patched, 1};
            PANIC_COND(!patch.apply(patcher, slide, size), "HWLibs", "Failed to apply PowerTuneServices patch (<12.0)");
        }
        const PatcherPlus::MaskedLookupPatch patch {&kextRadeonX5000HWLibs, kCreatePowerTuneServices2Original,
            kCreatePowerTuneServices2Mask, kCreatePowerTuneServices2Patched, 1};
        PANIC_COND(!patch.apply(patcher, slide, size), "HWLibs", "Failed to apply PowerTune patch (<14.4)");
    }

    if (NRed::singleton().getAttributes().isVenturaAndLater()) {
        const PatcherPlus::MaskedLookupPatch patch {&kextRadeonX5000HWLibs, kCailQueryAdapterInfoOriginal,
            kCailQueryAdapterInfoPatched, 1};
        PANIC_COND(!patch.apply(patcher, slide, size), "HWLibs", "Failed to apply CailQueryAdapterInfo patch");
    }
}

// Taking advantage of the fact device type "Navi 10" is not used in the original function.
void iVega::X5000HWLibs::wrapPopulateFirmwareDirectory(void *that) {
    FunctionCast(wrapPopulateFirmwareDirectory, singleton().orgGetIpFw)(that);

    auto *fwDir = singleton().fwDirField.get(that);
    assert(fwDir != nullptr);

    const auto &ravenFwData = getFirmwareNamed("ativvaxy_rv.dat");
    auto *ravenFw = singleton().orgCreateFirmware(ravenFwData.data, ravenFwData.length, 0x0100, "ativvaxy_rv.dat");
    assertf(ravenFw != nullptr, "Failed to create VCN 1.0 firmware");
    singleton().orgPutFirmware(fwDir, kAMDDeviceTypeNavi10, ravenFw);

    const auto &renoirFwData = getFirmwareNamed("ativvaxy_nv.dat");
    auto *renoirFw = singleton().orgCreateFirmware(renoirFwData.data, renoirFwData.length, 0x0202, "ativvaxy_nv.dat");
    assertf(renoirFw != nullptr, "Failed to create VCN 2.2 firmware");
    singleton().orgPutFirmware(fwDir, kAMDDeviceTypeNavi10, renoirFw);

    const auto &dmcubFwData = getFirmwareNamed("atidmcub_rn.dat");
    auto *dmcubFw = singleton().orgCreateFirmware(dmcubFwData.data, dmcubFwData.length, 0x0201, "atidmcub_0.dat");
    assertf(dmcubFw != nullptr, "Failed to create DMCUB firmware");
    singleton().orgPutFirmware(fwDir, kAMDDeviceTypeNavi10, dmcubFw);
}

bool iVega::X5000HWLibs::wrapGetIpFw(void *that, UInt32 ipVersion, const char *name, void *out) {
    if (strncmp(name, "ativvaxy_rv.dat", 16) == 0 || strncmp(name, "ativvaxy_nv.dat", 16) == 0) {
        const auto &fwDesc = getFirmwareNamed(name);
        getMember<const void *>(out, 0x0) = fwDesc.data;
        getMember<UInt32>(out, 0x8) = fwDesc.length;
        return true;
    }
    // This is fine because there won't be any other DCN ASICs in X5000HWLibs
    if (strncmp(name, "atidmcub_0.dat", 15) == 0) {
        const auto &fwDesc = getFirmwareNamed("atidmcub_rn.dat");
        getMember<const void *>(out, 0x0) = fwDesc.data;
        getMember<UInt32>(out, 0x8) = fwDesc.length;
        return true;
    }
    return FunctionCast(wrapGetIpFw, singleton().orgGetIpFw)(that, ipVersion, name, out);
}

CAILResult iVega::X5000HWLibs::cailGeneralFailure() { return kCAILResultInvalidParameters; }
CAILResult iVega::X5000HWLibs::cailUnsupported() { return kCAILResultUnsupported; }
CAILResult iVega::X5000HWLibs::cailNoop() { return kCAILResultOK; }

CAILResult iVega::X5000HWLibs::pspBootloaderLoadSos10(void *instance) {
    singleton().pspSOSField.set(instance, NRed::singleton().readReg32(MP0_BASE_0 + MP0_SMN_C2PMSG_59));
    (singleton().pspSOSField + 0x4).set(instance, NRed::singleton().readReg32(MP0_BASE_0 + MP0_SMN_C2PMSG_58));
    (singleton().pspSOSField + 0x8).set(instance, NRed::singleton().readReg32(MP0_BASE_0 + MP0_SMN_C2PMSG_58));
    return kCAILResultOK;
}

CAILResult iVega::X5000HWLibs::pspSecurityFeatureCapsSet10(void *instance) {
    auto &securityCaps = singleton().pspSecurityCapsField.get(instance);
    securityCaps &= ~static_cast<UInt8>(1);
    auto tOSVer = singleton().pspTOSVerField.get(instance);
    if ((tOSVer & 0xFFFF0000) == 0x80000 && (tOSVer & 0xFF) > 0x50) {
        auto policyVer = NRed::singleton().readReg32(MP0_BASE_0 + MP0_SMN_C2PMSG_91);
        SYSLOG_COND((policyVer & 0xFF000000) != 0xA000000, "HWLibs", "Invalid security policy version: 0x%X",
            policyVer);
        if (policyVer == 0xA02031A || ((policyVer & 0xFFFFFF00) == 0xA020400 && (policyVer & 0xFC) > 0x23) ||
            ((policyVer & 0xFFFFFF00) == 0xA020300 && (policyVer & 0xFE) > 0x1D)) {
            securityCaps |= 1;
        }
    }

    return kCAILResultOK;
}

CAILResult iVega::X5000HWLibs::pspSecurityFeatureCapsSet12(void *instance) {
    auto &securityCaps = singleton().pspSecurityCapsField.get(instance);
    securityCaps &= ~static_cast<UInt8>(1);
    auto tOSVer = singleton().pspTOSVerField.get(instance);
    if ((tOSVer & 0xFFFF0000) == 0x110000 && (tOSVer & 0xFF) > 0x2A) {
        auto policyVer = NRed::singleton().readReg32(MP0_BASE_0 + MP0_SMN_C2PMSG_91);
        SYSLOG_COND((policyVer & 0xFF000000) != 0xB000000, "HWLibs", "Invalid security policy version: 0x%X",
            policyVer);
        if ((policyVer & 0xFFFF0000) == 0xB090000 && (policyVer & 0xFE) > 0x35) { securityCaps |= 1; }
    }

    return kCAILResultOK;
}

CAILResult iVega::X5000HWLibs::wrapPspCmdKmSubmit(void *instance, void *cmd, void *outData, void *outResponse) {
    char filename[64];
    bzero(filename, sizeof(filename));

    auto *data = singleton().pspCommandDataField.get(instance);

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
            return FunctionCast(wrapPspCmdKmSubmit, singleton().orgPspCmdKmSubmit)(instance, cmd, outData, outResponse);
        }
        case kPSPCommandLoadASD: {
            strncpy(filename, "psp_asd.bin", 12);
            break;
        }
        default:
            return FunctionCast(wrapPspCmdKmSubmit, singleton().orgPspCmdKmSubmit)(instance, cmd, outData, outResponse);
    }

    const auto &fw = getFirmwareNamed(filename);
    memcpy(data, fw.data, fw.length);
    getMember<UInt32>(cmd, 0xC) = fw.length;

    return FunctionCast(wrapPspCmdKmSubmit, singleton().orgPspCmdKmSubmit)(instance, cmd, outData, outResponse);
}

CAILResult iVega::X5000HWLibs::smuReset() {
    // Linux has got no information on parameters of SoftReset.
    // There is no debug information nor is there debug prints in amdkmdag.sys related to this call.
    // Anyone wants to reverse engineer the SMU firmware and find what it means?
    return NRed::singleton().sendMsgToSmc(PPSMC_MSG_SoftReset, 0x40);
}

CAILResult iVega::X5000HWLibs::smuPowerUp() {
    auto res = NRed::singleton().sendMsgToSmc(PPSMC_MSG_ForceGfxContentSave);
    if (res != kCAILResultOK && res != kCAILResultUnsupported) { return res; }

    res = NRed::singleton().sendMsgToSmc(PPSMC_MSG_PowerUpSdma);
    if (res != kCAILResultOK) { return res; }

    res = NRed::singleton().sendMsgToSmc(PPSMC_MSG_PowerUpGfx);
    if (res != kCAILResultOK) { return res; }

    res = NRed::singleton().sendMsgToSmc(PPSMC_MSG_PowerGateMmHub);
    if (res == kCAILResultUnsupported) { return kCAILResultOK; }

    return res;
}

CAILResult iVega::X5000HWLibs::smuInternalSwInit(void *instance) {
    singleton().smuSwInitialisedFieldBase.set(instance, true);
    return kCAILResultOK;
}

CAILResult iVega::X5000HWLibs::smu10InternalHwInit(void *) {
    auto res = smuReset();
    if (res != kCAILResultOK) { return res; }

    return smuPowerUp();
}

CAILResult iVega::X5000HWLibs::smu12InternalHwInit(void *) {
    UInt32 i = 0;
    for (; i < AMD_MAX_USEC_TIMEOUT; i++) {
        if (NRed::singleton().readReg32(MP1_PUBLIC | MP1_FIRMWARE_FLAGS) & MP1_FIRMWARE_FLAGS_INTERRUPTS_ENABLED) {
            break;
        }
        IOSleep(1);
    }
    if (i >= AMD_MAX_USEC_TIMEOUT) { return kCAILResultInvalidParameters; }

    auto res = smuReset();
    if (res != kCAILResultOK) { return res; }

    res = smuPowerUp();
    if (res != kCAILResultOK) { return res; }

    res = NRed::singleton().sendMsgToSmc(PPSMC_MSG_PowerGateAtHub);
    if (res == kCAILResultUnsupported) { return kCAILResultOK; }

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
            return kCAILResultOK;
        default:
            SYSLOG("HWLibs", "Invalid input event to SMU notify event");
            return kCAILResultInvalidParameters;
    }
}

CAILResult iVega::X5000HWLibs::smu12NotifyEvent(void *, void *data) {
    auto event = getMember<UInt32>(data, 4);
    switch (event) {
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
            return kCAILResultOK;
        default:
            SYSLOG("HWLibs", "Invalid input event to SMU notify event: %d", event);
            return kCAILResultInvalidParameters;
    }
}

CAILResult iVega::X5000HWLibs::smuFullScreenEvent(void *, UInt32 event) {
    switch (event) {
        case 1:
            NRed::singleton().writeReg32(MP0_BASE_0 + MP1_SMN_FPS_CNT,
                NRed::singleton().readReg32(MP0_BASE_0 + MP1_SMN_FPS_CNT) + 1);
            return kCAILResultOK;
        case 2:
            NRed::singleton().writeReg32(MP0_BASE_0 + MP1_SMN_FPS_CNT, 0);
            return kCAILResultOK;
        default:
            SYSLOG("HWLibs", "Invalid input event to SMU full screen event: %d", event);
            return kCAILResultInvalidParameters;
    }
}

CAILResult iVega::X5000HWLibs::wrapSmuInitFunctionPointerList(void *instance, SWIPIPVersion ipVersion) {
    DBGLOG("HWLibs", "Entered `smu_init_function_pointer_list` wrap (ipVersion: {%d,%d,%d}).", ipVersion.major,
        ipVersion.minor, ipVersion.patch);

    auto ret =
        FunctionCast(wrapSmuInitFunctionPointerList, singleton().orgSmuInitFunctionPointerList)(instance, ipVersion);
    if (ret == kCAILResultOK) { return ret; }

    switch (ipVersion.major) {
        case 10:
            singleton().smuInternalHWInitField.set(instance, reinterpret_cast<void *>(smu10InternalHwInit));
            singleton().smuNotifyEventField.set(instance, reinterpret_cast<void *>(smu10NotifyEvent));
            break;
        case 12:
            singleton().smuInternalHWInitField.set(instance, reinterpret_cast<void *>(smu12InternalHwInit));
            singleton().smuNotifyEventField.set(instance, reinterpret_cast<void *>(smu12NotifyEvent));
            break;
        default:
            DBGLOG("HWLibs", "Early exiting `smu_init_function_pointer_list` wrap with result %d.", ret);
            return ret;
    }

    if (NRed::singleton().getAttributes().isCatalina()) {
        singleton().smuInternalSWInitField.set(instance, reinterpret_cast<void *>(cailNoop));
    } else {
        singleton().smuInternalSWInitField.set(instance, reinterpret_cast<void *>(smuInternalSwInit));
        singleton().smuGetUCodeConstsField.set(instance, reinterpret_cast<void *>(cailNoop));
    }
    singleton().smuFullscreenEventField.set(instance, reinterpret_cast<void *>(smuFullScreenEvent));
    singleton().smuInternalSWExitField.set(instance, reinterpret_cast<void *>(cailNoop));
    singleton().smuInternalHWExitField.set(instance, reinterpret_cast<void *>(smuInternalHwExit));
    singleton().smuFullAsicResetField.set(instance, reinterpret_cast<void *>(smuFullAsicReset));

    SYSLOG_COND(ADDPR(debugEnabled), "HWLibs", "Ignore error about unsupported SMU HW version.");

    DBGLOG("HWLibs", "Exiting `smu_init_function_pointer_list` wrap.");
    return kCAILResultOK;
}

// TODO: Triple check if this is actually correct.
static bool isAsicA0() {
    return (!NRed::singleton().getAttributes().isPicasso() && !NRed::singleton().getAttributes().isRaven2() &&
               NRed::singleton().getAttributes().isRaven()) ||
           (NRed::singleton().getAttributes().isPicasso() &&
               ((NRed::singleton().getPciRevision() >= 0xC8 && NRed::singleton().getPciRevision() <= 0xCC) ||
                   (NRed::singleton().getPciRevision() >= 0xD8 && NRed::singleton().getPciRevision() <= 0xDD)));
}

// Actual code creates an `IOMemoryDescriptor` and does nothing with it.
// I ran into issues so I just gave it a random OSObject it can call `release` on.
static inline void *allocMemHandle() { return OSBoolean::withBoolean(false); }

static inline void setGCFWData(void *instance, GCFirmwareInfo *fwData, GCFirmwareType i, const char *filename) {
    DBGLOG("HWLibs", "Inserting GC firmware `%s` into index %d", filename, i);

    const auto &fwMeta = getFirmwareNamed(filename);
    assertf(fwMeta.extra != nullptr, "Extra info for firmware `%s` is missing.", filename);

    fwData->entry[i] = static_cast<const GCFirmwareConstant *>(fwMeta.extra);
    fwData->handle[i] = allocMemHandle();
    assertf(fwData->handle[i] != nullptr, "Failed to create memory handle!");

    getMember<void *[]>(instance, 0x18)[i] = fwData->handle[i];
    fwData->count += 1;
}

void iVega::X5000HWLibs::gc91GetFwConstants(void *instance, GCFirmwareInfo *fwData) {
    setGCFWData(instance, fwData, kGCFirmwareTypeRLCSRListCntl, "gc_9_1_rlc_srlist_cntl.bin");
    setGCFWData(instance, fwData, kGCFirmwareTypeRLCSRListGPMMem, "gc_9_1_rlc_srlist_gpm_mem.bin");
    setGCFWData(instance, fwData, kGCFirmwareTypeRLCSRListSRMMem, "gc_9_1_rlc_srlist_srm_mem.bin");
    setGCFWData(instance, fwData, kGCFirmwareTypeRLC, isAsicA0() ? "gc_9_1_rlc_ucode_a0.bin" : "gc_9_1_rlc_ucode.bin");
    setGCFWData(instance, fwData, kGCFirmwareTypeME, "gc_9_1_me_ucode.bin");
    setGCFWData(instance, fwData, kGCFirmwareTypeCE, "gc_9_1_ce_ucode.bin");
    setGCFWData(instance, fwData, kGCFirmwareTypePFP, "gc_9_1_pfp_ucode.bin");
    setGCFWData(instance, fwData, kGCFirmwareTypeMEC1, "gc_9_1_mec_ucode.bin");
    setGCFWData(instance, fwData, kGCFirmwareTypeMECJT1, "gc_9_1_mec_jt_ucode.bin");
    // AMD: Yes, reuse that shit! Why would we waste a couple of bytes? It's not like we're wasting hundreds of MBs
    // already from the duplicate firmware files.
    fwData->entry[kGCFirmwareTypeMECJT2] = fwData->entry[kGCFirmwareTypeMECJT1];
    fwData->handle[kGCFirmwareTypeMECJT2] = fwData->handle[kGCFirmwareTypeMECJT1];
    fwData->count += 1;
    setGCFWData(instance, fwData, kGCFirmwareTypeRLCV, "gc_9_1_rlcv_ucode.bin");
}

void iVega::X5000HWLibs::gc92GetFwConstants(void *instance, GCFirmwareInfo *fwData) {
    setGCFWData(instance, fwData, kGCFirmwareTypeRLCSRListCntl, "gc_9_2_rlc_srlist_cntl.bin");
    setGCFWData(instance, fwData, kGCFirmwareTypeRLCSRListGPMMem, "gc_9_2_rlc_srlist_gpm_mem.bin");
    setGCFWData(instance, fwData, kGCFirmwareTypeRLCSRListSRMMem, "gc_9_2_rlc_srlist_srm_mem.bin");
    setGCFWData(instance, fwData, kGCFirmwareTypeRLC, isAsicA0() ? "gc_9_2_rlc_ucode_a0.bin" : "gc_9_2_rlc_ucode.bin");
    setGCFWData(instance, fwData, kGCFirmwareTypeME, "gc_9_2_me_ucode.bin");
    setGCFWData(instance, fwData, kGCFirmwareTypeCE, "gc_9_2_ce_ucode.bin");
    setGCFWData(instance, fwData, kGCFirmwareTypePFP, "gc_9_2_pfp_ucode.bin");
    setGCFWData(instance, fwData, kGCFirmwareTypeMEC1, "gc_9_2_mec_ucode.bin");
    setGCFWData(instance, fwData, kGCFirmwareTypeMECJT1, "gc_9_2_mec_jt_ucode.bin");
    fwData->entry[kGCFirmwareTypeMECJT2] = fwData->entry[kGCFirmwareTypeMECJT1];
    fwData->handle[kGCFirmwareTypeMECJT2] = fwData->handle[kGCFirmwareTypeMECJT1];
    fwData->count += 1;
    setGCFWData(instance, fwData, kGCFirmwareTypeRLCV, "gc_9_2_rlcv_ucode.bin");
}

void iVega::X5000HWLibs::gc93GetFwConstants(void *instance, GCFirmwareInfo *fwData) {
    setGCFWData(instance, fwData, kGCFirmwareTypeRLCSRListCntl, "gc_9_3_rlc_srlist_cntl.bin");
    setGCFWData(instance, fwData, kGCFirmwareTypeRLCSRListGPMMem, "gc_9_3_rlc_srlist_gpm_mem.bin");
    setGCFWData(instance, fwData, kGCFirmwareTypeRLCSRListSRMMem, "gc_9_3_rlc_srlist_srm_mem.bin");
    setGCFWData(instance, fwData, kGCFirmwareTypeRLC, "gc_9_3_rlc_ucode.bin");
    setGCFWData(instance, fwData, kGCFirmwareTypeME, "gc_9_3_me_ucode.bin");
    setGCFWData(instance, fwData, kGCFirmwareTypeCE, "gc_9_3_ce_ucode.bin");
    setGCFWData(instance, fwData, kGCFirmwareTypePFP, "gc_9_3_pfp_ucode.bin");
    setGCFWData(instance, fwData, kGCFirmwareTypeMEC1, "gc_9_3_mec_ucode.bin");
    setGCFWData(instance, fwData, kGCFirmwareTypeMECJT1, "gc_9_3_mec_jt_ucode.bin");
}

// Port of `*_char_to_int` from HWLibs.
static inline UInt32 charToInt(const char *str, size_t len) {
    if (str == nullptr || len == 0) { return 0; }
    UInt32 ret = 0;
    while (len > 0) {
        char c = *str;
        if (c < '0' || c > '9') {
            ret = 0;
        } else {
            ret *= 10;
            ret += static_cast<UInt32>(c - '0');
        }
        str += 1;
        len -= 1;
    }
    return ret;
}

static inline constexpr UInt32 gcGetHWVersion(SWIPIPVersion ipVersion) {
    return (ipVersion.major << 16) | (ipVersion.minor << 8) | ipVersion.patch;
}

void iVega::X5000HWLibs::processGCFWEntries(void *instance, void *initData) {
    DBGLOG("HWLibs", "Processing GC firmware entries.");
    auto &fwInfo = singleton().gcSwFirmwareField.get(instance);
    auto &fwEntries = getMember<GCFirmwareEntry[kGCFirmwareTypeCount]>(initData, 0x18);
    for (UInt32 i = 0, swIndex = 0; i < kGCFirmwareTypeCount; i++) {
        if (fwInfo.entry[i] == nullptr) { continue; }

        fwEntries[swIndex].valid = true;
        fwEntries[swIndex].id = static_cast<GCFirmwareType>(i);
        fwEntries[swIndex].rom = fwInfo.entry[i]->rom;
        fwEntries[swIndex].romSize = fwInfo.entry[i]->romSize;
        fwEntries[swIndex].handle = fwInfo.handle[i];
        fwEntries[swIndex].payloadOff = (i == kGCFirmwareTypeMEC1 || i == kGCFirmwareTypeMEC2) ? 0x1000 : 0x0;
        fwEntries[swIndex].version = charToInt(fwInfo.entry[i]->version, strlen(fwInfo.entry[i]->version));
        fwEntries[swIndex].field24 = fwInfo.entry[i]->field8;
        DBGLOG("HWLibs", "Found firmware at %d with ROM size 0x%X, version %d.", i, fwEntries[swIndex].romSize,
            fwEntries[swIndex].version);
        swIndex += 1;
        if (swIndex == fwInfo.count) { break; }
    }
    getMember<UInt32>(initData, 0x10) = fwInfo.count;
}

CAILResult iVega::X5000HWLibs::wrapGcSetFwEntryInfo(void *instance, SWIPIPVersion ipVersion, void *initData) {
    auto hwVersion = gcGetHWVersion(ipVersion);
    DBGLOG("HWLibs", "Entered `gc_set_fw_entry_info` (hwVersion: 0x%X).", hwVersion);
    auto *fwInfo = &singleton().gcSwFirmwareField.get(instance);
    fwInfo->count = 0;
    switch (hwVersion) {
        case gcGetHWVersion({9, 1, 0}):
            gc91GetFwConstants(instance, fwInfo);
            break;
        case gcGetHWVersion({9, 2, 0}):
            gc92GetFwConstants(instance, fwInfo);
            break;
        case gcGetHWVersion({9, 3, 0}):
            gc93GetFwConstants(instance, fwInfo);
            break;
        default:
            DBGLOG("HWLibs", "Exiting `gc_set_fw_entry_info` wrap to original logic.");
            return FunctionCast(wrapGcSetFwEntryInfo, singleton().orgGcSetFwEntryInfo)(instance, ipVersion, initData);
    }
    processGCFWEntries(instance, initData);
    DBGLOG("HWLibs", "Exiting `gc_set_fw_entry_info` wrap.");
    return kCAILResultOK;
}

static inline void setDMCUFWData(void *instance, DMCUFirmwareInfo *fwData, DMCUFirmwareType i, const char *filename) {
    DBGLOG("HWLibs", "Inserting DMCU firmware `%s` into index %d", filename, i);

    const auto &fwMeta = getFirmwareNamed(filename);
    assertf(fwMeta.extra != nullptr, "Extra info for firmware `%s` is missing.", filename);

    const auto *fwEntry = static_cast<const DMCUFirmwareConstant *>(fwMeta.extra);

    fwData->entry[i].loadAddress = fwEntry->loadAddress;
    fwData->entry[i].romSize = fwEntry->romSize;
    fwData->entry[i].rom = fwEntry->rom;
    fwData->entry[i].handle = allocMemHandle();
    assertf(fwData->entry[i].handle != nullptr, "Failed to create memory handle!");
    getMember<void *[]>(instance, 0x18)[i] = fwData->entry[i].handle;

    fwData->count += 1;
}

bool iVega::X5000HWLibs::getDcn1FwConstants(void *instance, DMCUFirmwareInfo *fwData) {
    DBGLOG("HWLibs", "Entered `get_dcn1_fw_constants` wrap.");
    auto enablePSPFWLoad = singleton().dmcuEnablePSPFWLoadField.get(instance);
    DBGLOG("HWLibs", "EnablePSPFWLoad = 0x%X", enablePSPFWLoad);
    if (enablePSPFWLoad == 2) { return true; }

    fwData->count = 0;

    auto abmLevel = singleton().dmcuABMLevelField.get(instance);
    DBGLOG("HWLibs", "ABM Level = 0x%X", abmLevel);
    switch (abmLevel) {
        case 0:
            setDMCUFWData(instance, fwData, kDMCUFirmwareTypeERAM, "dmcu_eram_dcn10_abm_2_1.bin");
            setDMCUFWData(instance, fwData, kDMCUFirmwareTypeISR, "dmcu_intvectors_dcn10_abm_2_1.bin");
            break;
        case 1:
            setDMCUFWData(instance, fwData, kDMCUFirmwareTypeERAM, "dmcu_eram_dcn10_abm_2_2.bin");
            setDMCUFWData(instance, fwData, kDMCUFirmwareTypeISR, "dmcu_intvectors_dcn10_abm_2_2.bin");
            break;
        case 2:
            setDMCUFWData(instance, fwData, kDMCUFirmwareTypeERAM, "dmcu_eram_dcn10_abm_2_3.bin");
            setDMCUFWData(instance, fwData, kDMCUFirmwareTypeISR, "dmcu_intvectors_dcn10_abm_2_3.bin");
            break;
        default:
            SYSLOG("HWLibs", "Invalid ABM Level (0x%X) for DCN 1!", abmLevel);
            return false;
    }

    DBGLOG("HWLibs", "Exiting `get_dcn1_fw_constants` wrap.");
    return true;
}

bool iVega::X5000HWLibs::getDcn21FwConstants(void *instance, DMCUFirmwareInfo *fwData) {
    DBGLOG("HWLibs", "Entered `get_dcn21_fw_constants` wrap.");
    auto enablePSPFWLoad = singleton().dmcuEnablePSPFWLoadField.get(instance);
    DBGLOG("HWLibs", "EnablePSPFWLoad = 0x%X", enablePSPFWLoad);
    if (enablePSPFWLoad == 2) { return true; }

    fwData->count = 0;

    auto abmLevel = singleton().dmcuABMLevelField.get(instance);
    DBGLOG("HWLibs", "ABM Level = 0x%X", abmLevel);
    switch (abmLevel) {
        case 0:
            setDMCUFWData(instance, fwData, kDMCUFirmwareTypeERAM, "dmcu_eram_dcn21_abm_2_1.bin");
            setDMCUFWData(instance, fwData, kDMCUFirmwareTypeISR, "dmcu_intvectors_dcn21_abm_2_1.bin");
            break;
        case 1:
            setDMCUFWData(instance, fwData, kDMCUFirmwareTypeERAM, "dmcu_eram_dcn21_abm_2_2.bin");
            setDMCUFWData(instance, fwData, kDMCUFirmwareTypeISR, "dmcu_intvectors_dcn21_abm_2_2.bin");
            break;
        case 2:
            setDMCUFWData(instance, fwData, kDMCUFirmwareTypeERAM, "dmcu_eram_dcn21_abm_2_3.bin");
            setDMCUFWData(instance, fwData, kDMCUFirmwareTypeISR, "dmcu_intvectors_dcn21_abm_2_3.bin");
            break;
        case 3:
            setDMCUFWData(instance, fwData, kDMCUFirmwareTypeERAM, "dmcu_eram_dcn21_abm_2_4.bin");
            setDMCUFWData(instance, fwData, kDMCUFirmwareTypeISR, "dmcu_intvectors_dcn21_abm_2_4.bin");
            break;
        default:
            SYSLOG("HWLibs", "Invalid ABM Level (0x%X) for DCN 2.1!", abmLevel);
            return false;
    }

    DBGLOG("HWLibs", "Exiting `get_dcn21_fw_constants` wrap.");
    return true;
}

static bool sdma41GetFWConstants(void *, const SDMAFWConstant **out) {
    const auto &fw = getFirmwareNamed("sdma_4_1_ucode.bin");
    assertf(fw.extra != nullptr, "SDMA 4.1 entry is missing extra metadata!");
    *out = static_cast<const SDMAFWConstant *>(fw.extra);
    return true;
}

// TODO: Use driver WReg/RReg instead.
static bool sdma412StartEngine(void *) {
    NRed::singleton().writeReg32(SDMA0_BASE_0 + SDMA0_F32_CNTL,
        NRed::singleton().readReg32(SDMA0_BASE_0 + SDMA0_F32_CNTL) & ~SDMA0_F32_CNTL_HALT);
    return true;
}

static inline constexpr UInt32 sdmaGetHWVersion(UInt32 major, UInt32 minor) { return minor | (major << 16); }

CAILResult iVega::X5000HWLibs::wrapSdmaInitFunctionPointerList(void *instance, UInt32 verMajor, UInt32 verMinor,
    UInt32 verPatch) {
    switch (sdmaGetHWVersion(verMajor, verMinor)) {
        case sdmaGetHWVersion(4, 1):
            singleton().sdmaGetFwConstantsField.set(instance, sdma41GetFWConstants);
            if (verPatch == 2) { singleton().sdmaStartEngineField.set(instance, sdma412StartEngine); }
            break;
        default:
            return FunctionCast(wrapSdmaInitFunctionPointerList, singleton().orgSdmaInitFunctionPointerList)(instance,
                verMajor, verMinor, verPatch);
    }
    return kCAILResultOK;
}
