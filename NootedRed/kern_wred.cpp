//  Copyright © 2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.0. See LICENSE for
//  details.

#include "kern_wred.hpp"
#include "kern_hwlibs.hpp"
#include "kern_model.hpp"
#include "kern_patches.hpp"
#include "kern_x5000.hpp"
#include "kern_x6000.hpp"
#include "kern_x6000fb.hpp"
#include <Headers/kern_api.hpp>
#include <Headers/kern_devinfo.hpp>

static const char *pathAGDP = "/System/Library/Extensions/AppleGraphicsControl.kext/Contents/PlugIns/"
                              "AppleGraphicsDevicePolicy.kext/Contents/MacOS/AppleGraphicsDevicePolicy";
static const char *pathBacklight = "/System/Library/Extensions/AppleBacklight.kext/Contents/MacOS/AppleBacklight";
static const char *pathMCCSControl = "/System/Library/Extensions/AppleMCCSControl.kext/Contents/MacOS/AppleMCCSControl";

static KernelPatcher::KextInfo kextAGDP {"com.apple.driver.AppleGraphicsDevicePolicy", &pathAGDP, 1, {true}, {},
    KernelPatcher::KextInfo::Unloaded};
static KernelPatcher::KextInfo kextBacklight {"com.apple.driver.AppleBacklight", &pathBacklight, 1, {true}, {},
    KernelPatcher::KextInfo::Unloaded};
static KernelPatcher::KextInfo kextMCCSControl {"com.apple.driver.AppleMCCSControl", &pathMCCSControl, 1, {true}, {},
    KernelPatcher::KextInfo::Unloaded};

WRed *WRed::callback = nullptr;

static X6000FB x6000fb;
static X5000HWLibs hwlibs;
static X5000 x5000;
static X6000 x6000;

void WRed::init() {
    SYSLOG(MODULE_SHORT, "Please don't support tonymacx86.com!");
    callback = this;

    lilu.onPatcherLoadForce(
        [](void *user, KernelPatcher &patcher) { static_cast<WRed *>(user)->processPatcher(patcher); }, this);
    lilu.onKextLoadForce(
        nullptr, 0,
        [](void *user, KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
            static_cast<WRed *>(user)->processKext(patcher, index, address, size);
        },
        this);
    lilu.onKextLoad(&kextAGDP);
    lilu.onKextLoad(&kextBacklight);
    lilu.onKextLoadForce(&kextMCCSControl);
    x6000fb.init();
    hwlibs.init();
    x5000.init();
    x6000.init();
}

void WRed::deinit() { OSSafeReleaseNULL(this->vbiosData); }

void WRed::processPatcher(KernelPatcher &patcher) {
    auto *devInfo = DeviceInfo::create();
    if (!devInfo) {
        SYSLOG(MODULE_SHORT, "Failed to create DeviceInfo");
        return;
    }

    devInfo->processSwitchOff();

    auto *videoBuiltin = devInfo->videoBuiltin;
    if (!videoBuiltin) {
        SYSLOG(MODULE_SHORT, "videoBuiltin null");
        for (size_t i = 0; i < devInfo->videoExternal.size(); i++) {
            if (!OSDynamicCast(IOPCIDevice, devInfo->videoExternal[i].video)) { continue; }
            if (WIOKit::readPCIConfigValue(devInfo->videoExternal[i].video, WIOKit::kIOPCIConfigVendorID) ==
                WIOKit::VendorID::ATIAMD) {
                videoBuiltin = devInfo->videoExternal[i].video;
                break;
            }
        }
    }

    PANIC_COND(!videoBuiltin, MODULE_SHORT, "videoBuiltin null");
    auto *iGPU = OSDynamicCast(IOPCIDevice, videoBuiltin);
    PANIC_COND(!iGPU, MODULE_SHORT, "videoBuiltin is not IOPCIDevice");
    PANIC_COND(WIOKit::readPCIConfigValue(iGPU, WIOKit::kIOPCIConfigVendorID) != WIOKit::VendorID::ATIAMD, MODULE_SHORT,
        "videoBuiltin is not AMD");

    callback->iGPU = iGPU;

    WIOKit::renameDevice(iGPU, "IGPU");
    WIOKit::awaitPublishing(iGPU);

    static uint8_t builtin[] = {0x01};
    iGPU->setProperty("built-in", builtin, arrsize(builtin));
    callback->deviceId = WIOKit::readPCIConfigValue(iGPU, WIOKit::kIOPCIConfigDeviceID);
    auto *model = getBranding(callback->deviceId, WIOKit::readPCIConfigValue(iGPU, WIOKit::kIOPCIConfigRevisionID));
    if (model) { iGPU->setProperty("model", model); }

    if (UNLIKELY(iGPU->getProperty("ATY,bin_image"))) {
        DBGLOG(MODULE_SHORT, "VBIOS manually overridden");
    } else {
        if (!callback->getVBIOSFromVFCT(iGPU)) {
            SYSLOG(MODULE_SHORT, "Failed to get VBIOS from VFCT.");
            PANIC_COND(!callback->getVBIOSFromVRAM(iGPU), MODULE_SHORT, "Failed to get VBIOS from VRAM");
        }
    }

    auto *objInfo = callback->getVBIOSDataTable<DispObjInfoTableV1_4>(0x16);
    auto n = objInfo->pathCount;
    for (size_t i = 0, j = 0; i < n; i++) {
        // Skip invalid device tags and TV/CV support
        if ((objInfo->supportedDevices & objInfo->dispPaths[i].devTag) &&
            !(objInfo->dispPaths[i].devTag == (1 << 2) || objInfo->dispPaths[i].devTag == (1 << 8))) {
            objInfo->dispPaths[j++] = objInfo->dispPaths[i];
        } else {
            objInfo->pathCount--;
        }
    }

    callback->rmmio = iGPU->mapDeviceMemoryWithRegister(kIOPCIConfigBaseAddress5);
    PANIC_COND(!callback->rmmio || !callback->rmmio->getLength(), MODULE_SHORT, "Failed to map RMMIO");
    callback->rmmioPtr = reinterpret_cast<uint32_t *>(callback->rmmio->getVirtualAddress());

    callback->fbOffset = static_cast<uint64_t>(callback->readReg32(0x296B)) << 24;
    callback->revision = (callback->readReg32(0xD2F) & 0xF000000) >> 0x18;
    switch (callback->deviceId) {
        case 0x15D8:
            if (callback->revision >= 0x8) {
                callback->chipType = ChipType::Raven2;
                callback->enumeratedRevision = 0x79;
            } else {
                callback->chipType = ChipType::Picasso;
                callback->enumeratedRevision = 0x41;
            }
            break;
        case 0x15DD:
            if (callback->revision >= 0x8) {
                callback->chipType = ChipType::Raven2;
                callback->enumeratedRevision = 0x79;
            } else {
                callback->chipType = ChipType::Raven;
                callback->enumeratedRevision = 0x10;
            }
            break;
        case 0x164C:
            [[fallthrough]];
        case 0x1636:
            callback->chipType = ChipType::Renoir;
            callback->enumeratedRevision = 0x91;
            break;
        case 0x15E7:
            [[fallthrough]];
        case 0x1638:
            callback->chipType = ChipType::GreenSardine;
            callback->enumeratedRevision = 0xA1;
            break;
        default:
            PANIC(MODULE_SHORT, "Unknown device ID");
    }

    DeviceInfo::deleter(devInfo);

    KernelPatcher::RouteRequest requests[] = {
        {"__ZN15OSMetaClassBase12safeMetaCastEPKS_PK11OSMetaClass", wrapSafeMetaCast, orgSafeMetaCast},
    };
    PANIC_COND(!patcher.routeMultiple(KernelPatcher::KernelID, requests), MODULE_SHORT,
        "Failed to route OSMetaClassBase::safeMetaCast");
}

OSMetaClassBase *WRed::wrapSafeMetaCast(const OSMetaClassBase *anObject, const OSMetaClass *toMeta) {
    auto ret = FunctionCast(wrapSafeMetaCast, callback->orgSafeMetaCast)(anObject, toMeta);
    if (!ret) {
        for (const auto &ent : callback->metaClassMap) {
            if (ent[0] == toMeta) {
                return FunctionCast(wrapSafeMetaCast, callback->orgSafeMetaCast)(anObject, ent[1]);
            } else if (ent[1] == toMeta) {
                return FunctionCast(wrapSafeMetaCast, callback->orgSafeMetaCast)(anObject, ent[0]);
            }
        }
    }
    return ret;
}

void WRed::processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
    if (kextAGDP.loadIndex == index) {
        KernelPatcher::LookupPatch patches[] = {
            {&kextAGDP, reinterpret_cast<const uint8_t *>(kAGDPBoardIDKeyOriginal),
                reinterpret_cast<const uint8_t *>(kAGDPBoardIDKeyPatched), arrsize(kAGDPBoardIDKeyOriginal), 1},
            {&kextAGDP, kAGDPFBCountCheckOriginal, kAGDPFBCountCheckPatched, arrsize(kAGDPFBCountCheckOriginal), 1},
        };
        for (auto &patch : patches) {
            patcher.applyLookupPatch(&patch);
            SYSLOG_COND(patcher.getError() != KernelPatcher::Error::NoError, MODULE_SHORT,
                "Failed to apply AGDP patch: %d", patcher.getError());
            patcher.clearError();
        }
        return;
    } else if (kextBacklight.loadIndex == index) {
        KernelPatcher::RouteRequest request {"__ZN15AppleIntelPanel10setDisplayEP9IODisplay", wrapApplePanelSetDisplay,
            orgApplePanelSetDisplay};
        if (patcher.routeMultiple(kextBacklight.loadIndex, &request, 1, address, size)) {
            const uint8_t find[] = {"F%uT%04x"};
            const uint8_t replace[] = {"F%uTxxxx"};
            KernelPatcher::LookupPatch patch = {&kextBacklight, find, replace, sizeof(find), 1};
            DBGLOG(MODULE_SHORT, "applying backlight patch");
            patcher.applyLookupPatch(&patch);
        }
        return;
    } else if (kextMCCSControl.loadIndex == index) {
        KernelPatcher::RouteRequest request[] = {
            {"__ZN25AppleMCCSControlGibraltar5probeEP9IOServicePi", wrapFunctionReturnZero},
            {"__ZN21AppleMCCSControlCello5probeEP9IOServicePi", wrapFunctionReturnZero},
        };
        patcher.routeMultiple(index, request, address, size);
        return;
    }
    if (x6000fb.processKext(patcher, index, address, size)) { return; }
    if (hwlibs.processKext(patcher, index, address, size)) { return; }
    if (x6000.processKext(patcher, index, address, size)) { return; }
    if (x5000.processKext(patcher, index, address, size)) { return; }
}

struct ApplePanelData {
    const char *deviceName;
    uint8_t deviceData[36];
};

static ApplePanelData appleBacklightData[] = {
    {"F14Txxxx", {0x00, 0x11, 0x00, 0x00, 0x00, 0x34, 0x00, 0x52, 0x00, 0x73, 0x00, 0x94, 0x00, 0xBE, 0x00, 0xFA, 0x01,
                     0x36, 0x01, 0x72, 0x01, 0xC5, 0x02, 0x2F, 0x02, 0xB9, 0x03, 0x60, 0x04, 0x1A, 0x05, 0x0A, 0x06,
                     0x0E, 0x07, 0x10}},
    {"F15Txxxx", {0x00, 0x11, 0x00, 0x00, 0x00, 0x36, 0x00, 0x54, 0x00, 0x7D, 0x00, 0xB2, 0x00, 0xF5, 0x01, 0x49, 0x01,
                     0xB1, 0x02, 0x2B, 0x02, 0xB8, 0x03, 0x59, 0x04, 0x13, 0x04, 0xEC, 0x05, 0xF3, 0x07, 0x34, 0x08,
                     0xAF, 0x0A, 0xD9}},
    {"F16Txxxx", {0x00, 0x11, 0x00, 0x00, 0x00, 0x18, 0x00, 0x27, 0x00, 0x3A, 0x00, 0x52, 0x00, 0x71, 0x00, 0x96, 0x00,
                     0xC4, 0x00, 0xFC, 0x01, 0x40, 0x01, 0x93, 0x01, 0xF6, 0x02, 0x6E, 0x02, 0xFE, 0x03, 0xAA, 0x04,
                     0x78, 0x05, 0x6C}},
    {"F17Txxxx", {0x00, 0x11, 0x00, 0x00, 0x00, 0x1F, 0x00, 0x34, 0x00, 0x4F, 0x00, 0x71, 0x00, 0x9B, 0x00, 0xCF, 0x01,
                     0x0E, 0x01, 0x5D, 0x01, 0xBB, 0x02, 0x2F, 0x02, 0xB9, 0x03, 0x60, 0x04, 0x29, 0x05, 0x1E, 0x06,
                     0x44, 0x07, 0xA1}},
    {"F18Txxxx", {0x00, 0x11, 0x00, 0x00, 0x00, 0x53, 0x00, 0x8C, 0x00, 0xD5, 0x01, 0x31, 0x01, 0xA2, 0x02, 0x2E, 0x02,
                     0xD8, 0x03, 0xAE, 0x04, 0xAC, 0x05, 0xE5, 0x07, 0x59, 0x09, 0x1C, 0x0B, 0x3B, 0x0D, 0xD0, 0x10,
                     0xEA, 0x14, 0x99}},
    {"F19Txxxx", {0x00, 0x11, 0x00, 0x00, 0x02, 0x8F, 0x03, 0x53, 0x04, 0x5A, 0x05, 0xA1, 0x07, 0xAE, 0x0A, 0x3D, 0x0E,
                     0x14, 0x13, 0x74, 0x1A, 0x5E, 0x24, 0x18, 0x31, 0xA9, 0x44, 0x59, 0x5E, 0x76, 0x83, 0x11, 0xB6,
                     0xC7, 0xFF, 0x7B}},
    {"F24Txxxx", {0x00, 0x11, 0x00, 0x01, 0x00, 0x34, 0x00, 0x52, 0x00, 0x73, 0x00, 0x94, 0x00, 0xBE, 0x00, 0xFA, 0x01,
                     0x36, 0x01, 0x72, 0x01, 0xC5, 0x02, 0x2F, 0x02, 0xB9, 0x03, 0x60, 0x04, 0x1A, 0x05, 0x0A, 0x06,
                     0x0E, 0x07, 0x10}},
};

size_t WRed::wrapFunctionReturnZero() { return 0; }

bool WRed::wrapApplePanelSetDisplay(IOService *that, IODisplay *display) {
    static bool once = false;
    if (!once) {
        once = true;
        auto panels = OSDynamicCast(OSDictionary, that->getProperty("ApplePanels"));
        if (panels) {
            auto rawPanels = panels->copyCollection();
            panels = OSDynamicCast(OSDictionary, rawPanels);

            if (panels) {
                for (auto &entry : appleBacklightData) {
                    auto pd = OSData::withBytes(entry.deviceData, sizeof(entry.deviceData));
                    if (pd) {
                        panels->setObject(entry.deviceName, pd);
                        // No release required by current AppleBacklight implementation.
                    } else {
                        SYSLOG(MODULE_SHORT, "Panel start cannot allocate %s data", entry.deviceName);
                    }
                }
                that->setProperty("ApplePanels", panels);
            }

            if (rawPanels) { rawPanels->release(); }
        } else {
            SYSLOG(MODULE_SHORT, "Panel start has no panels");
        }
    }

    bool result = FunctionCast(wrapApplePanelSetDisplay, callback->orgApplePanelSetDisplay)(that, display);
    DBGLOG(MODULE_SHORT, "Panel display set returned %d", result);

    return result;
}
