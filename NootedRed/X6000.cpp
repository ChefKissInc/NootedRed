// Copyright © 2022-2024 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#include "X6000.hpp"
#include "AMDCommon.hpp"
#include "NRed.hpp"
#include "PatcherPlus.hpp"
#include <Headers/kern_api.hpp>

static const char *pathRadeonX6000 = "/System/Library/Extensions/AMDRadeonX6000.kext/Contents/MacOS/AMDRadeonX6000";

static KernelPatcher::KextInfo kextRadeonX6000 = {
    "com.apple.kext.AMDRadeonX6000",
    &pathRadeonX6000,
    1,
    {true},
    {},
    KernelPatcher::KextInfo::Unloaded,
};

X6000 *X6000::callback = nullptr;

void X6000::init() {
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
            this->regBaseField = 0x590;
            break;
        default:
            PANIC("X6000", "Unknown kernel version %d", getKernelVersion());
    }

    SYSLOG("X6000", "Module initialised");

    callback = this;
    lilu.onKextLoadForce(&kextRadeonX6000);
}

bool X6000::processKext(KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size) {
    if (kextRadeonX6000.loadIndex == id) {
        SYSLOG_COND(ADDPR(debugEnabled), "X6000", "slide is 0x%llx", slide);
        NRed::callback->hwLateInit();

        void *orgFillUBMSurface, *orgConfigureDisplay, *orgGetDisplayInfo, *orgAllocateScanoutFB;

        SolveRequestPlus solveRequests[] = {
            {"__ZN31AMDRadeonX6000_AMDGFX10Hardware20allocateAMDHWDisplayEv", this->orgAllocateAMDHWDisplay},
            {"__ZN35AMDRadeonX6000_AMDAccelVideoContext10gMetaClassE", NRed::callback->metaClassMap[0][1]},
            {"__ZN37AMDRadeonX6000_AMDAccelDisplayMachine10gMetaClassE", NRed::callback->metaClassMap[1][1]},
            {"__ZN34AMDRadeonX6000_AMDAccelDisplayPipe10gMetaClassE", NRed::callback->metaClassMap[2][1]},
            {"__ZN30AMDRadeonX6000_AMDAccelChannel10gMetaClassE", NRed::callback->metaClassMap[3][0]},
            {"__ZN28AMDRadeonX6000_IAMDHWChannel10gMetaClassE", NRed::callback->metaClassMap[4][1]},
            {"__ZN27AMDRadeonX6000_AMDHWDisplay14fillUBMSurfaceEjP17_FRAMEBUFFER_INFOP13_UBM_SURFINFO",
                orgFillUBMSurface},
            {"__ZN27AMDRadeonX6000_AMDHWDisplay16configureDisplayEjjP17_FRAMEBUFFER_INFOP16IOAccelResource2",
                orgConfigureDisplay},
            {"__ZN27AMDRadeonX6000_AMDHWDisplay14getDisplayInfoEjbbPvP17_FRAMEBUFFER_INFO", orgGetDisplayInfo},
        };
        PANIC_COND(!SolveRequestPlus::solveAll(patcher, id, solveRequests, slide, size), "X6000",
            "Failed to resolve symbols");

        if (NRed::callback->attributes.isVenturaAndLater()) {
            orgAllocateScanoutFB = nullptr;
        } else {
            SolveRequestPlus request {"__ZN27AMDRadeonX6000_AMDHWDisplay17allocateScanoutFBEjP16IOAccelResource2S1_Py",
                orgAllocateScanoutFB};
            PANIC_COND(!request.solve(patcher, id, slide, size), "X6000", "Failed to resolve allocateScanout");
        }

        RouteRequestPlus request = {"__ZN37AMDRadeonX6000_AMDGraphicsAccelerator5startEP9IOService",
            wrapAccelStartX6000};
        PANIC_COND(!request.route(patcher, id, slide, size), "X6000", "Failed to route AMDGraphicsAccelerator::start");

        if (NRed::callback->attributes.isRaven()) {
            RouteRequestPlus request = {"__ZN30AMDRadeonX6000_AMDGFX10Display23initDCNRegistersOffsetsEv",
                wrapInitDCNRegistersOffsets, this->orgInitDCNRegistersOffsets};
            PANIC_COND(!request.route(patcher, id, slide, size), "X6000", "Failed to route initDCNRegistersOffsets");
        }

        if (NRed::callback->attributes.isCatalina()) {
            const LookupPatchPlus patches[] = {
                {&kextRadeonX6000, kHWChannelSubmitCommandBufferCatalinaOriginal,
                    kHWChannelSubmitCommandBufferCatalinaPatched, 1},
                {&kextRadeonX6000, kDummyWPTRUpdateDiagCallOriginal, kDummyWPTRUpdateDiagCallPatched, 1},
            };
            SYSLOG_COND(!LookupPatchPlus::applyAll(patcher, patches, slide, size, true), "X6000",
                "Failed to apply patches");
            patcher.clearError();
        } else {
            const LookupPatchPlus patch {&kextRadeonX6000, kHWChannelSubmitCommandBufferOriginal,
                kHWChannelSubmitCommandBufferPatched, 1};
            SYSLOG_COND(!patch.apply(patcher, slide, size), "X6000", "Failed to apply submitCommandBuffer patch");
            patcher.clearError();
        }

        const LookupPatchPlus patches[] = {
            {&kextRadeonX6000, kIsDeviceValidCallOriginal, kIsDeviceValidCallPatched,
                NRed::callback->attributes.isCatalina()        ? 20U :
                NRed::callback->attributes.isVenturaAndLater() ? 23 :
                NRed::callback->attributes.isMonterey()        ? 26 :
                                                                 24},
            {&kextRadeonX6000, kIsDevicePCITunnelledCallOriginal, kIsDevicePCITunnelledCallPatched,
                NRed::callback->attributes.isCatalina()        ? 9U :
                NRed::callback->attributes.isVenturaAndLater() ? 3 :
                                                                 1},
        };
        SYSLOG_COND(!LookupPatchPlus::applyAll(patcher, patches, slide, size, true), "X6000",
            "Failed to apply patches");
        patcher.clearError();

        if (NRed::callback->attributes.isCatalina()) {
            const LookupPatchPlus patches[] = {
                {&kextRadeonX6000, kWriteWaitForRenderingPipeCallOriginal, kWriteWaitForRenderingPipeCallPatched, 1},
                {&kextRadeonX6000, kGetTtlInterfaceCallOriginal, kGetTtlInterfaceCallOriginalMask,
                    kGetTtlInterfaceCallPatched, kGetTtlInterfaceCallPatchedMask, 38},
                {&kextRadeonX6000, kGetAMDHWHandlerCallOriginal, kGetAMDHWHandlerCallPatched, 19},
                {&kextRadeonX6000, kGetAMDHWHandlerCallOriginal, kGetAMDHWHandlerCallPatched,
                    arrsize(kGetAMDHWHandlerCallOriginal), 64, 1},
                {&kextRadeonX6000, kGetHWRegistersCallOriginal, kGetHWRegistersCallOriginalMask,
                    kGetHWRegistersCallPatched, kGetHWRegistersCallPatchedMask, 13},
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
                {&kextRadeonX6000, kGetChannelWriteBackFrameAddrCallOriginal,
                    kGetChannelWriteBackFrameAddrCallOriginalMask, kGetChannelWriteBackFrameAddrCallPatched,
                    kGetChannelWriteBackFrameAddrCallPatchedMask, 10},
                {&kextRadeonX6000, kGetDoorbellMemoryBaseAddressCallOriginal, kGetDoorbellMemoryBaseAddressCallPatched,
                    1},
                {&kextRadeonX6000, kGetChannelDoorbellOffsetCallOriginal, kGetChannelDoorbellOffsetCallPatched, 1},
                {&kextRadeonX6000, kGetIOPCIDeviceCallOriginal, kGetIOPCIDeviceCallPatched, 5},
                {&kextRadeonX6000, kGetSMLCallOriginal, kGetSMLCallPatched, 10},
                {&kextRadeonX6000, kGetPM4CommandUtilityCallOriginal, kGetPM4CommandUtilityCallPatched, 2},
                {&kextRadeonX6000, kDumpASICHangStateCallOriginal, kDumpASICHangStateCallPatched, 2},
                {&kextRadeonX6000, kGetSchedulerCallCatalinaOriginal, kGetSchedulerCallCatalinaPatched, 22},
            };
            SYSLOG_COND(!LookupPatchPlus::applyAll(patcher, patches, slide, size, true), "X6000",
                "Failed to apply patches");
            patcher.clearError();
        }

        if (NRed::callback->attributes.isVenturaAndLater()) {
            const LookupPatchPlus patch {&kextRadeonX6000, kGetSchedulerCallVenturaOriginal,
                kGetSchedulerCallVenturaPatched, 24};
            SYSLOG_COND(!patch.apply(patcher, slide, size), "X6000", "Failed to apply getScheduler patch");
            patcher.clearError();
        } else if (!NRed::callback->attributes.isCatalina()) {
            const LookupPatchPlus patch {&kextRadeonX6000, kGetSchedulerCallOriginal, kGetSchedulerCallPatched,
                NRed::callback->attributes.isMonterey() ? 21U : 22};
            SYSLOG_COND(!patch.apply(patcher, slide, size), "X6000", "Failed to apply getScheduler patch");
            patcher.clearError();
        }

        if (NRed::callback->attributes.isCatalina()) {
            const LookupPatchPlus patches[] = {
                {&kextRadeonX6000, kGetGpuDebugPolicyCallCatalinaOriginal, kGetGpuDebugPolicyCallCatalinaPatched, 27},
                {&kextRadeonX6000, kUpdateUtilizationStatisticsCounterCallOriginal,
                    kUpdateUtilizationStatisticsCounterCallPatched, 2},
                {&kextRadeonX6000, kDisableGfxOffCallOriginal, kDisableGfxOffCallPatched, 17},
                {&kextRadeonX6000, kEnableGfxOffCallOriginal, kEnableGfxOffCallPatched, 16},
                {&kextRadeonX6000, kFlushSystemCachesCallOriginal, kFlushSystemCachesCallPatched, 4},
                {&kextRadeonX6000, kGetUbmSwizzleModeCallOriginal, kGetUbmSwizzleModeCallPatched, 1},
                {&kextRadeonX6000, kGetUbmTileModeCallOriginal, kGetUbmTileModeCallPatched, 1},
            };
            SYSLOG_COND(!LookupPatchPlus::applyAll(patcher, patches, slide, size, true), "X6000",
                "Failed to apply patches");
            patcher.clearError();
        } else {
            const LookupPatchPlus patch {&kextRadeonX6000, kGetGpuDebugPolicyCallOriginal,
                kGetGpuDebugPolicyCallPatched,
                NRed::callback->attributes.isVentura1304Based() ? 38U :
                NRed::callback->attributes.isVenturaAndLater()  ? 37 :
                                                                  28};
            SYSLOG_COND(!patch.apply(patcher, slide, size), "X6000", "Failed to apply getGpuDebugPolicy patch");
            patcher.clearError();
        }

        // Now, for AMDHWDisplay, fix the VTable offsets to calls in HWAlignManager2.
        FillUBMSurfaceVTFix.apply(orgFillUBMSurface);
        ConfigureDisplayVTFix.apply(orgConfigureDisplay);
        GetDisplayInfoVTFix.apply(orgGetDisplayInfo);
        if (orgAllocateScanoutFB != nullptr) { AllocateScanoutFBVTFix.apply(orgAllocateScanoutFB); }

        return true;
    }

    return false;
}

/**
 * We don't want the `AMDRadeonX6000` personality defined in the `Info.plist` to do anything.
 * We only use it to force-load `AMDRadeonX6000` and snatch the VCN/DCN symbols.
 */
bool X6000::wrapAccelStartX6000() { return false; }

void X6000::wrapInitDCNRegistersOffsets(void *that) {
    FunctionCast(wrapInitDCNRegistersOffsets, callback->orgInitDCNRegistersOffsets)(that);
    auto base = callback->regBaseField.get(that);
    (callback->regBaseField + 0x10).set(that, base + mmHUBPREQ0_DCSURF_PRIMARY_SURFACE_ADDRESS);
    (callback->regBaseField + 0x48).set(that, base + mmHUBPREQ1_DCSURF_PRIMARY_SURFACE_ADDRESS);
    (callback->regBaseField + 0x80).set(that, base + mmHUBPREQ2_DCSURF_PRIMARY_SURFACE_ADDRESS);
    (callback->regBaseField + 0xB8).set(that, base + mmHUBPREQ3_DCSURF_PRIMARY_SURFACE_ADDRESS);
    (callback->regBaseField + 0x14).set(that, base + mmHUBPREQ0_DCSURF_PRIMARY_SURFACE_ADDRESS_HIGH);
    (callback->regBaseField + 0x4C).set(that, base + mmHUBPREQ1_DCSURF_PRIMARY_SURFACE_ADDRESS_HIGH);
    (callback->regBaseField + 0x84).set(that, base + mmHUBPREQ2_DCSURF_PRIMARY_SURFACE_ADDRESS_HIGH);
    (callback->regBaseField + 0xBC).set(that, base + mmHUBPREQ3_DCSURF_PRIMARY_SURFACE_ADDRESS_HIGH);
    (callback->regBaseField + 0x18).set(that, base + mmHUBP0_DCSURF_SURFACE_CONFIG);
    (callback->regBaseField + 0x50).set(that, base + mmHUBP1_DCSURF_SURFACE_CONFIG);
    (callback->regBaseField + 0x88).set(that, base + mmHUBP2_DCSURF_SURFACE_CONFIG);
    (callback->regBaseField + 0xC0).set(that, base + mmHUBP3_DCSURF_SURFACE_CONFIG);
    (callback->regBaseField + 0x1C).set(that, base + mmHUBPREQ0_DCSURF_SURFACE_PITCH);
    (callback->regBaseField + 0x54).set(that, base + mmHUBPREQ1_DCSURF_SURFACE_PITCH);
    (callback->regBaseField + 0x8C).set(that, base + mmHUBPREQ2_DCSURF_SURFACE_PITCH);
    (callback->regBaseField + 0xC4).set(that, base + mmHUBPREQ3_DCSURF_SURFACE_PITCH);
    (callback->regBaseField + 0x20).set(that, base + mmHUBP0_DCSURF_ADDR_CONFIG);
    (callback->regBaseField + 0x58).set(that, base + mmHUBP1_DCSURF_ADDR_CONFIG);
    (callback->regBaseField + 0x90).set(that, base + mmHUBP2_DCSURF_ADDR_CONFIG);
    (callback->regBaseField + 0xC8).set(that, base + mmHUBP3_DCSURF_ADDR_CONFIG);
    (callback->regBaseField + 0x24).set(that, base + mmHUBP0_DCSURF_TILING_CONFIG);
    (callback->regBaseField + 0x5C).set(that, base + mmHUBP1_DCSURF_TILING_CONFIG);
    (callback->regBaseField + 0x94).set(that, base + mmHUBP2_DCSURF_TILING_CONFIG);
    (callback->regBaseField + 0xCC).set(that, base + mmHUBP3_DCSURF_TILING_CONFIG);
    (callback->regBaseField + 0x28).set(that, base + mmHUBP0_DCSURF_PRI_VIEWPORT_START);
    (callback->regBaseField + 0x60).set(that, base + mmHUBP1_DCSURF_PRI_VIEWPORT_START);
    (callback->regBaseField + 0x98).set(that, base + mmHUBP2_DCSURF_PRI_VIEWPORT_START);
    (callback->regBaseField + 0xD0).set(that, base + mmHUBP3_DCSURF_PRI_VIEWPORT_START);
    (callback->regBaseField + 0x2C).set(that, base + mmHUBP0_DCSURF_PRI_VIEWPORT_DIMENSION);
    (callback->regBaseField + 0x64).set(that, base + mmHUBP1_DCSURF_PRI_VIEWPORT_DIMENSION);
    (callback->regBaseField + 0x9C).set(that, base + mmHUBP2_DCSURF_PRI_VIEWPORT_DIMENSION);
    (callback->regBaseField + 0xD4).set(that, base + mmHUBP3_DCSURF_PRI_VIEWPORT_DIMENSION);
    (callback->regBaseField + 0x30).set(that, base + mmOTG0_OTG_CONTROL);
    (callback->regBaseField + 0x68).set(that, base + mmOTG1_OTG_CONTROL);
    (callback->regBaseField + 0xA0).set(that, base + mmOTG2_OTG_CONTROL);
    (callback->regBaseField + 0xD8).set(that, base + mmOTG3_OTG_CONTROL);
    (callback->regBaseField + 0x110).set(that, base + mmOTG4_OTG_CONTROL);
    (callback->regBaseField + 0x148).set(that, base + mmOTG5_OTG_CONTROL);
    (callback->regBaseField + 0x34).set(that, base + mmOTG0_OTG_INTERLACE_CONTROL);
    (callback->regBaseField + 0x6C).set(that, base + mmOTG1_OTG_INTERLACE_CONTROL);
    (callback->regBaseField + 0xA4).set(that, base + mmOTG2_OTG_INTERLACE_CONTROL);
    (callback->regBaseField + 0xDC).set(that, base + mmOTG3_OTG_INTERLACE_CONTROL);
    (callback->regBaseField + 0x114).set(that, base + mmOTG4_OTG_INTERLACE_CONTROL);
    (callback->regBaseField + 0x14C).set(that, base + mmOTG5_OTG_INTERLACE_CONTROL);
    (callback->regBaseField + 0x38).set(that, base + mmHUBPREQ0_DCSURF_FLIP_CONTROL);
    (callback->regBaseField + 0x70).set(that, base + mmHUBPREQ1_DCSURF_FLIP_CONTROL);
    (callback->regBaseField + 0xA8).set(that, base + mmHUBPREQ2_DCSURF_FLIP_CONTROL);
    (callback->regBaseField + 0xE0).set(that, base + mmHUBPREQ3_DCSURF_FLIP_CONTROL);
    (callback->regBaseField + 0x3C).set(that, base + mmHUBPRET0_HUBPRET_CONTROL);
    (callback->regBaseField + 0x74).set(that, base + mmHUBPRET1_HUBPRET_CONTROL);
    (callback->regBaseField + 0xAC).set(that, base + mmHUBPRET2_HUBPRET_CONTROL);
    (callback->regBaseField + 0xE4).set(that, base + mmHUBPRET3_HUBPRET_CONTROL);
    (callback->regBaseField + 0x40).set(that, base + mmHUBPREQ0_DCSURF_SURFACE_EARLIEST_INUSE);
    (callback->regBaseField + 0x78).set(that, base + mmHUBPREQ1_DCSURF_SURFACE_EARLIEST_INUSE);
    (callback->regBaseField + 0xB0).set(that, base + mmHUBPREQ2_DCSURF_SURFACE_EARLIEST_INUSE);
    (callback->regBaseField + 0xE8).set(that, base + mmHUBPREQ3_DCSURF_SURFACE_EARLIEST_INUSE);
    (callback->regBaseField + 0x44).set(that, base + mmHUBPREQ0_DCSURF_SURFACE_EARLIEST_INUSE_HIGH);
    (callback->regBaseField + 0x7C).set(that, base + mmHUBPREQ1_DCSURF_SURFACE_EARLIEST_INUSE_HIGH);
    (callback->regBaseField + 0xB4).set(that, base + mmHUBPREQ2_DCSURF_SURFACE_EARLIEST_INUSE_HIGH);
    (callback->regBaseField + 0xEC).set(that, base + mmHUBPREQ3_DCSURF_SURFACE_EARLIEST_INUSE_HIGH);
}
