// Master plug-in logic
//
// Copyright © 2022-2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#include <Backlight.hpp>
#include <DebugEnabler.hpp>
#include <GPUDriversAMD/PowerPlay.hpp>
#include <GPUDriversAMD/RavenIPOffset.hpp>
#include <GPUDriversAMD/SMU.hpp>
#include <GPUDriversAMD/TTL/SWIP/SMU.hpp>
#include <Headers/kern_api.hpp>
#include <Headers/kern_devinfo.hpp>
#include <Hotfixes/AGDP.hpp>
#include <Hotfixes/X6000FB.hpp>
#include <IOKit/acpi/IOACPIPlatformExpert.h>
#include <Kexts.hpp>
#include <Model.hpp>
#include <NRed.hpp>
#include <PenguinWizardry/RuntimeMC.hpp>
#include <iVega/AppleGFXHDA.hpp>
#include <iVega/DriverInjector.hpp>
#include <iVega/HWLibs.hpp>
#include <iVega/Regs/GC.hpp>
#include <iVega/Regs/NBIO.hpp>
#include <iVega/Regs/SMU.hpp>
#include <iVega/X5000.hpp>
#include <iVega/X6000FB.hpp>

static NRed instance {};

NRed &NRed::singleton() { return instance; }

void NRed::init() {
    SYSLOG("NRed", "|-----------------------------------------------------------------|");
    SYSLOG("NRed", "| Copyright 2022-2025 ChefKiss.                                   |");
    SYSLOG("NRed", "| If you've paid for this, you've been scammed. Ask for a refund! |");
    SYSLOG("NRed", "| Do not support tonymacx86. Support us, we truly care.           |");
    SYSLOG("NRed", "| Change the world for the better.                                |");
    SYSLOG("NRed", "|-----------------------------------------------------------------|");

    Backlight::singleton().init();

    lilu.onKextLoadForce(&kextRadeonX6000Framebuffer);
    lilu.onKextLoadForce(&kextRadeonX5000HWLibs);
    lilu.onKextLoadForce(&kextRadeonX5000);
    lilu.onKextLoadForce(&kextAGDP);
    lilu.onKextLoadForce(&kextAppleGFXHDA);

    lilu.onPatcherLoadForce(
        [](void *const, KernelPatcher &patcher) {
            singleton().processPatcher();
            iVega::DriverInjector::singleton().processPatcher(patcher);
            PenguinWizardry::RuntimeMCManager::singleton().processPatcher(patcher);
        },
        nullptr);

    lilu.onKextLoadForce(
        nullptr, 0,
        [](void *const, KernelPatcher &patcher, const size_t id, const mach_vm_address_t slide, const size_t size) {
            Hotfixes::AGDP::singleton().processKext(patcher, id, slide, size);
            Hotfixes::X6000FB::singleton().processKext(patcher, id, slide, size);
            Backlight::singleton().processKext(patcher, id, slide, size);
            DebugEnabler::singleton().processKext(patcher, id, slide, size);
            iVega::X6000FB::singleton().processKext(patcher, id, slide, size);
            iVega::AppleGFXHDA::singleton().processKext(patcher, id, slide, size);
            iVega::X5000HWLibs::singleton().processKext(patcher, id, slide, size);
            iVega::X5000::singleton().processKext(patcher, id, slide, size);
        },
        nullptr);
}

void NRed::hwLateInit() {
    if (this->rmmio != nullptr) { return; }

    this->iGPU->setMemoryEnable(true);
    this->iGPU->setBusMasterEnable(true);

    PANIC_COND(!this->getVBIOS(), "NRed", "Failed to get VBIOS!");

    this->vbiosData->appendByte(0, ATOMBIOS_IMAGE_SIZE - this->vbiosData->getLength());

    this->iGPU->setProperty("ATY,bin_image", this->vbiosData);

    this->rmmio =
        this->iGPU->mapDeviceMemoryWithRegister(kIOPCIConfigBaseAddress5, kIOMapInhibitCache | kIOMapAnywhere);
    PANIC_COND(this->rmmio == nullptr || this->rmmio->getLength() == 0, "NRed", "Failed to map RMMIO");
    this->rmmioPtr = reinterpret_cast<UInt32 *>(this->rmmio->getVirtualAddress());

    this->fbOffset = static_cast<UInt64>(this->readReg32(GC_BASE_0 + MC_VM_FB_OFFSET)) << 24;
    this->devRevision = (this->readReg32(NBIO_BASE_2 + RCC_DEV0_EPF0_STRAP0) & RCC_DEV0_EPF0_STRAP0_ATI_REV_ID_MASK) >>
                        RCC_DEV0_EPF0_STRAP0_ATI_REV_ID_SHIFT;

    if (this->attributes.isRenoir()) {
        if (!this->attributes.isGreenSardine() && this->devRevision == 0 && this->pciRevision >= 0x80 &&
            this->pciRevision <= 0x84) {
            this->attributes.setRenoirE();
        }
    } else {
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
    }

    DBGLOG("NRed", "deviceID = 0x%X", this->deviceID);
    DBGLOG("NRed", "pciRevision = 0x%X", this->pciRevision);
    DBGLOG("NRed", "fbOffset = 0x%llX", this->fbOffset);
    DBGLOG("NRed", "devRevision = 0x%X", this->devRevision);
    DBGLOG("NRed", "isPicasso = %s", this->attributes.isPicasso() ? "true" : "false");
    DBGLOG("NRed", "isRaven2 = %s", this->attributes.isRaven2() ? "true" : "false");
    DBGLOG("NRed", "isRenoir = %s", this->attributes.isRenoir() ? "true" : "false");
    DBGLOG("NRed", "isGreenSardine = %s", this->attributes.isGreenSardine() ? "true" : "false");
    DBGLOG("NRed", "enumRevision = 0x%X", this->enumRevision);
}

// TODO: Remove!
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

void NRed::processPatcher() {
    auto *devInfo = DeviceInfo::create();
    assert(devInfo != nullptr);

    devInfo->processSwitchOff();

    PANIC_COND(devInfo->videoBuiltin == nullptr, "NRed", "No iGPU detected by Lilu");
    this->iGPU = OSRequiredCast(IOPCIDevice, devInfo->videoBuiltin);
    PANIC_COND(WIOKit::readPCIConfigValue(this->iGPU, WIOKit::kIOPCIConfigVendorID) != WIOKit::VendorID::ATIAMD, "NRed",
        "iGPU is not an AMD one");

    WIOKit::renameDevice(this->iGPU, "IGPU");
    WIOKit::awaitPublishing(this->iGPU);
    updatePropertiesForDevice(this->iGPU);

    this->deviceID = WIOKit::readPCIConfigValue(this->iGPU, WIOKit::kIOPCIConfigDeviceID);
    switch (this->deviceID) {
        case 0x15D8: {
            this->attributes.setPicasso();
        } break;
        case 0x15DD: {
        } break;
        case 0x164C:
        case 0x1636: {
            this->attributes.setRenoir();
            this->enumRevision = 0x91;
        } break;
        case 0x15E7:
        case 0x1638: {
            this->attributes.setRenoir();
            this->attributes.setGreenSardine();
            this->enumRevision = 0xA1;
        } break;
        default:
            PANIC("NRed", "Unknown device ID: 0x%X", this->deviceID);
    }
    this->pciRevision = WIOKit::readPCIConfigValue(this->iGPU, WIOKit::kIOPCIConfigRevisionID);

    char name[128];
    for (size_t i = 0, ii = 0; i < devInfo->videoExternal.size(); i++) {
        auto *device = OSDynamicCast(IOPCIDevice, devInfo->videoExternal[i].video);
        if (device == nullptr) { continue; }

        snprintf(name, arrsize(name), "GFX%zu", ii++);
        WIOKit::renameDevice(device, name);
        WIOKit::awaitPublishing(device);
        updatePropertiesForDevice(device);
    }

    DeviceInfo::deleter(devInfo);
}

void NRed::setProp32(const char *const key, const UInt32 value) const { this->iGPU->setProperty(key, value, 32); }

UInt32 NRed::readReg32(const UInt32 reg) const {
    if ((reg * sizeof(UInt32)) < this->rmmio->getLength()) {
        return this->rmmioPtr[reg];
    } else {
        this->rmmioPtr[PCIE_INDEX2] = reg;
        return this->rmmioPtr[PCIE_DATA2];
    }
}

void NRed::writeReg32(const UInt32 reg, const UInt32 val) const {
    if ((reg * sizeof(UInt32)) < this->rmmio->getLength()) {
        this->rmmioPtr[reg] = val;
    } else {
        this->rmmioPtr[PCIE_INDEX2] = reg;
        this->rmmioPtr[PCIE_DATA2] = val;
    }
}

static UInt64 getCurTimeInNS() {
    UInt64 uptime, uptimeNS;
    clock_get_uptime(&uptime);
    absolutetime_to_nanoseconds(uptime, &uptimeNS);
    return uptimeNS;
}

CAILResult NRed::waitForFunc(void *handle, bool (*func)(void *handle), const UInt32 timeoutMS) {
    if (timeoutMS == 0) {
        while (!func(handle)) {}
        return kCAILResultOK;
    }

    const auto startTime = getCurTimeInNS();
    const UInt64 timeoutNS = timeoutMS * 1000000;
    do {
        if (func(handle)) { return kCAILResultOK; }
    } while (getCurTimeInNS() - startTime <= timeoutNS);

    return kCAILResultNoResponse;
}

static bool smuWaitForResponseFunc(void *handle) {
    const auto outResp = static_cast<UInt32 *>(handle);

    if (outResp != nullptr) { *outResp = kSMUFWResponseNoResponse; }

    const auto fwResp = NRed::singleton().readReg32(MP0_BASE_0 + MP1_SMN_C2PMSG_90);
    if (fwResp != kSMUFWResponseNoResponse) {
        if (outResp != nullptr) { *outResp = fwResp; }
        return true;
    }

    return false;
}

CAILResult NRed::smuWaitForResponse(UInt32 *outResp) const { return waitForFunc(outResp, smuWaitForResponseFunc); }

CAILResult NRed::sendMsgToSmc(const UInt32 msg, const UInt32 param, UInt32 *const outParam) const {
    if (this->smuWaitForResponse(nullptr) != kCAILResultOK) { return kCAILResultInvalidParameters; }

    this->writeReg32(MP0_BASE_0 + MP1_SMN_C2PMSG_82, param);
    this->writeReg32(MP0_BASE_0 + MP1_SMN_C2PMSG_90, 0);
    this->writeReg32(MP0_BASE_0 + MP1_SMN_C2PMSG_66, msg);

    UInt32 resp;
    const auto res = this->smuWaitForResponse(&resp);

    if (res == kCAILResultOK && outParam != nullptr) { *outParam = this->readReg32(MP0_BASE_0 + MP1_SMN_C2PMSG_82); }

    return processSMUFWResponse(resp);
}

static bool checkAtomBios(const UInt8 *const bios, const size_t size) {
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

bool NRed::getVBIOSFromVFCT(const bool strict) {
    DBGLOG("NRed", "Fetching VBIOS from VFCT table");
    auto *expert = reinterpret_cast<AppleACPIPlatformExpert *>(this->iGPU->getPlatform());
    assert(expert != nullptr);

    auto *vfctData = expert->getACPITableData("VFCT", 0);
    if (vfctData == nullptr) {
        DBGLOG("NRed", "No VFCT from AppleACPIPlatformExpert");
        return false;
    }

    auto *vfct = static_cast<const VFCT *>(vfctData->getBytesNoCopy());
    assert(vfct != nullptr);

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
                assert(this->vbiosData != nullptr);
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
    assert(this->vbiosData != nullptr);
    OSSafeReleaseNULL(bar0);
    return true;
}

bool NRed::getVBIOSFromExpansionROM() {
    const auto expansionROMBase = this->iGPU->extendedConfigRead32(kIOPCIConfigExpansionROMBase);
    if (expansionROMBase == 0) {
        DBGLOG("NRed", "No PCI Expansion ROM available");
        return false;
    }

    auto *expansionROM =
        this->iGPU->mapDeviceMemoryWithRegister(kIOPCIConfigExpansionROMBase, kIOMapInhibitCache | kIOMapAnywhere);
    if (expansionROM == nullptr) { return false; }
    const auto expansionROMLength = min(expansionROM->getLength(), ATOMBIOS_IMAGE_SIZE);
    if (expansionROMLength == 0) {
        DBGLOG("NRed", "PCI Expansion ROM is empty");
        expansionROM->release();
        return false;
    }

    // Enable reading the expansion ROMs
    this->iGPU->extendedConfigWrite32(kIOPCIConfigExpansionROMBase, expansionROMBase | 1);

    this->vbiosData = OSData::withBytes(reinterpret_cast<const void *>(expansionROM->getVirtualAddress()),
        static_cast<UInt32>(expansionROMLength));
    assert(this->vbiosData != nullptr);
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
    const auto *biosImageProp = OSDynamicCast(OSData, this->iGPU->getProperty("ATY,bin_image"));
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
