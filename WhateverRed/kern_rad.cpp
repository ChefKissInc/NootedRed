//
//  kern_rad.cpp
//  WhateverRed
//
//  Copyright © 2017 vit9696. All rights reserved.
//  Copyright © 2022 ChefKiss Inc. All rights reserved.
//

#include "kern_rad.hpp"
#include "kern_fw.hpp"
#include "kern_netdbg.hpp"
#include <Availability.h>
#include <Headers/kern_api.hpp>
#include <Headers/kern_devinfo.hpp>
#include <Headers/kern_file.hpp>
#include <Headers/kern_iokit.hpp>
#include <IOKit/IOService.h>
#include <IOKit/acpi/IOACPIPlatformExpert.h>

static const char *pathFramebuffer = "/System/Library/Extensions/AMDFramebuffer.kext/Contents/MacOS/"
                                     "AMDFramebuffer";
static const char *pathSupport = "/System/Library/Extensions/AMDSupport.kext/Contents/MacOS/AMDSupport";
static const char *pathRadeonX6000 = "/System/Library/Extensions/AMDRadeonX6000.kext/Contents/MacOS/"
                                     "AMDRadeonX6000";
static const char *pathRadeonX5000HWLibs = "/System/Library/Extensions/AMDRadeonX5000HWServices.kext/Contents/PlugIns/"
                                           "AMDRadeonX5000HWLibs.kext/Contents/MacOS/AMDRadeonX5000HWLibs";
static const char *pathRadeonX5000 = "/System/Library/Extensions/AMDRadeonX5000.kext/Contents/MacOS/"
                                     "AMDRadeonX5000";
static const char *pathRadeonX6000Framebuffer =
    "/System/Library/Extensions/AMDRadeonX6000Framebuffer.kext/Contents/MacOS/"
    "AMDRadeonX6000Framebuffer";
static const char *pathIOAcceleratorFamily2 = "/System/Library/Extensions/IOAcceleratorFamily2.kext/Contents/MacOS/IOAcceleratorFamily2";

static KernelPatcher::KextInfo kextRadeonFramebuffer {
    "com.apple.kext.AMDFramebuffer",
    &pathFramebuffer,
    1,
    {},
    {},
    KernelPatcher::KextInfo::Unloaded,
};
static KernelPatcher::KextInfo kextRadeonSupport {
    "com.apple.kext.AMDSupport",
    &pathSupport,
    1,
    {},
    {},
    KernelPatcher::KextInfo::Unloaded,
};
static KernelPatcher::KextInfo kextRadeonX5000HWLibs {
    "com.apple.kext.AMDRadeonX5000HWLibs",
    &pathRadeonX5000HWLibs,
    1,
    {},
    {},
    KernelPatcher::KextInfo::Unloaded,
};
static KernelPatcher::KextInfo kextRadeonX6000Framebuffer {
    "com.apple.kext.AMDRadeonX6000Framebuffer",
    &pathRadeonX6000Framebuffer,
    1,
    {},
    {},
    KernelPatcher::KextInfo::Unloaded,
};
static KernelPatcher::KextInfo kextRadeonX5000 {
    "com.apple.kext.AMDRadeonX5000",
    &pathRadeonX5000,
    1,
    {},
    {},
    KernelPatcher::KextInfo::Unloaded,
};
static KernelPatcher::KextInfo kextRadeonX6000 = {
    "com.apple.kext.AMDRadeonX6000",
    &pathRadeonX6000,
    1,
    {},
    {},
    KernelPatcher::KextInfo::Unloaded,
};
static KernelPatcher::KextInfo kextIOAcceleratorFamily2 = {
    "com.apple.iokit.IOAcceleratorFamily2",
    &pathIOAcceleratorFamily2,
    1,
    {},
    {},
    KernelPatcher::KextInfo::Unloaded,
};

RAD *RAD::callbackRAD;

void RAD::init() {
    callbackRAD = this;

    currentPropProvider.init();

    force24BppMode = checkKernelArgument("-rad24");

    if (force24BppMode) lilu.onKextLoadForce(&kextRadeonFramebuffer);

    dviSingleLink = checkKernelArgument("-raddvi");

    lilu.onKextLoadForce(&kextRadeonX6000Framebuffer);
    lilu.onKextLoadForce(&kextRadeonSupport);
    lilu.onKextLoadForce(&kextRadeonX5000HWLibs);
    lilu.onKextLoadForce(&kextRadeonX5000);
    lilu.onKextLoadForce(&kextRadeonX6000);
    lilu.onKextLoadForce(&kextIOAcceleratorFamily2);
}

void RAD::deinit() {
    if (this->vbiosData) { vbiosData->release(); }
}

void RAD::processKernel(KernelPatcher &patcher) {
    KernelPatcher::RouteRequest requests[] = {
        {"__ZN15IORegistryEntry11setPropertyEPKcPvj", wrapSetProperty, orgSetProperty},
        {"__ZNK15IORegistryEntry11getPropertyEPKc", wrapGetProperty, orgGetProperty},
    };
    if (!patcher.routeMultipleLong(KernelPatcher::KernelID, requests)) { panic("RAD: Failed to route kernel symbols"); }
}

#pragma pack(push, 1)
struct VFCT {
    char signature[4];
    uint32_t length;
    uint8_t revision, checksum;
    char oemId[6];
    char oemTableId[8];
    uint32_t oemRevision;
    char creatorId[4];
    uint32_t creatorRevision;
    char tableUUID[16];
    uint32_t vbiosImageOffset, lib1ImageOffset;
    uint32_t reserved[4];
};

struct GOPVideoBIOSHeader {
    uint32_t pciBus, pciDevice, pciFunction;
    uint16_t vendorID, deviceID;
    uint16_t ssvId, ssId;
    uint32_t revision, imageLength;
};
#pragma pack(pop)

// Hack
class AppleACPIPlatformExpert : IOACPIPlatformExpert {
    friend class RAD;
};

void RAD::wrapAmdTtlServicesConstructor(IOService *that, IOPCIDevice *provider) {
    WIOKit::renameDevice(provider, "GFX0");
    if (!provider->getProperty("built-in")) {
        DBGLOG("wred", "fixing built-in");
        uint8_t builtBytes[] = {0x00};
        provider->setProperty("built-in", builtBytes, sizeof(builtBytes));
    } else {
        DBGLOG("wred", "found existing built-in");
    }

    NETDBG::enabled = true;
    NETLOG("rad", "patching device type table");
    PANIC_COND(MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) != KERN_SUCCESS, "rad",
        "Failed to enable kernel writing");
    auto deviceId = provider->extendedConfigRead16(kIOPCIConfigDeviceID);
    *callbackRAD->orgDeviceTypeTable = deviceId;
    *(callbackRAD->orgDeviceTypeTable + 1) = 6;
    MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);
    if (provider->getProperty("ATY,bin_image")) {
        NETLOG("rad", "VBIOS overridden using device property");
    } else {
        NETLOG("rad", "Automagically getting VBIOS from VFCT table");
        auto *expert = reinterpret_cast<AppleACPIPlatformExpert *>(provider->getPlatform());
        PANIC_COND(!expert, "rad", "Failed to get AppleACPIPlatformExpert");

        auto *vfctData = expert->getACPITableData("VFCT", 0);
        PANIC_COND(!vfctData, "rad", "Failed to get VFCT from AppleACPIPlatformExpert");

        auto *vfct = static_cast<const VFCT *>(vfctData->getBytesNoCopy());
        PANIC_COND(!vfct, "rad", "VFCT OSData::getBytesNoCopy returned null");

        auto *vbiosContent = static_cast<const GOPVideoBIOSHeader *>(
            vfctData->getBytesNoCopy(vfct->vbiosImageOffset, sizeof(GOPVideoBIOSHeader)));
        PANIC_COND(!vfct->vbiosImageOffset || !vbiosContent, "rad", "No VBIOS contained in VFCT table");

        auto *vbiosPtr =
            vfctData->getBytesNoCopy(vfct->vbiosImageOffset + sizeof(GOPVideoBIOSHeader), vbiosContent->imageLength);
        PANIC_COND(!vbiosPtr, "rad", "Bad VFCT: Offset + Size not within buffer boundaries");

        callbackRAD->vbiosData = OSData::withBytes(vbiosPtr, vbiosContent->imageLength);
        PANIC_COND(!callbackRAD->vbiosData, "rad", "OSData::withBytes failed");
        provider->setProperty("ATY,bin_image", callbackRAD->vbiosData);
    }

    NETLOG("rad", "calling original AmdTtlServices constructor");
    FunctionCast(wrapAmdTtlServicesConstructor, callbackRAD->orgAmdTtlServicesConstructor)(that, provider);
}

uint64_t RAD::wrapSmuGetHwVersion(uint64_t param1, uint32_t param2) {
    NETLOG("rad", "_smu_get_hw_version: param1 = 0x%llX param2 = 0x%X", param1, param2);
    auto ret = FunctionCast(wrapSmuGetHwVersion, callbackRAD->orgSmuGetHwVersion)(param1, param2);
    NETLOG("rad", "_smu_get_hw_version returned 0x%llX", ret);
    switch (ret) {
        case 0x2:
            NETLOG("rad", "Spoofing SMU v10 to v9.0.1");
            return 0x1;
        case 0xB:
            [[fallthrough]];
        case 0xC:
            NETLOG("rad", "Spoofing SMU v11/v12 to v11");
            return 0x3;
        default:
            return ret;
    }
}

uint64_t RAD::wrapPspSwInit(uint32_t *param1, uint32_t *param2) {
    NETLOG("rad", "_psp_sw_init: param1 = %p param2 = %p", param1, param2);
    NETLOG("rad", "_psp_sw_init: param1: 0:0x%X 1:0x%X 2:0x%X 3:0x%X 4:0x%X 5:0x%X", param1[0], param1[1], param1[2],
        param1[3], param1[4], param1[5]);
    switch (param1[3]) {
        case 0xA:
            [[fallthrough]];
        case 0xB:
            [[fallthrough]];
        case 0xC:
            NETLOG("rad", "Spoofing PSP version v11.x.x/v12.x.x to v11");
            param1[3] = 0xB;
            param1[4] = 0x0;
            param1[5] = 0x0;
            break;
        default:
            break;
    }
    auto ret = FunctionCast(wrapPspSwInit, callbackRAD->orgPspSwInit)(param1, param2);
    NETLOG("rad", "_psp_sw_init returned 0x%llX", ret);
    return ret;
}

uint32_t RAD::wrapGcGetHwVersion(uint32_t *param1) {
    NETLOG("rad", "_gc_get_hw_version: param1 = %p", param1);
    auto ret = FunctionCast(wrapGcGetHwVersion, callbackRAD->orgGcGetHwVersion)(param1);
    NETLOG("rad", "_gc_get_hw_version returned 0x%X", ret);
    switch (ret & 0xFF0000) {
        case 0x090000:
            NETLOG("rad", "Spoofing GC version v9.x.x to v9.2.1");
            return 0x090201;
        default:
            return ret;
    }
}

void RAD::wrapPopulateFirmwareDirectory(void *that) {
    NETLOG("rad", "AMDRadeonX5000_AMDRadeonHWLibsX5000::populateFirmwareDirectory this = %p", that);
    FunctionCast(wrapPopulateFirmwareDirectory, callbackRAD->orgPopulateFirmwareDirectory)(that);
    callbackRAD->callbackFirmwareDirectory = getMember<void *>(that, 0xB8);
    auto *fwDesc = getFWDescByName("renoir_dmcub.bin");
    PANIC_COND(!fwDesc, "rad", "Somehow renoir_dmcub.bin is missing");
    NETLOG("rad", "renoir_dmcub.bin => atidmcub_0.dat");
    auto *fwBackdoor = callbackRAD->orgCreateFirmware(fwDesc->var, fwDesc->size, 0x200, "atidmcub_0.dat");
    NETLOG("rad", "inserting atidmcub_0.dat!");
    PANIC_COND(!callbackRAD->orgPutFirmware(callbackRAD->callbackFirmwareDirectory, 6, fwBackdoor), "rad",
        "Failed to inject atidmcub_0.dat firmware");
    NETLOG("rad", "AMDRadeonX5000_AMDRadeonHWLibsX5000::populateFirmwareDirectory finished");
}

IOReturn RAD::wrapPopulateDeviceMemory(void *that, uint32_t reg) {
    DBGLOG("rad", "populateDeviceMemory: this = %p reg = 0x%X", that, reg);
    auto ret = FunctionCast(wrapPopulateDeviceMemory, callbackRAD->orgPopulateDeviceMemory)(that, reg);
    DBGLOG("rad", "populateDeviceMemory returned 0x%X", ret);
    return kIOReturnSuccess;
}

void *RAD::wrapCreatePowerTuneServices(void *param1, void *param2) {
    NETLOG("rad", "createPowerTuneServices: param1 = %p param2 = %p", param1, param2);
    auto *ret = IOMallocZero(0x18);
    callbackRAD->orgVega10PowerTuneConstructor(ret, param1, param2);
    return ret;
}

uint16_t RAD::wrapGetFamilyId() {
    /**
     * This function is hardcoded to return 0x8f which is Navi
     * So we now hard code it to return 0x8e, which is Raven/Renoir
     */
    return 0x8E;
}

uint16_t RAD::wrapGetEnumeratedRevision(void *that) {
    /**
     * `AMDRadeonX6000_AmdAsicInfoNavi10::getEnumeratedRevisionNumber`
     * Emulated Revision = Revision + Enumerated Revision.
     */
    auto *&pciDev = getMember<IOPCIDevice *>(that, 0x18);
    auto &revision = getMember<uint32_t>(that, 0x68);
    switch (pciDev->configRead16(kIOPCIConfigDeviceID)) {
        case 0x15D8:
            if (revision >= 0x8) {
                callbackRAD->asicType = ASICType::Raven2;
                return 0x79;
            }
            callbackRAD->asicType = ASICType::Picasso;
            return 0x41;
        case 0x15DD:
            if (revision >= 0x8) {
                callbackRAD->asicType = ASICType::Raven2;
                return 0x79;
            }
            callbackRAD->asicType = ASICType::Raven;
            return 0x10;
        case 0x15E7:
            [[fallthrough]];
        case 0x164C:
            [[fallthrough]];
        case 0x1636:
            [[fallthrough]];
        case 0x1638:
            callbackRAD->asicType = ASICType::Renoir;
            return 0x91;
        default:
            if (revision == 1) { return 0x20; }
            return 0x10;
    }
}

static bool injectedIPFirmware = false;

IOReturn RAD::wrapPopulateDeviceInfo(void *that) {
    NETLOG("rad", "AMDRadeonX6000_AmdAsicInfoNavi::populateDeviceInfo: this = %p", that);
    auto ret = FunctionCast(wrapPopulateDeviceInfo, callbackRAD->orgPopulateDeviceInfo)(that);
    auto &familyId = getMember<uint32_t>(that, 0x60);
    auto deviceId = getMember<IOPCIDevice *>(that, 0x18)->configRead16(kIOPCIConfigDeviceID);
    auto &revision = getMember<uint32_t>(that, 0x68);
    auto &emulatedRevision = getMember<uint32_t>(that, 0x6c);
    NETLOG("rad",
        "deviceId = 0x%X revision = 0x%X "
        "emulatedRevision = 0x%X",
        deviceId, revision, emulatedRevision);
    familyId = 0x8e;
    NETLOG("rad", "locating Init Caps entry");
    PANIC_COND(MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) != KERN_SUCCESS, "rad",
        "Failed to enable kernel writing");

    if (!injectedIPFirmware) {
        injectedIPFirmware = true;
        auto *asicName = getASICName();
        auto *filename = new char[128];
        snprintf(filename, 128, "%s_vcn.bin", asicName);
        auto *targetFilename = callbackRAD->asicType == ASICType::Renoir ? "ativvaxy_nv.dat" : "ativvaxy_rv.dat";
        NETLOG("rad", "%s => %s", filename, targetFilename);

        auto *fwDesc = getFWDescByName(filename);
        PANIC_COND(!fwDesc, "rad", "Somehow %s is missing", filename);

        auto *fw = callbackRAD->orgCreateFirmware(fwDesc->var, fwDesc->size, 0x200, targetFilename);
        NETLOG("rad", "fwDir = %p", callbackRAD->callbackFirmwareDirectory);
        NETLOG("rad", "inserting %s!", targetFilename);
        PANIC_COND(!callbackRAD->orgPutFirmware(callbackRAD->callbackFirmwareDirectory, 6, fw), "rad",
            "Failed to inject ativvaxy_rv.dat firmware");

        snprintf(filename, 128, "%s_rlc.bin", asicName);
        fwDesc = getFWDescByName(filename);
        PANIC_COND(!fwDesc, "rad", "Somehow %s is missing", filename);
        PANIC_COND(fwDesc->size != callbackRAD->orgGcRlcUcode->size, "rad", "%s size mismatch", filename);
        callbackRAD->orgGcRlcUcode->addr = 0x0;
        memmove(callbackRAD->orgGcRlcUcode->data, fwDesc->var, fwDesc->size);
        NETLOG("rad", "Injected %s!", filename);

        snprintf(filename, 128, "%s_me.bin", asicName);
        fwDesc = getFWDescByName(filename);
        PANIC_COND(!fwDesc, "rad", "Somehow %s is missing", filename);
        PANIC_COND(fwDesc->size != callbackRAD->orgGcMeUcode->size, "rad", "%s size mismatch", filename);
        callbackRAD->orgGcMeUcode->addr = 0x1000;
        memmove(callbackRAD->orgGcMeUcode->data, fwDesc->var, fwDesc->size);
        NETLOG("rad", "Injected %s!", filename);

        snprintf(filename, 128, "%s_ce.bin", asicName);
        fwDesc = getFWDescByName(filename);
        PANIC_COND(!fwDesc, "rad", "Somehow %s is missing", filename);
        PANIC_COND(fwDesc->size != callbackRAD->orgGcCeUcode->size, "rad", "%s size mismatch", filename);
        callbackRAD->orgGcCeUcode->addr = 0x800;
        memmove(callbackRAD->orgGcCeUcode->data, fwDesc->var, fwDesc->size);
        NETLOG("rad", "Injected %s!", filename);

        snprintf(filename, 128, "%s_pfp.bin", asicName);
        fwDesc = getFWDescByName(filename);
        PANIC_COND(!fwDesc, "rad", "Somehow %s is missing", filename);
        PANIC_COND(fwDesc->size != callbackRAD->orgGcPfpUcode->size, "rad", "%s size mismatch", filename);
        callbackRAD->orgGcPfpUcode->addr = 0x1400;
        memmove(callbackRAD->orgGcPfpUcode->data, fwDesc->var, fwDesc->size);
        NETLOG("rad", "Injected %s!", filename);

        snprintf(filename, 128, "%s_mec.bin", asicName);
        fwDesc = getFWDescByName(filename);
        PANIC_COND(!fwDesc, "rad", "Somehow %s is missing", filename);
        PANIC_COND(fwDesc->size != callbackRAD->orgGcMecUcode->size, "rad", "%s size mismatch", filename);
        callbackRAD->orgGcMecUcode->addr = 0x0;
        memmove(callbackRAD->orgGcMecUcode->data, fwDesc->var, fwDesc->size);
        NETLOG("rad", "Injected %s!", filename);

        snprintf(filename, 128, "%s_mec_jt.bin", asicName);
        fwDesc = getFWDescByName(filename);
        PANIC_COND(!fwDesc, "rad", "Somehow %s is missing", filename);
        PANIC_COND(fwDesc->size != callbackRAD->orgGcMecJtUcode->size, "rad", "%s size mismatch", filename);
        callbackRAD->orgGcMecJtUcode->addr = 0x104A4;
        memmove(callbackRAD->orgGcMecJtUcode->data, fwDesc->var, fwDesc->size);
        NETLOG("rad", "Injected %s!", filename);

        snprintf(filename, 128, "%s_sdma.bin", asicName);
        fwDesc = getFWDescByName(filename);
        PANIC_COND(!fwDesc, "rad", "Somehow %s is missing", filename);
        PANIC_COND(fwDesc->size != callbackRAD->orgSdmaUcode->size, "rad", "%s size mismatch", filename);
        memmove(callbackRAD->orgSdmaUcode->data, fwDesc->var, fwDesc->size);
        NETLOG("rad", "Injected %s!", filename);
        delete[] filename;
    }

    CailInitAsicCapEntry *initCaps = nullptr;
    for (size_t i = 0; i < 789; i++) {
        auto *temp = callbackRAD->orgAsicInitCapsTable + i;
        if (temp->familyId == 0x8e && temp->deviceId == deviceId && temp->emulatedRev == emulatedRevision) {
            initCaps = temp;
            break;
        }
    }
    if (!initCaps) {
        NETLOG("rad", "Warning! Using Fallback Init Caps mechanism");
        for (size_t i = 0; i < 789; i++) {
            auto *temp = callbackRAD->orgAsicInitCapsTable + i;
            if (temp->familyId == 0x8e && temp->deviceId == deviceId &&
                (temp->emulatedRev >= wrapGetEnumeratedRevision(that) || temp->emulatedRev <= emulatedRevision)) {
                initCaps = temp;
                break;
            }
        }
        if (!initCaps) { panic("rad: Failed to find Init Caps entry for device ID 0x%X", deviceId); }
    }
    callbackRAD->orgAsicCapsTable->familyId = callbackRAD->orgAsicCapsTableHWLibs->familyId = 0x8e;
    callbackRAD->orgAsicCapsTable->deviceId = callbackRAD->orgAsicCapsTableHWLibs->deviceId = deviceId;
    callbackRAD->orgAsicCapsTable->revision = callbackRAD->orgAsicCapsTableHWLibs->revision = revision;
    callbackRAD->orgAsicCapsTable->pciRev = callbackRAD->orgAsicCapsTableHWLibs->pciRev = 0xFFFFFFFF;
    callbackRAD->orgAsicCapsTable->emulatedRev = callbackRAD->orgAsicCapsTableHWLibs->emulatedRev = emulatedRevision;
    callbackRAD->orgAsicCapsTable->caps = callbackRAD->orgAsicCapsTableHWLibs->caps = initCaps->caps;
    MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);
    NETLOG("rad", "AMDRadeonX6000_AmdAsicInfoNavi::populateDeviceInfo returned 0x%X", ret);
    return ret;
}

uint64_t RAD::wrapSmuGetFwConstants() {
    /**
     * According to Linux AMDGPU source code,
     * on APUs, the System BIOS is the one that loads the SMC Firmware, and
     * therefore, we must not load any firmware ourselves.
     * We fix that by making `_smu_get_fw_constants` a no-op.
     */
    return 0;
}

uint64_t RAD::wrapSmuInternalHwInit() {
    /**
     * This is `_smu_9_0_1_internal_hw_init`.
     * The original function waits for the firmware to be loaded,
     * which in the Linux AMDGPU code is `smu9_is_smc_ram_running`.
     * SMU 10 doesn't do this, therefore, we just return 0.
     */
    return 0;
}

void RAD::wrapCosDebugPrint(char *fmt, ...) {
    NETDBG::printf("AMD TTL COS: ");
    va_list args, netdbg_args;
    va_start(args, fmt);
    va_copy(netdbg_args, args);
    NETDBG::vprintf(fmt, netdbg_args);
    va_end(netdbg_args);
    FunctionCast(wrapCosDebugPrint, callbackRAD->orgCosDebugPrint)(fmt, args);
    va_end(args);
}

void RAD::wrapMCILDebugPrint(uint32_t level_max, char *fmt, uint64_t param3, uint64_t param4, uint64_t param5,
    uint level) {
    NETDBG::printf("_MCILDebugPrint PARAM1 = 0x%X: ", level_max);
    NETDBG::printf(fmt, param3, param4, param5, level);
    FunctionCast(wrapMCILDebugPrint, callbackRAD->orgMCILDebugPrint)(level_max, fmt, param3, param4, param5, level);
}

void RAD::wrapCosDebugPrintVaList(void *ttl, char *header, char *fmt, va_list args) {
    NETDBG::printf("AMD TTL COS: %s ", header);
    va_list netdbg_args;
    va_copy(netdbg_args, args);
    NETDBG::vprintf(fmt, netdbg_args);
    NETDBG::printf("\n");
    va_end(netdbg_args);
    FunctionCast(wrapCosDebugPrintVaList, callbackRAD->orgCosDebugPrintVaList)(ttl, header, fmt, args);
}

void RAD::wrapCosReleasePrintVaList(void *ttl, char *header, char *fmt, va_list args) {
    NETDBG::printf("AMD TTL COS: %s ", header);
    va_list netdbg_args;
    va_copy(netdbg_args, args);
    NETDBG::vprintf(fmt, netdbg_args);
    va_end(netdbg_args);
    FunctionCast(wrapCosReleasePrintVaList, callbackRAD->orgCosReleasePrintVaList)(ttl, header, fmt, args);
}

uint32_t RAD::wrapGetVideoMemoryType(void *that) {
    NETLOG("rad", "getVideoMemoryType: this = %p", that);
    auto ret = FunctionCast(wrapGetVideoMemoryType, callbackRAD->orgGetVideoMemoryType)(that);
    NETLOG("rad", "getVideoMemoryType returned 0x%X", ret);
    return ret != 0 ? ret : 4;
}

uint32_t RAD::wrapGetVideoMemoryBitWidth(void *that) {
    NETLOG("rad", "getVideoMemoryBitWidth: this = %p", that);
    auto ret = FunctionCast(wrapGetVideoMemoryBitWidth, callbackRAD->orgGetVideoMemoryBitWidth)(that);
    NETLOG("rad",
        "getVideoMemoryBitWidth "
        "returned 0x%X",
        ret);
    return ret != 0 ? ret : 64;
}

IOReturn RAD::wrapPopulateVramInfo(void *that, void *param1) {
    NETLOG("rad", "populateVramInfo: this = %p param1 = %p", that, param1);
    return kIOReturnSuccess;
}

bool RAD::wrapGFX10AcceleratorStart() {
    /**
     * The `Info.plist` contains a personality for the
     * AMD X6000 Accelerator, but we don't want it to do
     * anything. We have it there only so it force-loads
     * the kext and we can resolve the symbols we need.
     */
    return false;
}

bool RAD::wrapAllocateHWEngines(void *that) {
    NETLOG("rad", "allocateHWEngines: this = %p", that);
    auto *pm4 = callbackRAD->orgGFX9PM4EngineNew(0x1e8);
    callbackRAD->orgGFX9PM4EngineConstructor(pm4);
    getMember<void *>(that, 0x3b8) = pm4;

    auto *sdma0 = callbackRAD->orgGFX9SDMAEngineNew(0x128);
    callbackRAD->orgGFX9SDMAEngineConstructor(sdma0);
    getMember<void *>(that, 0x3c0) = sdma0;

    auto *vcn2 = callbackRAD->orgGFX10VCN2EngineNew(0x198);
    callbackRAD->orgGFX10VCN2EngineConstructor(vcn2);
    getMember<void *>(that, 0x3f8) = vcn2;

    NETLOG("rad", "allocateHWEngines: returning true");
    return true;
}

void RAD::wrapSetupAndInitializeHWCapabilities(void *that) {
    NETLOG("rad", "wrapSetupAndInitializeCapabilities: this = %p", that);
    FunctionCast(wrapSetupAndInitializeHWCapabilities, callbackRAD->orgSetupAndInitializeHWCapabilities)(that);
    FunctionCast(wrapSetupAndInitializeHWCapabilities, callbackRAD->orgGFX10SetupAndInitializeHWCapabilities)(that);
    getMember<uint64_t>(that, 0xC0) = 0;
    NETLOG("rad", "wrapSetupAndInitializeCapabilities: done");
}

void RAD::wrapDumpASICHangStateCold(uint64_t param1) {
    NETLOG("rad", "dumpASICHangStateCold: param1 = 0x%llX", param1);
    IOSleep(3600000);
    FunctionCast(wrapDumpASICHangStateCold, callbackRAD->orgDumpASICHangStateCold)(param1);
    NETLOG("rad", "dumpASICHangStateCold finished");
}

uint64_t RAD::wrapGFX9RTRingGetHead(void *that) {
    NETLOG("rad", "GFX9RTRingGetHead: this = %p", that);
    auto ret = FunctionCast(wrapGFX9RTRingGetHead, callbackRAD->orgGFX9RTRingGetHead)(that);
    NETLOG("rad", "GFX9RTRingGetHead returned 0x%llX", ret);
    NETLOG("rad", "RTRing->field_0x54 = 0x%X", getMember<uint32_t>(that, 0x54));
    NETLOG("rad", "RTRing->field_0x74 = 0x%X", getMember<uint32_t>(that, 0x74));
    NETLOG("rad", "RTRing->field_0x8c = 0x%X", getMember<uint32_t>(that, 0x8c));
    return ret;
}

uint64_t RAD::wrapGFX10RTRingGetHead(void *that) {
    NETLOG("rad", "GFX10RTRingGetHead: this = %p", that);
    auto ret = FunctionCast(wrapGFX10RTRingGetHead, callbackRAD->orgGFX10RTRingGetHead)(that);
    NETLOG("rad", "GFX10RTRingGetHead returned 0x%llX", ret);
    NETLOG("rad", "RTRing->field_0x54 = 0x%X", getMember<uint32_t>(that, 0x54));
    NETLOG("rad", "RTRing->field_0x74 = 0x%X", getMember<uint32_t>(that, 0x74));
    NETLOG("rad", "RTRing->field_0x8c = 0x%X", getMember<uint32_t>(that, 0x8c));
    return ret;
}

uint64_t RAD::wrapHwRegRead(void *that, uint64_t addr) {
    NETLOG("rad", "hwRegRead: this = %p addr = 0x%llX", that, addr);
    auto ret = FunctionCast(wrapHwRegRead, callbackRAD->orgHwRegRead)(that, addr);
    NETLOG("rad", "hwRegRead returned 0x%llX", ret);
    return ret;
}

uint64_t RAD::wrapHwRegWrite(void *that, uint64_t addr, uint64_t val) {
    NETLOG("rad", "hwRegWrite: this = %p addr = 0x%llX val = 0x%llX", that, addr, val);
    auto ret = FunctionCast(wrapHwRegWrite, callbackRAD->orgHwRegWrite)(that, addr, val);
    NETLOG("rad", "hwRegWrite returned 0x%llX", ret);
    return ret;
}

const char *RAD::getASICName() {
    switch (callbackRAD->asicType) {
        case ASICType::Picasso:
            return "picasso";
        case ASICType::Raven:
            return "raven";
        case ASICType::Raven2:
            return "raven2";
        case ASICType::Renoir:
            return "renoir";
        default:
            panic("rad: ASIC type is unknown; this should never happen");
    }
}

uint32_t RAD::wrapPspAsdLoad(void *pspData) {
    /**
     * Hack: Add custom param 4 and 5 (pointer to firmware and size)
     * aka RCX and R8 registers
     * Complementary to `_psp_asd_load` patch-set.
     */
    auto *filename = new char[128];
    snprintf(filename, 128, "%s_asd.bin", getASICName());
    NETLOG("rad", "injecting %s!", filename);
    auto *org =
        reinterpret_cast<uint32_t (*)(void *, uint64_t, uint64_t, const void *, size_t)>(callbackRAD->orgPspAsdLoad);
    auto *fwDesc = getFWDescByName(filename);
    PANIC_COND(!fwDesc, "rad", "Somehow %s is missing", filename);
    delete[] filename;
    auto ret = org(pspData, 0, 0, fwDesc->var, fwDesc->size);
    NETLOG("rad", "_psp_asd_load returned 0x%X", ret);
    return ret;
}

uint32_t RAD::wrapPspDtmLoad(void *pspData) {
    /**
     * Same idea as `_psp_asd_load`.
     */
    auto *filename = new char[128];
    snprintf(filename, 128, "%s_dtm.bin", getASICName());
    NETLOG("rad", "injecting %s!", filename);
    auto *org =
        reinterpret_cast<uint32_t (*)(void *, uint64_t, uint64_t, const void *, size_t)>(callbackRAD->orgPspDtmLoad);
    auto *fwDesc = getFWDescByName(filename);
    PANIC_COND(!fwDesc, "rad", "Somehow %s is missing", filename);
    delete[] filename;
    auto ret = org(pspData, 0, 0, fwDesc->var, fwDesc->size);
    NETLOG("rad", "_psp_dtm_load returned 0x%X", ret);
    return 0;
}

uint32_t RAD::wrapPspHdcpLoad(void *pspData) {
    /**
     * Same idea as `_psp_asd_load`.
     */
    auto *filename = new char[128];
    snprintf(filename, 128, "%s_hdcp.bin", getASICName());
    NETLOG("rad", "injecting %s!", filename);
    auto *org =
        reinterpret_cast<uint32_t (*)(void *, uint64_t, uint64_t, const void *, size_t)>(callbackRAD->orgPspHdcpLoad);
    auto *fwDesc = getFWDescByName(filename);
    PANIC_COND(!fwDesc, "rad", "Somehow %s is missing", filename);
    delete[] filename;
    auto ret = org(pspData, 0, 0, fwDesc->var, fwDesc->size);
    NETLOG("rad", "_psp_hdcp_load returned 0x%X", ret);
    return ret;
}

void RAD::wrapAccelDisplayPipeWriteDiagnosisReport(void *that) {
    NETLOG("rad", "AccelDisplayPipeWriteDiagnosisReport: this = %p", that);
    // FunctionCast(wrapAccelDisplayPipeWriteDiagnosisReport,
    // callbackRAD->orgAccelDisplayPipeWriteDiagnosisReport)(that);
    NETLOG("rad", "AccelDisplayPipeWriteDiagnosisReport finished");
}

uint64_t RAD::wrapSetMemoryAllocationsEnabled(void *that, uint64_t param1) {
    NETLOG("rad", "setMemoryAllocationsEnabled: this = %p param1 = 0x%llX", that, param1);
    auto ret = FunctionCast(wrapSetMemoryAllocationsEnabled, callbackRAD->orgSetMemoryAllocationsEnabled)(that, param1);
    NETLOG("rad", "setMemoryAllocationsEnabled returned 0x%llX", ret);
    NETLOG("rad", "field_0x20=%llX field_0x28=%llX field_0x30=%llX", getMember<uint64_t>(that, 0x20),
        getMember<uint64_t>(that, 0x28), getMember<uint64_t>(that, 0x30));
    return ret;
}

bool RAD::wrapPowerUpHW(void *that) {
    NETLOG("rad", "powerUpHW: this = %p", that);
    auto ret = FunctionCast(wrapPowerUpHW, callbackRAD->orgPowerUpHW)(that);
    NETLOG("rad", "powerUpHW returned %d", ret);
    return ret;
}

void RAD::wrapHWsetMemoryAllocationsEnabled(void *that, bool param1) {
    NETLOG("rad", "HWsetMemoryAllocationsEnabled: this = %p param1 = %d", that, param1);
    FunctionCast(wrapHWsetMemoryAllocationsEnabled, callbackRAD->orgHWsetMemoryAllocationsEnabled)(that, param1);
    NETLOG("rad", "HWsetMemoryAllocationsEnabled finished");
}

uint64_t RAD::wrapRTGetHWChannel(void *that, uint32_t param1, uint32_t param2, uint32_t param3) {
    NETLOG("rad", "RTGetHWChannel: this = %p param1 = 0x%X param2 = 0x%X param3 = 0x%X", that, param1, param2, param3);
    if (param1 == 2 && param2 == 0 && param3 == 0) {
        param2 = 2;
        NETLOG("rad", "RTGetHWChannel: calling with param2 = 2");
    }
    auto ret = FunctionCast(wrapRTGetHWChannel, callbackRAD->orgRTGetHWChannel)(that, param1, param2, param3);
    NETLOG("rad", "RTGetHWChannel returned 0x%llX", ret);
    return ret;
}

void RAD::wrapCosDebugAssert(void *param1, uint8_t *param2, uint8_t *param3, uint32_t param4, uint8_t *param5) {
    NETLOG("rad", "cosDebugAssert: param1 = %p param2 = %p param3 = %p param4 = 0x%X param5 = %p", param1, param2,
        param3, param4, param5);
    FunctionCast(wrapCosDebugAssert, callbackRAD->orgCosDebugAssert)(param1, param2, param3, param4, param5);
}

uint32_t RAD::wrapHwReadReg32(void *that, uint32_t reg) {
    NETLOG("rad", "hwReadReg32: this = %p param1 = 0x%X", that, reg);
    if (reg == 0xD31) {
        /**
         * NBIO 7.4 -> NBIO 7.0
         * reg = SOC15_OFFSET(NBIO_BASE, 0, mmRCC_DEV0_EPF0_STRAP0);
         */
        reg = 0xD2F;
        NETLOG("rad", "hwReadReg32: redirecting reg 0xD31 to 0x%X", reg);
    }
    auto ret = FunctionCast(wrapHwReadReg32, callbackRAD->orgHwReadReg32)(that, reg);
    NETLOG("rad", "hwReadReg32 returned 0x%X", ret);
    return ret;
}

void RAD::powerUpSDMA(void *smumData) {
    NETLOG("rad", "Sending PPSMC_MSG_PowerUpSdma (0xE) to the SMC");
    auto smcRet = callbackRAD->orgSendMsgToSmc(smumData, 0xE, 0);
    NETLOG("rad", "_Raven_SendMsgToSmcWithParameter returned 0x%X", smcRet);
}

uint32_t RAD::wrapSmuRavenInitialize(void *smumData, uint32_t param2) {
    NETLOG("rad", "_SmuRaven_Initialize: param1 = %p param2 = 0x%X", smumData, param2);
    auto ret = FunctionCast(wrapSmuRavenInitialize, callbackRAD->orgSmuRavenInitialize)(smumData, param2);
    NETLOG("rad", "_SmuRaven_Initialize returned 0x%X", ret);
    powerUpSDMA(smumData);
    return ret;
}

uint32_t RAD::wrapSmuRenoirInitialize(void *smumData, uint32_t param2) {
    NETLOG("rad", "_SmuRenoir_Initialize: param1 = %p param2 = 0x%X", smumData, param2);
    auto ret = FunctionCast(wrapSmuRenoirInitialize, callbackRAD->orgSmuRenoirInitialize)(smumData, param2);
    NETLOG("rad", "_SmuRenoir_Initialize returned 0x%X", ret);
    powerUpSDMA(smumData);
    return ret;
}

uint64_t RAD::wrapCmdBufferPoolgetGPUVirtualAddress(void *that, uint64_t param1) {
    NETLOG("rad", "CmdBufferPoolgetGPUVirtualAddress: this = %p param1 = 0x%llX", that, param1);
    auto ret = FunctionCast(wrapCmdBufferPoolgetGPUVirtualAddress,
        callbackRAD->orgCmdBufferPoolgetGPUVirtualAddress)(that, param1);
    NETLOG("rad", "CmdBufferPoolgetGPUVirtualAddress returned 0x%llX", ret);
    return ret;
}

uint64_t RAD::wrapMemoryMapGetGPUVirtualAddress(void* that) {
    NETLOG("rad", "memoryMapGetGPUVirtualAddress: this = %p", that);
    auto ret = FunctionCast(wrapMemoryMapGetGPUVirtualAddress, callbackRAD->orgMemoryMapGetGPUVirtualAddress)(that);
    NETLOG("rad", "memoryMapGetGPUVirtualAddress returned 0x%llX", ret);
    return ret;
}

void* RAD::wrapSysMemGetPhysicalSegment(void* that, uint64_t param1, uint64_t* param2) {
    NETLOG("rad", "sysMemGetPhysicalSegment: this = %p param1 = 0x%llX param2 = %p", that, param1, param2);
    auto ret = FunctionCast(wrapSysMemGetPhysicalSegment, callbackRAD->orgSysMemGetPhysicalSegment)(that, param1, param2);
    NETLOG("rad", "sysMemGetPhysicalSegment returned %p", ret);
    return ret;
}

uint64_t RAD::wrapVidMemGetPhysicalSegment(void* that, uint64_t param1, uint64_t* param2) {
    NETLOG("rad", "vidMemGetPhysicalSegment: this = %p param1 = 0x%llX param2 = %p", that, param1, param2);
    auto ret = FunctionCast(wrapVidMemGetPhysicalSegment, callbackRAD->orgVidMemGetPhysicalSegment)(that, param1, param2);
    NETLOG("rad", "vidMemGetPhysicalSegment returned 0x%llX", ret);
    return ret;
}

void* RAD::wrapRemoteMemGetPhysicalSegment(void* that, uint64_t* param2) {
    NETLOG("rad", "remoteMemGetPhysicalSegment: this = %p param2 = %p", that, param2);
    auto ret = FunctionCast(wrapRemoteMemGetPhysicalSegment, callbackRAD->orgRemoteMemGetPhysicalSegment)(that, param2);
    NETLOG("rad", "remoteMemGetPhysicalSegment returned %p", ret);
    return ret;
}

bool RAD::processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
    if (kextRadeonFramebuffer.loadIndex == index) {
        if (force24BppMode) process24BitOutput(patcher, kextRadeonFramebuffer, address, size);

        return true;
    } else if (kextRadeonSupport.loadIndex == index) {
        processConnectorOverrides(patcher, address, size);

        KernelPatcher::RouteRequest requests[] = {
            {"__ZN13ATIController8TestVRAME13PCI_REG_INDEXb", doNotTestVram},
            {"__ZN16AtiDeviceControl16notifyLinkChangeE31kAGDCRegisterLinkControlEvent_tmj", wrapNotifyLinkChange,
                orgNotifyLinkChange},
            {"__ZN13ATIController20populateDeviceMemoryE13PCI_REG_INDEX", wrapPopulateDeviceMemory,
                orgPopulateDeviceMemory},
        };
        if (!patcher.routeMultipleLong(index, requests, address, size)) {
            panic("RAD: Failed to route AMDSupport symbols");
        }

        /**
         * Neutralises VRAM Info Null Check
         */
        uint8_t find[] = {0x48, 0x89, 0x83, 0x18, 0x01, 0x00, 0x00, 0x31, 0xc0, 0x48, 0x85, 0xc9, 0x75, 0x3e, 0x48,
            0x8d, 0x3d, 0xa4, 0xe2, 0x01, 0x00};
        uint8_t repl[] = {0x48, 0x89, 0x83, 0x18, 0x01, 0x00, 0x00, 0x31, 0xc0, 0x48, 0x85, 0xc9, 0x74, 0x3e, 0x48,
            0x8d, 0x3d, 0xa4, 0xe2, 0x01, 0x00};
        KernelPatcher::LookupPatch patch {&kextRadeonSupport, find, repl, arrsize(find), 1};
        patcher.applyLookupPatch(&patch);
        patcher.clearError();

        return true;
    } else if (kextRadeonX5000HWLibs.loadIndex == index) {
        KernelPatcher::SolveRequest solveRequests[] = {
            {"__ZL15deviceTypeTable", orgDeviceTypeTable},
            {"__ZN11AMDFirmware14createFirmwareEPhjjPKc", orgCreateFirmware},
            {"__ZN20AMDFirmwareDirectory11putFirmwareE16_AMD_DEVICE_TYPEP11AMDFirmware", orgPutFirmware},
            {"__ZN31AtiAppleVega10PowerTuneServicesC1EP11PP_InstanceP18PowerPlayCallbacks",
                orgVega10PowerTuneConstructor},
            {"__ZL20CAIL_ASIC_CAPS_TABLE", orgAsicCapsTableHWLibs},
            {"_CAILAsicCapsInitTable", orgAsicInitCapsTable},
            {"_gc_9_4_rlc_ucode", orgGcRlcUcode},
            {"_gc_9_4_me_ucode", orgGcMeUcode},
            {"_gc_9_4_ce_ucode", orgGcCeUcode},
            {"_gc_9_4_pfp_ucode", orgGcPfpUcode},
            {"_gc_9_4_mec_ucode", orgGcMecUcode},
            {"_gc_9_4_mec_jt_ucode", orgGcMecJtUcode},
            {"_sdma_4_1_ucode", orgSdmaUcode},
            {"_Raven_SendMsgToSmcWithParameter", orgSendMsgToSmc},
        };
        if (!patcher.solveMultiple(index, solveRequests, address, size)) {
            panic("RAD: Failed to resolve AMDRadeonX5000HWLibs symbols");
        }

        KernelPatcher::RouteRequest requests[] = {
            {"__ZN14AmdTtlServicesC2EP11IOPCIDevice", wrapAmdTtlServicesConstructor, orgAmdTtlServicesConstructor},
            {"_smu_get_hw_version", wrapSmuGetHwVersion, orgSmuGetHwVersion},
            {"_psp_sw_init", wrapPspSwInit, orgPspSwInit},
            {"_gc_get_hw_version", wrapGcGetHwVersion, orgGcGetHwVersion},
            {"__ZN35AMDRadeonX5000_AMDRadeonHWLibsX500025populateFirmwareDirectoryEv", wrapPopulateFirmwareDirectory,
                orgPopulateFirmwareDirectory},
            {"__ZN25AtiApplePowerTuneServices23createPowerTuneServicesEP11PP_InstanceP18PowerPlayCallbacks",
                wrapCreatePowerTuneServices},
            {"_smu_get_fw_constants", wrapSmuGetFwConstants},
            {"_smu_9_0_1_internal_hw_init", wrapSmuInternalHwInit},
            {"_smu_11_0_internal_hw_init", wrapSmuInternalHwInit},
            {"__ZN14AmdTtlServices13cosDebugPrintEPKcz", wrapCosDebugPrint, orgCosDebugPrint},
            {"_MCILDebugPrint", wrapMCILDebugPrint, orgMCILDebugPrint},
            {"__ZN14AmdTtlServices19cosDebugPrintVaListEPvPKcS2_P13__va_list_tag", wrapCosDebugPrintVaList,
                orgCosDebugPrintVaList},
            {"__ZN14AmdTtlServices21cosReleasePrintVaListEPvPKcS2_P13__va_list_tag", wrapCosReleasePrintVaList,
                orgCosReleasePrintVaList},
            {"_psp_asd_load", wrapPspAsdLoad, orgPspAsdLoad},
            {"_psp_dtm_load", wrapPspDtmLoad, orgPspDtmLoad},
            {"_psp_hdcp_load", wrapPspHdcpLoad, orgPspHdcpLoad},
            {"__ZN14AmdTtlServices14cosDebugAssertEPvPKcS2_jS2_", wrapCosDebugAssert, orgCosDebugAssert},
            {"_SmuRaven_Initialize", wrapSmuRavenInitialize, orgSmuRavenInitialize},
            {"_SmuRenoir_Initialize", wrapSmuRenoirInitialize, orgSmuRenoirInitialize},
        };
        if (!patcher.routeMultipleLong(index, requests, address, size)) {
            panic("RAD: Failed to route AMDRadeonX5000HWLibs symbols");
        }

        uint8_t find_asic_reset[] = {0x55, 0x48, 0x89, 0xe5, 0x8b, 0x56, 0x04, 0xbe, 0x3b, 0x00, 0x00, 0x00, 0x5d, 0xe9,
            0x51, 0xfe, 0xff, 0xff};
        uint8_t repl_asic_reset[] = {0x55, 0x48, 0x89, 0xe5, 0x8b, 0x56, 0x04, 0xbe, 0x1e, 0x00, 0x00, 0x00, 0x5d, 0xe9,
            0x51, 0xfe, 0xff, 0xff};
        static_assert(arrsize(find_asic_reset) == arrsize(repl_asic_reset), "Find/replace patch size mismatch");

        KernelPatcher::LookupPatch patches[] = {
            /**
             * Patch for `_smu_9_0_1_full_asic_reset`
             * This function performs a full ASIC reset.
             * The patch corrects the sent message to `0x1E`;
             * the original code sends `0x3B`, which is wrong for SMU 10.
             */
            {&kextRadeonX5000HWLibs, find_asic_reset, repl_asic_reset, arrsize(find_asic_reset), 2},
        };
        for (auto &patch : patches) {
            patcher.applyLookupPatch(&patch);
            patcher.clearError();
        }

        return true;
    } else if (kextRadeonX6000Framebuffer.loadIndex == index) {
        KernelPatcher::SolveRequest solveRequests[] = {
            {"__ZL20CAIL_ASIC_CAPS_TABLE", orgAsicCapsTable},
        };
        if (!patcher.solveMultiple(index, solveRequests, address, size, true)) {
            panic("RAD: Failed to resolve AMDRadeonX6000Framebuffer symbols");
        }

        KernelPatcher::RouteRequest requests[] = {
            {"__ZNK34AMDRadeonX6000_AmdBiosParserHelper18getVideoMemoryTypeEv", wrapGetVideoMemoryType,
                orgGetVideoMemoryType},
            {"__ZNK34AMDRadeonX6000_AmdBiosParserHelper22getVideoMemoryBitWidthEv", wrapGetVideoMemoryBitWidth,
                orgGetVideoMemoryBitWidth},
            {"__ZNK15AmdAtomVramInfo16populateVramInfoER16AtomFirmwareInfo", wrapPopulateVramInfo},
            {"__ZNK26AMDRadeonX6000_AmdAsicInfo11getFamilyIdEv", wrapGetFamilyId},
            {"__ZN30AMDRadeonX6000_AmdAsicInfoNavi18populateDeviceInfoEv", wrapPopulateDeviceInfo,
                orgPopulateDeviceInfo},
            {"__ZNK32AMDRadeonX6000_AmdAsicInfoNavi1027getEnumeratedRevisionNumberEv", wrapGetEnumeratedRevision},
            {"__ZN32AMDRadeonX6000_AmdRegisterAccess11hwReadReg32Ej", wrapHwReadReg32, orgHwReadReg32},
        };

        if (!patcher.routeMultiple(index, requests, address, size, true)) {
            panic("RAD: Failed to route AMDRadeonX6000Framebuffer symbols");
        }

        uint8_t find_null_check1[] = {0x48, 0x89, 0x83, 0x90, 0x00, 0x00, 0x00, 0x48, 0x85, 0xc0, 0x0f, 0x84, 0x89,
            0x00, 0x00, 0x00, 0x48, 0x8b, 0x7b, 0x18};
        uint8_t repl_null_check1[] = {0x48, 0x89, 0x83, 0x90, 0x00, 0x00, 0x00, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
            0x90, 0x90, 0x90, 0x48, 0x8b, 0x7b, 0x18};
        static_assert(arrsize(find_null_check1) == arrsize(repl_null_check1), "Find/replace patch size mismatch");

        uint8_t find_null_check2[] = {0x48, 0x89, 0x83, 0x88, 0x00, 0x00, 0x00, 0x48, 0x85, 0xc0, 0x0f, 0x84, 0xa1,
            0x00, 0x00, 0x00, 0x48, 0x8b, 0x7b, 0x18};
        uint8_t repl_null_check2[] = {0x48, 0x89, 0x83, 0x88, 0x00, 0x00, 0x00, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
            0x90, 0x90, 0x90, 0x48, 0x8b, 0x7b, 0x18};
        static_assert(arrsize(find_null_check2) == arrsize(repl_null_check2), "Find/replace patch size mismatch");

        uint8_t find_null_check3[] = {0x48, 0x83, 0xbb, 0x90, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x84, 0x90, 0x00, 0x00,
            0x00, 0x49, 0x89, 0xf7, 0xba, 0x60, 0x00, 0x00, 0x00};
        uint8_t repl_null_check3[] = {0x48, 0x83, 0xbb, 0x90, 0x00, 0x00, 0x00, 0x00, 0x90, 0x90, 0x90, 0x90, 0x90,
            0x90, 0x49, 0x89, 0xf7, 0xba, 0x60, 0x00, 0x00, 0x00};
        static_assert(arrsize(find_null_check3) == arrsize(repl_null_check3), "Find/replace patch size mismatch");

        KernelPatcher::LookupPatch patches[] = {
            /**
             * Neutralise VRAM Info creation null check
             * to proceed with Controller Core Services initialisation.
             */
            {&kextRadeonX6000Framebuffer, find_null_check1, repl_null_check1, arrsize(find_null_check1), 2},
            /**
             * Neutralise PSP Firmware Info creation null check
             * to proceed with Controller Core Services initialisation.
             */
            {&kextRadeonX6000Framebuffer, find_null_check2, repl_null_check2, arrsize(find_null_check2), 2},
            /**
             * Neutralise VRAM Info null check inside
             * `AmdAtomFwServices::getFirmwareInfo`.
             */
            {&kextRadeonX6000Framebuffer, find_null_check3, repl_null_check3, arrsize(find_null_check3), 2},
        };
        for (auto &patch : patches) {
            patcher.applyLookupPatch(&patch);
            patcher.clearError();
        }

        return true;
    } else if (kextRadeonX5000.loadIndex == index) {
        KernelPatcher::SolveRequest solveRequests[] = {
            {"__ZN31AMDRadeonX5000_AMDGFX9PM4EnginenwEm", orgGFX9PM4EngineNew},
            {"__ZN31AMDRadeonX5000_AMDGFX9PM4EngineC1Ev", orgGFX9PM4EngineConstructor},
            {"__ZN32AMDRadeonX5000_AMDGFX9SDMAEnginenwEm", orgGFX9SDMAEngineNew},
            {"__ZN32AMDRadeonX5000_AMDGFX9SDMAEngineC1Ev", orgGFX9SDMAEngineConstructor},
            {"__ZZN37AMDRadeonX5000_AMDGraphicsAccelerator19createAccelChannelsEbE12channelTypes", orgChannelTypes},
        };
        if (!patcher.solveMultiple(index, solveRequests, address, size)) {
            panic("RAD: Failed to resolve AMDRadeonX5000 symbols");
        }

        /**
         * Patch the data so that it only starts SDMA0.
         */
        PANIC_COND(MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) != KERN_SUCCESS, "rad",
            "Failed to enable kernel writing");
        callbackRAD->orgChannelTypes[5] = 1;
        MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);

        KernelPatcher::RouteRequest requests[] = {
            {"__ZN32AMDRadeonX5000_AMDVega10Hardware17allocateHWEnginesEv", wrapAllocateHWEngines},
            {"__ZN32AMDRadeonX5000_AMDVega10Hardware32setupAndInitializeHWCapabilitiesEv",
                wrapSetupAndInitializeHWCapabilities, orgSetupAndInitializeHWCapabilities},
            {"__ZN26AMDRadeonX5000_AMDHardware17dumpASICHangStateEb.cold.1", wrapDumpASICHangStateCold,
                orgDumpASICHangStateCold},
            {"__ZN24AMDRadeonX5000_AMDRTRing7getHeadEv", wrapGFX9RTRingGetHead, orgGFX9RTRingGetHead},
            {"__ZN29AMDRadeonX5000_AMDHWRegisters4readEj", wrapHwRegRead, orgHwRegRead},
            {"__ZN29AMDRadeonX5000_AMDHWRegisters5writeEjj", wrapHwRegWrite, orgHwRegWrite},
            {"__ZN34AMDRadeonX5000_AMDAccelDisplayPipe20writeDiagnosisReportERPcRj",
                wrapAccelDisplayPipeWriteDiagnosisReport, orgAccelDisplayPipeWriteDiagnosisReport},
            {"__ZN23AMDRadeonX5000_AMDHWVMM27setMemoryAllocationsEnabledEb", wrapSetMemoryAllocationsEnabled,
                orgSetMemoryAllocationsEnabled},
            {"__ZN37AMDRadeonX5000_AMDGraphicsAccelerator9powerUpHWEv", wrapPowerUpHW, orgPowerUpHW},
            {"__ZN26AMDRadeonX5000_AMDHardware27setMemoryAllocationsEnabledEb", wrapHWsetMemoryAllocationsEnabled,
                orgHWsetMemoryAllocationsEnabled},
            {"__ZN28AMDRadeonX5000_AMDRTHardware12getHWChannelE18_eAMD_CHANNEL_TYPE11SS_PRIORITYj", wrapRTGetHWChannel,
                orgRTGetHWChannel},
            {"__ZN35AMDRadeonX5000_AMDCommandBufferPool20getGPUVirtualAddressEPj",
                wrapCmdBufferPoolgetGPUVirtualAddress, orgCmdBufferPoolgetGPUVirtualAddress},
        };
        if (!patcher.routeMultipleLong(index, requests, address, size)) {
            panic("RAD: Failed to route AMDRadeonX5000 symbols");
        }

        uint8_t find_startHWEngines[] = {0x49, 0x89, 0xfe, 0x31, 0xdb, 0x48, 0x83, 0xfb, 0x02, 0x74, 0x50};
        uint8_t repl_startHWEngines[] = {0x49, 0x89, 0xfe, 0x31, 0xdb, 0x48, 0x83, 0xfb, 0x01, 0x74, 0x50};
        static_assert(sizeof(find_startHWEngines) == sizeof(repl_startHWEngines), "Find/replace size mismatch");

        KernelPatcher::LookupPatch patches[] = {
            {&kextRadeonX5000, find_startHWEngines, repl_startHWEngines, arrsize(find_startHWEngines), 2},
        };
        for (auto &patch : patches) {
            patcher.applyLookupPatch(&patch);
            patcher.clearError();
        }

        return true;
    } else if (kextRadeonX6000.loadIndex == index) {
        KernelPatcher::SolveRequest solveRequests[] = {
            {"__ZN30AMDRadeonX6000_AMDVCN2HWEnginenwEm", orgGFX10VCN2EngineNew},
            {"__ZN30AMDRadeonX6000_AMDVCN2HWEngineC1Ev", orgGFX10VCN2EngineConstructor},
            {"__ZN32AMDRadeonX6000_AMDNavi10Hardware32setupAndInitializeHWCapabilitiesEv",
                orgGFX10SetupAndInitializeHWCapabilities},
        };
        if (!patcher.solveMultiple(index, solveRequests, address, size)) {
            panic("RAD: Failed to resolve AMDRadeonX6000 symbols");
        }

        KernelPatcher::RouteRequest requests[] = {
            {"__ZN37AMDRadeonX6000_AMDGraphicsAccelerator5startEP9IOService", wrapGFX10AcceleratorStart},
            {"__ZN24AMDRadeonX6000_AMDRTRing7getHeadEv", wrapGFX10RTRingGetHead, orgGFX10RTRingGetHead},
        };
        if (!patcher.routeMultipleLong(index, requests, address, size)) {
            panic("RAD: Failed to route AMDRadeonX6000 symbols");
        }

        uint8_t find_hwchannel_init1[] = {0x74, 0x54, 0x49, 0x8b, 0x7c, 0x24, 0x18, 0x48, 0x8b, 0x07, 0xff, 0x90, 0xb8,
            0x03, 0x00, 0x00};
        uint8_t repl_hwchannel_init1[] = {0x74, 0x54, 0x49, 0x8b, 0x7c, 0x24, 0x18, 0x48, 0x8b, 0x07, 0xff, 0x90, 0xc0,
            0x03, 0x00, 0x00};
        static_assert(sizeof(find_hwchannel_init1) == sizeof(repl_hwchannel_init1), "Find/replace size mismatch");

        uint8_t find_hwchannel_init2[] = {0xff, 0x90, 0xc0, 0x03, 0x00, 0x00, 0xa8, 0x01, 0x74, 0x12, 0x49, 0x8b, 0x04,
            0x24, 0x4c, 0x89, 0xe7, 0xbe, 0x01, 0x00, 0x00, 0x00, 0xff, 0x90, 0x18, 0x02, 0x00, 0x00, 0x49, 0x8b, 0x7c,
            0x24, 0x18, 0x48, 0x8b, 0x07, 0xff, 0x90, 0xc0, 0x03, 0x00, 0x00, 0xa8, 0x02, 0x74, 0x12, 0x49, 0x8b, 0x04,
            0x24, 0x4c, 0x89, 0xe7, 0xbe, 0x02, 0x00, 0x00, 0x00, 0xff, 0x90, 0x18, 0x02, 0x00, 0x00, 0x49, 0x8b, 0x7c,
            0x24, 0x18, 0x48, 0x8b, 0x07, 0xff, 0x90, 0xc0, 0x03, 0x00, 0x00, 0x0f, 0xba, 0xe0, 0x0b, 0x73, 0x12, 0x49,
            0x8b, 0x04, 0x24, 0x4c, 0x89, 0xe7, 0xbe, 0x08, 0x00, 0x00, 0x00, 0xff, 0x90, 0x18, 0x02, 0x00, 0x00, 0x49,
            0x8b, 0x7c, 0x24, 0x18, 0x48, 0x8b, 0x07, 0xff, 0x90, 0xc0, 0x03, 0x00, 0x00, 0x0f, 0xba, 0xe0, 0x0a, 0x73,
            0x12, 0x49, 0x8b, 0x04, 0x24, 0x4c, 0x89, 0xe7, 0xbe, 0x10, 0x00, 0x00, 0x00, 0xff, 0x90, 0x18, 0x02, 0x00,
            0x00, 0x49, 0x8b, 0x7c, 0x24, 0x18, 0x48, 0x8b, 0x07, 0xff, 0x90, 0xc0, 0x03, 0x00, 0x00};
        uint8_t repl_hwchannel_init2[] = {0xff, 0x90, 0xc8, 0x03, 0x00, 0x00, 0xa8, 0x01, 0x74, 0x12, 0x49, 0x8b, 0x04,
            0x24, 0x4c, 0x89, 0xe7, 0xbe, 0x01, 0x00, 0x00, 0x00, 0xff, 0x90, 0x18, 0x02, 0x00, 0x00, 0x49, 0x8b, 0x7c,
            0x24, 0x18, 0x48, 0x8b, 0x07, 0xff, 0x90, 0xc8, 0x03, 0x00, 0x00, 0xa8, 0x02, 0x74, 0x12, 0x49, 0x8b, 0x04,
            0x24, 0x4c, 0x89, 0xe7, 0xbe, 0x02, 0x00, 0x00, 0x00, 0xff, 0x90, 0x18, 0x02, 0x00, 0x00, 0x49, 0x8b, 0x7c,
            0x24, 0x18, 0x48, 0x8b, 0x07, 0xff, 0x90, 0xc8, 0x03, 0x00, 0x00, 0x0f, 0xba, 0xe0, 0x0b, 0x73, 0x12, 0x49,
            0x8b, 0x04, 0x24, 0x4c, 0x89, 0xe7, 0xbe, 0x08, 0x00, 0x00, 0x00, 0xff, 0x90, 0x18, 0x02, 0x00, 0x00, 0x49,
            0x8b, 0x7c, 0x24, 0x18, 0x48, 0x8b, 0x07, 0xff, 0x90, 0xc8, 0x03, 0x00, 0x00, 0x0f, 0xba, 0xe0, 0x0a, 0x73,
            0x12, 0x49, 0x8b, 0x04, 0x24, 0x4c, 0x89, 0xe7, 0xbe, 0x10, 0x00, 0x00, 0x00, 0xff, 0x90, 0x18, 0x02, 0x00,
            0x00, 0x49, 0x8b, 0x7c, 0x24, 0x18, 0x48, 0x8b, 0x07, 0xff, 0x90, 0xc8, 0x03, 0x00, 0x00};
        static_assert(sizeof(find_hwchannel_init2) == sizeof(repl_hwchannel_init2), "Find/replace size mismatch");

        uint8_t find_hwchannel_submitCommandBuffer[] = {0x48, 0x8b, 0x7b, 0x18, 0x48, 0x8b, 0x07, 0xff, 0x90, 0x30,
            0x02, 0x00, 0x00, 0x48, 0x8b, 0x43, 0x50};
        uint8_t repl_hwchannel_submitCommandBuffer[] = {0x48, 0x8b, 0x7b, 0x18, 0x48, 0x8b, 0x07, 0x90, 0x90, 0x90,
            0x90, 0x90, 0x90, 0x48, 0x8b, 0x43, 0x50};
        static_assert(sizeof(find_hwchannel_submitCommandBuffer) == sizeof(repl_hwchannel_submitCommandBuffer),
            "Find/replace size mismatch");

        uint8_t find_hwchannel_waitForHwStamp[] = {0x49, 0x8b, 0x7d, 0x18, 0x48, 0x8b, 0x07, 0xff, 0x90, 0xa0, 0x02,
            0x00, 0x00, 0x84, 0xc0, 0x74, 0x2e, 0x44, 0x39, 0xfb};
        uint8_t repl_hwchannel_waitForHwStamp[] = {0x49, 0x8b, 0x7d, 0x18, 0x48, 0x8b, 0x07, 0xff, 0x90, 0x98, 0x02,
            0x00, 0x00, 0x84, 0xc0, 0x74, 0x2e, 0x44, 0x39, 0xfb};
        static_assert(sizeof(find_hwchannel_waitForHwStamp) == sizeof(repl_hwchannel_waitForHwStamp),
            "Find/replace size mismatch");

        uint8_t find_hwchannel_reset[] = {0x48, 0x8b, 0x7b, 0x18, 0x48, 0x8b, 0x07, 0xff, 0x90, 0xb8, 0x03, 0x00, 0x00,
            0x49, 0x89, 0xc6, 0x48, 0x8b, 0x03};
        uint8_t repl_hwchannel_reset[] = {0x48, 0x8b, 0x7b, 0x18, 0x48, 0x8b, 0x07, 0xff, 0x90, 0xc0, 0x03, 0x00, 0x00,
            0x49, 0x89, 0xc6, 0x48, 0x8b, 0x03};
        static_assert(sizeof(find_hwchannel_reset) == sizeof(repl_hwchannel_reset), "Find/replace size mismatch");

        uint8_t find_hwchannel_timestampUpdated1[] = {0x74, 0x20, 0x48, 0x8b, 0x7b, 0x18, 0x48, 0x8b, 0x07, 0xff, 0x90,
            0xb8, 0x03, 0x00, 0x00, 0x48, 0x8b, 0xb3, 0xc8, 0x00, 0x00, 0x00};
        uint8_t repl_hwchannel_timestampUpdated1[] = {0x74, 0x20, 0x48, 0x8b, 0x7b, 0x18, 0x48, 0x8b, 0x07, 0xff, 0x90,
            0xc0, 0x03, 0x00, 0x00, 0x48, 0x8b, 0xb3, 0xc8, 0x00, 0x00, 0x00};
        static_assert(sizeof(find_hwchannel_timestampUpdated1) == sizeof(repl_hwchannel_timestampUpdated1),
            "Find/replace size mismatch");

        uint8_t find_hwchannel_timestampUpdated2[] = {0x48, 0x8b, 0x7b, 0x18, 0x48, 0x8b, 0x07, 0xff, 0x90, 0xb8, 0x03,
            0x00, 0x00, 0x49, 0x8b, 0xb6, 0x50, 0x03, 0x00, 0x00, 0x48, 0x89, 0xc7};
        uint8_t repl_hwchannel_timestampUpdated2[] = {0x48, 0x8b, 0x7b, 0x18, 0x48, 0x8b, 0x07, 0xff, 0x90, 0xc0, 0x03,
            0x00, 0x00, 0x49, 0x8b, 0xb6, 0x50, 0x03, 0x00, 0x00, 0x48, 0x89, 0xc7};
        static_assert(sizeof(find_hwchannel_timestampUpdated2) == sizeof(repl_hwchannel_timestampUpdated2),
            "Find/replace size mismatch");

        uint8_t find_hwchannel_enableTimestampInterrupt[] = {0x85, 0xc0, 0x74, 0x14, 0x48, 0x8b, 0x7b, 0x18, 0x48, 0x8b,
            0x07, 0xff, 0x90, 0xa0, 0x02, 0x00, 0x00, 0x41, 0x89, 0xc6, 0x41, 0x80, 0xf6, 0x01};
        uint8_t repl_hwchannel_enableTimestampInterrupt[] = {0x85, 0xc0, 0x74, 0x14, 0x48, 0x8b, 0x7b, 0x18, 0x48, 0x8b,
            0x07, 0xff, 0x90, 0x98, 0x02, 0x00, 0x00, 0x41, 0x89, 0xc6, 0x41, 0x80, 0xf6, 0x01};
        static_assert(sizeof(find_hwchannel_enableTimestampInterrupt) ==
                          sizeof(repl_hwchannel_enableTimestampInterrupt),
            "Find/replace size mismatch");

        uint8_t find_hwchannel_writeDiagnosisReport[] = {0x49, 0x8b, 0x7c, 0x24, 0x18, 0x48, 0x8b, 0x07, 0xff, 0x90,
            0xb8, 0x03, 0x00, 0x00, 0x49, 0x8b, 0xb4, 0x24, 0xc8, 0x00, 0x00, 0x00, 0xb9, 0x01, 0x00, 0x00, 0x00};
        uint8_t repl_hwchannel_writeDiagnosisReport[] = {0x49, 0x8b, 0x7c, 0x24, 0x18, 0x48, 0x8b, 0x07, 0xff, 0x90,
            0xc0, 0x03, 0x00, 0x00, 0x49, 0x8b, 0xb4, 0x24, 0xc8, 0x00, 0x00, 0x00, 0xb9, 0x01, 0x00, 0x00, 0x00};
        static_assert(sizeof(find_hwchannel_writeDiagnosisReport) == sizeof(repl_hwchannel_writeDiagnosisReport),
            "Find/replace size mismatch");

        uint8_t find_setupAndInitializeHWCapabilities_pt1[] = {0x4c, 0x89, 0xf7, 0xff, 0x90, 0xa0, 0x02, 0x00, 0x00,
            0x84, 0xc0, 0x0f, 0x84, 0x6e, 0x02, 0x00, 0x00};
        uint8_t repl_setupAndInitializeHWCapabilities_pt1[] = {0x4c, 0x89, 0xf7, 0xff, 0x90, 0x98, 0x02, 0x00, 0x00,
            0x84, 0xc0, 0x0f, 0x84, 0x6e, 0x02, 0x00, 0x00};
        static_assert(sizeof(find_setupAndInitializeHWCapabilities_pt1) ==
                          sizeof(repl_setupAndInitializeHWCapabilities_pt1),
            "Find/replace size mismatch");

        uint8_t find_setupAndInitializeHWCapabilities_pt2[] = {0xff, 0x50, 0x70, 0x85, 0xc0, 0x74, 0x0a, 0x41, 0xc6,
            0x46, 0x28, 0x00, 0xe9, 0xb0, 0x01, 0x00, 0x00};
        uint8_t repl_setupAndInitializeHWCapabilities_pt2[] = {0x66, 0x90, 0x90, 0x85, 0xc0, 0x66, 0x90, 0x41, 0xc6,
            0x46, 0x28, 0x00, 0xe9, 0xb0, 0x01, 0x00, 0x00};
        static_assert(sizeof(find_setupAndInitializeHWCapabilities_pt2) ==
                          sizeof(repl_setupAndInitializeHWCapabilities_pt2),
            "Find/replace size mismatch");

        /**
         * HWEngine/HWChannel call HWInterface virtual methods.
         * The X5000 HWInterface virtual table offsets are
         * slightly different than the X6000 ones,
         * so we have to make patches to correct the offsets.
         */
        KernelPatcher::LookupPatch patches[] = {
            /**
             * Mismatched VTable Call to getScheduler.
             */
            {&kextRadeonX6000, find_hwchannel_init1, repl_hwchannel_init1, arrsize(find_hwchannel_init1), 1},
            /**
             * Mismatched VTable Calls to getGpuDebugPolicy.
             */
            {&kextRadeonX6000, find_hwchannel_init2, repl_hwchannel_init2, arrsize(find_hwchannel_init2), 1},
            /**
             * VTable Call to signalGPUWorkSubmitted.
             * Doesn't exist on X5000, but looks like it isn't necessary,
             * so we just NO-OP it.
             */
            {&kextRadeonX6000, find_hwchannel_submitCommandBuffer, repl_hwchannel_submitCommandBuffer,
                arrsize(find_hwchannel_submitCommandBuffer), 1},
            /**
             * Mismatched VTable Call to isDeviceValid.
             */
            {&kextRadeonX6000, find_hwchannel_waitForHwStamp, repl_hwchannel_waitForHwStamp,
                arrsize(find_hwchannel_waitForHwStamp), 1},
            /**
             * Mismatched VTable Call to getScheduler.
             */
            {&kextRadeonX6000, find_hwchannel_reset, repl_hwchannel_reset, arrsize(find_hwchannel_reset), 1},
            /**
             * Mismatched VTable Calls to getScheduler.
             */
            {&kextRadeonX6000, find_hwchannel_timestampUpdated1, repl_hwchannel_timestampUpdated1,
                arrsize(find_hwchannel_timestampUpdated1), 1},
            {&kextRadeonX6000, find_hwchannel_timestampUpdated2, repl_hwchannel_timestampUpdated2,
                arrsize(find_hwchannel_timestampUpdated2), 1},
            /**
             * Mismatched VTable Call to isDeviceValid.
             */
            {&kextRadeonX6000, find_hwchannel_enableTimestampInterrupt, repl_hwchannel_enableTimestampInterrupt,
                arrsize(find_hwchannel_enableTimestampInterrupt), 1},
            /**
             * Mismatched VTable Call to getScheduler.
             */
            {&kextRadeonX6000, find_hwchannel_writeDiagnosisReport, repl_hwchannel_writeDiagnosisReport,
                arrsize(find_hwchannel_writeDiagnosisReport), 1},
            /**
             * Mismatched VTable Call to isDeviceValid.
             */
            {&kextRadeonX6000, find_setupAndInitializeHWCapabilities_pt1, repl_setupAndInitializeHWCapabilities_pt1,
                arrsize(find_setupAndInitializeHWCapabilities_pt1), 1},
            /**
             * Remove call to TTL.
             */
            {&kextRadeonX6000, find_setupAndInitializeHWCapabilities_pt2, repl_setupAndInitializeHWCapabilities_pt2,
                arrsize(find_setupAndInitializeHWCapabilities_pt2), 1},
        };
        for (auto &patch : patches) {
            patcher.applyLookupPatch(&patch);
            patcher.clearError();
        }

        return true;
    } else if (kextIOAcceleratorFamily2.loadIndex == index) {
        KernelPatcher::RouteRequest requests[] = {
            {"__ZN16IOAccelMemoryMap20getGPUVirtualAddressEv", wrapMemoryMapGetGPUVirtualAddress, orgMemoryMapGetGPUVirtualAddress},
            {"__ZN16IOAccelSysMemory18getPhysicalSegmentEyPy", wrapSysMemGetPhysicalSegment, orgSysMemGetPhysicalSegment},
            {"__ZN16IOAccelVidMemory18getPhysicalSegmentEyPy", wrapVidMemGetPhysicalSegment, orgVidMemGetPhysicalSegment},
            {"__ZN19IOAccelRemoteMemory18getPhysicalSegmentEyPy", wrapRemoteMemGetPhysicalSegment, orgRemoteMemGetPhysicalSegment},
        };

        if (!patcher.routeMultipleLong(index, requests, address, size)) {
            panic("RAD: Failed to route IOAcceleratorFamily2 symbols");
        }
    }

    return false;
}

void RAD::process24BitOutput(KernelPatcher &patcher, KernelPatcher::KextInfo &info, mach_vm_address_t address,
    size_t size) {
    auto bitsPerComponent = patcher.solveSymbol<int *>(info.loadIndex, "__ZL18BITS_PER_COMPONENT", address, size);
    if (bitsPerComponent) {
        while (bitsPerComponent && *bitsPerComponent) {
            if (*bitsPerComponent == 10) {
                auto ret = MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock);
                if (ret == KERN_SUCCESS) {
                    DBGLOG("rad", "fixing BITS_PER_COMPONENT");
                    *bitsPerComponent = 8;
                    MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);
                } else {
                    NETLOG("rad", "failed to disable write protection for "
                                  "BITS_PER_COMPONENT");
                }
            }
            bitsPerComponent++;
        }
    } else {
        NETLOG("rad", "failed to find BITS_PER_COMPONENT");
        patcher.clearError();
    }

    DBGLOG("rad", "fixing pixel types");

    KernelPatcher::LookupPatch pixelPatch {&info, reinterpret_cast<const uint8_t *>("--RRRRRRRRRRGGGGGGGGGGBBBBBBBBBB"),
        reinterpret_cast<const uint8_t *>("--------RRRRRRRRGGGGGGGGBBBBBBBB"), 32, 2};

    patcher.applyLookupPatch(&pixelPatch);
    if (patcher.getError() != KernelPatcher::Error::NoError) {
        NETLOG("rad", "failed to patch RGB mask for 24-bit output");
        patcher.clearError();
    }
}

void RAD::processConnectorOverrides(KernelPatcher &patcher, mach_vm_address_t address, size_t size) {
    KernelPatcher::RouteRequest requests[] = {
        {"__ZN14AtiBiosParser116getConnectorInfoEP13ConnectorInfoRh", wrapGetConnectorsInfoV1, orgGetConnectorsInfoV1},
        {"__ZN14AtiBiosParser216getConnectorInfoEP13ConnectorInfoRh", wrapGetConnectorsInfoV2, orgGetConnectorsInfoV2},
        {"__"
         "ZN14AtiBiosParser126translateAtomConnectorInfoERN30AtiObjectInfoTable"
         "Interface_V117AtomConnectorInfoER13ConnectorInfo",
            wrapTranslateAtomConnectorInfoV1, orgTranslateAtomConnectorInfoV1},
        {"__"
         "ZN14AtiBiosParser226translateAtomConnectorInfoERN30AtiObjectInfoTable"
         "Interface_V217AtomConnectorInfoER13ConnectorInfo",
            wrapTranslateAtomConnectorInfoV2, orgTranslateAtomConnectorInfoV2},
        {"__ZN13ATIController5startEP9IOService", wrapATIControllerStart, orgATIControllerStart},
    };
    patcher.routeMultiple(kextRadeonSupport.loadIndex, requests, address, size);
}

void RAD::mergeProperty(OSDictionary *props, const char *name, OSObject *value) {
    // The only type we could make from device properties is data.
    // To be able to override other types we do a conversion here.
    auto data = OSDynamicCast(OSData, value);
    if (data) {
        // It is hard to make a boolean even from ACPI, so we make a hack here:
        // 1-byte OSData with 0x01 / 0x00 values becomes boolean.
        auto val = static_cast<const uint8_t *>(data->getBytesNoCopy());
        auto len = data->getLength();
        if (val && len == sizeof(uint8_t)) {
            if (val[0] == 1) {
                props->setObject(name, kOSBooleanTrue);
                DBGLOG("rad", "prop %s was merged as kOSBooleanTrue", name);
                return;
            } else if (val[0] == 0) {
                props->setObject(name, kOSBooleanFalse);
                DBGLOG("rad", "prop %s was merged as kOSBooleanFalse", name);
                return;
            }
        }

        // Consult the original value to make a decision
        auto orgValue = props->getObject(name);
        if (val && orgValue) {
            DBGLOG("rad", "prop %s has original value", name);
            if (len == sizeof(uint32_t) && OSDynamicCast(OSNumber, orgValue)) {
                auto num = *reinterpret_cast<const uint32_t *>(val);
                auto osnum = OSNumber::withNumber(num, 32);
                if (osnum) {
                    DBGLOG("rad", "prop %s was merged as number %u", name, num);
                    props->setObject(name, osnum);
                    osnum->release();
                }
                return;
            } else if (len > 0 && val[len - 1] == '\0' && OSDynamicCast(OSString, orgValue)) {
                auto str = reinterpret_cast<const char *>(val);
                auto osstr = OSString::withCString(str);
                if (osstr) {
                    DBGLOG("rad", "prop %s was merged as string %s", name, str);
                    props->setObject(name, osstr);
                    osstr->release();
                }
                return;
            }
        } else {
            DBGLOG("rad", "prop %s has no original value", name);
        }
    }

    // Default merge as is
    props->setObject(name, value);
    DBGLOG("rad", "prop %s was merged", name);
}

void RAD::mergeProperties(OSDictionary *props, const char *prefix, IOService *provider) {
    // Should be ok, but in case there are issues switch to
    // dictionaryWithProperties();
    auto dict = provider->getPropertyTable();
    if (dict) {
        auto iterator = OSCollectionIterator::withCollection(dict);
        if (iterator) {
            OSSymbol *propname;
            size_t prefixlen = strlen(prefix);
            while ((propname = OSDynamicCast(OSSymbol, iterator->getNextObject())) != nullptr) {
                auto name = propname->getCStringNoCopy();
                if (name && propname->getLength() > prefixlen && !strncmp(name, prefix, prefixlen)) {
                    auto prop = dict->getObject(propname);
                    if (prop) mergeProperty(props, name + prefixlen, prop);
                    else { DBGLOG("rad", "prop %s was not merged due to no value", name); }
                } else {
                    DBGLOG("rad", "prop %s does not match %s prefix", safeString(name), prefix);
                }
            }

            iterator->release();
        } else {
            NETLOG("rad", "prop merge failed to iterate over properties");
        }
    } else {
        NETLOG("rad", "prop merge failed to get properties");
    }
}

void RAD::applyPropertyFixes(IOService *service, uint32_t connectorNum) {
    if (!service->getProperty("CFG,CFG_FB_LIMIT")) {
        DBGLOG("rad", "setting fb limit to %u", connectorNum);
        service->setProperty("CFG_FB_LIMIT", connectorNum, 32);
    }
}

uint32_t RAD::wrapGetConnectorsInfoV1(void *that, RADConnectors::Connector *connectors, uint8_t *sz) {
    NETLOG("rad", "getConnectorsInfoV1: this = %p connectors = %p sz = %p", that, connectors, sz);
    uint32_t code = FunctionCast(wrapGetConnectorsInfoV1, callbackRAD->orgGetConnectorsInfoV1)(that, connectors, sz);
    NETLOG("rad", "getConnectorsInfoV1 returned 0x%X", code);
    auto props = callbackRAD->currentPropProvider.get();

    if (code == 0 && sz && props && *props) {
        callbackRAD->updateConnectorsInfo(nullptr, nullptr, *props, connectors, sz);
    } else
        NETLOG("rad", "getConnectorsInfoV1 failed %X or undefined %d", code, props == nullptr);

    return code;
}

void RAD::updateConnectorsInfo(void *atomutils, t_getAtomObjectTableForType gettable, IOService *ctrl,
    RADConnectors::Connector *connectors, uint8_t *sz) {
    if (atomutils) {
        NETLOG("rad", "getConnectorsInfo found %u connectors", *sz);
        RADConnectors::print(connectors, *sz);
    }

    auto cons = ctrl->getProperty("connectors");
    if (cons) {
        auto consData = OSDynamicCast(OSData, cons);
        if (consData) {
            auto consPtr = consData->getBytesNoCopy();
            auto consSize = consData->getLength();

            uint32_t consCount;
            if (WIOKit::getOSDataValue(ctrl, "connector-count", consCount)) {
                *sz = consCount;
                NETLOG("rad", "getConnectorsInfo got size override to %u", *sz);
            }

            if (consPtr && consSize > 0 && *sz > 0 && RADConnectors::valid(consSize, *sz)) {
                RADConnectors::copy(connectors, *sz, static_cast<const RADConnectors::Connector *>(consPtr), consSize);
                NETLOG("rad", "getConnectorsInfo installed %u connectors", *sz);
                applyPropertyFixes(ctrl, *sz);
            } else {
                NETLOG("rad",
                    "getConnectorsInfo conoverrides have invalid size %u "
                    "for %u num",
                    consSize, *sz);
            }
        } else {
            NETLOG("rad", "getConnectorsInfo conoverrides have invalid type");
        }
    } else {
        if (atomutils) {
            NETLOG("rad", "getConnectorsInfo attempting to autofix connectors");
            uint8_t sHeader = 0, displayPathNum = 0, connectorObjectNum = 0;
            auto baseAddr =
                static_cast<uint8_t *>(gettable(atomutils, AtomObjectTableType::Common, &sHeader)) - sizeof(uint32_t);
            auto displayPaths = static_cast<AtomDisplayObjectPath *>(
                gettable(atomutils, AtomObjectTableType::DisplayPath, &displayPathNum));
            auto connectorObjects = static_cast<AtomConnectorObject *>(
                gettable(atomutils, AtomObjectTableType::ConnectorObject, &connectorObjectNum));
            if (displayPathNum == connectorObjectNum) {
                autocorrectConnectors(baseAddr, displayPaths, displayPathNum, connectorObjects, connectorObjectNum,
                    connectors, *sz);
            } else {
                NETLOG("rad",
                    "getConnectorsInfo found different displaypaths %u and "
                    "connectors %u",
                    displayPathNum, connectorObjectNum);
            }
        }

        applyPropertyFixes(ctrl, *sz);

        const uint8_t *senseList = nullptr;
        uint8_t senseNum = 0;
        auto priData = OSDynamicCast(OSData, ctrl->getProperty("connector-priority"));
        if (priData) {
            senseList = static_cast<const uint8_t *>(priData->getBytesNoCopy());
            senseNum = static_cast<uint8_t>(priData->getLength());
            NETLOG("rad", "getConnectorInfo found %u senses in connector-priority", senseNum);
            reprioritiseConnectors(senseList, senseNum, connectors, *sz);
        } else {
            NETLOG("rad", "getConnectorInfo leaving unchaged priority");
        }
    }

    NETLOG("rad", "getConnectorsInfo resulting %u connectors follow", *sz);
    RADConnectors::print(connectors, *sz);
}

uint32_t RAD::wrapTranslateAtomConnectorInfoV1(void *that, RADConnectors::AtomConnectorInfo *info,
    RADConnectors::Connector *connector) {
    uint32_t code = FunctionCast(wrapTranslateAtomConnectorInfoV1, callbackRAD->orgTranslateAtomConnectorInfoV1)(that,
        info, connector);

    if (code == 0 && info && connector) {
        RADConnectors::print(connector, 1);

        uint8_t sense = getSenseID(info->i2cRecord);
        if (sense) {
            NETLOG("rad", "translateAtomConnectorInfoV1 got sense id %02X", sense);

            // We need to extract usGraphicObjIds from info->hpdRecord, which is
            // of type ATOM_SRC_DST_TABLE_FOR_ONE_OBJECT: struct
            // ATOM_SRC_DST_TABLE_FOR_ONE_OBJECT {
            //   uint8_t ucNumberOfSrc;
            //   uint16_t usSrcObjectID[ucNumberOfSrc];
            //   uint8_t ucNumberOfDst;
            //   uint16_t usDstObjectID[ucNumberOfDst];
            // };
            // The value we need is in usSrcObjectID. The structure is
            // byte-packed.

            uint8_t ucNumberOfSrc = info->hpdRecord[0];
            for (uint8_t i = 0; i < ucNumberOfSrc; i++) {
                auto usSrcObjectID =
                    *reinterpret_cast<uint16_t *>(info->hpdRecord + sizeof(uint8_t) + i * sizeof(uint16_t));
                NETLOG("rad", "translateAtomConnectorInfoV1 checking %04X object id", usSrcObjectID);
                if (((usSrcObjectID & OBJECT_TYPE_MASK) >> OBJECT_TYPE_SHIFT) == GRAPH_OBJECT_TYPE_ENCODER) {
                    uint8_t txmit = 0, enc = 0;
                    if (getTxEnc(usSrcObjectID, txmit, enc))
                        callbackRAD->autocorrectConnector(getConnectorID(info->usConnObjectId),
                            getSenseID(info->i2cRecord), txmit, enc, connector, 1);
                    break;
                }
            }
        } else {
            NETLOG("rad", "translateAtomConnectorInfoV1 failed to detect sense for "
                          "translated connector");
        }
    }

    return code;
}

void RAD::autocorrectConnectors(uint8_t *baseAddr, AtomDisplayObjectPath *displayPaths, uint8_t displayPathNum,
    AtomConnectorObject *connectorObjects, [[maybe_unused]] uint8_t connectorObjectNum,
    RADConnectors::Connector *connectors, uint8_t sz) {
    for (uint8_t i = 0; i < displayPathNum; i++) {
        if (!isEncoder(displayPaths[i].usGraphicObjIds)) {
            NETLOG("rad", "autocorrectConnectors not encoder %X at %u", displayPaths[i].usGraphicObjIds, i);
            continue;
        }

        uint8_t txmit = 0, enc = 0;
        if (!getTxEnc(displayPaths[i].usGraphicObjIds, txmit, enc)) { continue; }

        uint8_t sense = getSenseID(baseAddr + connectorObjects[i].usRecordOffset);
        if (!sense) {
            NETLOG("rad", "autocorrectConnectors failed to detect sense for %u connector", i);
            continue;
        }

        NETLOG("rad",
            "autocorrectConnectors found txmit %02X enc %02X sense %02X for %u "
            "connector",
            txmit, enc, sense, i);

        autocorrectConnector(getConnectorID(displayPaths[i].usConnObjectId), sense, txmit, enc, connectors, sz);
    }
}

void RAD::autocorrectConnector(uint8_t connector, uint8_t sense, uint8_t txmit, [[maybe_unused]] uint8_t enc,
    RADConnectors::Connector *connectors, uint8_t sz) {
    if (callbackRAD->dviSingleLink) {
        if (connector != CONNECTOR_OBJECT_ID_DUAL_LINK_DVI_I && connector != CONNECTOR_OBJECT_ID_DUAL_LINK_DVI_D &&
            connector != CONNECTOR_OBJECT_ID_LVDS) {
            NETLOG("rad", "autocorrectConnector found unsupported connector type %02X", connector);
            return;
        }

        auto fixTransmit = [](auto &con, uint8_t idx, uint8_t sense, uint8_t txmit) {
            if (con.sense == sense) {
                if (con.transmitter != txmit && (con.transmitter & 0xCF) == con.transmitter) {
                    NETLOG("rad",
                        "autocorrectConnector replacing txmit %02X with "
                        "%02X for %u "
                        "connector sense %02X",
                        con.transmitter, txmit, idx, sense);
                    con.transmitter = txmit;
                }
                return true;
            }
            return false;
        };

        bool isModern = RADConnectors::modern();
        for (uint8_t j = 0; j < sz; j++) {
            if (isModern) {
                auto &con = (&connectors->modern)[j];
                if (fixTransmit(con, j, sense, txmit)) break;
            } else {
                auto &con = (&connectors->legacy)[j];
                if (fixTransmit(con, j, sense, txmit)) break;
            }
        }
    } else
        NETLOG("rad", "autocorrectConnector use -raddvi to enable dvi autocorrection");
}

void RAD::reprioritiseConnectors(const uint8_t *senseList, uint8_t senseNum, RADConnectors::Connector *connectors,
    uint8_t sz) {
    static constexpr uint32_t typeList[] = {
        RADConnectors::ConnectorLVDS,
        RADConnectors::ConnectorDigitalDVI,
        RADConnectors::ConnectorHDMI,
        RADConnectors::ConnectorDP,
        RADConnectors::ConnectorVGA,
    };
    static constexpr uint8_t typeNum {static_cast<uint8_t>(arrsize(typeList))};

    bool isModern = RADConnectors::modern();
    uint16_t priCount = 1;
    for (uint8_t i = 0; i < senseNum + typeNum + 1; i++) {
        for (uint8_t j = 0; j < sz; j++) {
            auto reorder = [&](auto &con) {
                if (i == senseNum + typeNum) {
                    if (con.priority == 0) con.priority = priCount++;
                } else if (i < senseNum) {
                    if (con.sense == senseList[i]) {
                        NETLOG("rad",
                            "reprioritiseConnectors setting priority of "
                            "sense %02X to "
                            "%u by sense",
                            con.sense, priCount);
                        con.priority = priCount++;
                        return true;
                    }
                } else {
                    if (con.priority == 0 && con.type == typeList[i - senseNum]) {
                        NETLOG("rad",
                            "reprioritiseConnectors setting priority of "
                            "sense %02X to "
                            "%u by type",
                            con.sense, priCount);
                        con.priority = priCount++;
                    }
                }
                return false;
            };

            if ((isModern && reorder((&connectors->modern)[j])) || (!isModern && reorder((&connectors->legacy)[j]))) {
                break;
            }
        }
    }
}

bool RAD::wrapSetProperty(IORegistryEntry *that, const char *aKey, void *bytes, unsigned length) {
    if (length > 10 && aKey && reinterpret_cast<const uint32_t *>(aKey)[0] == 'edom' &&
        reinterpret_cast<const uint16_t *>(aKey)[2] == 'l') {
        DBGLOG("rad", "SetProperty caught model %u (%.*s)", length, length, static_cast<char *>(bytes));
        if (*static_cast<uint32_t *>(bytes) == ' DMA' || *static_cast<uint32_t *>(bytes) == ' ITA' ||
            *static_cast<uint32_t *>(bytes) == 'edaR') {
            if (FunctionCast(wrapGetProperty, callbackRAD->orgGetProperty)(that, aKey)) {
                DBGLOG("rad", "SetProperty ignored setting %s to %s", aKey, static_cast<char *>(bytes));
                return true;
            }
            DBGLOG("rad", "SetProperty missing %s, fallback to %s", aKey, static_cast<char *>(bytes));
        }
    }

    return FunctionCast(wrapSetProperty, callbackRAD->orgSetProperty)(that, aKey, bytes, length);
}

OSObject *RAD::wrapGetProperty(IORegistryEntry *that, const char *aKey) {
    auto obj = FunctionCast(wrapGetProperty, callbackRAD->orgGetProperty)(that, aKey);
    auto props = OSDynamicCast(OSDictionary, obj);

    if (props && aKey) {
        const char *prefix {nullptr};
        auto provider = OSDynamicCast(IOService, that->getParentEntry(gIOServicePlane));
        if (provider) {
            if (aKey[0] == 'a') {
                if (!strcmp(aKey, "aty_config")) prefix = "CFG,";
                else if (!strcmp(aKey, "aty_properties"))
                    prefix = "PP,";
            } else if (aKey[0] == 'c' && !strcmp(aKey, "cail_properties")) {
                prefix = "CAIL,";
            }

            if (prefix) {
                DBGLOG("rad", "GetProperty discovered property merge request for %s", aKey);
                auto rawProps = props->copyCollection();
                if (rawProps) {
                    auto newProps = OSDynamicCast(OSDictionary, rawProps);
                    if (newProps) {
                        callbackRAD->mergeProperties(newProps, prefix, provider);
                        that->setProperty(aKey, newProps);
                        obj = newProps;
                    }
                    rawProps->release();
                }
            }
        }
    }

    return obj;
}

uint32_t RAD::wrapGetConnectorsInfoV2(void *that, RADConnectors::Connector *connectors, uint8_t *sz) {
    NETLOG("rad", "getConnectorsInfoV2: this = %p connectors = %p sz = %p", that, connectors, sz);
    uint32_t code = FunctionCast(wrapGetConnectorsInfoV2, callbackRAD->orgGetConnectorsInfoV2)(that, connectors, sz);
    NETLOG("rad", "getConnectorsInfoV2 returned 0x%X", code);
    auto props = callbackRAD->currentPropProvider.get();

    if (code == 0 && sz && props && *props) callbackRAD->updateConnectorsInfo(nullptr, nullptr, *props, connectors, sz);
    else
        NETLOG("rad", "getConnectorsInfoV2 failed %X or undefined %d", code, props == nullptr);

    return code;
}

uint32_t RAD::wrapTranslateAtomConnectorInfoV2(void *that, RADConnectors::AtomConnectorInfo *info,
    RADConnectors::Connector *connector) {
    uint32_t code = FunctionCast(wrapTranslateAtomConnectorInfoV2, callbackRAD->orgTranslateAtomConnectorInfoV2)(that,
        info, connector);

    if (code == 0 && info && connector) {
        RADConnectors::print(connector, 1);

        uint8_t sense = getSenseID(info->i2cRecord);
        if (sense) {
            NETLOG("rad", "translateAtomConnectorInfoV2 got sense id %02X", sense);
            uint8_t txmit = 0, enc = 0;
            if (getTxEnc(info->usGraphicObjIds, txmit, enc))
                callbackRAD->autocorrectConnector(getConnectorID(info->usConnObjectId), getSenseID(info->i2cRecord),
                    txmit, enc, connector, 1);
        } else {
            NETLOG("rad", "translateAtomConnectorInfoV2 failed to detect sense for "
                          "translated connector");
        }
    }

    return code;
}

bool RAD::wrapATIControllerStart(IOService *ctrl, IOService *provider) {
    NETLOG("rad", "starting controller " PRIKADDR, CASTKADDR(current_thread()));

    callbackRAD->currentPropProvider.set(provider);
    bool r = FunctionCast(wrapATIControllerStart, callbackRAD->orgATIControllerStart)(ctrl, provider);
    NETLOG("rad", "starting controller done %d " PRIKADDR, r, CASTKADDR(current_thread()));
    callbackRAD->currentPropProvider.erase();

    return r;
}

bool RAD::doNotTestVram([[maybe_unused]] IOService *ctrl, [[maybe_unused]] uint32_t reg,
    [[maybe_unused]] bool retryOnFail) {
    NETLOG("rad", "TestVRAM called! Returning true");
    return true;
}

bool RAD::wrapNotifyLinkChange(void *atiDeviceControl, kAGDCRegisterLinkControlEvent_t event, void *eventData,
    uint32_t eventFlags) {
    auto ret = FunctionCast(wrapNotifyLinkChange, callbackRAD->orgNotifyLinkChange)(atiDeviceControl, event, eventData,
        eventFlags);

    if (event == kAGDCValidateDetailedTiming) {
        auto cmd = static_cast<AGDCValidateDetailedTiming_t *>(eventData);
        NETLOG("rad", "AGDCValidateDetailedTiming %u -> %d (%u)", cmd->framebufferIndex, ret, cmd->modeStatus);
        if (ret == false || cmd->modeStatus < 1 || cmd->modeStatus > 3) {
            cmd->modeStatus = 2;
            ret = true;
        }
    }

    return ret;
}

void RAD::updateGetHWInfo(IOService *accelVideoCtx, void *hwInfo) {
    IOService *accel, *pciDev;
    accel = OSDynamicCast(IOService, accelVideoCtx->getParentEntry(gIOServicePlane));
    if (accel == NULL) {
        NETLOG("rad", "getHWInfo: no parent found for accelVideoCtx!");
        return;
    }
    pciDev = OSDynamicCast(IOService, accel->getParentEntry(gIOServicePlane));
    if (pciDev == NULL) {
        NETLOG("rad", "getHWInfo: no parent found for accel!");
        return;
    }
    uint16_t &org = getMember<uint16_t>(hwInfo, 0x4);
    uint32_t dev = org;
    if (!WIOKit::getOSDataValue(pciDev, "codec-device-id", dev)) { WIOKit::getOSDataValue(pciDev, "device-id", dev); }
    NETLOG("rad", "getHWInfo: original PID: 0x%04X, replaced PID: 0x%04X", org, dev);
    org = static_cast<uint16_t>(dev);
}
