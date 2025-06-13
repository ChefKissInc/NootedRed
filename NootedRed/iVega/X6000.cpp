// Copyright © 2022-2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#include <Headers/kern_api.hpp>
#include <PrivateHeaders/NRed.hpp>
#include <PrivateHeaders/PatcherPlus.hpp>
#include <PrivateHeaders/iVega/Regs/DCN1.hpp>
#include <PrivateHeaders/iVega/X6000.hpp>

//------ Target Kexts ------//

static const char *pathRadeonX6000 = "/System/Library/Extensions/AMDRadeonX6000.kext/Contents/MacOS/AMDRadeonX6000";

static KernelPatcher::KextInfo kextRadeonX6000 = {
    "com.apple.kext.AMDRadeonX6000",
    &pathRadeonX6000,
    1,
    {true},
    {},
    KernelPatcher::KextInfo::Unloaded,
};

//------ Patches ------//

// Mismatched `getTtlInterface` virtual calls
static const UInt8 kGetTtlInterfaceCallOriginal[] = {0x40, 0x80, 0x00, 0xFF, 0x90, 0xC8, 0x02, 0x00, 0x00};
static const UInt8 kGetTtlInterfaceCallOriginalMask[] = {0xF0, 0xF0, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static const UInt8 kGetTtlInterfaceCallPatched[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x00, 0x00, 0x00};
static const UInt8 kGetTtlInterfaceCallPatchedMask[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00};

// Mismatched `getGpuDebugPolicy` virtual calls.
static const UInt8 kGetGpuDebugPolicyCallOriginal[] = {0x48, 0x8B, 0x07, 0xFF, 0x90, 0xC0, 0x03, 0x00, 0x00};
static const UInt8 kGetGpuDebugPolicyCallPatched[] = {0x48, 0x8B, 0x07, 0xFF, 0x90, 0xC8, 0x03, 0x00, 0x00};

// Ditto
static const UInt8 kGetGpuDebugPolicyCallOriginal1015[] = {0x48, 0x8B, 0x07, 0xFF, 0x90, 0xC8, 0x03, 0x00, 0x00};
static const UInt8 kGetGpuDebugPolicyCallPatched1015[] = {0x48, 0x8B, 0x07, 0xFF, 0x90, 0xC0, 0x03, 0x00, 0x00};

// VTable Call to signalGPUWorkSubmitted.
// Doesn't exist on X5000, but looks like it isn't necessary, so we just NO-OP it.
static const UInt8 kHWChannelSubmitCommandBufferOriginal[] = {0x48, 0x8B, 0x07, 0xFF, 0x90, 0x30, 0x02, 0x00, 0x00,
    0x48, 0x8B, 0x43};
static const UInt8 kHWChannelSubmitCommandBufferPatched[] = {0x48, 0x8B, 0x07, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x48,
    0x8B, 0x43};

// Ditto
static const UInt8 kHWChannelSubmitCommandBufferOriginal1015[] = {0x48, 0x8B, 0x07, 0xFF, 0x90, 0x20, 0x02, 0x00, 0x00,
    0x49, 0x8B, 0x45};
static const UInt8 kHWChannelSubmitCommandBufferPatched1015[] = {0x48, 0x8B, 0x07, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90,
    0x49, 0x8B, 0x45};

// Mismatched `getScheduler` virtual calls.
static const UInt8 kGetSchedulerCallOriginal[] = {0x48, 0x8B, 0x07, 0xFF, 0x90, 0xB8, 0x03, 0x00, 0x00};
static const UInt8 kGetSchedulerCallPatched[] = {0x48, 0x8B, 0x07, 0xFF, 0x90, 0xC0, 0x03, 0x00, 0x00};

// Ditto
static const UInt8 kGetSchedulerCallOriginal13[] = {0x48, 0x8B, 0x07, 0xFF, 0x90, 0xB0, 0x03, 0x00, 0x00};
static const UInt8 kGetSchedulerCallPatched13[] = {0x48, 0x8B, 0x07, 0xFF, 0x90, 0xB8, 0x03, 0x00, 0x00};

// Ditto
static const UInt8 kGetSchedulerCallOriginal1015[] = {0x48, 0x8B, 0x07, 0xFF, 0x90, 0xC0, 0x03, 0x00, 0x00};
static const UInt8 kGetSchedulerCallPatched1015[] = {0x48, 0x8B, 0x07, 0xFF, 0x90, 0xB8, 0x03, 0x00, 0x00};

// Mismatched `isDeviceValid` virtual calls.
static const UInt8 kIsDeviceValidCallOriginal[] = {0x48, 0x8B, 0x07, 0xFF, 0x90, 0xA0, 0x02, 0x00, 0x00};
static const UInt8 kIsDeviceValidCallPatched[] = {0x48, 0x8B, 0x07, 0xFF, 0x90, 0x98, 0x02, 0x00, 0x00};

// Mismatched `isDevicePCITunnelled` virtual calls.
static const UInt8 kIsDevicePCITunnelledCallOriginal[] = {0x48, 0x8B, 0x07, 0xFF, 0x90, 0xB0, 0x02, 0x00, 0x00};
static const UInt8 kIsDevicePCITunnelledCallPatched[] = {0x48, 0x8B, 0x07, 0xFF, 0x90, 0xA8, 0x02, 0x00, 0x00};

// Mismatched `getSML` virtual calls.
static const UInt8 kGetSMLCallOriginal[] = {0x48, 0x8B, 0x07, 0xFF, 0x90, 0x98, 0x03, 0x00, 0x00};
static const UInt8 kGetSMLCallPatched[] = {0x48, 0x8B, 0x07, 0xFF, 0x90, 0x90, 0x03, 0x00, 0x00};

// Mismatched `getUbmSwizzleMode` virtual call in `fillUBMSurface`.
static const UInt8 kGetUbmSwizzleModeCallOriginal[] = {0xFF, 0x91, 0x78, 0x04, 0x00, 0x00};
static const UInt8 kGetUbmSwizzleModeCallPatched[] = {0xFF, 0x91, 0xA0, 0x04, 0x00, 0x00};

// Mismatched `getUbmTileMode` virtual call in `fillUBMSurface`.
static const UInt8 kGetUbmTileModeCallOriginal[] = {0xFF, 0x90, 0x80, 0x04, 0x00, 0x00};
static const UInt8 kGetUbmTileModeCallPatched[] = {0xFF, 0x90, 0xA8, 0x04, 0x00, 0x00};

// Mismatched `writeWaitForRenderingPipe` virtual call in `writeUpdateFrameBufferOffsetCommands`.
static const UInt8 kWriteWaitForRenderingPipeCallOriginal[] = {0xFF, 0x90, 0xB8, 0x02, 0x00, 0x00, 0x89, 0x45, 0xAC};
static const UInt8 kWriteWaitForRenderingPipeCallPatched[] = {0xFF, 0x90, 0xB0, 0x02, 0x00, 0x00, 0x89, 0x45, 0xAC};

// Mismatched `dummyWPTRUpdateDiag` virtual call in `WPTRDiagnostic`.
static const UInt8 kDummyWPTRUpdateDiagCallOriginal[] = {0x48, 0x8B, 0x80, 0x50, 0x02, 0x00, 0x00};
static const UInt8 kDummyWPTRUpdateDiagCallPatched[] = {0x48, 0x8B, 0x80, 0x48, 0x02, 0x00, 0x00};

// Mismatched `getPM4CommandsUtility` virtual call in `init`.
static const UInt8 kGetPM4CommandUtilityCallOriginal[] = {0xFF, 0x90, 0xA0, 0x03, 0x00, 0x00};
static const UInt8 kGetPM4CommandUtilityCallPatched[] = {0xFF, 0x90, 0x98, 0x03, 0x00, 0x00};

// Mismatched `getChannelDoorbellOffset` virtual call in `allocateMemoryResources`.
static const UInt8 kGetChannelDoorbellOffsetCallOriginal[] = {0x48, 0x8B, 0x07, 0xFF, 0x90, 0x88, 0x03, 0x00, 0x00};
static const UInt8 kGetChannelDoorbellOffsetCallPatched[] = {0x48, 0x8B, 0x07, 0xFF, 0x90, 0x80, 0x03, 0x00, 0x00};

// Mismatched `getDoorbellMemoryBaseAddress` virtual call in `allocateMemoryResources`.
static const UInt8 kGetDoorbellMemoryBaseAddressCallOriginal[] = {0xFF, 0x90, 0x80, 0x03, 0x00, 0x00};
static const UInt8 kGetDoorbellMemoryBaseAddressCallPatched[] = {0xFF, 0x90, 0x78, 0x03, 0x00, 0x00};

// Mismatched `updateUtilizationStatisticsCounter` virtual calls.
static const UInt8 kUpdateUtilizationStatisticsCounterCallOriginal[] = {0x41, 0xFF, 0x90, 0xE0, 0x03, 0x00, 0x00};
static const UInt8 kUpdateUtilizationStatisticsCounterCallPatched[] = {0x41, 0xFF, 0x90, 0xD8, 0x03, 0x00, 0x00};

// Mismatched `dumpASICHangState` virtual calls in `submitCommandBuffer`.
static const UInt8 kDumpASICHangStateCallOriginal[] = {0xFF, 0x90, 0xA8, 0x03, 0x00, 0x00};
static const UInt8 kDumpASICHangStateCallPatched[] = {0xFF, 0x90, 0xA0, 0x03, 0x00, 0x00};

// Mismatched `registerChannel` virtual call in `init`.
static const UInt8 kRegisterChannelCallOriginal[] = {0x4C, 0x89, 0xEE, 0xFF, 0x90, 0x28, 0x03, 0x00, 0x00};
static const UInt8 kRegisterChannelCallPatched[] = {0x4C, 0x89, 0xEE, 0xFF, 0x90, 0x20, 0x03, 0x00, 0x00};

// Mismatched `disableGfxOff` virtual calls.
static const UInt8 kDisableGfxOffCallOriginal[] = {0xFF, 0x90, 0x00, 0x04, 0x00, 0x00};
static const UInt8 kDisableGfxOffCallPatched[] = {0xFF, 0x90, 0xF8, 0x03, 0x00, 0x00};

// Mismatched `enableGfxOff` virtual calls.
static const UInt8 kEnableGfxOffCallOriginal[] = {0xFF, 0x90, 0x08, 0x04, 0x00, 0x00};
static const UInt8 kEnableGfxOffCallPatched[] = {0xFF, 0x90, 0x00, 0x04, 0x00, 0x00};

// Mismatched `getHWMemory` virtual calls.
static const UInt8 kGetHWMemoryCallOriginal[] = {0x18, 0x48, 0x8B, 0x07, 0xFF, 0x90, 0xE0, 0x02, 0x00, 0x00, 0x48};
static const UInt8 kGetHWMemoryCallPatched[] = {0x18, 0x48, 0x8B, 0x07, 0xFF, 0x90, 0xD8, 0x02, 0x00, 0x00, 0x48};

// Mismatched `getHWGart` virtual calls.
static const UInt8 kGetHWGartCallOriginal[] = {0x48, 0x8B, 0x07, 0xFF, 0x90, 0xE8, 0x02, 0x00, 0x00};
static const UInt8 kGetHWGartCallPatched[] = {0x48, 0x8B, 0x07, 0xFF, 0x90, 0xE0, 0x02, 0x00, 0x00};

// Mismatched `getChannelCount` virtual calls.
static const UInt8 kGetChannelCountCallOriginal[] = {0x48, 0x8B, 0x07, 0xFF, 0x90, 0x38, 0x03, 0x00, 0x00};
static const UInt8 kGetChannelCountCallPatched[] = {0x48, 0x8B, 0x07, 0xFF, 0x90, 0x30, 0x03, 0x00, 0x00};

// Mismatched `flushSystemCaches` virtual calls.
static const UInt8 kFlushSystemCachesCallOriginal[] = {0xFF, 0x90, 0xA8, 0x04, 0x00, 0x00};
static const UInt8 kFlushSystemCachesCallPatched[] = {0xFF, 0x90, 0xD0, 0x04, 0x00, 0x00};

// Mismatched `getIOPCIDevice` virtual calls.
static const UInt8 kGetIOPCIDeviceCallOriginal[] = {0xFF, 0x90, 0x90, 0x03, 0x00, 0x00};
static const UInt8 kGetIOPCIDeviceCallPatched[] = {0xFF, 0x90, 0x88, 0x03, 0x00, 0x00};

// Mismatched `getHWRegisters` virtual calls.
static const UInt8 kGetHWRegistersCallOriginal[] = {0x40, 0x80, 0x00, 0xFF, 0x90, 0xD8, 0x02, 0x00, 0x00};
static const UInt8 kGetHWRegistersCallOriginalMask[] = {0xF0, 0xF0, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static const UInt8 kGetHWRegistersCallPatched[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0xD0, 0x00, 0x00, 0x00};
static const UInt8 kGetHWRegistersCallPatchedMask[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00};

// Mismatched `getChannelWriteBackFrameAddr` virtual calls.
static const UInt8 kGetChannelWriteBackFrameAddrCallOriginal[] = {0xFF, 0x90, 0x50, 0x03, 0x00, 0x00, 0x40};
static const UInt8 kGetChannelWriteBackFrameAddrCallOriginalMask[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0};
static const UInt8 kGetChannelWriteBackFrameAddrCallPatched[] = {0x00, 0x00, 0x48, 0x00, 0x00, 0x00, 0x00};
static const UInt8 kGetChannelWriteBackFrameAddrCallPatchedMask[] = {0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00};

// Mismatched `getChannelWriteBackFrameOffset` virtual calls.
static const UInt8 kGetChannelWriteBackFrameOffsetCall1Original[] = {0x48, 0x8B, 0x07, 0xFF, 0x90, 0x48, 0x03, 0x00,
    0x00};
static const UInt8 kGetChannelWriteBackFrameOffsetCall1Patched[] = {0x48, 0x8B, 0x07, 0xFF, 0x90, 0x40, 0x03, 0x00,
    0x00};

// TODO: Provide an explanation here.
static const UInt8 kGetChannelWriteBackFrameOffsetCall2Original[] = {0x89, 0xDE, 0xFF, 0x90, 0x48, 0x03, 0x00, 0x00};
static const UInt8 kGetChannelWriteBackFrameOffsetCall2Patched[] = {0x89, 0xDE, 0xFF, 0x90, 0x40, 0x03, 0x00, 0x00};

// Mismatched `getHWChannel` virtual calls.
static const UInt8 kGetHWChannelCall1Original[] = {0x48, 0x8B, 0x07, 0xFF, 0x90, 0x20, 0x03, 0x00, 0x00, 0x48, 0x85,
    0xC0};
static const UInt8 kGetHWChannelCall1Patched[] = {0x48, 0x8B, 0x07, 0xFF, 0x90, 0x18, 0x03, 0x00, 0x00, 0x48, 0x85,
    0xC0};

// TODO: Provide an explanation here.
static const UInt8 kGetHWChannelCall2Original[] = {0x31, 0xD2, 0xFF, 0x90, 0x18, 0x03, 0x00, 0x00};
static const UInt8 kGetHWChannelCall2Patched[] = {0x31, 0xD2, 0xFF, 0x90, 0x10, 0x03, 0x00, 0x00};

// TODO: Provide an explanation here.
static const UInt8 kGetHWChannelCall3Original[] = {0x00, 0x00, 0x00, 0xFF, 0x90, 0x18, 0x03, 0x00, 0x00};
static const UInt8 kGetHWChannelCall3Patched[] = {0x00, 0x00, 0x00, 0xFF, 0x90, 0x10, 0x03, 0x00, 0x00};

// Mismatched `getHWAlignManager` virtual calls.
static const UInt8 kGetHWAlignManagerCall1Original[] = {0x48, 0x80, 0x00, 0xFF, 0x90, 0x00, 0x03, 0x00, 0x00};
static const UInt8 kGetHWAlignManagerCall1OriginalMask[] = {0xFF, 0xF0, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static const UInt8 kGetHWAlignManagerCall1Patched[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x02, 0x00, 0x00};
static const UInt8 kGetHWAlignManagerCall1PatchedMask[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00};

// TODO: Provide an explanation here.
static const UInt8 kGetHWAlignManagerCall2Original[] = {0x49, 0x89, 0xD4, 0xFF, 0x90, 0x00, 0x03, 0x00, 0x00};
static const UInt8 kGetHWAlignManagerCall2Patched[] = {0x49, 0x89, 0xD4, 0xFF, 0x90, 0xF8, 0x02, 0x00, 0x00};

// Mismatched `getHWEngine` virtual calls.
static const UInt8 kGetHWEngineCallOriginal[] = {0x00, 0x00, 0x00, 0xFF, 0x90, 0x10, 0x03, 0x00, 0x00};
static const UInt8 kGetHWEngineCallPatched[] = {0x00, 0x00, 0x00, 0xFF, 0x90, 0x08, 0x03, 0x00, 0x00};

// Mismatched `getAMDHWHandler` virtual calls.
static const UInt8 kGetAMDHWHandlerCallOriginal[] = {0xFF, 0x90, 0xD0, 0x02, 0x00, 0x00};
static const UInt8 kGetAMDHWHandlerCallPatched[] = {0xFF, 0x90, 0xC8, 0x02, 0x00, 0x00};

template<UInt32 N>
struct HWAlignVTableFix {
    const UInt32 offs[N];
    const UInt32 occurances[N];
    const UInt32 len {N};

    void apply(void *toFunction) const {
        for (UInt32 i = 0; i < this->len; i += 1) {
            const UInt32 off = this->offs[i];
            const UInt32 newOff = (off == 0x128) ? 0x230 : (off - 8);
            const UInt32 count = this->occurances[i];
            const UInt8 vtableCallPattern[] = {0xFF, 0x00, static_cast<UInt8>(off & 0xFF),
                static_cast<UInt8>((off >> 8) & 0xFF), static_cast<UInt8>((off >> 16) & 0xFF),
                static_cast<UInt8>((off >> 24) & 0xFF)};
            const UInt8 vtableCallMask[] = {0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF};
            const UInt8 vtableCallReplacement[] = {0xFF, 0x00, static_cast<UInt8>(newOff & 0xFF),
                static_cast<UInt8>((newOff >> 8) & 0xFF), static_cast<UInt8>((newOff >> 16) & 0xFF),
                static_cast<UInt8>((newOff >> 24) & 0xFF)};
            PANIC_COND(!KernelPatcher::findAndReplaceWithMask(toFunction, PAGE_SIZE, vtableCallPattern, vtableCallMask,
                           vtableCallReplacement, vtableCallMask, count, 0),
                "X6000", "Failed to apply virtual call fix");
        }
    }
};

static const HWAlignVTableFix<2> FillUBMSurfaceVTFix {
    {0x1B8, 0x218},
    {1, 1},
};

static const HWAlignVTableFix<3> ConfigureDisplayVTFix {
    {0x1B8, 0x200, 0x218},
    {2, 2, 2},
};

static const HWAlignVTableFix<4> GetDisplayInfoVTFix {
    {0x128, 0x130, 0x138, 0x1D0},
    {1, 2, 2, 4},
};

static const HWAlignVTableFix<5> AllocateScanoutFBVTFix {
    {0x130, 0x138, 0x190, 0x1B0, 0x218},
    {1, 1, 1, 1, 1},
};

//------ Module Logic ------//

static iVega::X6000 instance {};

iVega::X6000 &iVega::X6000::singleton() { return instance; }

void iVega::X6000::init() {
    PANIC_COND(this->initialised, "X6000", "Attempted to initialise module twice!");
    this->initialised = true;

    switch (getKernelVersion()) {
        case KernelVersion::Catalina:
            this->regBaseField = 0x4838;
            break;
        case KernelVersion::BigSur:
        case KernelVersion::Monterey:
            this->regBaseField = 0x4830;
            break;
        case KernelVersion::Ventura:
        case KernelVersion::Sonoma:
        case KernelVersion::Sequoia:
        case KernelVersion::Tahoe:
            this->regBaseField = 0x590;
            break;
        default:
            PANIC("X6000", "Unknown kernel version %d", getKernelVersion());
    }

    SYSLOG("X6000", "Module initialised.");

    lilu.onKextLoadForce(
        &kextRadeonX6000, 1,
        [](void *user, KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size) {
            static_cast<iVega::X6000 *>(user)->processKext(patcher, id, slide, size);
        },
        this);
}

void iVega::X6000::processKext(KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size) {
    if (kextRadeonX6000.loadIndex != id) { return; }

    NRed::singleton().hwLateInit();

    void *orgFillUBMSurface = nullptr, *orgConfigureDisplay = nullptr, *orgGetDisplayInfo = nullptr,
         *orgAllocateScanoutFB = nullptr;

    PatcherPlus::PatternSolveRequest solveRequests[] = {
        {"__ZN31AMDRadeonX6000_AMDGFX10Hardware20allocateAMDHWDisplayEv", this->orgAllocateAMDHWDisplay},
        {"__ZN35AMDRadeonX6000_AMDAccelVideoContext10gMetaClassE", NRed::singleton().metaClassMap[0][1]},
        {"__ZN37AMDRadeonX6000_AMDAccelDisplayMachine10gMetaClassE", NRed::singleton().metaClassMap[1][1]},
        {"__ZN34AMDRadeonX6000_AMDAccelDisplayPipe10gMetaClassE", NRed::singleton().metaClassMap[2][1]},
        {"__ZN30AMDRadeonX6000_AMDAccelChannel10gMetaClassE", NRed::singleton().metaClassMap[3][0]},
        {"__ZN28AMDRadeonX6000_IAMDHWChannel10gMetaClassE", NRed::singleton().metaClassMap[4][1]},
        {"__ZN27AMDRadeonX6000_AMDHWDisplay14fillUBMSurfaceEjP17_FRAMEBUFFER_INFOP13_UBM_SURFINFO", orgFillUBMSurface},
        {"__ZN27AMDRadeonX6000_AMDHWDisplay16configureDisplayEjjP17_FRAMEBUFFER_INFOP16IOAccelResource2",
            orgConfigureDisplay},
        {"__ZN27AMDRadeonX6000_AMDHWDisplay14getDisplayInfoEjbbPvP17_FRAMEBUFFER_INFO", orgGetDisplayInfo},
    };
    PANIC_COND(!PatcherPlus::PatternSolveRequest::solveAll(patcher, id, solveRequests, slide, size), "X6000",
        "Failed to resolve symbols");

    if (!NRed::singleton().getAttributes().isVenturaAndLater()) {
        PatcherPlus::PatternSolveRequest request {
            "__ZN27AMDRadeonX6000_AMDHWDisplay17allocateScanoutFBEjP16IOAccelResource2S1_Py", orgAllocateScanoutFB};
        PANIC_COND(!request.solve(patcher, id, slide, size), "X6000", "Failed to resolve allocateScanout");
    }

    PatcherPlus::PatternRouteRequest accelStartRequest {"__ZN37AMDRadeonX6000_AMDGraphicsAccelerator5startEP9IOService",
        accelStartX6000};
    PANIC_COND(!accelStartRequest.route(patcher, id, slide, size), "X6000",
        "Failed to route AMDGraphicsAccelerator::start");

    if (NRed::singleton().getAttributes().isRaven()) {
        PatcherPlus::PatternRouteRequest request {"__ZN30AMDRadeonX6000_AMDGFX10Display23initDCNRegistersOffsetsEv",
            wrapInitDCNRegistersOffsets, this->orgInitDCNRegistersOffsets};
        PANIC_COND(!request.route(patcher, id, slide, size), "X6000", "Failed to route initDCNRegistersOffsets");
    }

    if (NRed::singleton().getAttributes().isCatalina()) {
        const PatcherPlus::MaskedLookupPatch patches[] = {
            {&kextRadeonX6000, kHWChannelSubmitCommandBufferOriginal1015, kHWChannelSubmitCommandBufferPatched1015, 1},
            {&kextRadeonX6000, kDummyWPTRUpdateDiagCallOriginal, kDummyWPTRUpdateDiagCallPatched, 1},
        };
        SYSLOG_COND(!PatcherPlus::MaskedLookupPatch::applyAll(patcher, patches, slide, size, true), "X6000",
            "Failed to apply patches");
        patcher.clearError();
    } else {
        const PatcherPlus::MaskedLookupPatch patch {&kextRadeonX6000, kHWChannelSubmitCommandBufferOriginal,
            kHWChannelSubmitCommandBufferPatched, 1};
        SYSLOG_COND(!patch.apply(patcher, slide, size), "X6000", "Failed to apply submitCommandBuffer patch");
        patcher.clearError();
    }

    const PatcherPlus::MaskedLookupPatch patches[] = {
        {&kextRadeonX6000, kIsDeviceValidCallOriginal, kIsDeviceValidCallPatched,
            NRed::singleton().getAttributes().isCatalina()        ? 20U :
            NRed::singleton().getAttributes().isVenturaAndLater() ? 23 :
            NRed::singleton().getAttributes().isMonterey()        ? 26 :
                                                                    24},
        {&kextRadeonX6000, kIsDevicePCITunnelledCallOriginal, kIsDevicePCITunnelledCallPatched,
            NRed::singleton().getAttributes().isCatalina()        ? 9U :
            NRed::singleton().getAttributes().isVenturaAndLater() ? 3 :
                                                                    1},
    };
    SYSLOG_COND(!PatcherPlus::MaskedLookupPatch::applyAll(patcher, patches, slide, size, true), "X6000",
        "Failed to apply patches");
    patcher.clearError();

    if (NRed::singleton().getAttributes().isCatalina()) {
        const PatcherPlus::MaskedLookupPatch patches[] = {
            {&kextRadeonX6000, kWriteWaitForRenderingPipeCallOriginal, kWriteWaitForRenderingPipeCallPatched, 1},
            {&kextRadeonX6000, kGetTtlInterfaceCallOriginal, kGetTtlInterfaceCallOriginalMask,
                kGetTtlInterfaceCallPatched, kGetTtlInterfaceCallPatchedMask, 38},
            {&kextRadeonX6000, kGetAMDHWHandlerCallOriginal, kGetAMDHWHandlerCallPatched, 19},
            {&kextRadeonX6000, kGetAMDHWHandlerCallOriginal, kGetAMDHWHandlerCallPatched,
                arrsize(kGetAMDHWHandlerCallOriginal), 64, 1},
            {&kextRadeonX6000, kGetHWRegistersCallOriginal, kGetHWRegistersCallOriginalMask, kGetHWRegistersCallPatched,
                kGetHWRegistersCallPatchedMask, 13},
            {&kextRadeonX6000, kGetHWMemoryCallOriginal, kGetHWMemoryCallPatched, 11},
            {&kextRadeonX6000, kGetHWGartCallOriginal, kGetHWGartCallPatched, 9},
            {&kextRadeonX6000, kGetHWAlignManagerCall1Original, kGetHWAlignManagerCall1OriginalMask,
                kGetHWAlignManagerCall1Patched, kGetHWAlignManagerCall1PatchedMask, 33},
            {&kextRadeonX6000, kGetHWAlignManagerCall2Original, kGetHWAlignManagerCall2Patched, 1},
            {&kextRadeonX6000, kGetHWEngineCallOriginal, kGetHWEngineCallPatched, 31},
            {&kextRadeonX6000, kGetHWChannelCall1Original, kGetHWChannelCall1Patched, 2},
            {&kextRadeonX6000, kGetHWChannelCall2Original, kGetHWChannelCall2Patched, 53},
            {&kextRadeonX6000, kGetHWChannelCall3Original, kGetHWChannelCall3Patched, 20},
            {&kextRadeonX6000, kRegisterChannelCallOriginal, kRegisterChannelCallPatched, 1},
            {&kextRadeonX6000, kGetChannelCountCallOriginal, kGetChannelCountCallPatched, 7},
            {&kextRadeonX6000, kGetChannelWriteBackFrameOffsetCall1Original,
                kGetChannelWriteBackFrameOffsetCall1Patched, 4},
            {&kextRadeonX6000, kGetChannelWriteBackFrameOffsetCall2Original,
                kGetChannelWriteBackFrameOffsetCall2Patched, 1},
            {&kextRadeonX6000, kGetChannelWriteBackFrameAddrCallOriginal, kGetChannelWriteBackFrameAddrCallOriginalMask,
                kGetChannelWriteBackFrameAddrCallPatched, kGetChannelWriteBackFrameAddrCallPatchedMask, 10},
            {&kextRadeonX6000, kGetDoorbellMemoryBaseAddressCallOriginal, kGetDoorbellMemoryBaseAddressCallPatched, 1},
            {&kextRadeonX6000, kGetChannelDoorbellOffsetCallOriginal, kGetChannelDoorbellOffsetCallPatched, 1},
            {&kextRadeonX6000, kGetIOPCIDeviceCallOriginal, kGetIOPCIDeviceCallPatched, 5},
            {&kextRadeonX6000, kGetSMLCallOriginal, kGetSMLCallPatched, 10},
            {&kextRadeonX6000, kGetPM4CommandUtilityCallOriginal, kGetPM4CommandUtilityCallPatched, 2},
            {&kextRadeonX6000, kDumpASICHangStateCallOriginal, kDumpASICHangStateCallPatched, 2},
            {&kextRadeonX6000, kGetSchedulerCallOriginal1015, kGetSchedulerCallPatched1015, 22},
        };
        SYSLOG_COND(!PatcherPlus::MaskedLookupPatch::applyAll(patcher, patches, slide, size, true), "X6000",
            "Failed to apply patches");
        patcher.clearError();
    }

    if (NRed::singleton().getAttributes().isVenturaAndLater()) {
        const PatcherPlus::MaskedLookupPatch patch {&kextRadeonX6000, kGetSchedulerCallOriginal13,
            kGetSchedulerCallPatched13, 24};
        SYSLOG_COND(!patch.apply(patcher, slide, size), "X6000", "Failed to apply getScheduler patch");
        patcher.clearError();
    } else if (!NRed::singleton().getAttributes().isCatalina()) {
        const PatcherPlus::MaskedLookupPatch patch {&kextRadeonX6000, kGetSchedulerCallOriginal,
            kGetSchedulerCallPatched, NRed::singleton().getAttributes().isMonterey() ? 21U : 22};
        SYSLOG_COND(!patch.apply(patcher, slide, size), "X6000", "Failed to apply getScheduler patch");
        patcher.clearError();
    }

    if (NRed::singleton().getAttributes().isCatalina()) {
        const PatcherPlus::MaskedLookupPatch patches[] = {
            {&kextRadeonX6000, kGetGpuDebugPolicyCallOriginal1015, kGetGpuDebugPolicyCallPatched1015, 27},
            {&kextRadeonX6000, kUpdateUtilizationStatisticsCounterCallOriginal,
                kUpdateUtilizationStatisticsCounterCallPatched, 2},
            {&kextRadeonX6000, kDisableGfxOffCallOriginal, kDisableGfxOffCallPatched, 17},
            {&kextRadeonX6000, kEnableGfxOffCallOriginal, kEnableGfxOffCallPatched, 16},
            {&kextRadeonX6000, kFlushSystemCachesCallOriginal, kFlushSystemCachesCallPatched, 4},
            {&kextRadeonX6000, kGetUbmSwizzleModeCallOriginal, kGetUbmSwizzleModeCallPatched, 1},
            {&kextRadeonX6000, kGetUbmTileModeCallOriginal, kGetUbmTileModeCallPatched, 1},
        };
        SYSLOG_COND(!PatcherPlus::MaskedLookupPatch::applyAll(patcher, patches, slide, size, true), "X6000",
            "Failed to apply patches");
        patcher.clearError();
    } else {
        const PatcherPlus::MaskedLookupPatch patch {&kextRadeonX6000, kGetGpuDebugPolicyCallOriginal,
            kGetGpuDebugPolicyCallPatched,
            NRed::singleton().getAttributes().isVentura1304Based() ? 38U :
            NRed::singleton().getAttributes().isVenturaAndLater()  ? 37 :
                                                                     28};
        SYSLOG_COND(!patch.apply(patcher, slide, size), "X6000", "Failed to apply getGpuDebugPolicy patch");
        patcher.clearError();
    }

    // Now, for AMDHWDisplay, fix the VTable offsets to calls in HWAlignManager2.
    FillUBMSurfaceVTFix.apply(orgFillUBMSurface);
    ConfigureDisplayVTFix.apply(orgConfigureDisplay);
    GetDisplayInfoVTFix.apply(orgGetDisplayInfo);
    if (orgAllocateScanoutFB != nullptr) { AllocateScanoutFBVTFix.apply(orgAllocateScanoutFB); }
}

/**
 * We don't want the `AMDRadeonX6000` personality defined in the `Info.plist` to do anything.
 * We only use it to force-load `AMDRadeonX6000` and snatch the VCN/DCN symbols.
 */
bool iVega::X6000::accelStartX6000() { return false; }

void iVega::X6000::wrapInitDCNRegistersOffsets(void *that) {
    FunctionCast(wrapInitDCNRegistersOffsets, singleton().orgInitDCNRegistersOffsets)(that);
    auto base = singleton().regBaseField.get(that);
    (singleton().regBaseField + 0x10).set(that, base + HUBPREQ0_DCSURF_PRIMARY_SURFACE_ADDRESS);
    (singleton().regBaseField + 0x48).set(that, base + HUBPREQ1_DCSURF_PRIMARY_SURFACE_ADDRESS);
    (singleton().regBaseField + 0x80).set(that, base + HUBPREQ2_DCSURF_PRIMARY_SURFACE_ADDRESS);
    (singleton().regBaseField + 0xB8).set(that, base + HUBPREQ3_DCSURF_PRIMARY_SURFACE_ADDRESS);
    (singleton().regBaseField + 0x14).set(that, base + HUBPREQ0_DCSURF_PRIMARY_SURFACE_ADDRESS_HIGH);
    (singleton().regBaseField + 0x4C).set(that, base + HUBPREQ1_DCSURF_PRIMARY_SURFACE_ADDRESS_HIGH);
    (singleton().regBaseField + 0x84).set(that, base + HUBPREQ2_DCSURF_PRIMARY_SURFACE_ADDRESS_HIGH);
    (singleton().regBaseField + 0xBC).set(that, base + HUBPREQ3_DCSURF_PRIMARY_SURFACE_ADDRESS_HIGH);
    (singleton().regBaseField + 0x18).set(that, base + HUBP0_DCSURF_SURFACE_CONFIG);
    (singleton().regBaseField + 0x50).set(that, base + HUBP1_DCSURF_SURFACE_CONFIG);
    (singleton().regBaseField + 0x88).set(that, base + HUBP2_DCSURF_SURFACE_CONFIG);
    (singleton().regBaseField + 0xC0).set(that, base + HUBP3_DCSURF_SURFACE_CONFIG);
    (singleton().regBaseField + 0x1C).set(that, base + HUBPREQ0_DCSURF_SURFACE_PITCH);
    (singleton().regBaseField + 0x54).set(that, base + HUBPREQ1_DCSURF_SURFACE_PITCH);
    (singleton().regBaseField + 0x8C).set(that, base + HUBPREQ2_DCSURF_SURFACE_PITCH);
    (singleton().regBaseField + 0xC4).set(that, base + HUBPREQ3_DCSURF_SURFACE_PITCH);
    (singleton().regBaseField + 0x20).set(that, base + HUBP0_DCSURF_ADDR_CONFIG);
    (singleton().regBaseField + 0x58).set(that, base + HUBP1_DCSURF_ADDR_CONFIG);
    (singleton().regBaseField + 0x90).set(that, base + HUBP2_DCSURF_ADDR_CONFIG);
    (singleton().regBaseField + 0xC8).set(that, base + HUBP3_DCSURF_ADDR_CONFIG);
    (singleton().regBaseField + 0x24).set(that, base + HUBP0_DCSURF_TILING_CONFIG);
    (singleton().regBaseField + 0x5C).set(that, base + HUBP1_DCSURF_TILING_CONFIG);
    (singleton().regBaseField + 0x94).set(that, base + HUBP2_DCSURF_TILING_CONFIG);
    (singleton().regBaseField + 0xCC).set(that, base + HUBP3_DCSURF_TILING_CONFIG);
    (singleton().regBaseField + 0x28).set(that, base + HUBP0_DCSURF_PRI_VIEWPORT_START);
    (singleton().regBaseField + 0x60).set(that, base + HUBP1_DCSURF_PRI_VIEWPORT_START);
    (singleton().regBaseField + 0x98).set(that, base + HUBP2_DCSURF_PRI_VIEWPORT_START);
    (singleton().regBaseField + 0xD0).set(that, base + HUBP3_DCSURF_PRI_VIEWPORT_START);
    (singleton().regBaseField + 0x2C).set(that, base + HUBP0_DCSURF_PRI_VIEWPORT_DIMENSION);
    (singleton().regBaseField + 0x64).set(that, base + HUBP1_DCSURF_PRI_VIEWPORT_DIMENSION);
    (singleton().regBaseField + 0x9C).set(that, base + HUBP2_DCSURF_PRI_VIEWPORT_DIMENSION);
    (singleton().regBaseField + 0xD4).set(that, base + HUBP3_DCSURF_PRI_VIEWPORT_DIMENSION);
    (singleton().regBaseField + 0x30).set(that, base + OTG0_OTG_CONTROL);
    (singleton().regBaseField + 0x68).set(that, base + OTG1_OTG_CONTROL);
    (singleton().regBaseField + 0xA0).set(that, base + OTG2_OTG_CONTROL);
    (singleton().regBaseField + 0xD8).set(that, base + OTG3_OTG_CONTROL);
    (singleton().regBaseField + 0x110).set(that, base + OTG4_OTG_CONTROL);
    (singleton().regBaseField + 0x148).set(that, base + OTG5_OTG_CONTROL);
    (singleton().regBaseField + 0x34).set(that, base + OTG0_OTG_INTERLACE_CONTROL);
    (singleton().regBaseField + 0x6C).set(that, base + OTG1_OTG_INTERLACE_CONTROL);
    (singleton().regBaseField + 0xA4).set(that, base + OTG2_OTG_INTERLACE_CONTROL);
    (singleton().regBaseField + 0xDC).set(that, base + OTG3_OTG_INTERLACE_CONTROL);
    (singleton().regBaseField + 0x114).set(that, base + OTG4_OTG_INTERLACE_CONTROL);
    (singleton().regBaseField + 0x14C).set(that, base + OTG5_OTG_INTERLACE_CONTROL);
    (singleton().regBaseField + 0x38).set(that, base + HUBPREQ0_DCSURF_FLIP_CONTROL);
    (singleton().regBaseField + 0x70).set(that, base + HUBPREQ1_DCSURF_FLIP_CONTROL);
    (singleton().regBaseField + 0xA8).set(that, base + HUBPREQ2_DCSURF_FLIP_CONTROL);
    (singleton().regBaseField + 0xE0).set(that, base + HUBPREQ3_DCSURF_FLIP_CONTROL);
    (singleton().regBaseField + 0x3C).set(that, base + HUBPRET0_HUBPRET_CONTROL);
    (singleton().regBaseField + 0x74).set(that, base + HUBPRET1_HUBPRET_CONTROL);
    (singleton().regBaseField + 0xAC).set(that, base + HUBPRET2_HUBPRET_CONTROL);
    (singleton().regBaseField + 0xE4).set(that, base + HUBPRET3_HUBPRET_CONTROL);
    (singleton().regBaseField + 0x40).set(that, base + HUBPREQ0_DCSURF_SURFACE_EARLIEST_INUSE);
    (singleton().regBaseField + 0x78).set(that, base + HUBPREQ1_DCSURF_SURFACE_EARLIEST_INUSE);
    (singleton().regBaseField + 0xB0).set(that, base + HUBPREQ2_DCSURF_SURFACE_EARLIEST_INUSE);
    (singleton().regBaseField + 0xE8).set(that, base + HUBPREQ3_DCSURF_SURFACE_EARLIEST_INUSE);
    (singleton().regBaseField + 0x44).set(that, base + HUBPREQ0_DCSURF_SURFACE_EARLIEST_INUSE_HIGH);
    (singleton().regBaseField + 0x7C).set(that, base + HUBPREQ1_DCSURF_SURFACE_EARLIEST_INUSE_HIGH);
    (singleton().regBaseField + 0xB4).set(that, base + HUBPREQ2_DCSURF_SURFACE_EARLIEST_INUSE_HIGH);
    (singleton().regBaseField + 0xEC).set(that, base + HUBPREQ3_DCSURF_SURFACE_EARLIEST_INUSE_HIGH);
}
