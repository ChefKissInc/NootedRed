// AMDRadeonX6000Framebuffer patches for AMD Vega iGPUs
//
// Copyright © 2022-2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#include <GPUDriversAMD/CAIL/ASICCaps.hpp>
#include <GPUDriversAMD/FB/VidMemType.hpp>
#include <GPUDriversAMD/Family.hpp>
#include <GPUDriversAMD/RavenIPOffset.hpp>
#include <Kexts.hpp>
#include <NRed.hpp>
#include <PenguinWizardry/KernelVersion.hpp>
#include <PenguinWizardry/PatcherPlus.hpp>
#include <iVega/ASICCaps.hpp>
#include <iVega/Regs/OSSSYS_4.hpp>
#include <iVega/Regs/SMUIO.hpp>
#include <iVega/X6000FB.hpp>

static const UInt8 kCailAsicCapsTablePattern[] = {0x6E, 0x00, 0x00, 0x00, 0x98, 0x67, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00};

static const UInt8 kPopulateVramInfoPattern[] = {0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x53, 0x48, 0x81, 0xEC,
    0x08, 0x01, 0x00, 0x00, 0x40, 0x89, 0xF0, 0x40, 0x89, 0xF0, 0x4C, 0x8D, 0xBD, 0xE0, 0xFE, 0xFF, 0xFF};
static const UInt8 kPopulateVramInfoPatternMask[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0xFF, 0xF0, 0xF0, 0xFF, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

static const UInt8 kGetNumberOfConnectorsPattern[] = {0x55, 0x48, 0x89, 0xE5, 0x40, 0x8B, 0x40, 0x28, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x85, 0x00, 0x74, 0x00};
static const UInt8 kGetNumberOfConnectorsPatternMask[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0xFF, 0xF0, 0xFF, 0x00, 0x00,
    0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00};

static const UInt8 kIH40IVRingInitHardwarePattern[] = {0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x55, 0x41,
    0x54, 0x53, 0x50, 0x40, 0x89, 0xF0, 0x49, 0x89, 0xF0, 0x40, 0x8B, 0x00, 0x00, 0x44, 0x00, 0x00};
static const UInt8 kIH40IVRingInitHardwarePatternMask[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0xFF, 0xF0, 0xFF, 0xFF, 0xF0, 0xF0, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xFF};

static const UInt8 kIRQMGRWriteRegisterPattern[] = {0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x55, 0x41,
    0x54, 0x53, 0x50, 0x41, 0x89, 0xD6, 0x49, 0x89, 0xF7, 0x48, 0x89, 0xFB, 0x48, 0x8B, 0x87, 0xB0, 0x00, 0x00, 0x00,
    0x48, 0x85, 0xC0};
static const UInt8 kIRQMGRWriteRegisterPattern1404[] = {0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x55,
    0x41, 0x54, 0x53, 0x50, 0x89, 0xD3, 0x49, 0x89, 0xF7, 0x49, 0x89, 0xFE, 0x48, 0x8B, 0x87, 0xB0, 0x00, 0x00, 0x00,
    0x48, 0x85, 0xC0};

// Fix register read (0xD31 -> 0xD2F) and family ID (0x8F -> 0x8E).
static const UInt8 kPopulateDeviceInfoOriginal[] {0xBE, 0x31, 0x0D, 0x00, 0x00, 0xFF, 0x90, 0x40, 0x01, 0x00, 0x00,
    0xC7, 0x43, 0x00, 0x8F, 0x00, 0x00, 0x00};
static const UInt8 kPopulateDeviceInfoMask[] {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF};
static const UInt8 kPopulateDeviceInfoPatched[] {0xBE, 0x2F, 0x0D, 0x00, 0x00, 0xFF, 0x90, 0x40, 0x01, 0x00, 0x00, 0xC7,
    0x43, 0x00, 0x8E, 0x00, 0x00, 0x00};

// Neutralise `AmdAtomVramInfo` creation null check.
// We don't have this entry in our VBIOS.
static const UInt8 kAmdAtomVramInfoNullCheckOriginal[] = {0x48, 0x89, 0x83, 0x90, 0x00, 0x00, 0x00, 0x48, 0x85, 0xC0,
    0x0F, 0x84, 0x89, 0x00, 0x00, 0x00, 0x48, 0x8B, 0x7B, 0x18};
static const UInt8 kAmdAtomVramInfoNullCheckPatched[] = {0x48, 0x89, 0x83, 0x90, 0x00, 0x00, 0x00, 0x66, 0x90, 0x66,
    0x90, 0x66, 0x90, 0x66, 0x90, 0x90, 0x48, 0x8B, 0x7B, 0x18};

// Ditto
static const UInt8 kAmdAtomVramInfoNullCheckOriginal1015[] = {0x48, 0x89, 0x83, 0x80, 0x00, 0x00, 0x00, 0x48, 0x85,
    0xC0, 0x74, 0x00};
static const UInt8 kAmdAtomVramInfoNullCheckOriginalMask1015[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0x00};
static const UInt8 kAmdAtomVramInfoNullCheckPatched1015[] = {0x48, 0x89, 0x83, 0x80, 0x00, 0x00, 0x00, 0x66, 0x90, 0x66,
    0x90, 0x90};

// Neutralise `AmdAtomPspDirectory` creation null check.
// We don't have this entry in our VBIOS.
static const UInt8 kAmdAtomPspDirectoryNullCheckOriginal[] = {0x48, 0x89, 0x83, 0x88, 0x00, 0x00, 0x00, 0x48, 0x85,
    0xC0, 0x0F, 0x84, 0xA1, 0x00, 0x00, 0x00, 0x48, 0x8B, 0x7B, 0x18};
static const UInt8 kAmdAtomPspDirectoryNullCheckPatched[] = {0x48, 0x89, 0x83, 0x88, 0x00, 0x00, 0x00, 0x66, 0x90, 0x66,
    0x90, 0x66, 0x90, 0x66, 0x90, 0x90, 0x48, 0x8B, 0x7B, 0x18};

// Neutralise `AmdAtomVramInfo` null check.
static const UInt8 kGetFirmwareInfoNullCheckOriginal[] = {0x48, 0x83, 0xBB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x84,
    0x00, 0x00, 0x00, 0x00, 0x49, 0x89};
static const UInt8 kGetFirmwareInfoNullCheckOriginalMask[] = {0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF};
static const UInt8 kGetFirmwareInfoNullCheckPatched[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x66, 0x90,
    0x66, 0x90, 0x66, 0x90, 0x00, 0x00};
static const UInt8 kGetFirmwareInfoNullCheckPatchedMask[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00};
static const UInt8 kGetFirmwareInfoNullCheckOriginal1404[] = {0x49, 0x83, 0xBC, 0x24, 0x90, 0x00, 0x00, 0x00, 0x00,
    0x0F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x49, 0x89};
static const UInt8 kGetFirmwareInfoNullCheckOriginalMask1404[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF};
static const UInt8 kGetFirmwareInfoNullCheckPatched1404[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x66,
    0x90, 0x66, 0x90, 0x66, 0x90, 0x00, 0x00};
static const UInt8 kGetFirmwareInfoNullCheckPatchedMask1404[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00};

// Tell AGDC that we're an iGPU.
static const UInt8 kGetVendorInfoOriginal[] = {0x48, 0x00, 0x02, 0x10, 0x00, 0x00, 0x02};
static const UInt8 kGetVendorInfoMask[] = {0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static const UInt8 kGetVendorInfoPatched[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
static const UInt8 kGetVendorInfoPatchedMask[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF};
static const UInt8 kGetVendorInfoOriginal1404[] = {0xC7, 0x00, 0x24, 0x02, 0x10, 0x00, 0x00, 0xC7, 0x00, 0x28, 0x02,
    0x00, 0x00, 0x00};
static const UInt8 kGetVendorInfoMask1404[] = {0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF};
static const UInt8 kGetVendorInfoPatched1404[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00};
static const UInt8 kGetVendorInfoPatchedMask1404[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF,
    0x00, 0x00, 0x00};

// Remove check for Navi family
static const UInt8 kInitializeDmcubServices1Original[] = {0x81, 0x79, 0x2C, 0x8F, 0x00, 0x00, 0x00};
static const UInt8 kInitializeDmcubServices1Patched[] = {0x39, 0xC0, 0x66, 0x90, 0x66, 0x90, 0x90};

// Set DMCUB ASIC constant to DCN 2.1
static const UInt8 kInitializeDmcubServices2Original[] = {0x83, 0xC0, 0xC4, 0x83, 0xF8, 0x0A, 0xB8, 0x03, 0x00, 0x00,
    0x00, 0x83, 0xD0, 0x00};
static const UInt8 kInitializeDmcubServices2Patched[] = {0xB8, 0x02, 0x00, 0x00, 0x00, 0x66, 0x90, 0x66, 0x90, 0x66,
    0x90, 0x66, 0x90, 0x90};

// Ditto, 14.4+
static const UInt8 kInitializeDmcubServices2Original1404[] = {0x83, 0xC0, 0xC4, 0x31, 0xC9, 0x83, 0xF8, 0x0A, 0x83,
    0xD1, 0x03};
static const UInt8 kInitializeDmcubServices2Patched1404[] = {0xB9, 0x02, 0x00, 0x00, 0x00, 0x66, 0x90, 0x66, 0x90, 0x66,
    0x90};

// Ditto, 10.15
static const UInt8 kInitializeDmcubServices2Original1015[] = {0xC7, 0x46, 0x20, 0x01, 0x00, 0x00, 0x00};
static const UInt8 kInitializeDmcubServices2Patched1015[] = {0xC7, 0x46, 0x20, 0x02, 0x00, 0x00, 0x00};

// 10.15: Set inst_const_size/bss_data_size to 0. To disable DMCUB firmware loading logic.
static const UInt8 kInitializeHardware1Original[] = {0x49, 0xBC, 0x00, 0x0A, 0x01, 0x00, 0xF4, 0x01, 0x00, 0x00};
static const UInt8 kInitializeHardware1Patched[] = {0x49, 0xC7, 0xC4, 0x00, 0x00, 0x00, 0x00, 0x90, 0x90, 0x90};

// 10.15: Set fw_inst_const to nullptr, pt.2 of above.
static const UInt8 kInitializeHardware2Original[] = {0x4C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4C, 0x00, 0x00, 0x10,
    0x4C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4C};
static const UInt8 kInitializeHardware2OriginalMask[] = {0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00,
    0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF};
static const UInt8 kInitializeHardware2Patched[] = {0x49, 0xC7, 0xC5, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const UInt8 kInitializeHardware2PatchedMask[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// 10.15: Disable DMCUB firmware loading from DAL. HWLibs should be doing that.
static const UInt8 kAmdDalServicesInitializeOriginal[] = {0xBE, 0x01, 0x00, 0x00, 0x00, 0xE8, 0x00, 0x00, 0x00, 0x00,
    0x49, 0x00, 0x00, 0x60};
static const UInt8 kAmdDalServicesInitializeOriginalMask[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00,
    0x00, 0xFF, 0x00, 0x00, 0xFF};
static const UInt8 kAmdDalServicesInitializePatched[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00};
static const UInt8 kAmdDalServicesInitializePatchedMask[] = {0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00};

// Raven: Change cursor and underflow tracker count to 4 instead of 6.
static const UInt8 kCreateControllerServicesOriginal[] = {0x40, 0x00, 0x00, 0x40, 0x83, 0x00, 0x06};
static const UInt8 kCreateControllerServicesOriginalMask[] = {0xF0, 0x00, 0x00, 0xF0, 0xFF, 0x00, 0xFF};
static const UInt8 kCreateControllerServicesPatched[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04};
static const UInt8 kCreateControllerServicesPatchedMask[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F};

// Ditto, 10.15.
static const UInt8 kCreateControllerServicesOriginal1015[] = {0x48, 0x00, 0x00, 0x48, 0x83, 0x00, 0x05};
static const UInt8 kCreateControllerServicesOriginalMask1015[] = {0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0xFF};
static const UInt8 kCreateControllerServicesPatched1015[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03};
static const UInt8 kCreateControllerServicesPatchedMask1015[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F};

// Raven: Change cursor count to 4 instead of 6.
static const UInt8 kSetupCursorsOriginal[] = {0x40, 0x83, 0x00, 0x05};
static const UInt8 kSetupCursorsOriginalMask[] = {0xF0, 0xFF, 0x00, 0xFF};
static const UInt8 kSetupCursorsPatched[] = {0x00, 0x00, 0x00, 0x03};
static const UInt8 kSetupCursorsPatchedMask[] = {0x00, 0x00, 0x00, 0x0F};

// Ditto, 12.0+.
static const UInt8 kSetupCursorsOriginal12[] = {0x40, 0x83, 0x00, 0x06};
static const UInt8 kSetupCursorsOriginalMask12[] = {0xF0, 0xFF, 0x00, 0xFF};
static const UInt8 kSetupCursorsPatched12[] = {0x00, 0x00, 0x00, 0x04};
static const UInt8 kSetupCursorsPatchedMask12[] = {0x00, 0x00, 0x00, 0x0F};

// Raven: Change link count to 4 instead of 6.
static const UInt8 kCreateLinksOriginal[] = {0x06, 0x00, 0x00, 0x00, 0x40};
static const UInt8 kCreateLinksOriginalMask[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xF0};
static const UInt8 kCreateLinksPatched[] = {0x04, 0x00, 0x00, 0x00, 0x00};
static const UInt8 kCreateLinksPatchedMask[] = {0x0F, 0x00, 0x00, 0x00, 0x00};

static iVega::X6000FB instance {};

iVega::X6000FB &iVega::X6000FB::singleton() { return instance; }

void iVega::X6000FB::processKext(KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size) {
    if (kextRadeonX6000Framebuffer.loadIndex != id) { return; }

    NRed::singleton().hwLateInit();

    CAILAsicCapsEntry *orgAsicCapsTable = nullptr;
    PenguinWizardry::PatternSolveRequest cailAsicCapsSolveRequest {"__ZL20CAIL_ASIC_CAPS_TABLE", orgAsicCapsTable,
        kCailAsicCapsTablePattern};
    PANIC_COND(!cailAsicCapsSolveRequest.solve(patcher, id, slide, size), "X6000FB",
        "Failed to resolve CAIL_ASIC_CAPS_TABLE");

    PenguinWizardry::PatternRouteRequest requests[] = {
        {"__ZNK15AmdAtomVramInfo16populateVramInfoER16AtomFirmwareInfo", populateVramInfo, kPopulateVramInfoPattern,
            kPopulateVramInfoPatternMask},
        {"__ZNK32AMDRadeonX6000_AmdAsicInfoNavi1027getEnumeratedRevisionNumberEv", getEnumeratedRevision},
        {"__ZNK22AmdAtomObjectInfo_V1_421getNumberOfConnectorsEv", wrapGetNumberOfConnectors,
            this->orgGetNumberOfConnectors, kGetNumberOfConnectorsPattern, kGetNumberOfConnectorsPatternMask},
    };
    PANIC_COND(!PenguinWizardry::PatternRouteRequest::routeAll(patcher, id, requests, slide, size), "X6000FB",
        "Failed to route symbols");

    if (currentKernelVersion() >= MACOS_11) {
        KernelPatcher::RouteRequest request {
            "__ZN32AMDRadeonX6000_AmdRegisterAccess20createRegisterAccessERNS_8InitDataE", wrapCreateRegisterAccess,
            this->orgCreateRegisterAccess};
        PANIC_COND(!patcher.routeMultiple(id, &request, 1, slide, size), "X6000FB",
            "Failed to route createRegisterAccess");
    }

    if (NRed::singleton().getAttributes().isRenoir()) {
        this->mapMemorySubRange = patcher.solveSymbol<mapMemorySubRange_t>(id,
            "__ZN37AMDRadeonX6000_AmdDeviceMemoryManager17mapMemorySubRangeE25AmdReservedMemorySelectoryyj", slide,
            size, true);
        PANIC_COND(this->mapMemorySubRange == nullptr, "X6000FB", "Failed to solve mapMemorySubRange");
        PenguinWizardry::PatternRouteRequest requests[] = {
            {"_IH_4_0_IVRing_InitHardware", wrapIH40IVRingInitHardware, this->orgIH40IVRingInitHardware,
                kIH40IVRingInitHardwarePattern, kIH40IVRingInitHardwarePatternMask},
            {"__ZN41AMDRadeonX6000_AmdDeviceMemoryManagerNavi21intializeReservedVramEv", initialiseReservedVRAM},
        };
        PANIC_COND(!PenguinWizardry::PatternRouteRequest::routeAll(patcher, id, requests, slide, size), "X6000FB",
            "Failed to route IH_4_0_IVRing_InitHardware and intializeReservedVram");
        if (currentKernelVersion() >= MACOS_14_4) {
            PenguinWizardry::PatternRouteRequest request {"_IRQMGR_WriteRegister", wrapIRQMGRWriteRegister,
                this->orgIRQMGRWriteRegister, kIRQMGRWriteRegisterPattern1404};
            PANIC_COND(!request.route(patcher, id, slide, size), "X6000FB",
                "Failed to route IRQMGR_WriteRegister (14.4+)");
        } else {
            PenguinWizardry::PatternRouteRequest request {"_IRQMGR_WriteRegister", wrapIRQMGRWriteRegister,
                this->orgIRQMGRWriteRegister, kIRQMGRWriteRegisterPattern};
            PANIC_COND(!request.route(patcher, id, slide, size), "X6000FB", "Failed to route IRQMGR_WriteRegister");
        }
    }

    const PenguinWizardry::MaskedLookupPatch patch {&kextRadeonX6000Framebuffer, kPopulateDeviceInfoOriginal,
        kPopulateDeviceInfoMask, kPopulateDeviceInfoPatched, kPopulateDeviceInfoMask, 1};
    PANIC_COND(!patch.apply(patcher, slide, size), "X6000FB", "Failed to apply populateDeviceInfo patch");

    if (currentKernelVersion() >= MACOS_14_4) {
        const PenguinWizardry::MaskedLookupPatch patches[] = {
            {&kextRadeonX6000Framebuffer, kGetFirmwareInfoNullCheckOriginal1404,
                kGetFirmwareInfoNullCheckOriginalMask1404, kGetFirmwareInfoNullCheckPatched1404,
                kGetFirmwareInfoNullCheckPatchedMask1404, 1},
            {&kextRadeonX6000Framebuffer, kGetVendorInfoOriginal1404, kGetVendorInfoMask1404, kGetVendorInfoPatched1404,
                kGetVendorInfoPatchedMask1404, 1},
        };
        PANIC_COND(!PenguinWizardry::MaskedLookupPatch::applyAll(patcher, patches, slide, size), "X6000FB",
            "Failed to apply patches (14.4)");
    } else {
        const PenguinWizardry::MaskedLookupPatch patches[] = {
            {&kextRadeonX6000Framebuffer, kGetFirmwareInfoNullCheckOriginal, kGetFirmwareInfoNullCheckOriginalMask,
                kGetFirmwareInfoNullCheckPatched, kGetFirmwareInfoNullCheckPatchedMask, 1},
            {&kextRadeonX6000Framebuffer, kGetVendorInfoOriginal, kGetVendorInfoMask, kGetVendorInfoPatched,
                kGetVendorInfoPatchedMask, 1},
        };
        PANIC_COND(!PenguinWizardry::MaskedLookupPatch::applyAll(patcher, patches, slide, size), "X6000FB",
            "Failed to apply patches");
    }

    if (currentKernelVersion() == MACOS_10_15) {
        const PenguinWizardry::MaskedLookupPatch patch {&kextRadeonX6000Framebuffer,
            kAmdAtomVramInfoNullCheckOriginal1015, kAmdAtomVramInfoNullCheckOriginalMask1015,
            kAmdAtomVramInfoNullCheckPatched1015, 1};
        PANIC_COND(!patch.apply(patcher, slide, size), "X6000FB", "Failed to apply null check patch");
    } else {
        const PenguinWizardry::MaskedLookupPatch patches[] = {
            {&kextRadeonX6000Framebuffer, kAmdAtomVramInfoNullCheckOriginal, kAmdAtomVramInfoNullCheckPatched, 1},
            {&kextRadeonX6000Framebuffer, kAmdAtomPspDirectoryNullCheckOriginal, kAmdAtomPspDirectoryNullCheckPatched,
                1},
        };
        PANIC_COND(!PenguinWizardry::MaskedLookupPatch::applyAll(patcher, patches, slide, size), "X6000FB",
            "Failed to apply null check patches");
    }

    if (NRed::singleton().getAttributes().isRenoir()) {
        const PenguinWizardry::MaskedLookupPatch patch {&kextRadeonX6000Framebuffer, kInitializeDmcubServices1Original,
            kInitializeDmcubServices1Patched, 1};
        PANIC_COND(!patch.apply(patcher, slide, size), "X6000FB",
            "Failed to apply initializeDmcubServices family id patch");
        if (currentKernelVersion() == MACOS_10_15) {
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
        } else if (currentKernelVersion() >= MACOS_14_4) {
            const PenguinWizardry::MaskedLookupPatch patch {&kextRadeonX6000Framebuffer,
                kInitializeDmcubServices2Original1404, kInitializeDmcubServices2Patched1404, 1};
            PANIC_COND(!patch.apply(patcher, slide, size), "X6000FB",
                "Failed to apply initializeDmcubServices ASIC patch (14.4+)");
        } else {
            const PenguinWizardry::MaskedLookupPatch patch {&kextRadeonX6000Framebuffer,
                kInitializeDmcubServices2Original, kInitializeDmcubServices2Patched, 1};
            PANIC_COND(!patch.apply(patcher, slide, size), "X6000FB",
                "Failed to apply initializeDmcubServices ASIC patch");
        }
    }

    PANIC_COND(MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) != KERN_SUCCESS, "X6000FB",
        "Failed to enable kernel writing");
    orgAsicCapsTable->familyId = AMD_FAMILY_RAVEN;
    orgAsicCapsTable->ddiCaps = NRed::singleton().getAttributes().isRenoirE() ? ddiCapsRenoirE :
                                NRed::singleton().getAttributes().isRenoir()  ? ddiCapsRenoir :
                                                                                ddiCapsRaven;
    orgAsicCapsTable->deviceId = NRed::singleton().getDeviceID();
    orgAsicCapsTable->revision = NRed::singleton().getDevRevision();
    orgAsicCapsTable->extRevision =
        static_cast<UInt32>(NRed::singleton().getEnumRevision()) + NRed::singleton().getDevRevision();
    orgAsicCapsTable->pciRevision = NRed::singleton().getPciRevision();
    MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);
    DBGLOG("X6000FB", "Applied DDI Caps patches");

    // XX: DCN 2 and newer have 6 display pipes, while DCN 1 (which is what Raven has) has only 4.
    // We need to patch the kext to create only 4 cursors, links and underflow trackers.
    if (!NRed::singleton().getAttributes().isRenoir()) {
        auto *const orgCreateControllerServices = patcher.solveSymbol<void *>(id,
            "__ZN40AMDRadeonX6000_AmdRadeonControllerNavi1024createControllerServicesEv", slide, size, true);
        PANIC_COND(orgCreateControllerServices == nullptr, "X6000FB", "Failed to solve createControllerServices");

        auto *const orgSetupCursors = patcher.solveSymbol<void *>(id,
            "__ZN34AMDRadeonX6000_AmdRadeonController12setupCursorsEv", slide, size, true);
        PANIC_COND(orgSetupCursors == nullptr, "X6000FB", "Failed to solve setupCursors");

        auto *const orgCreateLinks = patcher.solveSymbol<void *>(id,
            "__ZN34AMDRadeonX6000_AmdRadeonController11createLinksEv", slide, size, true);
        PANIC_COND(orgCreateLinks == nullptr, "X6000FB", "Failed to solve createLinks");

        if (currentKernelVersion() == MACOS_10_15) {
            PANIC_COND(!KernelPatcher::findAndReplaceWithMask(orgCreateControllerServices, PAGE_SIZE,
                           kCreateControllerServicesOriginal1015, kCreateControllerServicesOriginalMask1015,
                           kCreateControllerServicesPatched1015, kCreateControllerServicesPatchedMask1015, 1, 0),
                "X6000FB", "Failed to apply createControllerServices patch (10.15)");
        } else {
            PANIC_COND(!KernelPatcher::findAndReplaceWithMask(orgCreateControllerServices, PAGE_SIZE,
                           kCreateControllerServicesOriginal, kCreateControllerServicesOriginalMask,
                           kCreateControllerServicesPatched, kCreateControllerServicesPatchedMask, 2, 0),
                "X6000FB", "Failed to apply createControllerServices patch");
        }

        if (currentKernelVersion() >= MACOS_12) {
            PANIC_COND(!KernelPatcher::findAndReplaceWithMask(orgSetupCursors, PAGE_SIZE, kSetupCursorsOriginal12,
                           kSetupCursorsOriginalMask12, kSetupCursorsPatched12, kSetupCursorsPatchedMask12, 1, 0),
                "X6000FB", "Failed to apply setupCursors patch (12.0+)");
        } else {
            PANIC_COND(!KernelPatcher::findAndReplaceWithMask(orgSetupCursors, PAGE_SIZE, kSetupCursorsOriginal,
                           kSetupCursorsOriginalMask, kSetupCursorsPatched, kSetupCursorsPatchedMask, 1, 0),
                "X6000FB", "Failed to apply setupCursors patch");
        }

        PANIC_COND(!KernelPatcher::findAndReplaceWithMask(orgCreateLinks, PAGE_SIZE, kCreateLinksOriginal,
                       kCreateLinksOriginalMask, kCreateLinksPatched, kCreateLinksPatchedMask, 1, 0),
            "X6000FB", "Failed to apply createLinks patch");
    }
}

UInt16 iVega::X6000FB::getEnumeratedRevision() { return NRed::singleton().getEnumRevision(); }

IOReturn iVega::X6000FB::populateVramInfo(void *const, void *const fwInfo) {
    UInt32 channelCount = 1;
    auto *const table = NRed::singleton().getVBIOSDataTable<const IGPSystemInfo>(0x1E);
    UInt8 memoryType = 0;
    if (table == nullptr) {
        SYSLOG("X6000FB", "No iGPU System Info in Master Data Table");
    } else {
        DBGLOG("X6000FB", "Fetching VRAM info from iGPU System Info");
        if (table->header.formatRev == 1 && table->header.contentRev >= 11 && table->header.contentRev <= 12) {
            if (table->infoV11.umaChannelCount) { channelCount = table->infoV11.umaChannelCount; }
            memoryType = table->infoV11.memoryType;
        } else if (table->header.formatRev == 2 && table->header.contentRev >= 1 && table->header.contentRev <= 2) {
            if (table->infoV2.umaChannelCount) { channelCount = table->infoV2.umaChannelCount; }
            memoryType = table->infoV2.memoryType;
        } else {
            SYSLOG("X6000FB", "Unsupported formatRev, contentRev (%d, %d)", table->header.formatRev,
                table->header.contentRev);
        }
    }
    auto &videoMemoryType = getMember<VideoMemoryType>(fwInfo, 0x1C);
    switch (memoryType) {
        case kDDR2MemType:
        case kDDR2FBDIMMMemType:
        case kLPDDR2MemType: {
            videoMemoryType = VideoMemoryType::DDR2;
        } break;
        case kDDR3MemType:
        case kLPDDR3MemType: {
            videoMemoryType = VideoMemoryType::DDR3;
        } break;
        case kDDR4MemType:
        case kLPDDR4MemType:
        case kDDR5MemType:    // the kexts don't know about DDR5
        case kLPDDR5MemType: {
            videoMemoryType = VideoMemoryType::DDR4;
        } break;
        default: {
            DBGLOG("X6000FB", "Unsupported memory type %d. Assuming DDR4", memoryType);
            videoMemoryType = VideoMemoryType::DDR4;
        } break;
    }
    getMember<UInt32>(fwInfo, 0x20) = channelCount * 64;    // VRAM Width (64-bit channels)
    return kIOReturnSuccess;
}

UInt32 iVega::X6000FB::wrapGetNumberOfConnectors(void *const self) {
    if (!singleton().fixedVBIOS) {
        singleton().fixedVBIOS = true;
        struct DispObjInfoTableV1_4 *const objInfo = getMember<DispObjInfoTableV1_4 *>(self, 0x28);
        if (objInfo->formatRev == 1 && (objInfo->contentRev == 4 || objInfo->contentRev == 5)) {
            DBGLOG("X6000FB", "getNumberOfConnectors: Fixing VBIOS connectors");
            const auto n = objInfo->pathCount;
            for (size_t i = 0, j = 0; i < n; i++) {
                // Skip invalid device tags
                if (objInfo->paths[i].devTag == 0) {
                    objInfo->pathCount--;
                } else {
                    objInfo->paths[j++] = objInfo->paths[i];
                }
            }
        }
    }
    return FunctionCast(wrapGetNumberOfConnectors, singleton().orgGetNumberOfConnectors)(self);
}

bool iVega::X6000FB::wrapIH40IVRingInitHardware(void *const ctx, void *const param2) {
    auto ret = FunctionCast(wrapIH40IVRingInitHardware, singleton().orgIH40IVRingInitHardware)(ctx, param2);
    NRed::singleton().writeReg32(IH_CHICKEN, NRed::singleton().readReg32(IH_CHICKEN) | IH_MC_SPACE_GPA_ENABLE);
    return ret;
}

void iVega::X6000FB::wrapIRQMGRWriteRegister(void *const ctx, const UInt64 off, UInt32 value) {
    if (off == IH_CLK_CTRL) {
        if ((value & getBit(IH_DBUS_MUX_CLK_SOFT_OVERRIDE_SHIFT)) != 0) {
            value |= getBit(IH_IH_BUFFER_MEM_CLK_SOFT_OVERRIDE_SHIFT);
        }
    }
    FunctionCast(wrapIRQMGRWriteRegister, singleton().orgIRQMGRWriteRegister)(ctx, off, value);
}

void *iVega::X6000FB::wrapCreateRegisterAccess(void *const initData) {
    getMember<UInt32>(initData, 0x24) = SMUIO_BASE_0 + ROM_INDEX;
    getMember<UInt32>(initData, 0x28) = SMUIO_BASE_0 + ROM_DATA;
    return FunctionCast(wrapCreateRegisterAccess, singleton().orgCreateRegisterAccess)(initData);
}

IOReturn iVega::X6000FB::initialiseReservedVRAM(void *const self) {
    static constexpr IOOptionBits mapOptions = kIOMapWriteCombineCache | kIOMapAnywhere;
    auto ret = singleton().mapMemorySubRange(self, AmdReservedMemorySelector::Cursor1_32bpp, 0, 0x40000, mapOptions);
    if (ret != kIOReturnSuccess) { return ret; }
    ret = singleton().mapMemorySubRange(self, AmdReservedMemorySelector::Cursor1_2bpp, 0x40000, 0x40000, mapOptions);
    if (ret != kIOReturnSuccess) { return ret; }
    ret = singleton().mapMemorySubRange(self, AmdReservedMemorySelector::Cursor2_32bpp, 0x80000, 0x40000, mapOptions);
    if (ret != kIOReturnSuccess) { return ret; }
    ret = singleton().mapMemorySubRange(self, AmdReservedMemorySelector::Cursor2_2bpp, 0xC0000, 0x40000, mapOptions);
    if (ret != kIOReturnSuccess) { return ret; }
    ret = singleton().mapMemorySubRange(self, AmdReservedMemorySelector::Cursor3_32bpp, 0x100000, 0x40000, mapOptions);
    if (ret != kIOReturnSuccess) { return ret; }
    ret = singleton().mapMemorySubRange(self, AmdReservedMemorySelector::Cursor3_2bpp, 0x140000, 0x40000, mapOptions);
    if (ret != kIOReturnSuccess) { return ret; }
    ret = singleton().mapMemorySubRange(self, AmdReservedMemorySelector::Cursor4_32bpp, 0x180000, 0x40000, mapOptions);
    if (ret != kIOReturnSuccess) { return ret; }
    ret = singleton().mapMemorySubRange(self, AmdReservedMemorySelector::Cursor4_2bpp, 0x1C0000, 0x40000, mapOptions);
    if (ret != kIOReturnSuccess) { return ret; }
    ret = singleton().mapMemorySubRange(self, AmdReservedMemorySelector::Cursor5_32bpp, 0x200000, 0x40000, mapOptions);
    if (ret != kIOReturnSuccess) { return ret; }
    ret = singleton().mapMemorySubRange(self, AmdReservedMemorySelector::Cursor5_2bpp, 0x240000, 0x40000, mapOptions);
    if (ret != kIOReturnSuccess) { return ret; }
    ret = singleton().mapMemorySubRange(self, AmdReservedMemorySelector::Cursor6_32bpp, 0x280000, 0x40000, mapOptions);
    if (ret != kIOReturnSuccess) { return ret; }
    ret = singleton().mapMemorySubRange(self, AmdReservedMemorySelector::Cursor6_2bpp, 0x2C0000, 0x40000, mapOptions);
    if (ret != kIOReturnSuccess) { return ret; }
    ret = singleton().mapMemorySubRange(self, AmdReservedMemorySelector::PPLIBReserved, 0x300000, 0x100000, mapOptions);
    if (ret != kIOReturnSuccess) { return ret; }
    ret = singleton().mapMemorySubRange(self, AmdReservedMemorySelector::DMCUBReserved, 0x400000, 0x100000, mapOptions);
    if (ret != kIOReturnSuccess) { return ret; }
    return singleton().mapMemorySubRange(self, AmdReservedMemorySelector::ReserveVRAM, 0, 0x500000, mapOptions);
}
