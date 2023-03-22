//  Copyright © 2022-2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.0. See LICENSE for
//  details.

#include "kern_x6000fb.hpp"
#include "kern_nred.hpp"
#include "kern_patches.hpp"
#include <Headers/kern_api.hpp>

static const char *pathRadeonX6000Framebuffer =
    "/System/Library/Extensions/AMDRadeonX6000Framebuffer.kext/Contents/MacOS/AMDRadeonX6000Framebuffer";

static KernelPatcher::KextInfo kextRadeonX6000Framebuffer {"com.apple.kext.AMDRadeonX6000Framebuffer",
    &pathRadeonX6000Framebuffer, 1, {}, {}, KernelPatcher::KextInfo::Unloaded};

X6000FB *X6000FB::callback = nullptr;

void X6000FB::init() {
    callback = this;
    lilu.onKextLoadForce(&kextRadeonX6000Framebuffer);
}

bool X6000FB::processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
    if (kextRadeonX6000Framebuffer.loadIndex == index) {
        CailAsicCapEntry *orgAsicCapsTable = nullptr;

        KernelPatcher::SolveRequest solveRequests[] = {
            {"__ZL20CAIL_ASIC_CAPS_TABLE", orgAsicCapsTable},
            {"_dce_driver_set_backlight", this->orgDceDriverSetBacklight},
        };
        PANIC_COND(!patcher.solveMultiple(index, solveRequests, address, size), "x6000fb", "Failed to resolve symbols");

        KernelPatcher::RouteRequest requests[] = {
            {"__ZNK15AmdAtomVramInfo16populateVramInfoER16AtomFirmwareInfo", wrapPopulateVramInfo},
            {"__ZN30AMDRadeonX6000_AmdAsicInfoNavi18populateDeviceInfoEv", wrapPopulateDeviceInfo,
                this->orgPopulateDeviceInfo},
            {"__ZNK32AMDRadeonX6000_AmdAsicInfoNavi1027getEnumeratedRevisionNumberEv", wrapGetEnumeratedRevision},
            {"__ZN32AMDRadeonX6000_AmdRegisterAccess11hwReadReg32Ej", wrapHwReadReg32, this->orgHwReadReg32},
            {"__ZN24AMDRadeonX6000_AmdLogger15initWithPciInfoEP11IOPCIDevice", wrapInitWithPciInfo,
                this->orgInitWithPciInfo},
            {"__ZN34AMDRadeonX6000_AmdRadeonController10doGPUPanicEPKcz", wrapDoGPUPanic},
            {"_dce_panel_cntl_hw_init", wrapDcePanelCntlHwInit, this->orgDcePanelCntlHwInit},
            {"__ZN35AMDRadeonX6000_AmdRadeonFramebuffer25setAttributeForConnectionEijm", wrapFramebufferSetAttribute,
                this->orgFramebufferSetAttribute},
            {"__ZN35AMDRadeonX6000_AmdRadeonFramebuffer25getAttributeForConnectionEijPm", wrapFramebufferGetAttribute,
                this->orgFramebufferGetAttribute},
        };
        PANIC_COND(!patcher.routeMultiple(index, requests, address, size), "x6000fb", "Failed to route symbols");

        KernelPatcher::LookupPatch patches[] = {
            {&kextRadeonX6000Framebuffer, kAmdAtomVramInfoNullCheck1Original, kAmdAtomVramInfoNullCheck1Patched,
                arrsize(kAmdAtomVramInfoNullCheck1Original), 1},
            {&kextRadeonX6000Framebuffer, kAmdAtomPspDirectoryNullCheckOriginal, kAmdAtomPspDirectoryNullCheckPatched,
                arrsize(kAmdAtomPspDirectoryNullCheckOriginal), 1},
            {&kextRadeonX6000Framebuffer, kAmdAtomVramInfoNullCheck2Original, kAmdAtomVramInfoNullCheck2Patched,
                arrsize(kAmdAtomVramInfoNullCheck2Original), 1},
            {&kextRadeonX6000Framebuffer, kAgdcServicesGetVendorInfoOriginal, kAgdcServicesGetVendorInfoPatched,
                arrsize(kAgdcServicesGetVendorInfoOriginal), 1},
        };
        for (auto &patch : patches) {
            patcher.applyLookupPatch(&patch);
            patcher.clearError();
        }

        PANIC_COND(MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) != KERN_SUCCESS, "x5000",
            "Failed to enable kernel writing");
        orgAsicCapsTable->familyId = AMDGPU_FAMILY_RAVEN;
        orgAsicCapsTable->caps = NRed::callback->chipType < ChipType::Renoir ? ddiCapsRaven : ddiCapsRenoir;
        orgAsicCapsTable->deviceId = NRed::callback->deviceId;
        orgAsicCapsTable->revision = NRed::callback->revision;
        orgAsicCapsTable->emulatedRev = NRed::callback->enumeratedRevision + NRed::callback->revision;
        orgAsicCapsTable->pciRev = 0xFFFFFFFF;
        MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);
        DBGLOG("x6000fb", "Applied DDI Caps patches");

        return true;
    }

    return false;
}

uint16_t X6000FB::wrapGetEnumeratedRevision() { return NRed::callback->enumeratedRevision; }

IOReturn X6000FB::wrapPopulateDeviceInfo(void *that) {
    if (!callback->dispNotif) { callback->registerDispMaxBrightnessNotif(); }
    auto ret = FunctionCast(wrapPopulateDeviceInfo, callback->orgPopulateDeviceInfo)(that);
    getMember<uint32_t>(that, 0x60) = AMDGPU_FAMILY_RAVEN;
    return ret;
}

IOReturn X6000FB::wrapPopulateVramInfo([[maybe_unused]] void *that, void *fwInfo) {
    uint32_t channelCount = 1;
    auto *table = NRed::callback->getVBIOSDataTable<IgpSystemInfo>(0x1E);
    if (table) {
        DBGLOG("x6000fb", "Fetching VRAM info from iGPU System Info");
        switch (table->header.formatRev) {
            case 1:
                switch (table->header.contentRev) {
                    case 11:
                        [[fallthrough]];
                    case 12:
                        if (table->infoV11.umaChannelCount) channelCount = table->infoV11.umaChannelCount;
                        break;
                    default:
                        DBGLOG("x6000fb", "Unsupported contentRev %d", table->header.contentRev);
                        break;
                }
                break;
            case 2:
                switch (table->header.contentRev) {
                    case 1:
                        [[fallthrough]];
                    case 2:
                        if (table->infoV2.umaChannelCount) channelCount = table->infoV2.umaChannelCount;
                        break;
                    default:
                        DBGLOG("x6000fb", "Unsupported contentRev %d", table->header.contentRev);
                        break;
                }
                break;
            default:
                DBGLOG("x6000fb", "Unsupported formatRev %d", table->header.formatRev);
                break;
        }
    } else {
        DBGLOG("x6000fb", "No iGPU System Info in Master Data Table");
    }
    getMember<uint32_t>(fwInfo, 0x1C) = 4;                    // VRAM Type (DDR4)
    getMember<uint32_t>(fwInfo, 0x20) = channelCount * 64;    // VRAM Width (64-bit channels)
    return kIOReturnSuccess;
}

uint32_t X6000FB::wrapHwReadReg32(void *that, uint32_t reg) {
    return FunctionCast(wrapHwReadReg32, callback->orgHwReadReg32)(that, reg == 0xD31 ? 0xD2F : reg);
}

bool X6000FB::wrapInitWithPciInfo(void *that, void *param1) {
    auto ret = FunctionCast(wrapInitWithPciInfo, callback->orgInitWithPciInfo)(that, param1);
    // Hack AMDRadeonX6000_AmdLogger to log everything
    getMember<uint64_t>(that, 0x28) = ~0ULL;
    getMember<uint32_t>(that, 0x30) = 0xFF;
    return ret;
}

bool X6000FB::OnAppleBacklightDisplayLoad([[maybe_unused]] void *target, [[maybe_unused]] void *refCon,
    IOService *newService, [[maybe_unused]] IONotifier *notifier) {
    OSDictionary *params = OSDynamicCast(OSDictionary, newService->getProperty("IODisplayParameters"));
    if (!params) {
        DBGLOG("x6000fb", "OnAppleBacklightDisplayLoad: No 'IODisplayParameters' property");
        return false;
    }

    OSDictionary *linearBrightness = OSDynamicCast(OSDictionary, params->getObject("linear-brightness"));
    if (!linearBrightness) {
        DBGLOG("x6000fb", "OnAppleBacklightDisplayLoad: No 'linear-brightness' property");
        return false;
    }

    OSNumber *maxBrightness = OSDynamicCast(OSNumber, linearBrightness->getObject("max"));
    if (!maxBrightness) {
        DBGLOG("x6000fb", "OnAppleBacklightDisplayLoad: No 'max' property");
        return false;
    }

    callback->maxPwmBacklightLvl = maxBrightness->unsigned32BitValue();
    DBGLOG("x6000fb", "OnAppleBacklightDisplayLoad: Max brightness: 0x%X", callback->maxPwmBacklightLvl);

    return true;
}

void X6000FB::registerDispMaxBrightnessNotif() {
    auto *matching = IOService::serviceMatching("AppleBacklightDisplay");
    if (!matching) {
        DBGLOG("x6000fb", "registerDispMaxBrightnessNotif: Failed to create match dictionary");
        return;
    }

    callback->dispNotif =
        IOService::addMatchingNotification(gIOFirstMatchNotification, matching, OnAppleBacklightDisplayLoad, nullptr);
    DBGLOG_COND(!callback->dispNotif, "x6000fb", "registerDispMaxBrightnessNotif: Failed to register notification");
    OSSafeReleaseNULL(matching);
}

void X6000FB::wrapDoGPUPanic() {
    DBGLOG("x6000fb", "doGPUPanic << ()");
    while (true) { IOSleep(3600000); }
}

uint32_t X6000FB::wrapDcePanelCntlHwInit(void *panelCntl) {
    callback->panelCntlPtr = panelCntl;
    return FunctionCast(wrapDcePanelCntlHwInit, callback->orgDcePanelCntlHwInit)(panelCntl);
}

IOReturn X6000FB::wrapFramebufferSetAttribute(IOService *framebuffer, IOIndex connectIndex, IOSelect attribute,
    uintptr_t value) {
    auto ret = FunctionCast(wrapFramebufferSetAttribute, callback->orgFramebufferSetAttribute)(framebuffer,
        connectIndex, attribute, value);
    if (attribute != static_cast<UInt32>('bklt')) { return ret; }

    if (!callback->maxPwmBacklightLvl) {
        DBGLOG("x6000fb", "wrapFramebufferSetAttribute: maxPwmBacklightLvl is 0");
        return kIOReturnSuccess;
    }

    if (!callback->panelCntlPtr) {
        DBGLOG("x6000fb", "wrapFramebufferSetAttribute: panelCntl is null");
        return kIOReturnSuccess;
    }

    if (!callback->orgDceDriverSetBacklight) {
        DBGLOG("x6000fb", "wrapFramebufferSetAttribute: orgDceDriverSetBacklight is null");
        return kIOReturnSuccess;
    }

    // Set the backlight
    callback->curPwmBacklightLvl = static_cast<uint32_t>(value);
    uint32_t percentage = callback->curPwmBacklightLvl * 100 / callback->maxPwmBacklightLvl;
    uint32_t pwmValue = 0;
    if (percentage >= 100) {
        // This is from the dmcu_set_backlight_level function of Linux source
        // ...
        // if (backlight_pwm_u16_16 & 0x10000)
        // 	   backlight_8_bit = 0xFF;
        // else
        // 	   backlight_8_bit = (backlight_pwm_u16_16 >> 8) & 0xFF;
        // ...
        // The max brightness should have 0x10000 bit set
        pwmValue = 0x1FF00;
    } else {
        pwmValue = ((percentage * 0xFF) / 100) << 8U;
    }

    callback->orgDceDriverSetBacklight(callback->panelCntlPtr, pwmValue);
    return kIOReturnSuccess;
}

IOReturn X6000FB::wrapFramebufferGetAttribute(IOService *framebuffer, IOIndex connectIndex, IOSelect attribute,
    uintptr_t *value) {
    auto ret = FunctionCast(wrapFramebufferGetAttribute, callback->orgFramebufferGetAttribute)(framebuffer,
        connectIndex, attribute, value);
    if (attribute == static_cast<UInt32>('bklt')) {
        // Enable the backlight feature of AMD navi10 driver
        *value = callback->curPwmBacklightLvl;
        return kIOReturnSuccess;
    }
    return ret;
}
