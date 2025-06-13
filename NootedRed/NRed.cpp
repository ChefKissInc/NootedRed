// Copyright © 2022-2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#include <Headers/kern_api.hpp>
#include <Headers/kern_devinfo.hpp>
#include <Headers/kern_iokit.hpp>
#include <IOKit/acpi/IOACPIPlatformExpert.h>
#include <PrivateHeaders/Backlight.hpp>
#include <PrivateHeaders/DebugEnabler.hpp>
#include <PrivateHeaders/Firmware.hpp>
#include <PrivateHeaders/GPUDriversAMD/Driver.hpp>
#include <PrivateHeaders/Hotfixes/AGDP.hpp>
#include <PrivateHeaders/Hotfixes/X6000FB.hpp>
#include <PrivateHeaders/Model.hpp>
#include <PrivateHeaders/NRed.hpp>
#include <PrivateHeaders/PatcherPlus.hpp>
#include <PrivateHeaders/iVega/AppleGFXHDA.hpp>
#include <PrivateHeaders/iVega/HWLibs.hpp>
#include <PrivateHeaders/iVega/IPOffset.hpp>
#include <PrivateHeaders/iVega/Regs/GC.hpp>
#include <PrivateHeaders/iVega/Regs/NBIO.hpp>
#include <PrivateHeaders/iVega/Regs/SMU.hpp>
#include <PrivateHeaders/iVega/X5000.hpp>
#include <PrivateHeaders/iVega/X6000.hpp>
#include <PrivateHeaders/iVega/X6000FB.hpp>

//------ Module Logic ------//

static NRed instance {};

NRed &NRed::singleton() { return instance; }

void NRed::init() {
    PANIC_COND(this->initialised, "NRed", "Attempted to initialise module twice!");
    this->initialised = true;

    SYSLOG("NRed", "Copyright 2022-2025 ChefKiss. If you've paid for this, you've been scammed.");

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
        case KernelVersion::Tahoe:
            this->attributes.setBigSurAndLater();
            this->attributes.setMontereyAndLater();
            this->attributes.setVenturaAndLater();
            this->attributes.setVentura1304AndLater();
            this->attributes.setSonoma1404AndLater();
            break;
        default:
            PANIC("NRed", "Unknown kernel version %d", getKernelVersion());
    }

    SYSLOG("NRed", "Module initialised.");
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

    Hotfixes::AGDP::singleton().init();
    Hotfixes::X6000FB::singleton().init();
    Backlight::singleton().init();
    DebugEnabler::singleton().init();
    iVega::X6000FB::singleton().init();
    iVega::AppleGFXHDA::singleton().init();
    iVega::X5000HWLibs::singleton().init();
    iVega::X6000::singleton().init();
    iVega::X5000::singleton().init();

    lilu.onPatcherLoadForce(
        [](void *user, KernelPatcher &patcher) { static_cast<NRed *>(user)->processPatcher(patcher); }, this);
}

void NRed::hwLateInit() {
    if (this->rmmio != nullptr) { return; }

    this->iGPU->setMemoryEnable(true);
    this->iGPU->setBusMasterEnable(true);

    PANIC_COND(!this->getVBIOS(), "NRed", "Failed to get VBIOS!");

    auto len = this->vbiosData->getLength();
    if (len < ATOMBIOS_IMAGE_SIZE) {
        DBGLOG("NRed", "Padding VBIOS to %u bytes (was %u).", ATOMBIOS_IMAGE_SIZE, len);
        this->vbiosData->appendByte(0, ATOMBIOS_IMAGE_SIZE - len);
    }

    this->iGPU->setProperty("ATY,bin_image", this->vbiosData);

    this->rmmio =
        this->iGPU->mapDeviceMemoryWithRegister(kIOPCIConfigBaseAddress5, kIOMapInhibitCache | kIOMapAnywhere);
    PANIC_COND(this->rmmio == nullptr || this->rmmio->getLength() == 0, "NRed", "Failed to map RMMIO");
    this->rmmioPtr = reinterpret_cast<UInt32 *>(this->rmmio->getVirtualAddress());

    this->fbOffset = static_cast<UInt64>(this->readReg32(GC_BASE_0 + MC_VM_FB_OFFSET)) << 24;
    this->devRevision = (this->readReg32(NBIO_BASE_2 + RCC_DEV0_EPF0_STRAP0) & RCC_DEV0_EPF0_STRAP0_ATI_REV_ID_MASK) >>
                        RCC_DEV0_EPF0_STRAP0_ATI_REV_ID_SHIFT;

    if (this->attributes.isRaven()) {
        if (this->devRevision >= 0x8) {
            this->attributes.setRaven2();
            this->enumRevision = 0x79;
        } else if (this->attributes.isPicasso()) {
            this->enumRevision = 0x41;
        } else if (this->devRevision == 1) {
            this->enumRevision = 0x20;
        } else {
            this->enumRevision = 0x1;
        }
    } else if (this->attributes.isRenoir() && !this->attributes.isGreenSardine() && this->devRevision == 0 &&
               this->pciRevision >= 0x80 && this->pciRevision <= 0x84) {
        this->attributes.setRenoirE();
    }

    DBGLOG("NRed", "deviceID = 0x%X", this->deviceID);
    DBGLOG("NRed", "pciRevision = 0x%X", this->pciRevision);
    DBGLOG("NRed", "fbOffset = 0x%llX", this->fbOffset);
    DBGLOG("NRed", "devRevision = 0x%X", this->devRevision);
    DBGLOG("NRed", "isRaven = %s", this->attributes.isRaven() ? "yes" : "no");
    DBGLOG("NRed", "isPicasso = %s", this->attributes.isPicasso() ? "yes" : "no");
    DBGLOG("NRed", "isRaven2 = %s", this->attributes.isRaven2() ? "yes" : "no");
    DBGLOG("NRed", "isRenoir = %s", this->attributes.isRenoir() ? "yes" : "no");
    DBGLOG("NRed", "isGreenSardine = %s", this->attributes.isGreenSardine() ? "yes" : "no");
    DBGLOG("NRed", "enumRevision = 0x%X", this->enumRevision);
    DBGLOG("NRed", "If any of the above values look incorrect, please report this to the developers.");
}

static void updatePropertiesForDevice(IOPCIDevice *device) {
    UInt8 builtIn[] = {0x00};
    device->setProperty("built-in", builtIn, arrsize(builtIn));
    auto vendorId = WIOKit::readPCIConfigValue(device, WIOKit::kIOPCIConfigVendorID);
    auto deviceId = WIOKit::readPCIConfigValue(device, WIOKit::kIOPCIConfigDeviceID);
    SYSLOG_COND(device->getProperty("model") != nullptr, "NRed",
        "[%X:%X] WARNING!!! Overriding the model is no longer supported!", vendorId, deviceId);

    auto *model = getBrandingNameForDev(device);
    if (model == nullptr) {
        SYSLOG("NRed", "[%X:%X] Warning: No model found.", vendorId, deviceId);
        return;
    }
    auto modelLen = static_cast<UInt32>(strlen(model) + 1);
    device->setProperty("model", const_cast<char *>(model), modelLen);
    // Device name is everything after AMD Radeon RX/Pro.
    if (model[11] == 'P' && model[12] == 'r' && model[13] == 'o' && model[14] == ' ') {
        device->setProperty("ATY,FamilyName", const_cast<char *>("Radeon Pro"), 11);
        device->setProperty("ATY,DeviceName", const_cast<char *>(model) + 15, modelLen - 15);
    } else {
        device->setProperty("ATY,FamilyName", const_cast<char *>("Radeon RX"), 10);
        device->setProperty("ATY,DeviceName", const_cast<char *>(model) + 14, modelLen - 14);
    }
    device->setProperty("AAPL,slot-name", const_cast<char *>("built-in"), 9);
}

void NRed::processPatcher(KernelPatcher &patcher) {
    auto *devInfo = DeviceInfo::create();
    PANIC_COND(devInfo == nullptr, "NRed", "Failed to create device info!");

    devInfo->processSwitchOff();

    this->iGPU = OSDynamicCast(IOPCIDevice, devInfo->videoBuiltin);
    PANIC_COND(this->iGPU == nullptr, "NRed", "videoBuiltin is not IOPCIDevice");
    PANIC_COND(WIOKit::readPCIConfigValue(this->iGPU, WIOKit::kIOPCIConfigVendorID) != WIOKit::VendorID::ATIAMD, "NRed",
        "videoBuiltin is not AMD");

    WIOKit::renameDevice(this->iGPU, "IGPU");
    WIOKit::awaitPublishing(this->iGPU);
    updatePropertiesForDevice(this->iGPU);

    this->deviceID = WIOKit::readPCIConfigValue(this->iGPU, WIOKit::kIOPCIConfigDeviceID);
    switch (this->deviceID) {
        case 0x15D8:
            this->attributes.setRaven();
            this->attributes.setPicasso();
            break;
        case 0x15DD:
            this->attributes.setRaven();
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
    this->pciRevision = WIOKit::readPCIConfigValue(this->iGPU, WIOKit::kIOPCIConfigRevisionID);

    char name[128];
    bzero(name, sizeof(name));
    for (size_t i = 0, ii = 0; i < devInfo->videoExternal.size(); i++) {
        auto *device = OSDynamicCast(IOPCIDevice, devInfo->videoExternal[i].video);
        if (device == nullptr) { continue; }

        snprintf(name, arrsize(name), "GFX%zu", ii++);
        WIOKit::renameDevice(device, name);
        WIOKit::awaitPublishing(device);
        updatePropertiesForDevice(device);
    }

    DeviceInfo::deleter(devInfo);

    KernelPatcher::RouteRequest requests[] = {
        {"__ZN15OSMetaClassBase12safeMetaCastEPKS_PK11OSMetaClass", wrapSafeMetaCast, this->orgSafeMetaCast},
        {"__ZN11IOCatalogue10addDriversEP7OSArrayb", wrapAddDrivers, this->orgAddDrivers},
    };
    PANIC_COND(!patcher.routeMultipleLong(KernelPatcher::KernelID, requests), "NRed", "Failed to route kernel symbols");
}

void NRed::setProp32(const char *key, UInt32 value) { this->iGPU->setProperty(key, value, 32); }

UInt32 NRed::readReg32(UInt32 reg) const {
    if ((reg * sizeof(UInt32)) < this->rmmio->getLength()) {
        return this->rmmioPtr[reg];
    } else {
        this->rmmioPtr[PCIE_INDEX2] = reg;
        return this->rmmioPtr[PCIE_DATA2];
    }
}

void NRed::writeReg32(UInt32 reg, UInt32 val) const {
    if ((reg * sizeof(UInt32)) < this->rmmio->getLength()) {
        this->rmmioPtr[reg] = val;
    } else {
        this->rmmioPtr[PCIE_INDEX2] = reg;
        this->rmmioPtr[PCIE_DATA2] = val;
    }
}

UInt32 NRed::smuWaitForResponse() const {
    UInt32 ret = AMDSMUFWResponse::kSMUFWResponseNoResponse;
    for (UInt32 i = 0; i < AMD_MAX_USEC_TIMEOUT; i++) {
        ret = this->readReg32(MP0_BASE_0 + MP1_SMN_C2PMSG_90);
        if (ret != AMDSMUFWResponse::kSMUFWResponseNoResponse) { break; }

        IOSleep(1);
    }

    return ret;
}

CAILResult NRed::sendMsgToSmc(UInt32 msg, UInt32 param, UInt32 *outParam) const {
    this->smuWaitForResponse();

    this->writeReg32(MP0_BASE_0 + MP1_SMN_C2PMSG_82, param);
    this->writeReg32(MP0_BASE_0 + MP1_SMN_C2PMSG_90, 0);
    this->writeReg32(MP0_BASE_0 + MP1_SMN_C2PMSG_66, msg);

    const auto resp = this->smuWaitForResponse();

    if (outParam != nullptr) { *outParam = this->readReg32(MP0_BASE_0 + MP1_SMN_C2PMSG_82); }

    return processSMUFWResponse(resp);
}

static bool checkAtomBios(const UInt8 *bios, size_t size) {
    if (size < 0x49) {
        DBGLOG("NRed", "VBIOS size is invalid");
        return false;
    }

    if (bios[0] != 0x55 || bios[1] != 0xAA) {
        DBGLOG("NRed", "VBIOS signature <%x %x> is invalid", bios[0], bios[1]);
        return false;
    }

    UInt16 bios_header_start = bios[0x48] | static_cast<UInt16>(bios[0x49] << 8);
    if (!bios_header_start) {
        DBGLOG("NRed", "Unable to locate VBIOS header");
        return false;
    }

    UInt16 tmp = bios_header_start + 4;
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

bool NRed::getVBIOSFromVFCT(bool strict) {
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

    if (sizeof(VFCT) > vfctData->getLength()) {
        DBGLOG("NRed", "VFCT table present but broken (too short).");
        return false;
    }

    auto vendor = WIOKit::readPCIConfigValue(this->iGPU, WIOKit::kIOPCIConfigVendorID);
    auto busNum = this->iGPU->getBusNumber();
    auto devNum = this->iGPU->getDeviceNumber();
    auto devFunc = this->iGPU->getFunctionNumber();

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

        if (vHdr->imageLength != 0 &&
            (!strict || (vHdr->pciBus == busNum && vHdr->pciDevice == devNum && vHdr->pciFunction == devFunc)) &&
            vHdr->vendorID == vendor && vHdr->deviceID == this->deviceID) {
            if (checkAtomBios(vContent, vHdr->imageLength)) {
                this->vbiosData = OSData::withBytes(vContent, vHdr->imageLength);
                PANIC_COND(this->vbiosData == nullptr, "NRed", "VFCT OSData::withBytes failed");
                return true;
            }

            DBGLOG("NRed", "VFCT VBIOS is not an ATOMBIOS");
            return false;
        } else {
            DBGLOG("NRed",
                "VFCT image does not match (pciBus: 0x%X pciDevice: 0x%X pciFunction: 0x%X "
                "vendorID: 0x%X deviceID: 0x%X) or length is 0 (imageLength: 0x%X)",
                vHdr->pciBus, vHdr->pciDevice, vHdr->pciFunction, vHdr->vendorID, vHdr->deviceID, vHdr->imageLength);
        }
    }

    DBGLOG("NRed", "VFCT table present but broken.");
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
    OSSafeReleaseNULL(bar0);
    return true;
}

bool NRed::getVBIOSFromExpansionROM() {
    auto expansionROMBase = this->iGPU->extendedConfigRead32(kIOPCIConfigExpansionROMBase);
    if (expansionROMBase == 0) {
        DBGLOG("NRed", "No PCI Expansion ROM available");
        return false;
    }

    auto *expansionROM =
        this->iGPU->mapDeviceMemoryWithRegister(kIOPCIConfigExpansionROMBase, kIOMapInhibitCache | kIOMapAnywhere);
    if (expansionROM == nullptr) { return false; }
    auto expansionROMLength = min(expansionROM->getLength(), ATOMBIOS_IMAGE_SIZE);
    if (expansionROMLength == 0) {
        DBGLOG("NRed", "PCI Expansion ROM is empty");
        expansionROM->release();
        return false;
    }

    // Enable reading the expansion ROMs
    this->iGPU->extendedConfigWrite32(kIOPCIConfigExpansionROMBase, expansionROMBase | 1);

    this->vbiosData = OSData::withBytes(reinterpret_cast<const void *>(expansionROM->getVirtualAddress()),
        static_cast<UInt32>(expansionROMLength));
    PANIC_COND(this->vbiosData == nullptr, "NRed", "PCI Expansion ROM OSData::withBytes failed");
    expansionROM->release();

    // Disable reading the expansion ROMs
    this->iGPU->extendedConfigWrite32(kIOPCIConfigExpansionROMBase, expansionROMBase);

    if (checkAtomBios(static_cast<const UInt8 *>(this->vbiosData->getBytesNoCopy()), expansionROMLength)) {
        return true;
    } else {
        DBGLOG("NRed", "PCI Expansion ROM VBIOS is not an ATOMBIOS");
        OSSafeReleaseNULL(this->vbiosData);
        return false;
    }
}

bool NRed::getVBIOS() {
    auto *biosImageProp = OSDynamicCast(OSData, this->iGPU->getProperty("ATY,bin_image"));
    if (biosImageProp != nullptr) {
        if (checkAtomBios(static_cast<const UInt8 *>(biosImageProp->getBytesNoCopy()), biosImageProp->getLength())) {
            this->vbiosData = OSData::withData(biosImageProp);
            SYSLOG("NRed", "Warning: VBIOS manually overridden, make sure you know what you're doing.");
            return true;
        } else {
            SYSLOG("NRed", "Error: VBIOS override is invalid.");
        }
    }
    if (this->getVBIOSFromVFCT(true)) {
        DBGLOG("NRed", "Got VBIOS from VFCT.");
    } else {
        SYSLOG("NRed", "Failed to get VBIOS from VFCT, trying to get it from VRAM.");
        if (this->getVBIOSFromVRAM()) {
            DBGLOG("NRed", "Got VBIOS from VRAM.");
        } else {
            SYSLOG("NRed", "Failed to get VBIOS from VRAM, trying to get it from PCI Expansion ROM.");
            if (this->getVBIOSFromExpansionROM()) {
                DBGLOG("NRed", "Got VBIOS from PCI Expansion ROM.");
            } else {
                SYSLOG("NRed",
                    "Failed to get VBIOS from PCI Expansion ROM, trying to get it from VFCT (relaxed matches mode).");
                if (this->getVBIOSFromVFCT(false)) {
                    DBGLOG("NRed", "Got VBIOS from VFCT (relaxed matches mode).");
                } else {
                    SYSLOG("NRed", "Failed to get VBIOS from VFCT (relaxed matches mode).");
                    return false;
                }
            }
        }
    }
    return true;
}

static const char *getDriverXMLForBundle(const char *bundleIdentifier, size_t *len) {
    const auto identifierLen = strlen(bundleIdentifier);
    const auto totalLen = identifierLen + 5;
    auto *filename = new char[totalLen];
    memcpy(filename, bundleIdentifier, identifierLen);
    strlcat(filename, ".xml", totalLen);

    const auto &driversXML = getFirmwareNamed(filename);
    delete[] filename;

    *len = driversXML.length;
    return reinterpret_cast<const char *>(driversXML.data);
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

    return FunctionCast(wrapAddDrivers, singleton().orgAddDrivers)(that, array, doNubMatching);
}

// TODO: Remove this unholy mess.
OSMetaClassBase *NRed::wrapSafeMetaCast(const OSMetaClassBase *anObject, const OSMetaClass *toMeta) {
    auto ret = FunctionCast(wrapSafeMetaCast, singleton().orgSafeMetaCast)(anObject, toMeta);

    if (LIKELY(ret)) { return ret; }

    for (const auto &ent : singleton().metaClassMap) {
        if (UNLIKELY(ent[0] == toMeta)) {
            return FunctionCast(wrapSafeMetaCast, singleton().orgSafeMetaCast)(anObject, ent[1]);
        } else if (UNLIKELY(ent[1] == toMeta)) {
            return FunctionCast(wrapSafeMetaCast, singleton().orgSafeMetaCast)(anObject, ent[0]);
        }
    }

    return nullptr;
}
