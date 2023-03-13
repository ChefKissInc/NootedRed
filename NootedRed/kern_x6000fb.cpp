//  Copyright © 2022-2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.0. See LICENSE for
//  details.

#include "kern_x6000fb.hpp"
#include "kern_patches.hpp"
#include "kern_wred.hpp"
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
            {"_dce_driver_set_backlight", orgDceDriverSetBacklight},
        };
        PANIC_COND(!patcher.solveMultiple(index, solveRequests, address, size), "x6000fb", "Failed to resolve symbols");

        PANIC_COND(MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) != KERN_SUCCESS, "x6000fb",
            "Failed to enable kernel writing");
        orgAsicCapsTable->familyId = AMDGPU_FAMILY_RV;
        orgAsicCapsTable->caps = WRed::callback->chipType < ChipType::Renoir ? ddiCapsRaven : ddiCapsRenoir;
        orgAsicCapsTable->deviceId = WRed::callback->deviceId;
        orgAsicCapsTable->revision = WRed::callback->revision;
        orgAsicCapsTable->emulatedRev = WRed::callback->enumeratedRevision + WRed::callback->revision;
        orgAsicCapsTable->pciRev = 0xFFFFFFFF;
        MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);

        KernelPatcher::RouteRequest requests[] = {
            {"__ZNK15AmdAtomVramInfo16populateVramInfoER16AtomFirmwareInfo", wrapPopulateVramInfo},
            {"__ZN30AMDRadeonX6000_AmdAsicInfoNavi18populateDeviceInfoEv", wrapPopulateDeviceInfo,
                orgPopulateDeviceInfo},
            {"__ZNK32AMDRadeonX6000_AmdAsicInfoNavi1027getEnumeratedRevisionNumberEv", wrapGetEnumeratedRevision},
            {"__ZN32AMDRadeonX6000_AmdRegisterAccess11hwReadReg32Ej", wrapHwReadReg32, orgHwReadReg32},
            {"__ZN24AMDRadeonX6000_AmdLogger15initWithPciInfoEP11IOPCIDevice", wrapInitWithPciInfo, orgInitWithPciInfo},
            {"__ZN34AMDRadeonX6000_AmdRadeonController10doGPUPanicEPKcz", wrapDoGPUPanic},
            {"_dce_panel_cntl_hw_init", wrapDcePanelCntlHwInit, orgDcePanelCntlHwInit},
            {"__ZN35AMDRadeonX6000_AmdRadeonFramebuffer25setAttributeForConnectionEijm",
                wrapAMDRadeonX6000AmdRadeonFramebufferSetAttribute, orgAMDRadeonX6000AmdRadeonFramebufferSetAttribute},
            {"__ZN35AMDRadeonX6000_AmdRadeonFramebuffer25getAttributeForConnectionEijPm",
                wrapAMDRadeonX6000AmdRadeonFramebufferGetAttribute, orgAMDRadeonX6000AmdRadeonFramebufferGetAttribute},
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

        return true;
    }

    return false;
}

uint16_t X6000FB::wrapGetEnumeratedRevision() { return WRed::callback->enumeratedRevision; }

IOReturn X6000FB::wrapPopulateDeviceInfo(void *that) {
    auto ret = FunctionCast(wrapPopulateDeviceInfo, callback->orgPopulateDeviceInfo)(that);
    getMember<uint32_t>(that, 0x60) = AMDGPU_FAMILY_RV;
    return ret;
}

IOReturn X6000FB::wrapPopulateVramInfo([[maybe_unused]] void *that, void *fwInfo) {
    auto *vbios = static_cast<const uint8_t *>(WRed::callback->vbiosData->getBytesNoCopy());
    auto base = *reinterpret_cast<const uint16_t *>(vbios + ATOM_ROM_TABLE_PTR);
    auto dataTable = *reinterpret_cast<const uint16_t *>(vbios + base + ATOM_ROM_DATA_PTR);
    auto *mdt = reinterpret_cast<const uint16_t *>(vbios + dataTable + 4);
    uint32_t channelCount = 1;
    if (mdt[0x1E]) {
        DBGLOG("x6000fb", "Fetching VRAM info from iGPU System Info");
        uint32_t offset = 0x1E * 2 + 4;
        auto index = *reinterpret_cast<const uint16_t *>(vbios + dataTable + offset);
        auto *table = reinterpret_cast<const IgpSystemInfo *>(vbios + index);
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

void X6000FB::updatePwmMaxBrightnessFromInternalDisplay() {
    auto *matching = IOService::serviceMatching("AppleBacklightDisplay");
    if (!matching) {
        DBGLOG("x6000fb", "updatePwmMaxBrightnessFromInternalDisplay null AppleBacklightDisplay");
        return;
    }

    auto *display = IOService::waitForMatchingService(matching);
    if (!display) {
        DBGLOG("x6000fb", "updatePwmMaxBrightnessFromInternalDisplay null matching");
        OSSafeReleaseNULL(matching);
        return;
    }

    OSDictionary *ioDispParam = OSDynamicCast(OSDictionary, display->getProperty("IODisplayParameters"));
    if (!ioDispParam) {
        DBGLOG("x6000fb", "updatePwmMaxBrightnessFromInternalDisplay null IODisplayParameters");
        OSSafeReleaseNULL(matching);
        return;
    }

    OSDictionary *linearBrightness = OSDynamicCast(OSDictionary, ioDispParam->getObject("linear-brightness"));
    if (!linearBrightness) {
        DBGLOG("x6000fb", "linear-brightness property is null");
        OSSafeReleaseNULL(matching);
        return;
    }

    OSNumber *maxBrightness = OSDynamicCast(OSNumber, linearBrightness->getObject("max"));
    if (!maxBrightness) {
        DBGLOG("x6000fb", "max property is null");
        OSSafeReleaseNULL(matching);
        return;
    }

    callback->maxPwmBacklightLvl = maxBrightness->unsigned32BitValue();
    DBGLOG("x6000fb", "Got max brightness: 0x%x", callback->maxPwmBacklightLvl);

    OSSafeReleaseNULL(matching);
}

uint32_t X6000FB::wrapDcePanelCntlHwInit(void *panel_cntl) {
    callback->panelCntlPtr = panel_cntl;
    callback->updatePwmMaxBrightnessFromInternalDisplay();    // read max brightness value from IOReg
    uint32_t ret = FunctionCast(wrapDcePanelCntlHwInit, callback->orgDcePanelCntlHwInit)(panel_cntl);
    return ret;
}

IOReturn X6000FB::wrapAMDRadeonX6000AmdRadeonFramebufferSetAttribute(IOService *framebuffer, IOIndex connectIndex,
    IOSelect attribute, uintptr_t value) {
    IOReturn ret = FunctionCast(wrapAMDRadeonX6000AmdRadeonFramebufferSetAttribute,
        callback->orgAMDRadeonX6000AmdRadeonFramebufferSetAttribute)(framebuffer, connectIndex, attribute, value);
    if (attribute != (UInt32)'bklt') { return ret; }

    if (callback->maxPwmBacklightLvl == 0) {
        DBGLOG("x6000fb", "wrapAMDRadeonX6000AmdRadeonFramebufferSetAttribute zero maxPwmBacklightLvl");
        return 0;
    }

    if (callback->panelCntlPtr == nullptr) {
        DBGLOG("x6000fb", "wrapAMDRadeonX6000AmdRadeonFramebufferSetAttribute null panel cntl");
        return 0;
    }

    if (callback->orgDceDriverSetBacklight == nullptr) {
        DBGLOG("x6000fb", "wrapAMDRadeonX6000AmdRadeonFramebufferSetAttribute null orgDcLinkSetBacklightLevel");
        return 0;
    }

    // Set the backlight
    callback->curPwmBacklightLvl = (uint32_t)value;
    uint32_t btlper = callback->curPwmBacklightLvl * 100 / callback->maxPwmBacklightLvl;
    uint32_t pwmval = 0;
    if (btlper >= 100) {
        // This is from the dmcu_set_backlight_level function of Linux source
        // ...
        // if (backlight_pwm_u16_16 & 0x10000)
        // 	   backlight_8_bit = 0xFF;
        // else
        // 	   backlight_8_bit = (backlight_pwm_u16_16 >> 8) & 0xFF;
        // ...
        // The max brightness should have 0x10000 bit set
        pwmval = 0x1FF00;
    } else {
        pwmval = ((btlper * 0xFF) / 100) << 8U;
    }

    callback->orgDceDriverSetBacklight(callback->panelCntlPtr, pwmval);
    return 0;
}

IOReturn X6000FB::wrapAMDRadeonX6000AmdRadeonFramebufferGetAttribute(IOService *framebuffer, IOIndex connectIndex,
    IOSelect attribute, uintptr_t *value) {
    IOReturn ret = FunctionCast(wrapAMDRadeonX6000AmdRadeonFramebufferGetAttribute,
        callback->orgAMDRadeonX6000AmdRadeonFramebufferGetAttribute)(framebuffer, connectIndex, attribute, value);
    if (attribute == (UInt32)'bklt') {
        // enable the backlight feature of AMD navi10 driver
        *value = callback->curPwmBacklightLvl;
        ret = 0;
    }
    return ret;
}

void X6000FB::wrapDoGPUPanic() {
    DBGLOG("x6000fb", "doGPUPanic << ()");
    while (true) { IOSleep(3600000); }
}
