// Copyright © 2022-2024 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#include "NRed.hpp"
#include "AMDCommon.hpp"
#include "Firmware.hpp"
#include "Model.hpp"
#include "PatcherPlus.hpp"
#include <Headers/kern_api.hpp>
#include <Headers/kern_devinfo.hpp>

static const char *pathAGDP = "/System/Library/Extensions/AppleGraphicsControl.kext/Contents/PlugIns/"
                              "AppleGraphicsDevicePolicy.kext/Contents/MacOS/AppleGraphicsDevicePolicy";
static const char *pathBacklight = "/System/Library/Extensions/AppleBacklight.kext/Contents/MacOS/AppleBacklight";
static const char *pathMCCSControl = "/System/Library/Extensions/AppleMCCSControl.kext/Contents/MacOS/AppleMCCSControl";

static KernelPatcher::KextInfo kextAGDP {
    "com.apple.driver.AppleGraphicsDevicePolicy",
    &pathAGDP,
    1,
    {},
    {},
    KernelPatcher::KextInfo::Unloaded,
};
static KernelPatcher::KextInfo kextBacklight {
    "com.apple.driver.AppleBacklight",
    &pathBacklight,
    1,
    {},
    {},
    KernelPatcher::KextInfo::Unloaded,
};
static KernelPatcher::KextInfo kextMCCSControl {
    "com.apple.driver.AppleMCCSControl",
    &pathMCCSControl,
    1,
    {},
    {},
    KernelPatcher::KextInfo::Unloaded,
};

NRed *NRed::callback = nullptr;

void NRed::init() {
    SYSLOG("NRed", "Copyright 2022-2024 ChefKiss. If you've paid for this, you've been scammed.");

    bool backlightBootArg = false;
    if (PE_parse_boot_argn("AMDBacklight", &backlightBootArg, sizeof(backlightBootArg))) {
        if (backlightBootArg) { this->attributes.setBacklightEnabled(); }
    } else if (BaseDeviceInfo::get().modelType == WIOKit::ComputerModel::ComputerLaptop) {
        this->attributes.setBacklightEnabled();
    }

    switch (getKernelVersion()) {
        case KernelVersion::Catalina:
            this->attributes.setCatalina();
            break;
        case KernelVersion::BigSur:
            this->attributes.setBigSurAndLater();
            break;
        case KernelVersion::Monterey:
            this->attributes.setBigSurAndLater();
            this->attributes.setMonterey();
            this->attributes.setMontereyAndLater();
            break;
        case KernelVersion::Ventura:
            this->attributes.setBigSurAndLater();
            this->attributes.setMontereyAndLater();
            this->attributes.setVentura();
            this->attributes.setVenturaAndLater();
            if (getKernelMinorVersion() >= 5) {
                this->attributes.setVentura1304Based();
                this->attributes.setVentura1304AndLater();
            }
            break;
        case KernelVersion::Sonoma:
            this->attributes.setBigSurAndLater();
            this->attributes.setMontereyAndLater();
            this->attributes.setVenturaAndLater();
            this->attributes.setVentura1304AndLater();
            if (getKernelMinorVersion() >= 4) { this->attributes.setSonoma1404AndLater(); }
            break;
        case KernelVersion::Sequoia:
            this->attributes.setBigSurAndLater();
            this->attributes.setMontereyAndLater();
            this->attributes.setVenturaAndLater();
            this->attributes.setVentura1304AndLater();
            this->attributes.setSonoma1404AndLater();
            break;
        default:
            PANIC("NRed", "Unknown kernel version %d", getKernelVersion());
    }

    SYSLOG("NRed", "Module initialised");
    DBGLOG("NRed", "catalina = %s", this->attributes.isCatalina() ? "yes" : "no");
    DBGLOG("NRed", "bigSurAndLater = %s", this->attributes.isBigSurAndLater() ? "yes" : "no");
    DBGLOG("NRed", "monterey = %s", this->attributes.isMonterey() ? "yes" : "no");
    DBGLOG("NRed", "montereyAndLater = %s", this->attributes.isMontereyAndLater() ? "yes" : "no");
    DBGLOG("NRed", "ventura = %s", this->attributes.isVentura() ? "yes" : "no");
    DBGLOG("NRed", "venturaAndLater = %s", this->attributes.isVenturaAndLater() ? "yes" : "no");
    DBGLOG("NRed", "ventura1304Based = %s", this->attributes.isVentura1304Based() ? "yes" : "no");
    DBGLOG("NRed", "ventura1304AndLater = %s", this->attributes.isVentura1304AndLater() ? "yes" : "no");
    DBGLOG("NRed", "sonoma1404AndLater = %s", this->attributes.isSonoma1404AndLater() ? "yes" : "no");
    DBGLOG("NRed", "If any of the above values look incorrect, please report this to the developers.");

    callback = this;
    lilu.onKextLoadForce(&kextAGDP);
    if (this->attributes.isBacklightEnabled()) {
        lilu.onKextLoadForce(&kextBacklight);
        lilu.onKextLoadForce(&kextMCCSControl);
    }
    this->x6000fb.init();
    this->agfxhda.init();
    this->hwlibs.init();
    this->x6000.init();
    this->x5000.init();

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
    auto *devInfo = DeviceInfo::create();
    PANIC_COND(devInfo == nullptr, "NRed", "Failed to create device info!");

    devInfo->processSwitchOff();

    if (devInfo->videoBuiltin == nullptr) {
        for (size_t i = 0; i < devInfo->videoExternal.size(); i++) {
            auto *device = OSDynamicCast(IOPCIDevice, devInfo->videoExternal[i].video);
            if (device == nullptr) { continue; }
            auto devid = WIOKit::readPCIConfigValue(device, WIOKit::kIOPCIConfigDeviceID) & 0xFF00;
            if (WIOKit::readPCIConfigValue(device, WIOKit::kIOPCIConfigVendorID) == WIOKit::VendorID::ATIAMD &&
                (devid == 0x1500 || devid == 0x1600)) {
                this->iGPU = device;
                break;
            }
        }
        PANIC_COND(this->iGPU == nullptr, "NRed", "No iGPU found");
    } else {
        this->iGPU = OSDynamicCast(IOPCIDevice, devInfo->videoBuiltin);
        PANIC_COND(this->iGPU == nullptr, "NRed", "videoBuiltin is not IOPCIDevice");
        PANIC_COND(WIOKit::readPCIConfigValue(this->iGPU, WIOKit::kIOPCIConfigVendorID) != WIOKit::VendorID::ATIAMD,
            "NRed", "videoBuiltin is not AMD");
    }

    WIOKit::renameDevice(this->iGPU, "IGPU");
    WIOKit::awaitPublishing(this->iGPU);

    static UInt8 builtin[] = {0x00};
    this->iGPU->setProperty("built-in", builtin, arrsize(builtin));
    this->deviceID = WIOKit::readPCIConfigValue(this->iGPU, WIOKit::kIOPCIConfigDeviceID);
    this->pciRevision = WIOKit::readPCIConfigValue(NRed::callback->iGPU, WIOKit::kIOPCIConfigRevisionID);

    SYSLOG_COND(this->iGPU->getProperty("model") != nullptr, "NRed",
        "WARNING!!! Attempted to manually override the model, this is no longer supported!!");

    auto *model = getBranding(this->deviceID, this->pciRevision);
    auto modelLen = static_cast<UInt32>(strlen(model) + 1);
    this->iGPU->setProperty("model", const_cast<char *>(model), modelLen);
    this->iGPU->setProperty("ATY,FamilyName", const_cast<char *>("Radeon"), 7);
    this->iGPU->setProperty("ATY,DeviceName", const_cast<char *>(model) + 11, modelLen - 11);    // Vega ...
    this->iGPU->setProperty("AAPL,slot-name", const_cast<char *>("built-in"), 9);

    char name[128];
    bzero(name, sizeof(name));
    for (size_t i = 0, ii = 0; i < devInfo->videoExternal.size(); i++) {
        auto *device = OSDynamicCast(IOPCIDevice, devInfo->videoExternal[i].video);
        if (device != nullptr &&
            (WIOKit::readPCIConfigValue(device, WIOKit::kIOPCIConfigVendorID) != WIOKit::VendorID::ATIAMD ||
                WIOKit::readPCIConfigValue(device, WIOKit::kIOPCIConfigDeviceID) != this->deviceID)) {
            snprintf(name, arrsize(name), "GFX%zu", ii++);
            WIOKit::renameDevice(device, name);
            WIOKit::awaitPublishing(device);
        }
    }

    auto *prop = OSDynamicCast(OSData, this->iGPU->getProperty("ATY,bin_image"));
    if (prop == nullptr) {
        if (!this->getVBIOSFromVFCT()) {
            SYSLOG("NRed", "Failed to get VBIOS from VFCT, trying to get it from VRAM!");
            PANIC_COND(!this->getVBIOSFromVRAM(), "NRed", "Failed to get VBIOS!");
        }
    } else {
        SYSLOG("NRed", "WARNING!!! VBIOS MANUALLY OVERRIDDEN, ONLY DO THIS IF YOU KNOW WHAT YOU'RE DOING!!");
        this->vbiosData = OSData::withBytes(prop->getBytesNoCopy(), prop->getLength());
        PANIC_COND(this->vbiosData == nullptr, "NRed", "Failed to allocate VBIOS data!");
    }

    auto len = this->vbiosData->getLength();
    if (len < 65536) {
        DBGLOG("NRed", "Padding VBIOS to 65536 bytes (was %u).", len);
        this->vbiosData->appendByte(0, 65536 - len);
    }

    DeviceInfo::deleter(devInfo);

    KernelPatcher::RouteRequest requests[] = {
        {"__ZN15OSMetaClassBase12safeMetaCastEPKS_PK11OSMetaClass", wrapSafeMetaCast, this->orgSafeMetaCast},
        {"__ZN11IOCatalogue10addDriversEP7OSArrayb", wrapAddDrivers, this->orgAddDrivers},
    };
    PANIC_COND(!patcher.routeMultipleLong(KernelPatcher::KernelID, requests), "NRed", "Failed to route kernel symbols");

    this->x6000fb.registerDispMaxBrightnessNotif();
}

static const char *getDriverXMLForBundle(const char *bundleIdentifier, size_t *len) {
    const auto identifierLen = strlen(bundleIdentifier);
    const auto totalLen = identifierLen + 5;
    auto *filename = new char[totalLen];
    memcpy(filename, bundleIdentifier, identifierLen);
    strlcat(filename, ".xml", totalLen);

    const auto &driversXML = getFWByName(filename);
    delete[] filename;

    *len = driversXML.length + 1;
    auto *dataNull = new char[*len];
    memcpy(dataNull, driversXML.data, driversXML.length);
    dataNull[driversXML.length] = 0;

    return dataNull;
}

static const char *DriverBundleIdentifiers[] = {
    "com.apple.kext.AMDRadeonX5000",
    "com.apple.kext.AMDRadeonX5000HWServices",
    "com.apple.kext.AMDRadeonX6000",
    "com.apple.kext.AMDRadeonX6000Framebuffer",
    "com.apple.driver.AppleGFXHDA",
};

static UInt8 matchedDrivers = 0;

bool NRed::wrapAddDrivers(void *that, OSArray *array, bool doNubMatching) {
    UInt32 driverCount = array->getCount();
    for (UInt32 driverIndex = 0; driverIndex < driverCount; driverIndex += 1) {
        OSObject *object = array->getObject(driverIndex);
        PANIC_COND(object == nullptr, "NRed", "Critical error in addDrivers: Index is out of bounds.");
        auto *dict = OSDynamicCast(OSDictionary, object);
        if (dict == nullptr) { continue; }
        auto *bundleIdentifier = OSDynamicCast(OSString, dict->getObject("CFBundleIdentifier"));
        if (bundleIdentifier == nullptr || bundleIdentifier->getLength() == 0) { continue; }
        auto *bundleIdentifierCStr = bundleIdentifier->getCStringNoCopy();
        if (bundleIdentifierCStr == nullptr) { continue; }

        for (size_t identifierIndex = 0; identifierIndex < arrsize(DriverBundleIdentifiers); identifierIndex += 1) {
            if ((matchedDrivers & (1U << identifierIndex)) != 0) { continue; }

            if (strcmp(bundleIdentifierCStr, DriverBundleIdentifiers[identifierIndex]) == 0) {
                matchedDrivers |= (1U << identifierIndex);

                DBGLOG("NRed", "Matched %s, injecting.", bundleIdentifierCStr);

                size_t len;
                auto *driverXML = getDriverXMLForBundle(bundleIdentifierCStr, &len);

                OSString *errStr = nullptr;
                auto *dataUnserialized = OSUnserializeXML(driverXML, len, &errStr);
                delete[] driverXML;

                PANIC_COND(dataUnserialized == nullptr, "NRed", "Failed to unserialize driver XML for %s: %s",
                    bundleIdentifierCStr, errStr ? errStr->getCStringNoCopy() : "(nil)");

                auto *drivers = OSDynamicCast(OSArray, dataUnserialized);
                PANIC_COND(drivers == nullptr, "NRed", "Failed to cast %s driver data", bundleIdentifierCStr);
                UInt32 injectedDriverCount = drivers->getCount();

                array->ensureCapacity(driverCount + injectedDriverCount);

                for (UInt32 injectedDriverIndex = 0; injectedDriverIndex < injectedDriverCount;
                     injectedDriverIndex += 1) {
                    array->setObject(driverIndex, drivers->getObject(injectedDriverIndex));
                    driverIndex += 1;
                    driverCount += 1;
                }

                dataUnserialized->release();
                break;
            }
        }
    }

    return FunctionCast(wrapAddDrivers, callback->orgAddDrivers)(that, array, doNubMatching);
}

OSMetaClassBase *NRed::wrapSafeMetaCast(const OSMetaClassBase *anObject, const OSMetaClass *toMeta) {
    auto ret = FunctionCast(wrapSafeMetaCast, callback->orgSafeMetaCast)(anObject, toMeta);

    if (LIKELY(ret)) { return ret; }

    for (const auto &ent : callback->metaClassMap) {
        if (UNLIKELY(ent[0] == toMeta)) {
            return FunctionCast(wrapSafeMetaCast, callback->orgSafeMetaCast)(anObject, ent[1]);
        } else if (UNLIKELY(ent[1] == toMeta)) {
            return FunctionCast(wrapSafeMetaCast, callback->orgSafeMetaCast)(anObject, ent[0]);
        }
    }

    return nullptr;
}

void NRed::ensureRMMIO() {
    if (this->rmmio != nullptr) { return; }

    this->rmmio =
        this->iGPU->mapDeviceMemoryWithRegister(kIOPCIConfigBaseAddress5, kIOMapInhibitCache | kIOMapAnywhere);
    PANIC_COND(this->rmmio == nullptr || this->rmmio->getLength() == 0, "NRed", "Failed to map RMMIO");
    this->rmmioPtr = reinterpret_cast<UInt32 *>(this->rmmio->getVirtualAddress());

    this->fbOffset = static_cast<UInt64>(this->readReg32(0x296B)) << 24;
    this->devRevision = (this->readReg32(0xD2F) & 0xF000000) >> 0x18;

    switch (this->deviceID) {
        case 0x15D8:
            this->attributes.setRaven();
            this->attributes.setPicasso();
            if (this->devRevision >= 0x8) {
                this->attributes.setRaven2();
                this->enumRevision = 0x79;
            } else {
                this->enumRevision = 0x41;
            }
            break;
        case 0x15DD:
            this->attributes.setRaven();
            if (this->devRevision >= 0x8) {
                this->attributes.setRaven2();
                this->enumRevision = 0x79;
            } else {
                this->enumRevision = this->devRevision == 1 ? 0x20 : 0x1;
            }
            break;
        case 0x164C:
        case 0x1636:
            this->attributes.setRenoir();
            this->enumRevision = 0x91;
            break;
        case 0x15E7:
        case 0x1638:
            this->attributes.setRenoir();
            this->attributes.setGreenSardine();
            this->enumRevision = 0xA1;
            break;
        default:
            PANIC("NRed", "Unknown device ID: 0x%X", this->deviceID);
    }

    DBGLOG("NRed", "Device ID = 0x%X", this->deviceID);
    DBGLOG("NRed", "Device PCI Revision = 0x%X", this->pciRevision);
    DBGLOG("NRed", "Device Framebuffer Offset = 0x%llX", this->fbOffset);
    DBGLOG("NRed", "Device Revision = 0x%X", this->devRevision);
    DBGLOG("NRed", "Device Is Raven = %s", this->attributes.isRaven() ? "yes" : "no");
    DBGLOG("NRed", "Device Is Picasso = %s", this->attributes.isPicasso() ? "yes" : "no");
    DBGLOG("NRed", "Device Is Raven2 = %s", this->attributes.isRaven2() ? "yes" : "no");
    DBGLOG("NRed", "Device Is Renoir = %s", this->attributes.isRenoir() ? "yes" : "no");
    DBGLOG("NRed", "Device Is Green Sardine = %s", this->attributes.isGreenSardine() ? "yes" : "no");
    DBGLOG("NRed", "Device Enumerated Revision = 0x%X", this->enumRevision);
    DBGLOG("NRed", "If any of the above values look incorrect, please report this to the developers.");
}

void NRed::processKext(KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size) {
    if (kextAGDP.loadIndex == id) {
        const LookupPatchPlus patch {&kextAGDP, kAGDPBoardIDKeyOriginal, kAGDPBoardIDKeyPatched, 1};
        SYSLOG_COND(!patch.apply(patcher, slide, size), "NRed", "Failed to apply AGDP board-id patch");

        if (this->attributes.isVentura()) {
            const LookupPatchPlus patch {&kextAGDP, kAGDPFBCountCheckVenturaOriginal, kAGDPFBCountCheckVenturaPatched,
                1};
            SYSLOG_COND(!patch.apply(patcher, slide, size), "NRed", "Failed to apply AGDP FB count check patch");
        } else {
            const LookupPatchPlus patch {&kextAGDP, kAGDPFBCountCheckOriginal, kAGDPFBCountCheckPatched, 1};
            SYSLOG_COND(!patch.apply(patcher, slide, size), "NRed", "Failed to apply AGDP FB count check patch");
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
    } else if (this->agfxhda.processKext(patcher, id, slide, size)) {
        DBGLOG("NRed", "Processed AppleGFXHDA");
    } else if (this->x6000fb.processKext(patcher, id, slide, size)) {
        DBGLOG("NRed", "Processed AMDRadeonX6000Framebuffer");
    } else if (this->hwlibs.processKext(patcher, id, slide, size)) {
        DBGLOG("NRed", "Processed AMDRadeonX5000HWLibs");
    } else if (this->x6000.processKext(patcher, id, slide, size)) {
        DBGLOG("NRed", "Processed AMDRadeonX6000");
    } else if (this->x5000.processKext(patcher, id, slide, size)) {
        DBGLOG("NRed", "Processed AMDRadeonX5000");
    }
}

const char *NRed::getChipName() {
    if (this->attributes.isRaven2()) {
        return "raven2";
    } else if (this->attributes.isPicasso()) {
        return "picasso";
    } else if (this->attributes.isRaven()) {
        return "raven";
    } else if (this->attributes.isGreenSardine()) {
        return "green_sardine";
    } else if (this->attributes.isRenoir()) {
        return "renoir";
    } else {
        PANIC("NRed", "Internal error: Device is unknown");
    }
}

const char *NRed::getGCPrefix() {
    if (this->attributes.isRaven2()) {
        return "gc_9_2_";
    } else if (this->attributes.isRaven()) {
        return "gc_9_1_";
    } else if (this->attributes.isRenoir()) {
        return "gc_9_3_";
    } else {
        PANIC("NRed", "Internal error: Device is unknown");
    }
}

UInt32 NRed::readReg32(UInt32 reg) {
    if ((reg * sizeof(UInt32)) < this->rmmio->getLength()) {
        return this->rmmioPtr[reg];
    } else {
        this->rmmioPtr[mmPCIE_INDEX2] = reg;
        return this->rmmioPtr[mmPCIE_DATA2];
    }
}

void NRed::writeReg32(UInt32 reg, UInt32 val) {
    if ((reg * sizeof(UInt32)) < this->rmmio->getLength()) {
        this->rmmioPtr[reg] = val;
    } else {
        this->rmmioPtr[mmPCIE_INDEX2] = reg;
        this->rmmioPtr[mmPCIE_DATA2] = val;
    }
}

CAILResult NRed::sendMsgToSmc(UInt32 msg, UInt32 param, UInt32 *outParam) {
    this->smuWaitForResponse();

    this->writeReg32(MP_BASE + mmMP1_SMN_C2PMSG_90, 0);
    this->writeReg32(MP_BASE + mmMP1_SMN_C2PMSG_82, param);
    this->writeReg32(MP_BASE + mmMP1_SMN_C2PMSG_66, msg);

    const auto resp = this->smuWaitForResponse();

    if (outParam != nullptr) { *outParam = this->readReg32(MP_BASE + mmMP1_SMN_C2PMSG_82); }

    return processSMUFWResponse(resp);
}

// https://elixir.bootlin.com/linux/latest/source/drivers/gpu/drm/amd/amdgpu/amdgpu_bios.c#L49
static bool checkAtomBios(const UInt8 *bios, size_t size) {
    UInt16 tmp, bios_header_start;

    if (size < 0x49) {
        DBGLOG("NRed", "VBIOS size is invalid");
        return false;
    }

    if (bios[0] != 0x55 || bios[1] != 0xAA) {
        DBGLOG("NRed", "VBIOS signature <%x %x> is invalid", bios[0], bios[1]);
        return false;
    }

    bios_header_start = bios[0x48] | (bios[0x49] << 8);
    if (!bios_header_start) {
        DBGLOG("NRed", "Unable to locate VBIOS header");
        return false;
    }

    tmp = bios_header_start + 4;
    if (size < tmp) {
        DBGLOG("NRed", "BIOS header is broken");
        return false;
    }

    if (!memcmp(bios + tmp, "ATOM", 4) || !memcmp(bios + tmp, "MOTA", 4)) {
        DBGLOG("NRed", "ATOMBIOS detected");
        return true;
    }

    return false;
}

// Hack
class AppleACPIPlatformExpert : IOACPIPlatformExpert {
    friend class NRed;
};

bool NRed::getVBIOSFromVFCT() {
    DBGLOG("NRed", "Fetching VBIOS from VFCT table");
    auto *expert = reinterpret_cast<AppleACPIPlatformExpert *>(this->iGPU->getPlatform());
    PANIC_COND(expert == nullptr, "NRed", "Failed to get AppleACPIPlatformExpert");

    auto *vfctData = expert->getACPITableData("VFCT", 0);
    if (vfctData == nullptr) {
        DBGLOG("NRed", "No VFCT from AppleACPIPlatformExpert");
        return false;
    }

    auto *vfct = static_cast<const VFCT *>(vfctData->getBytesNoCopy());
    PANIC_COND(vfct == nullptr, "NRed", "VFCT OSData::getBytesNoCopy returned null");

    for (auto offset = vfct->vbiosImageOffset; offset < vfctData->getLength();) {
        auto *vHdr =
            static_cast<const GOPVideoBIOSHeader *>(vfctData->getBytesNoCopy(offset, sizeof(GOPVideoBIOSHeader)));
        if (vHdr == nullptr) {
            DBGLOG("NRed", "VFCT header out of bounds");
            return false;
        }

        auto *vContent = static_cast<const UInt8 *>(
            vfctData->getBytesNoCopy(offset + sizeof(GOPVideoBIOSHeader), vHdr->imageLength));
        if (vContent == nullptr) {
            DBGLOG("NRed", "VFCT VBIOS image out of bounds");
            return false;
        }

        offset += sizeof(GOPVideoBIOSHeader) + vHdr->imageLength;

        if (vHdr->deviceID == this->deviceID) {
            if (!checkAtomBios(vContent, vHdr->imageLength)) {
                DBGLOG("NRed", "VFCT VBIOS is not an ATOMBIOS");
                return false;
            }

            this->vbiosData = OSData::withBytes(vContent, vHdr->imageLength);
            PANIC_COND(this->vbiosData == nullptr, "NRed", "VFCT OSData::withBytes failed");
            this->iGPU->setProperty("ATY,bin_image", this->vbiosData);

            return true;
        }
    }

    return false;
}

bool NRed::getVBIOSFromVRAM() {
    auto *bar0 =
        this->iGPU->mapDeviceMemoryWithRegister(kIOPCIConfigBaseAddress0, kIOMapWriteCombineCache | kIOMapAnywhere);
    if (!bar0 || !bar0->getLength()) {
        DBGLOG("NRed", "FB BAR not enabled");
        OSSafeReleaseNULL(bar0);
        return false;
    }
    auto *fb = reinterpret_cast<const UInt8 *>(bar0->getVirtualAddress());
    UInt32 size = 256 * 1024;    // ???
    if (!checkAtomBios(fb, size)) {
        DBGLOG("NRed", "VRAM VBIOS is not an ATOMBIOS");
        OSSafeReleaseNULL(bar0);
        return false;
    }
    this->vbiosData = OSData::withBytes(fb, size);
    PANIC_COND(this->vbiosData == nullptr, "NRed", "VRAM OSData::withBytes failed");
    this->iGPU->setProperty("ATY,bin_image", this->vbiosData);
    OSSafeReleaseNULL(bar0);
    return true;
}

UInt32 NRed::smuWaitForResponse() {
    for (UInt32 i = 0; i < AMDGPU_MAX_USEC_TIMEOUT; i++) {
        UInt32 ret = this->readReg32(MP_BASE + mmMP1_SMN_C2PMSG_90);
        if (ret != AMDSMUFWResponse::kSMUFWResponseNoResponse) { return ret; }

        IOSleep(1);
    }

    return AMDSMUFWResponse::kSMUFWResponseNoResponse;
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
                        // No release required by current AppleBacklight implementation.
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
