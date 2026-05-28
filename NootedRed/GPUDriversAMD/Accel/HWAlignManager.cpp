// Version-independent interface to the `AMDRadeonX5000_AMDHWAlignManager` class
//
// Copyright © 2026 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#include <GPUDriversAMD/Accel/HWAlignManager.hpp>
#include <PenguinWizardry/PatcherPlus.hpp>

static const char kAddr2GetPreferredSurfaceSettingSym[] =
    "__ZNK4Addr2V23Lib31Addr2GetPreferredSurfaceSettingEPK39_ADDR2_GET_PREFERRED_SURF_SETTING_INPUTP40_ADDR2_GET_"
    "PREFERRED_SURF_SETTING_OUTPUT";
static const UInt8 kAddr2GetPreferredSurfaceSettingPattern[]    = {0x55, 0x48, 0x89, 0xE5, 0xF6, 0x47, 0x34,
                                                                   0x04, 0x74, 0x0A, 0x83, 0x3E, 0x44, 0x75,
                                                                   0x12, 0x83, 0x3A, 0x20, 0x75, 0x0D};
static const UInt8 kAddr2GetPreferredSurfaceSettingPatternNew[] = {0x55, 0x48, 0x89, 0xE5, 0xF6, 0x47, 0x34,
                                                                   0x04, 0x74, 0x0A, 0x83, 0x3E, 0x50, 0x75,
                                                                   0x12, 0x83, 0x3A, 0x20, 0x75, 0x0D};

AMDRadeonX5000_AMDHWAlignManager::Constants AMDRadeonX5000_AMDHWAlignManager::constants;

// Decompiled from AMDRadeonX6000 combined with info from mesa's AddrLib
UInt32 AMDRadeonX5000_AMDHWAlignManager::getPreferredSwizzleMode2(ADDR2_COMPUTE_SURFACE_INFO_INPUT* infoInput)
{
    ADDR2_GET_PREFERRED_SURF_SETTING_OUTPUT output;
    if (constants.addr2GetPreferredSurfaceSettingNew != nullptr) {
        ADDR2_GET_PREFERRED_SURF_SETTING_INPUT_NEW input;
        input.width              = infoInput->width;
        input.height             = infoInput->height;
        input.bpp                = infoInput->bpp;
        input.resourceType       = infoInput->resourceType;
        input.format             = infoInput->format;
        input.numSamples         = infoInput->numSamples;
        input.numSlices          = infoInput->numSlices;
        input.flags.display      = infoInput->flags.display;
        input.forbiddenBlock.var = 1;    // No variable swizzle modes
        if (constants.addr2GetPreferredSurfaceSettingNew(getMember<void*>(this, 0x18), &input, &output) == 0) {
            return output.swizzleMode;
        }
    }
    else {
        ADDR2_GET_PREFERRED_SURF_SETTING_INPUT input;
        input.width              = infoInput->width;
        input.height             = infoInput->height;
        input.bpp                = infoInput->bpp;
        input.resourceType       = infoInput->resourceType;
        input.format             = infoInput->format;
        input.numSamples         = infoInput->numSamples;
        input.numSlices          = infoInput->numSlices;
        input.flags.display      = infoInput->flags.display;
        input.forbiddenBlock.var = 1;    // No variable swizzle modes
        if (constants.addr2GetPreferredSurfaceSetting(getMember<void*>(this, 0x18), &input, &output) == 0) {
            return output.swizzleMode;
        }
    }
    return 0U;
}

void AMDRadeonX5000_AMDHWAlignManager::resolve(KernelPatcher& patcher, const size_t id, const mach_vm_address_t start,
                                               const size_t size)
{
    PenguinWizardry::PatternSolveRequest request{kAddr2GetPreferredSurfaceSettingSym,
                                                 constants.addr2GetPreferredSurfaceSetting,
                                                 kAddr2GetPreferredSurfaceSettingPattern};
    if (!request.solve(patcher, id, start, size)) {
        DBGLOG("X5000HWAlignManager",
               "Failed to resolve Addr::V2::Lib::Addr2GetPreferredSurfaceSetting via old pattern, trying new.");
        PenguinWizardry::PatternSolveRequest requestNew{kAddr2GetPreferredSurfaceSettingSym,
                                                        constants.addr2GetPreferredSurfaceSettingNew,
                                                        kAddr2GetPreferredSurfaceSettingPatternNew};
        PANIC_COND(!requestNew.solve(patcher, id, start, size), "X5000HWAlignManager",
                   "Failed to resolve Addr::V2::Lib::Addr2GetPreferredSurfaceSetting");
    }
}
