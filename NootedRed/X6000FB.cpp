// AMDRadeonX6000Framebuffer Patches
//
// Copyright © 2022-2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#include "AmdAtomVramInfoIGP.hpp"
#include <ASICCaps.hpp>
#include <GPUDriversAMD/ATOMBIOS.hpp>
#include <GPUDriversAMD/CAIL/ASICCaps.hpp>
#include <GPUDriversAMD/FB/AmdAsicInfo.hpp>
#include <GPUDriversAMD/FB/AmdDeviceMemoryManager.hpp>
#include <GPUDriversAMD/FB/VidMemType.hpp>
#include <GPUDriversAMD/Family.hpp>
#include <GPUDriversAMD/RavenIPOffset.hpp>
#include <Headers/kern_mach.hpp>
#include <Headers/kern_patcher.hpp>
#include <Headers/kern_util.hpp>
#include <IOKit/IOReturn.h>
#include <IOKit/IOTypes.h>
#include <Kexts.hpp>
#include <NRed.hpp>
#include <PenguinWizardry/KernelVersion.hpp>
#include <PenguinWizardry/PatcherPlus.hpp>
#include <Regs/OSSSYS_4.hpp>
#include <Regs/SMUIO.hpp>
#include <X6000FB.hpp>
#include <libkern/OSTypes.h>
#include <mach/i386/vm_param.h>
#include <mach/i386/vm_types.h>
#include <mach/kern_return.h>

static const UInt8 kCailAsicCapsTablePattern[] = {0x6E, 0x00, 0x00, 0x00, 0x98, 0x67, 0x00, 0x00,
                                                  0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
                                                  0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00};

static const UInt8 kPopulateVramInfoPattern[]     = {0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x53, 0x48,
                                                     0x81, 0xEC, 0x08, 0x01, 0x00, 0x00, 0x40, 0x89, 0xF0, 0x40,
                                                     0x89, 0xF0, 0x4C, 0x8D, 0xBD, 0xE0, 0xFE, 0xFF, 0xFF};
static const UInt8 kPopulateVramInfoPatternMask[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                                     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0xFF, 0xF0, 0xF0,
                                                     0xFF, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

static const UInt8 kIH40IVRingInitHardwarePattern[]     = {0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41,
                                                           0x55, 0x41, 0x54, 0x53, 0x50, 0x40, 0x89, 0xF0, 0x49,
                                                           0x89, 0xF0, 0x40, 0x8B, 0x00, 0x00, 0x44, 0x00, 0x00};
static const UInt8 kIH40IVRingInitHardwarePatternMask[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                                           0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0xFF, 0xF0, 0xFF,
                                                           0xFF, 0xF0, 0xF0, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xFF};

static const UInt8 kIRQMGRWriteRegisterPattern[] = {0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x55, 0x41,
                                                    0x54, 0x53, 0x50, 0x41, 0x89, 0xD6, 0x49, 0x89, 0xF7, 0x48, 0x89,
                                                    0xFB, 0x48, 0x8B, 0x87, 0xB0, 0x00, 0x00, 0x00, 0x48, 0x85, 0xC0};
static const UInt8 kIRQMGRWriteRegisterPattern1404[] = {
    0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x55, 0x41, 0x54, 0x53, 0x50, 0x89, 0xD3,
    0x49, 0x89, 0xF7, 0x49, 0x89, 0xFE, 0x48, 0x8B, 0x87, 0xB0, 0x00, 0x00, 0x00, 0x48, 0x85, 0xC0};

static const UInt8 kDpReceiverPowerCtrlPattern[] = {0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x54, 0x53,
                                                    0x48, 0x83, 0xEC, 0x10, 0x89, 0xF3, 0xB0, 0x02, 0x28, 0xD8};
static const UInt8 kDpReceiverPowerCtrlPattern1404[] = {0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56,
                                                        0x41, 0x54, 0x53, 0x48, 0x83, 0xEC, 0x10, 0x41,
                                                        0x89, 0xF7, 0xB0, 0x02, 0x44, 0x28, 0xF8};

static const UInt8      kCreateVramInfoPattern[]         = {0x48, 0x8B, 0x7B, 0x18, 0x48, 0x8B, 0x43, 0x20, 0x0F,
                                                            0xB7, 0x70, 0x3C, 0xE8, 0x00, 0x00, 0x00, 0x00};
static const UInt8      kCreateVramInfoPatternMask[]     = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                                            0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00};
static constexpr UInt32 kCreateVramInfoPatternJumpOffset = 12;

// Fix register read (0xD31 -> 0xD2F) and family ID (0x8F -> 0x8E).
static const UInt8 kPopulateDeviceInfoOriginal[]{0xBE, 0x31, 0x0D, 0x00, 0x00, 0xFF, 0x90, 0x40, 0x01,
                                                 0x00, 0x00, 0xC7, 0x43, 0x00, 0x8F, 0x00, 0x00, 0x00};
static const UInt8 kPopulateDeviceInfoMask[]{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                             0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF};
static const UInt8 kPopulateDeviceInfoPatched[]{0xBE, 0x2F, 0x0D, 0x00, 0x00, 0xFF, 0x90, 0x40, 0x01,
                                                0x00, 0x00, 0xC7, 0x43, 0x00, 0x8E, 0x00, 0x00, 0x00};

// Neutralise `AmdAtomPspDirectory` creation null check.
// We don't have this entry in our VBIOS.
static const UInt8 kAmdAtomPspDirectoryNullCheckOriginal[] = {0x48, 0x89, 0x83, 0x88, 0x00, 0x00, 0x00,
                                                              0x48, 0x85, 0xC0, 0x0F, 0x84, 0xA1, 0x00,
                                                              0x00, 0x00, 0x48, 0x8B, 0x7B, 0x18};
static const UInt8 kAmdAtomPspDirectoryNullCheckPatched[]  = {0x48, 0x89, 0x83, 0x88, 0x00, 0x00, 0x00,
                                                              0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x66,
                                                              0x90, 0x90, 0x48, 0x8B, 0x7B, 0x18};

// Tell AGDC that we're an iGPU.
static const UInt8 kGetVendorInfoOriginal[]        = {0x48, 0x00, 0x02, 0x10, 0x00, 0x00, 0x02};
static const UInt8 kGetVendorInfoMask[]            = {0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static const UInt8 kGetVendorInfoPatched[]         = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
static const UInt8 kGetVendorInfoPatchedMask[]     = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF};
static const UInt8 kGetVendorInfoOriginal1404[]    = {0xC7, 0x00, 0x24, 0x02, 0x10, 0x00, 0x00,
                                                      0xC7, 0x00, 0x28, 0x02, 0x00, 0x00, 0x00};
static const UInt8 kGetVendorInfoMask1404[]        = {0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                                      0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static const UInt8 kGetVendorInfoPatched1404[]     = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                      0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};
static const UInt8 kGetVendorInfoPatchedMask1404[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                      0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00};

// Remove check for Navi family
static const UInt8 kInitializeDmcubServices1Original[] = {0x81, 0x79, 0x2C, 0x8F, 0x00, 0x00, 0x00};
static const UInt8 kInitializeDmcubServices1Patched[]  = {0x39, 0xC0, 0x66, 0x90, 0x66, 0x90, 0x90};

// Set DMCUB ASIC constant to DCN 2.1
static const UInt8 kInitializeDmcubServices2Original[] = {0x83, 0xC0, 0xC4, 0x83, 0xF8, 0x0A, 0xB8,
                                                          0x03, 0x00, 0x00, 0x00, 0x83, 0xD0, 0x00};
static const UInt8 kInitializeDmcubServices2Patched[]  = {0xB8, 0x02, 0x00, 0x00, 0x00, 0x66, 0x90,
                                                          0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x90};

// Ditto, 14.4+
static const UInt8 kInitializeDmcubServices2Original1404[] = {0x83, 0xC0, 0xC4, 0x31, 0xC9, 0x83,
                                                              0xF8, 0x0A, 0x83, 0xD1, 0x03};
static const UInt8 kInitializeDmcubServices2Patched1404[]  = {0xB9, 0x02, 0x00, 0x00, 0x00, 0x66,
                                                              0x90, 0x66, 0x90, 0x66, 0x90};

// Ditto, 10.15
static const UInt8 kInitializeDmcubServices2Original1015[] = {0xC7, 0x46, 0x20, 0x01, 0x00, 0x00, 0x00};
static const UInt8 kInitializeDmcubServices2Patched1015[]  = {0xC7, 0x46, 0x20, 0x02, 0x00, 0x00, 0x00};

// 10.15: Set inst_const_size/bss_data_size to 0. To disable DMCUB firmware loading logic.
static const UInt8 kInitializeHardware1Original[] = {0x49, 0xBC, 0x00, 0x0A, 0x01, 0x00, 0xF4, 0x01, 0x00, 0x00};
static const UInt8 kInitializeHardware1Patched[]  = {0x49, 0xC7, 0xC4, 0x00, 0x00, 0x00, 0x00, 0x90, 0x90, 0x90};

// 10.15: Set fw_inst_const to nullptr, pt.2 of above.
static const UInt8 kInitializeHardware2Original[]     = {0x4C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4C, 0x00, 0x00,
                                                         0x10, 0x4C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4C};
static const UInt8 kInitializeHardware2OriginalMask[] = {0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00,
                                                         0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF};
static const UInt8 kInitializeHardware2Patched[]      = {0x49, 0xC7, 0xC5, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const UInt8 kInitializeHardware2PatchedMask[]  = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00,
                                                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// 10.15: Disable DMCUB firmware loading from DAL. HWLibs should be doing that.
static const UInt8 kAmdDalServicesInitializeOriginal[]     = {0xBE, 0x01, 0x00, 0x00, 0x00, 0xE8, 0x00,
                                                              0x00, 0x00, 0x00, 0x49, 0x00, 0x00, 0x60};
static const UInt8 kAmdDalServicesInitializeOriginalMask[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,
                                                              0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF};
static const UInt8 kAmdDalServicesInitializePatched[]      = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const UInt8 kAmdDalServicesInitializePatchedMask[]  = {0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00,
                                                              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// Change cursor and underflow tracker count to 4 instead of 6.
static const UInt8 kCreateControllerServicesOriginal[]     = {0x40, 0x00, 0x00, 0x40, 0x83, 0x00, 0x06};
static const UInt8 kCreateControllerServicesOriginalMask[] = {0xF0, 0x00, 0x00, 0xF0, 0xFF, 0x00, 0xFF};
static const UInt8 kCreateControllerServicesPatched[]      = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04};
static const UInt8 kCreateControllerServicesPatchedMask[]  = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F};

// Ditto, 10.15.
static const UInt8 kCreateControllerServicesOriginal1015[]     = {0x48, 0x00, 0x00, 0x48, 0x83, 0x00, 0x05};
static const UInt8 kCreateControllerServicesOriginalMask1015[] = {0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0xFF};
static const UInt8 kCreateControllerServicesPatched1015[]      = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03};
static const UInt8 kCreateControllerServicesPatchedMask1015[]  = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F};

// Change cursor count to 4 instead of 6.
static const UInt8 kSetupCursorsOriginal[]     = {0x40, 0x83, 0x00, 0x05};
static const UInt8 kSetupCursorsOriginalMask[] = {0xF0, 0xFF, 0x00, 0xFF};
static const UInt8 kSetupCursorsPatched[]      = {0x00, 0x00, 0x00, 0x03};
static const UInt8 kSetupCursorsPatchedMask[]  = {0x00, 0x00, 0x00, 0x0F};

// Ditto, 12.0+.
static const UInt8 kSetupCursorsOriginal12[]     = {0x40, 0x83, 0x00, 0x06};
static const UInt8 kSetupCursorsOriginalMask12[] = {0xF0, 0xFF, 0x00, 0xFF};
static const UInt8 kSetupCursorsPatched12[]      = {0x00, 0x00, 0x00, 0x04};
static const UInt8 kSetupCursorsPatchedMask12[]  = {0x00, 0x00, 0x00, 0x0F};

// Change link count to 4 instead of 6.
static const UInt8 kCreateLinksOriginal[]     = {0x06, 0x00, 0x00, 0x00, 0x40};
static const UInt8 kCreateLinksOriginalMask[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xF0};
static const UInt8 kCreateLinksPatched[]      = {0x04, 0x00, 0x00, 0x00, 0x00};
static const UInt8 kCreateLinksPatchedMask[]  = {0x0F, 0x00, 0x00, 0x00, 0x00};

// Remove new FB count condition so we can restore the original behaviour before Ventura.
static const UInt8 kControllerPowerUpOriginal[]     = {0x38, 0xC8, 0x0F, 0x42, 0xC8, 0x88, 0x8F,
                                                       0xBC, 0x00, 0x00, 0x00, 0x72, 0x00};
static const UInt8 kControllerPowerUpOriginalMask[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                                       0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00};
static const UInt8 kControllerPowerUpReplace[]      = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                       0x00, 0x00, 0x00, 0x00, 0xEB, 0x00};
static const UInt8 kControllerPowerUpReplaceMask[]  = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                       0x00, 0x00, 0x00, 0x00, 0xFF, 0x00};

// Remove new problematic Ventura pixel clock multiplier calculation which causes timing validation mishaps.
static const UInt8 kValidateDetailedTimingOriginal[] = {0x66, 0x0F, 0x2E, 0xC1, 0x76, 0x06, 0xF2, 0x0F, 0x5E, 0xC1};
static const UInt8 kValidateDetailedTimingPatched[]  = {0x66, 0x0F, 0x2E, 0xC1, 0x66, 0x90, 0xF2, 0x0F, 0x5E, 0xC1};

static const UInt8 kGetNumberOfConnectorsPattern[]     = {0x55, 0x48, 0x89, 0xE5, 0x40, 0x8B, 0x40, 0x28, 0x00,
                                                          0x00, 0x00, 0x00, 0x00, 0x85, 0x00, 0x74, 0x00};
static const UInt8 kGetNumberOfConnectorsPatternMask[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0xFF, 0xF0, 0xFF, 0x00,
                                                          0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00};

static X6000FB moduleInstance;

X6000FB& X6000FB::singleton() { return moduleInstance; }

void X6000FB::processKext(KernelPatcher& patcher, size_t id, mach_vm_address_t slide, size_t size)
{
    if (kextRadeonX6000Framebuffer.loadIndex != id) { return; }

    NRed::singleton().hwLateInit();

    CAILAsicCapsEntry*                   orgAsicCapsTable       = nullptr;
    void*                                orgAmdAsicInfoNavi10VT = nullptr;
    PenguinWizardry::PatternSolveRequest solveRequests[]        = {
        {"__ZL20CAIL_ASIC_CAPS_TABLE", orgAsicCapsTable, kCailAsicCapsTablePattern},
        {"__ZN37AMDRadeonX6000_AmdDeviceMemoryManager17mapMemorySubRangeE25AmdReservedMemorySelectoryyj",
         this->mapMemorySubRange},
        {"__ZTV32AMDRadeonX6000_AmdAsicInfoNavi10", orgAmdAsicInfoNavi10VT},
    };
    PANIC_COND(!PenguinWizardry::PatternSolveRequest::solveAll(patcher, id, solveRequests, slide, size), "X6000FB",
               "Failed to resolve symbols");

    PANIC_COND(MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) != KERN_SUCCESS, "X6000FB",
               "Failed to enable kernel writing");
    getMember<decltype(getGpuBrandingNameListRenoir)*>(orgAmdAsicInfoNavi10VT, 0x228) =
        NRed::singleton().getAttributes().isRenoir()  ? getGpuBrandingNameListRenoir :
        NRed::singleton().getAttributes().isPicasso() ? getGpuBrandingNameListPicasso :
                                                        getGpuBrandingNameListRaven;
    MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);

    if (checkKernelArgument("-NRedDPDelay")) {
        if (currentKernelVersion() >= MACOS_14_4) {
            PenguinWizardry::PatternRouteRequest request{"_dp_receiver_power_ctrl", wrapDpReceiverPowerCtrl,
                                                         this->orgDpReceiverPowerCtrl, kDpReceiverPowerCtrlPattern1404};
            PANIC_COND(!request.route(patcher, id, slide, size), "X6000FB",
                       "Failed to route dp_receiver_power_ctrl (14.4+)");
        }
        else {
            PenguinWizardry::PatternRouteRequest request{"_dp_receiver_power_ctrl", wrapDpReceiverPowerCtrl,
                                                         this->orgDpReceiverPowerCtrl, kDpReceiverPowerCtrlPattern};
            PANIC_COND(!request.route(patcher, id, slide, size), "X6000FB", "Failed to route dp_receiver_power_ctrl");
        }
    }

    if (currentKernelVersion() >= MACOS_13) {
        this->orgMessageAccelerator = patcher.solveSymbol<decltype(this->orgMessageAccelerator)>(
            id, "__ZNK34AMDRadeonX6000_AmdRadeonController18messageAcceleratorE25_eAMDAccelIOFBRequestTypePvS1_S1_",
            slide, size);
        PANIC_COND(this->orgMessageAccelerator == nullptr, "X6000FB", "Failed to resolve messageAccelerator");

        KernelPatcher::RouteRequest request{"__ZN34AMDRadeonX6000_AmdRadeonController7powerUpEv", wrapControllerPowerUp,
                                            this->orgControllerPowerUp};
        PANIC_COND(!patcher.routeMultiple(id, &request, 1, slide, size), "X6000FB", "Failed to route powerUp");

        const PenguinWizardry::MaskedLookupPatch patches[] = {
            {&kextRadeonX6000Framebuffer, kControllerPowerUpOriginal, kControllerPowerUpOriginalMask,
             kControllerPowerUpReplace, kControllerPowerUpReplaceMask, 1},
            {&kextRadeonX6000Framebuffer, kValidateDetailedTimingOriginal, kValidateDetailedTimingPatched, 1},
        };
        PANIC_COND(!PenguinWizardry::MaskedLookupPatch::applyAll(patcher, patches, slide, size), "X6000FB",
                   "Failed to apply logic revert patches");
    }

    PenguinWizardry::PatternRouteRequest requests[] = {
        {"__ZNK15AmdAtomVramInfo16populateVramInfoER16AtomFirmwareInfo", wrapPopulateVramInfo, kPopulateVramInfoPattern,
         kPopulateVramInfoPatternMask},
        {"__ZNK32AMDRadeonX6000_AmdAsicInfoNavi1027getEnumeratedRevisionNumberEv", getEnumeratedRevision},
        {"__ZN41AMDRadeonX6000_AmdDeviceMemoryManagerNavi21intializeReservedVramEv", initialiseReservedVRAM},
        {"__ZN38AMDRadeonX6000_AmdRadeonControllerNavi19setupBootWatermarksEv", dummyIOReturnSuccess},
        {"__ZNK22AmdAtomObjectInfo_V1_421getNumberOfConnectorsEv", wrapGetNumberOfConnectors,
         this->orgGetNumberOfConnectors, kGetNumberOfConnectorsPattern, kGetNumberOfConnectorsPatternMask},
    };
    PANIC_COND(!PenguinWizardry::PatternRouteRequest::routeAll(patcher, id, requests, slide, size), "X6000FB",
               "Failed to route symbols");

    PenguinWizardry::JumpPatternRouteRequest createVramInfoRequest{
        "__ZN15AmdAtomVramInfo14createVramInfoEP15AmdAtomFwHelperj",
        wrapCreateVramInfo,
        this->orgCreateVramInfo,
        kCreateVramInfoPattern,
        kCreateVramInfoPatternMask,
        kCreateVramInfoPatternJumpOffset};
    PANIC_COND(!createVramInfoRequest.route(patcher, id, slide, size), "X6000FB", "Failed to route createVramInfo");

    if (currentKernelVersion() >= MACOS_11 && currentKernelVersion() <= MACOS_12_X) {
        PenguinWizardry::PatternRouteRequest getTriageHardwareDataRequest{
            "__ZN38AMDRadeonX6000_AmdRadeonControllerNavi21getTriageHardwareDataEjP12_AMD_TRIAGE_",
            NRed::singleton().getAttributes().isRenoir() ? getTriageHardwareDataRN : getTriageHardwareDataRV};
        PANIC_COND(!getTriageHardwareDataRequest.route(patcher, id, slide, size), "X6000FB",
                   "Failed to route getTriageHardwareData");
    }

    if (currentKernelVersion() >= MACOS_11) {
        KernelPatcher::RouteRequest request{
            "__ZN32AMDRadeonX6000_AmdRegisterAccess20createRegisterAccessERNS_8InitDataE", wrapCreateRegisterAccess,
            this->orgCreateRegisterAccess};
        PANIC_COND(!patcher.routeMultiple(id, &request, 1, slide, size), "X6000FB",
                   "Failed to route createRegisterAccess");
    }

    if (NRed::singleton().getAttributes().isRenoir()) {
        PenguinWizardry::PatternRouteRequest request{"_IH_4_0_IVRing_InitHardware", wrapIH40IVRingInitHardware,
                                                     this->orgIH40IVRingInitHardware, kIH40IVRingInitHardwarePattern,
                                                     kIH40IVRingInitHardwarePatternMask};
        PANIC_COND(!request.route(patcher, id, slide, size), "X6000FB", "Failed to route IH_4_0_IVRing_InitHardware");
        if (currentKernelVersion() >= MACOS_14_4) {
            PenguinWizardry::PatternRouteRequest request{"_IRQMGR_WriteRegister", wrapIRQMGRWriteRegister,
                                                         this->orgIRQMGRWriteRegister, kIRQMGRWriteRegisterPattern1404};
            PANIC_COND(!request.route(patcher, id, slide, size), "X6000FB",
                       "Failed to route IRQMGR_WriteRegister (14.4+)");
        }
        else {
            PenguinWizardry::PatternRouteRequest request{"_IRQMGR_WriteRegister", wrapIRQMGRWriteRegister,
                                                         this->orgIRQMGRWriteRegister, kIRQMGRWriteRegisterPattern};
            PANIC_COND(!request.route(patcher, id, slide, size), "X6000FB", "Failed to route IRQMGR_WriteRegister");
        }
    }

    const PenguinWizardry::MaskedLookupPatch patch{&kextRadeonX6000Framebuffer, kPopulateDeviceInfoOriginal,
                                                   kPopulateDeviceInfoMask,     kPopulateDeviceInfoPatched,
                                                   kPopulateDeviceInfoMask,     1};
    PANIC_COND(!patch.apply(patcher, slide, size), "X6000FB", "Failed to apply populateDeviceInfo patch");

    if (currentKernelVersion() >= MACOS_14_4) {
        const PenguinWizardry::MaskedLookupPatch patch{&kextRadeonX6000Framebuffer,   kGetVendorInfoOriginal1404,
                                                       kGetVendorInfoMask1404,        kGetVendorInfoPatched1404,
                                                       kGetVendorInfoPatchedMask1404, 1};
        PANIC_COND(!patch.apply(patcher, slide, size), "X6000FB", "Failed to apply getVendorInfo patch (14.4)");
    }
    else {
        const PenguinWizardry::MaskedLookupPatch patch{&kextRadeonX6000Framebuffer, kGetVendorInfoOriginal,
                                                       kGetVendorInfoMask,          kGetVendorInfoPatched,
                                                       kGetVendorInfoPatchedMask,   1};
        PANIC_COND(!patch.apply(patcher, slide, size), "X6000FB", "Failed to apply getVendorInfo patch");
    }

    if (currentKernelVersion() >= MACOS_11) {
        const PenguinWizardry::MaskedLookupPatch patch{&kextRadeonX6000Framebuffer,
                                                       kAmdAtomPspDirectoryNullCheckOriginal,
                                                       kAmdAtomPspDirectoryNullCheckPatched, 1};
        PANIC_COND(!patch.apply(patcher, slide, size), "X6000FB", "Failed to apply null check patch");
    }

    if (NRed::singleton().getAttributes().isRenoir()) {
        const PenguinWizardry::MaskedLookupPatch patch{&kextRadeonX6000Framebuffer, kInitializeDmcubServices1Original,
                                                       kInitializeDmcubServices1Patched, 1};
        PANIC_COND(!patch.apply(patcher, slide, size), "X6000FB",
                   "Failed to apply initializeDmcubServices family id patch");
        if (currentKernelVersion() <= MACOS_10_15_X) {
            const PenguinWizardry::MaskedLookupPatch patches[] = {
                {&kextRadeonX6000Framebuffer, kInitializeDmcubServices2Original1015,
                 kInitializeDmcubServices2Patched1015, 1},
                {&kextRadeonX6000Framebuffer, kInitializeHardware1Original, kInitializeHardware1Patched, 1},
                {&kextRadeonX6000Framebuffer, kInitializeHardware2Original, kInitializeHardware2OriginalMask,
                 kInitializeHardware2Patched, kInitializeHardware2PatchedMask, 1},
                {&kextRadeonX6000Framebuffer, kAmdDalServicesInitializeOriginal, kAmdDalServicesInitializeOriginalMask,
                 kAmdDalServicesInitializePatched, kAmdDalServicesInitializePatchedMask, 1},
            };
            PANIC_COND(!PenguinWizardry::MaskedLookupPatch::applyAll(patcher, patches, slide, size), "X6000FB",
                       "Failed to apply AmdDalDmcubService and AmdDalServices::initialize patches (10.15)");
        }
        else if (currentKernelVersion() >= MACOS_14_4) {
            const PenguinWizardry::MaskedLookupPatch patch{&kextRadeonX6000Framebuffer,
                                                           kInitializeDmcubServices2Original1404,
                                                           kInitializeDmcubServices2Patched1404, 1};
            PANIC_COND(!patch.apply(patcher, slide, size), "X6000FB",
                       "Failed to apply initializeDmcubServices ASIC patch (14.4+)");
        }
        else {
            const PenguinWizardry::MaskedLookupPatch patch{
                &kextRadeonX6000Framebuffer, kInitializeDmcubServices2Original, kInitializeDmcubServices2Patched, 1};
            PANIC_COND(!patch.apply(patcher, slide, size), "X6000FB",
                       "Failed to apply initializeDmcubServices ASIC patch");
        }
    }

    PANIC_COND(MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) != KERN_SUCCESS, "X6000FB",
               "Failed to enable kernel writing");
    orgAsicCapsTable->familyId = AMD_FAMILY_RAVEN;
    orgAsicCapsTable->ddiCaps  = NRed::singleton().getAttributes().isRenoirE() ? ddiCapsRenoirE :
                                 NRed::singleton().getAttributes().isRenoir()  ? ddiCapsRenoir :
                                                                                 ddiCapsRaven;
    orgAsicCapsTable->deviceId = NRed::singleton().getDeviceID();
    orgAsicCapsTable->revision = NRed::singleton().getDevRevision();
    orgAsicCapsTable->extRevision =
        static_cast<UInt32>(NRed::singleton().getEnumRevision()) + NRed::singleton().getDevRevision();
    orgAsicCapsTable->pciRevision = NRed::singleton().getPciRevision();
    MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);
    DBGLOG("X6000FB", "Applied DDI Caps patches");

    // We need to patch the kext to create only 4 cursors, links and underflow trackers.
    auto* const orgCreateControllerServices = patcher.solveSymbol<void*>(
        id, "__ZN40AMDRadeonX6000_AmdRadeonControllerNavi1024createControllerServicesEv", slide, size, true);
    PANIC_COND(orgCreateControllerServices == nullptr, "X6000FB", "Failed to solve createControllerServices");

    auto* const orgSetupCursors =
        patcher.solveSymbol<void*>(id, "__ZN34AMDRadeonX6000_AmdRadeonController12setupCursorsEv", slide, size, true);
    PANIC_COND(orgSetupCursors == nullptr, "X6000FB", "Failed to solve setupCursors");

    auto* const orgCreateLinks =
        patcher.solveSymbol<void*>(id, "__ZN34AMDRadeonX6000_AmdRadeonController11createLinksEv", slide, size, true);
    PANIC_COND(orgCreateLinks == nullptr, "X6000FB", "Failed to solve createLinks");

    if (currentKernelVersion() <= MACOS_10_15_X) {
        PANIC_COND(!KernelPatcher::findAndReplaceWithMask(
                       orgCreateControllerServices, PAGE_SIZE, kCreateControllerServicesOriginal1015,
                       kCreateControllerServicesOriginalMask1015, kCreateControllerServicesPatched1015,
                       kCreateControllerServicesPatchedMask1015, 1, 0),
                   "X6000FB", "Failed to apply createControllerServices patch (10.15)");
    }
    else {
        PANIC_COND(!KernelPatcher::findAndReplaceWithMask(
                       orgCreateControllerServices, PAGE_SIZE, kCreateControllerServicesOriginal,
                       kCreateControllerServicesOriginalMask, kCreateControllerServicesPatched,
                       kCreateControllerServicesPatchedMask, 2, 0),
                   "X6000FB", "Failed to apply createControllerServices patch");
    }

    if (currentKernelVersion() >= MACOS_12) {
        PANIC_COND(!KernelPatcher::findAndReplaceWithMask(orgSetupCursors, PAGE_SIZE, kSetupCursorsOriginal12,
                                                          kSetupCursorsOriginalMask12, kSetupCursorsPatched12,
                                                          kSetupCursorsPatchedMask12, 1, 0),
                   "X6000FB", "Failed to apply setupCursors patch (12.0+)");
    }
    else {
        PANIC_COND(!KernelPatcher::findAndReplaceWithMask(orgSetupCursors, PAGE_SIZE, kSetupCursorsOriginal,
                                                          kSetupCursorsOriginalMask, kSetupCursorsPatched,
                                                          kSetupCursorsPatchedMask, 1, 0),
                   "X6000FB", "Failed to apply setupCursors patch");
    }

    PANIC_COND(!KernelPatcher::findAndReplaceWithMask(orgCreateLinks, PAGE_SIZE, kCreateLinksOriginal,
                                                      kCreateLinksOriginalMask, kCreateLinksPatched,
                                                      kCreateLinksPatchedMask, 1, 0),
               "X6000FB", "Failed to apply createLinks patch");
}

UInt16 X6000FB::getEnumeratedRevision() { return NRed::singleton().getEnumRevision(); }

bool X6000FB::wrapIH40IVRingInitHardware(void* const ctx, void* const param2)
{
    auto ret = FunctionCast(wrapIH40IVRingInitHardware, singleton().orgIH40IVRingInitHardware)(ctx, param2);
    NRed::singleton().writeReg32(IH_CHICKEN, NRed::singleton().readReg32(IH_CHICKEN) | IH_MC_SPACE_GPA_ENABLE);
    return ret;
}

void X6000FB::wrapIRQMGRWriteRegister(void* const ctx, const UInt64 off, UInt32 value)
{
    if (off == IH_CLK_CTRL) {
        if ((value & getBit(IH_DBUS_MUX_CLK_SOFT_OVERRIDE_SHIFT)) != 0) {
            value |= getBit(IH_IH_BUFFER_MEM_CLK_SOFT_OVERRIDE_SHIFT);
        }
    }
    FunctionCast(wrapIRQMGRWriteRegister, singleton().orgIRQMGRWriteRegister)(ctx, off, value);
}

void* X6000FB::wrapCreateRegisterAccess(void* const initData)
{
    getMember<UInt32>(initData, 0x24) = SMUIO_BASE_0 + ROM_INDEX;
    getMember<UInt32>(initData, 0x28) = SMUIO_BASE_0 + ROM_DATA;
    return FunctionCast(wrapCreateRegisterAccess, singleton().orgCreateRegisterAccess)(initData);
}

IOReturn X6000FB::initialiseReservedVRAM(void* const self)
{
#define CHECK(_expr)                                                     \
    if (const auto ret = _expr; ret != kIOReturnSuccess) { return ret; }
    static constexpr IOOptionBits mapOptions = kIOMapWriteCombineCache | kIOMapAnywhere;
    CHECK(singleton().mapMemorySubRange(self, AmdReservedMemorySelector::Cursor1_32bpp, 0, 0x40000, mapOptions));
    CHECK(singleton().mapMemorySubRange(self, AmdReservedMemorySelector::Cursor1_2bpp, 0x40000, 0x40000, mapOptions));
    CHECK(singleton().mapMemorySubRange(self, AmdReservedMemorySelector::Cursor2_32bpp, 0x80000, 0x40000, mapOptions));
    CHECK(singleton().mapMemorySubRange(self, AmdReservedMemorySelector::Cursor2_2bpp, 0xC0000, 0x40000, mapOptions));
    CHECK(singleton().mapMemorySubRange(self, AmdReservedMemorySelector::Cursor3_32bpp, 0x100000, 0x40000, mapOptions));
    CHECK(singleton().mapMemorySubRange(self, AmdReservedMemorySelector::Cursor3_2bpp, 0x140000, 0x40000, mapOptions));
    CHECK(singleton().mapMemorySubRange(self, AmdReservedMemorySelector::Cursor4_32bpp, 0x180000, 0x40000, mapOptions));
    CHECK(singleton().mapMemorySubRange(self, AmdReservedMemorySelector::Cursor4_2bpp, 0x1C0000, 0x40000, mapOptions));
    CHECK(
        singleton().mapMemorySubRange(self, AmdReservedMemorySelector::PPLIBReserved, 0x200000, 0x100000, mapOptions));
    if (NRed::singleton().getAttributes().isRenoir()) {
        CHECK(singleton().mapMemorySubRange(self, AmdReservedMemorySelector::DMCUBReserved, 0x300000, 0x100000,
                                            mapOptions));
        return singleton().mapMemorySubRange(self, AmdReservedMemorySelector::ReserveVRAM, 0, 0x400000, mapOptions);
    }
    return singleton().mapMemorySubRange(self, AmdReservedMemorySelector::ReserveVRAM, 0, 0x300000, mapOptions);
#undef CHECK
}

static const AmdAsicBrandingTableEntry ravenBrandingTable[] = {
    {0x15DD, 0x81, "Radeon RX", "Vega 11"}, {0x15DD, 0x82, "Radeon RX", "Vega 8"},
    {0x15DD, 0x83, "Radeon RX", "Vega 8"},  {0x15DD, 0x84, "Radeon RX", "Vega 6"},
    {0x15DD, 0x85, "Radeon RX", "Vega 3"},  {0x15DD, 0x86, "Radeon RX", "Vega 11"},
    {0x15DD, 0x88, "Radeon RX", "Vega 8"},  {0x15DD, 0xC1, "Radeon RX", "Vega 11"},
    {0x15DD, 0xC2, "Radeon RX", "Vega 8"},  {0x15DD, 0xC3, "Radeon RX", "Vega 10"},
    {0x15DD, 0xC4, "Radeon RX", "Vega 8"},  {0x15DD, 0xC5, "Radeon RX", "Vega 3"},
    {0x15DD, 0xC6, "Radeon RX", "Vega 11"}, {0x15DD, 0xC8, "Radeon RX", "Vega 8"},
    {0x15DD, 0xC9, "Radeon RX", "Vega 11"}, {0x15DD, 0xCA, "Radeon RX", "Vega 8"},
    {0x15DD, 0xCB, "Radeon RX", "Vega 3"},  {0x15DD, 0xCC, "Radeon RX", "Vega 6"},
    {0x15DD, 0xCE, "Radeon RX", "Vega 3"},  {0x15DD, 0xCF, "Radeon RX", "Vega 3"},
    {0x15DD, 0xD0, "Radeon RX", "Vega 10"}, {0x15DD, 0xD1, "Radeon RX", "Vega 8"},
    {0x15DD, 0xD3, "Radeon RX", "Vega 11"}, {0x15DD, 0xD5, "Radeon RX", "Vega 8"},
    {0x15DD, 0xD6, "Radeon RX", "Vega 11"}, {0x15DD, 0xD7, "Radeon RX", "Vega 8"},
    {0x15DD, 0xD8, "Radeon RX", "Vega 3"},  {0x15DD, 0xD9, "Radeon RX", "Vega 6"},
    {0x15DD, 0xE1, "Radeon RX", "Vega 3"},  {0x15DD, 0xE2, "Radeon RX", "Vega 3"},
    {"Radeon RX", "Raven Graphics"},
};

static const AmdAsicBrandingTableEntry picassoBrandingTable[] = {
    {0x15D8, 0x00, "Radeon RX", "Vega 8 WS"}, {0x15D8, 0x91, "Radeon RX", "Vega 3"},
    {0x15D8, 0x92, "Radeon RX", "Vega 3"},    {0x15D8, 0x93, "Radeon RX", "Vega 1"},
    {0x15D8, 0xA1, "Radeon RX", "Vega 10"},   {0x15D8, 0xA2, "Radeon RX", "Vega 8"},
    {0x15D8, 0xA3, "Radeon RX", "Vega 6"},    {0x15D8, 0xA4, "Radeon RX", "Vega 3"},
    {0x15D8, 0xB1, "Radeon RX", "Vega 10"},   {0x15D8, 0xB2, "Radeon RX", "Vega 8"},
    {0x15D8, 0xB3, "Radeon RX", "Vega 6"},    {0x15D8, 0xB4, "Radeon RX", "Vega 3"},
    {0x15D8, 0xC1, "Radeon RX", "Vega 10"},   {0x15D8, 0xC2, "Radeon RX", "Vega 8"},
    {0x15D8, 0xC3, "Radeon RX", "Vega 6"},    {0x15D8, 0xC4, "Radeon RX", "Vega 3"},
    {0x15D8, 0xC5, "Radeon RX", "Vega 3"},    {0x15D8, 0xC8, "Radeon RX", "Vega 11"},
    {0x15D8, 0xC9, "Radeon RX", "Vega 8"},    {0x15D8, 0xCA, "Radeon RX", "Vega 11"},
    {0x15D8, 0xCB, "Radeon RX", "Vega 8"},    {0x15D8, 0xCC, "Radeon RX", "Vega 3"},
    {0x15D8, 0xCE, "Radeon RX", "Vega 3"},    {0x15D8, 0xCF, "Radeon RX", "Vega 3"},
    {0x15D8, 0xD1, "Radeon RX", "Vega 10"},   {0x15D8, 0xD2, "Radeon RX", "Vega 8"},
    {0x15D8, 0xD3, "Radeon RX", "Vega 6"},    {0x15D8, 0xD4, "Radeon RX", "Vega 3"},
    {0x15D8, 0xD8, "Radeon RX", "Vega 11"},   {0x15D8, 0xD9, "Radeon RX", "Vega 8"},
    {0x15D8, 0xDA, "Radeon RX", "Vega 11"},   {0x15D8, 0xDB, "Radeon RX", "Vega 8"},
    {0x15D8, 0xDC, "Radeon RX", "Vega 3"},    {0x15D8, 0xDD, "Radeon RX", "Vega 3"},
    {0x15D8, 0xDE, "Radeon RX", "Vega 3"},    {0x15D8, 0xDF, "Radeon RX", "Vega 3"},
    {0x15D8, 0xE1, "Radeon RX", "Vega 11"},   {0x15D8, 0xE2, "Radeon RX", "Vega 9"},
    {0x15D8, 0xE3, "Radeon RX", "Vega 3"},    {0x15D8, 0xE4, "Radeon RX", "Vega 3"},
    {"Radeon RX", "Picasso Graphics"},
};

static const AmdAsicBrandingTableEntry renoirBrandingTable[] = {
    {0x1636, 0xD1, "Radeon Pro", "Graphics"},
    {0x1636, 0xD3, "Radeon Pro", "Graphics"},
    {"Radeon RX", "Renoir Graphics"},
};

const AmdAsicBrandingTableEntry* X6000FB::getGpuBrandingNameListRaven(const void* const) { return ravenBrandingTable; }

const AmdAsicBrandingTableEntry* X6000FB::getGpuBrandingNameListPicasso(const void* const)
{ return picassoBrandingTable; }

const AmdAsicBrandingTableEntry* X6000FB::getGpuBrandingNameListRenoir(const void* const)
{ return renoirBrandingTable; }

IOReturn X6000FB::dummyIOReturnSuccess() { return kIOReturnSuccess; }

IOReturn X6000FB::getTriageHardwareDataRV(void* const, const UInt32 fbIndex, void* const triageData)
{
    auto& bufferPointer = getMember<char*>(triageData, 0x0);
    auto& bufferSize    = getMember<UInt32>(triageData, 0x8);

    if (bufferSize < 2) { return kIOReturnNoResources; }
    if (fbIndex >= 4) { return kIOReturnSuccess; }

    const auto odmOptcInputGlobalControl = NRed::singleton().readReg32(DCN_BASE_2 + 0x1ACA + (0x10 * fbIndex));
    const auto otgMasterEn               = NRed::singleton().readReg32(DCN_BASE_2 + 0x1B5F + (0x80 * fbIndex));
    const auto hubpClkControl            = NRed::singleton().readReg32(DCN_BASE_2 + 0x567 + (0xC4 * fbIndex));
    const auto digBeEnControl            = NRed::singleton().readReg32(DCN_BASE_2 + 0x20B0 + (0x100 * fbIndex));

    const auto chars = snprintf(bufferPointer, bufferSize, "%x %x %x %x", odmOptcInputGlobalControl, otgMasterEn,
                                hubpClkControl, digBeEnControl);
    if (chars < 0) { return kIOReturnError; }
    const auto realChars = static_cast<UInt32>(chars) > bufferSize ? bufferSize : static_cast<UInt32>(chars);

    bufferSize    -= realChars;
    bufferPointer += realChars;

    return kIOReturnSuccess;
}

IOReturn X6000FB::getTriageHardwareDataRN(void* const, const UInt32 fbIndex, void* const triageData)
{
    auto& bufferPointer = getMember<char*>(triageData, 0x0);
    auto& bufferSize    = getMember<UInt32>(triageData, 0x8);

    if (bufferSize < 2) { return kIOReturnNoResources; }
    if (fbIndex >= 4) { return kIOReturnSuccess; }

    const auto odmOptcInputGlobalControl = NRed::singleton().readReg32(DCN_BASE_2 + 0x1ACA + (0x10 * fbIndex));
    const auto otgMasterEn               = NRed::singleton().readReg32(DCN_BASE_2 + 0x1B5C + (0x80 * fbIndex));
    const auto hubpClkControl            = NRed::singleton().readReg32(DCN_BASE_2 + 0x5F4 + (0xDC * fbIndex));
    const auto digBeEnControl            = NRed::singleton().readReg32(DCN_BASE_2 + 0x20B0 + (0x100 * fbIndex));

    const auto chars = snprintf(bufferPointer, bufferSize, "%x %x %x %x", odmOptcInputGlobalControl, otgMasterEn,
                                hubpClkControl, digBeEnControl);
    if (chars < 0) { return kIOReturnError; }
    const auto realChars = static_cast<UInt32>(chars) > bufferSize ? bufferSize : static_cast<UInt32>(chars);

    bufferSize    -= realChars;
    bufferPointer += realChars;

    return kIOReturnSuccess;
}

UInt32 X6000FB::wrapControllerPowerUp(void* self)
{
    auto& m_flags  = getMember<UInt8>(self, 0x5F18);
    auto  send     = (m_flags & 2) == 0;
    m_flags       |= 4;    // All framebuffers enabled
    auto ret       = FunctionCast(wrapControllerPowerUp, singleton().orgControllerPowerUp)(self);
    if (send) { singleton().orgMessageAccelerator(self, IOFBRequestControllerEnabled, nullptr, nullptr, nullptr); }
    return ret;
}

void X6000FB::wrapDpReceiverPowerCtrl(void* link, bool power_on)
{
    FunctionCast(wrapDpReceiverPowerCtrl, singleton().orgDpReceiverPowerCtrl)(link, power_on);
    IOSleep(250);
}

UInt32 X6000FB::wrapGetNumberOfConnectors(void* const self)
{
    if (!singleton().fixedVBIOS) {
        singleton().fixedVBIOS = true;
        const auto objInfo     = getMember<DispObjInfoTableV1_4*>(self, 0x28);
        if (objInfo->formatRev == 1 && (objInfo->contentRev == 4 || objInfo->contentRev == 5)) {
            DBGLOG("X6000FB", "getNumberOfConnectors: Fixing VBIOS connectors");
            const auto n = objInfo->pathCount;
            for (size_t i = 0, j = 0; i < n; i++) {
                // Skip invalid device tags
                if (objInfo->paths[i].devTag == 0) { objInfo->pathCount--; }
                else {
                    objInfo->paths[j++] = objInfo->paths[i];
                }
            }
        }
    }
    return FunctionCast(wrapGetNumberOfConnectors, singleton().orgGetNumberOfConnectors)(self);
}

static UInt32 getTableOffset(AmdAtomFwHelper* const biosHelper, const UInt32 index)
{
    const auto romTableOffset = static_cast<const UInt16*>(biosHelper->getImage(ATOM_ROM_TABLE_PTR, sizeof(UInt16)));
    if (romTableOffset == nullptr) { return 0; }
    const auto mdtOffset =
        static_cast<const UInt16*>(biosHelper->getImage(*romTableOffset + ATOM_ROM_DATA_PTR, sizeof(UInt32)));
    if (mdtOffset == nullptr) { return 0; }
    const auto mdt =
        static_cast<const UInt8*>(biosHelper->getImage(*mdtOffset, /*sizeof(atom_master_data_table_v2_1)*/ 0x4A));
    if (mdt == nullptr) { return 0; }
    return reinterpret_cast<const UInt16*>(mdt + sizeof(ATOMCommonTableHeader))[index];
}

AmdAtomVramInfo* X6000FB::wrapCreateVramInfo(AmdAtomFwHelper* const biosHelper, const UInt32 tableOffset)
{
    if (biosHelper == nullptr || tableOffset != 0) {    // tableOffset is 0 on iGPUs
        return FunctionCast(wrapCreateVramInfo, singleton().orgCreateVramInfo)(biosHelper, tableOffset);
    }
    return AmdAtomVramInfoIGP::createVramInfoIGP(biosHelper, getTableOffset(biosHelper, 0x1E));
}

IOReturn X6000FB::wrapPopulateVramInfo(AmdAtomVramInfo* self, AtomFirmwareInfo& fwInfo)
{ return self->populateVramInfo(fwInfo); }
