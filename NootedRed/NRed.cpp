//! Copyright © 2022-2024 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5.
//! See LICENSE for details.

#include "NRed.hpp"
#include "AppleGFXHDA.hpp"
#include "Firmware.hpp"
#include "HWLibs.hpp"
#include "Model.hpp"
#include "PatcherPlus.hpp"
#include "X5000.hpp"
#include "X6000.hpp"
#include "X6000FB.hpp"
#include <Headers/kern_api.hpp>
#include <Headers/kern_devinfo.hpp>
#include <IOKit/IOCatalogue.h>

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

NRed *NRed::callback = nullptr;

static X6000FB x6000fb;
static AppleGFXHDA agfxhda;
static X5000HWLibs hwlibs;
static X6000 x6000;
static X5000 x5000;

void NRed::init() {
    SYSLOG("NRed", "Copyright 2022-2024 ChefKiss Inc. If you've paid for this, you've been scammed.");
    callback = this;

    lilu.onKextLoadForce(&kextAGDP);
    lilu.onKextLoadForce(&kextBacklight);
    lilu.onKextLoadForce(&kextMCCSControl);
    x6000fb.init();
    agfxhda.init();
    hwlibs.init();
    x6000.init();
    x5000.init();

    lilu.onPatcherLoadForce(
        [](void *user, KernelPatcher &patcher) { static_cast<NRed *>(user)->processPatcher(patcher); }, this);
    lilu.onKextLoadForce(
        nullptr, 0,
        [](void *user, KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size) {
            static_cast<NRed *>(user)->processKext(patcher, id, slide, size);
        },
        this);
}

void NRed::processPatcher(KernelPatcher &patcher) {
    bool backlightBootArg = false;
    PE_parse_boot_argn("AMDBacklight", &backlightBootArg, sizeof(backlightBootArg));
    this->enableBacklight =
        backlightBootArg || BaseDeviceInfo::get().modelType == WIOKit::ComputerModel::ComputerLaptop;
    if (!this->enableBacklight) {
        kextBacklight.switchOff();
        kextMCCSControl.switchOff();
    }
    auto *devInfo = DeviceInfo::create();
    if (devInfo) {
        devInfo->processSwitchOff();

        if (!devInfo->videoBuiltin) {
            for (size_t i = 0; i < devInfo->videoExternal.size(); i++) {
                auto *device = OSDynamicCast(IOPCIDevice, devInfo->videoExternal[i].video);
                if (!device) { continue; }
                auto devid = WIOKit::readPCIConfigValue(device, WIOKit::kIOPCIConfigDeviceID) & 0xFF00;
                if (WIOKit::readPCIConfigValue(device, WIOKit::kIOPCIConfigVendorID) == WIOKit::VendorID::ATIAMD &&
                    (devid == 0x1500 || devid == 0x1600)) {
                    this->iGPU = device;
                    break;
                }
            }
            PANIC_COND(!this->iGPU, "NRed", "No iGPU found");
        } else {
            PANIC_COND(!devInfo->videoBuiltin, "NRed", "videoBuiltin null");
            this->iGPU = OSDynamicCast(IOPCIDevice, devInfo->videoBuiltin);
            PANIC_COND(!this->iGPU, "NRed", "videoBuiltin is not IOPCIDevice");
            PANIC_COND(WIOKit::readPCIConfigValue(this->iGPU, WIOKit::kIOPCIConfigVendorID) != WIOKit::VendorID::ATIAMD,
                "NRed", "videoBuiltin is not AMD");
        }

        WIOKit::renameDevice(this->iGPU, "IGPU");
        WIOKit::awaitPublishing(this->iGPU);

        static UInt8 builtin[] = {0x00};
        this->iGPU->setProperty("built-in", builtin, arrsize(builtin));
        this->deviceId = WIOKit::readPCIConfigValue(this->iGPU, WIOKit::kIOPCIConfigDeviceID);
        this->pciRevision = WIOKit::readPCIConfigValue(NRed::callback->iGPU, WIOKit::kIOPCIConfigRevisionID);
        auto *model = getBranding(this->deviceId, this->pciRevision);
        auto modelLen = static_cast<UInt32>(strlen(model) + 1);
        if (!this->iGPU->getProperty("model")) {
            this->iGPU->setProperty("model", const_cast<char *>(model), modelLen);
        }
        this->iGPU->setProperty("ATY,FamilyName", const_cast<char *>("Radeon"), 7);
        this->iGPU->setProperty("ATY,DeviceName", const_cast<char *>(model) + 11, modelLen - 11);    //! Vega ...
        this->iGPU->setProperty("AAPL,slot-name", const_cast<char *>("built-in"), 9);

        char name[128] = {0};
        for (size_t i = 0, ii = 0; i < devInfo->videoExternal.size(); i++) {
            auto *device = OSDynamicCast(IOPCIDevice, devInfo->videoExternal[i].video);
            if (device && WIOKit::readPCIConfigValue(device, WIOKit::kIOPCIConfigDeviceID) != this->deviceId) {
                snprintf(name, arrsize(name), "GFX%zu", ii++);
                WIOKit::renameDevice(device, name);
                WIOKit::awaitPublishing(device);
            }
        }

        auto *prop = OSDynamicCast(OSData, this->iGPU->getProperty("ATY,bin_image"));
        if (prop) {
            DBGLOG("NRed", "VBIOS manually overridden");
            this->vbiosData = OSData::withBytes(prop->getBytesNoCopy(), prop->getLength());
            PANIC_COND(UNLIKELY(!this->vbiosData), "NRed", "Failed to allocate VBIOS data");
        } else if (!this->getVBIOSFromVFCT()) {
            SYSLOG("NRed", "Failed to get VBIOS from VFCT");
            PANIC_COND(!this->getVBIOSFromVRAM(), "NRed", "Failed to get VBIOS from VRAM");
        }
        auto len = this->vbiosData->getLength();
        if (len < 65536) {
            DBGLOG("NRed", "Padding VBIOS to 65536 bytes (was %u)", len);
            this->vbiosData->appendByte(0, 65536 - len);
        }

        DeviceInfo::deleter(devInfo);
    } else {
        SYSLOG("NRed", "Failed to create DeviceInfo");
    }

    KernelPatcher::RouteRequest request {"__ZN15OSMetaClassBase12safeMetaCastEPKS_PK11OSMetaClass", wrapSafeMetaCast,
        this->orgSafeMetaCast};
    PANIC_COND(!patcher.routeMultipleLong(KernelPatcher::KernelID, &request, 1), "nred",
        "Failed to route kernel symbols");

    x6000fb.registerDispMaxBrightnessNotif();

    if ((lilu.getRunMode() & LiluAPI::RunningInstallerRecovery) || checkKernelArgument("-CKFBOnly")) { return; }

    const auto &driversXML = getFWByName("Drivers.xml");
    auto *dataNull = new char[driversXML.length + 1];
    memcpy(dataNull, driversXML.data, driversXML.length);
    dataNull[driversXML.length] = 0;
    OSString *errStr = nullptr;
    auto *dataUnserialized = OSUnserializeXML(dataNull, driversXML.length + 1, &errStr);
    delete[] dataNull;
    PANIC_COND(!dataUnserialized, "NRed", "Failed to unserialize Drivers.xml: %s",
        errStr ? errStr->getCStringNoCopy() : "Unspecified");
    auto *drivers = OSDynamicCast(OSArray, dataUnserialized);
    PANIC_COND(!drivers, "NRed", "Failed to cast Drivers.xml data");
    PANIC_COND(!gIOCatalogue->addDrivers(drivers), "NRed", "Failed to add drivers");
    OSSafeReleaseNULL(dataUnserialized);
}

OSMetaClassBase *NRed::wrapSafeMetaCast(const OSMetaClassBase *anObject, const OSMetaClass *toMeta) {
    auto ret = FunctionCast(wrapSafeMetaCast, callback->orgSafeMetaCast)(anObject, toMeta);
    if (UNLIKELY(!ret)) {
        for (const auto &ent : callback->metaClassMap) {
            if (LIKELY(ent[0] == toMeta)) {
                return FunctionCast(wrapSafeMetaCast, callback->orgSafeMetaCast)(anObject, ent[1]);
            } else if (UNLIKELY(ent[1] == toMeta)) {
                return FunctionCast(wrapSafeMetaCast, callback->orgSafeMetaCast)(anObject, ent[0]);
            }
        }
    }
    return ret;
}

void NRed::setRMMIOIfNecessary() {
    if (this->rmmio && this->rmmio->getLength()) { return; }

    this->rmmio = this->iGPU->mapDeviceMemoryWithRegister(kIOPCIConfigBaseAddress5, kIOInhibitCache | kIOMapAnywhere);
    PANIC_COND(UNLIKELY(!this->rmmio || !this->rmmio->getLength()), "NRed", "Failed to map RMMIO");
    this->rmmioPtr = reinterpret_cast<UInt32 *>(this->rmmio->getVirtualAddress());

    this->fbOffset = static_cast<UInt64>(this->readReg32(0x296B)) << 24;
    this->revision = (this->readReg32(0xD2F) & 0xF000000) >> 0x18;
    switch (this->deviceId) {
        case 0x15D8:
            if (LIKELY(this->revision >= 0x8)) {
                this->chipType = ChipType::Raven2;
                this->enumRevision = 0x79;
                break;
            }
            this->chipType = ChipType::Picasso;
            this->enumRevision = 0x41;
            break;
        case 0x15DD:
            if (LIKELY(this->revision >= 0x8)) {
                this->chipType = ChipType::Raven2;
                this->enumRevision = 0x79;
                break;
            }
            this->chipType = ChipType::Raven;
            this->enumRevision = 0x10;
            break;
        case 0x164C:
            [[fallthrough]];
        case 0x1636:
            this->chipType = ChipType::Renoir;
            this->enumRevision = 0x91;
            break;
        case 0x15E7:
            [[fallthrough]];
        case 0x1638:
            this->chipType = ChipType::GreenSardine;
            this->enumRevision = 0xA1;
            break;
        default:
            PANIC("NRed", "Unknown device ID");
    }
}

void NRed::processKext(KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size) {
    if (kextAGDP.loadIndex == id) {
        const LookupPatchPlus patch {&kextAGDP, kAGDPBoardIDKeyOriginal, kAGDPBoardIDKeyPatched, 1};
        SYSLOG_COND(!patch.apply(patcher, slide, size), "NRed", "Failed to apply AGDP board-id patch");

        if (getKernelVersion() == KernelVersion::Ventura) {
            const LookupPatchPlus patch {&kextAGDP, kAGDPFBCountCheckVenturaOriginal, kAGDPFBCountCheckVenturaPatched,
                1};
            SYSLOG_COND(!patch.apply(patcher, slide, size), "NRed", "Failed to apply AGDP fb count check patch");
        } else {
            const LookupPatchPlus patch {&kextAGDP, kAGDPFBCountCheckOriginal, kAGDPFBCountCheckPatched, 1};
            SYSLOG_COND(!patch.apply(patcher, slide, size), "NRed", "Failed to apply AGDP fb count check patch");
        }
    } else if (kextBacklight.loadIndex == id) {
        KernelPatcher::RouteRequest request {"__ZN15AppleIntelPanel10setDisplayEP9IODisplay", wrapApplePanelSetDisplay,
            orgApplePanelSetDisplay};
        if (patcher.routeMultiple(kextBacklight.loadIndex, &request, 1, slide, size)) {
            const UInt8 find[] = {"F%uT%04x"};
            const UInt8 replace[] = {"F%uTxxxx"};
            const LookupPatchPlus patch {&kextBacklight, find, replace, 1};
            SYSLOG_COND(!patch.apply(patcher, slide, size), "NRed", "Failed to apply backlight patch");
        }
    } else if (kextMCCSControl.loadIndex == id) {
        KernelPatcher::RouteRequest requests[] = {
            {"__ZN25AppleMCCSControlGibraltar5probeEP9IOServicePi", wrapFunctionReturnZero},
            {"__ZN21AppleMCCSControlCello5probeEP9IOServicePi", wrapFunctionReturnZero},
        };
        patcher.routeMultiple(id, requests, slide, size);
        patcher.clearError();
    } else if (agfxhda.processKext(patcher, id, slide, size)) {
        DBGLOG("NRed", "Processed AppleGFXHDA");
    } else if (x6000fb.processKext(patcher, id, slide, size)) {
        DBGLOG("NRed", "Processed AMDRadeonX6000Framebuffer");
    } else if (hwlibs.processKext(patcher, id, slide, size)) {
        DBGLOG("NRed", "Processed AMDRadeonX5000HWLibs");
    } else if (x6000.processKext(patcher, id, slide, size)) {
        DBGLOG("NRed", "Processed AMDRadeonX6000");
    } else if (x5000.processKext(patcher, id, slide, size)) {
        DBGLOG("NRed", "Processed AMDRadeonX5000");
    }
}

struct ApplePanelData {
    const char *deviceName;
    UInt8 deviceData[36];
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

size_t NRed::wrapFunctionReturnZero() { return 0; }

bool NRed::wrapApplePanelSetDisplay(IOService *that, IODisplay *display) {
    static bool once = false;
    if (!once) {
        once = true;
        auto *panels = OSDynamicCast(OSDictionary, that->getProperty("ApplePanels"));
        if (panels) {
            auto *rawPanels = panels->copyCollection();
            panels = OSDynamicCast(OSDictionary, rawPanels);

            if (panels) {
                for (auto &entry : appleBacklightData) {
                    auto pd = OSData::withBytes(entry.deviceData, sizeof(entry.deviceData));
                    if (pd) {
                        panels->setObject(entry.deviceName, pd);
                        //! No release required by current AppleBacklight implementation.
                    } else {
                        SYSLOG("NRed", "setDisplay: Cannot allocate data for %s", entry.deviceName);
                    }
                }
                that->setProperty("ApplePanels", panels);
            }

            OSSafeReleaseNULL(rawPanels);
        } else {
            SYSLOG("NRed", "setDisplay: Missing ApplePanels property");
        }
    }

    bool ret = FunctionCast(wrapApplePanelSetDisplay, callback->orgApplePanelSetDisplay)(that, display);
    DBGLOG("NRed", "setDisplay >> %d", ret);
    return ret;
}
