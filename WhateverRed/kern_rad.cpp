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
static const char *pathIOAcceleratorFamily2 =
    "/System/Library/Extensions/IOAcceleratorFamily2.kext/Contents/MacOS/IOAcceleratorFamily2";    // TODO: Remove this

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
    // TODO: Remove this
    "com.apple.iokit.IOAcceleratorFamily2",
    &pathIOAcceleratorFamily2,
    1,
    {},
    {},
    KernelPatcher::KextInfo::Unloaded,
};

RAD *RAD::callbackRAD = nullptr;

void RAD::init() {
    callbackRAD = this;

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

    static uint8_t builtBytes[] = {0x01};
    provider->setProperty("built-in", builtBytes, sizeof(builtBytes));

    NETDBG::enabled = true;
    NETLOG("rad", "patching device type table");
    PANIC_COND(MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) != KERN_SUCCESS, "rad",
        "Failed to enable kernel writing");
    auto deviceId = provider->extendedConfigRead16(kIOPCIConfigDeviceID);
    callbackRAD->orgDeviceTypeTable[0] = deviceId;
    callbackRAD->orgDeviceTypeTable[1] = 6;
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
            NETLOG("rad", "Spoofing PSP version v10 to v9.0.2");
            param1[3] = 0x9;
            param1[4] = 0x0;
            param1[5] = 0x2;
            break;
        case 0xB:
            [[fallthrough]];
        case 0xC:
            NETLOG("rad", "Spoofing PSP version v11/v12 to v11");
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
    switch (ret & 0xFFFF00) {
        case 0x090100:
            [[fallthrough]];
        case 0x090200:
            NETLOG("rad", "Spoofing GC version v9.{1,2}.x to v9.0.1");
            return 0x090001;
        case 0x090300:
            NETLOG("rad", "Spoofing GC version v9.3 to v9.2.1");
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

void *RAD::wrapCreatePowerTuneServices(void *param1, void *param2) {
    NETLOG("rad", "createPowerTuneServices: param1 = %p param2 = %p", param1, param2);
    auto *ret = IOMallocZero(0x18);
    callbackRAD->orgVega10PowerTuneConstructor(ret, param1, param2);
    return ret;
}

uint16_t RAD::wrapGetFamilyId() { return 0x8E; }    // Normally hardcoded to return `0x8F`, which is Navi.

uint16_t RAD::wrapGetEnumeratedRevision(void *that) {
    /**
     * `AMDRadeonX6000_AmdAsicInfoNavi10::getEnumeratedRevisionNumber`
     * Emulated Revision = Revision + Enumerated Revision.
     */
    auto *&pciDev = getMember<IOPCIDevice *>(that, 0x18);
    auto &revision = getMember<uint32_t>(that, 0x68);

    // https://elixir.bootlin.com/linux/v5.16.9/source/drivers/gpu/drm/amd/amdgpu/gmc_v9_0.c#L1532
    callbackRAD->isThreeLevelVMPT = revision == 0 || revision == 1;

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
        DBGLOG("rad", "%s => %s", filename, targetFilename);

        auto *fwDesc = getFWDescByName(filename);
        PANIC_COND(!fwDesc, "rad", "Somehow %s is missing", filename);

        auto *fw = callbackRAD->orgCreateFirmware(fwDesc->var, fwDesc->size, 0x200, targetFilename);
        DBGLOG("rad", "Inserting %s!", targetFilename);
        PANIC_COND(!callbackRAD->orgPutFirmware(callbackRAD->callbackFirmwareDirectory, 6, fw), "rad",
            "Failed to inject ativvaxy_rv.dat firmware");

        snprintf(filename, 128, "%s_rlc.bin", asicName);
        fwDesc = getFWDescByName(filename);
        PANIC_COND(!fwDesc, "rad", "Somehow %s is missing", filename);
        callbackRAD->orgGcRlcUcode->addr = 0x0;
        memmove(callbackRAD->orgGcRlcUcode->data, fwDesc->var, fwDesc->size);
        DBGLOG("rad", "Injected %s!", filename);

        snprintf(filename, 128, "%s_me.bin", asicName);
        fwDesc = getFWDescByName(filename);
        PANIC_COND(!fwDesc, "rad", "Somehow %s is missing", filename);
        callbackRAD->orgGcMeUcode->addr = 0x1000;
        memmove(callbackRAD->orgGcMeUcode->data, fwDesc->var, fwDesc->size);
        DBGLOG("rad", "Injected %s!", filename);

        snprintf(filename, 128, "%s_ce.bin", asicName);
        fwDesc = getFWDescByName(filename);
        PANIC_COND(!fwDesc, "rad", "Somehow %s is missing", filename);
        callbackRAD->orgGcCeUcode->addr = 0x800;
        memmove(callbackRAD->orgGcCeUcode->data, fwDesc->var, fwDesc->size);
        DBGLOG("rad", "Injected %s!", filename);

        snprintf(filename, 128, "%s_pfp.bin", asicName);
        fwDesc = getFWDescByName(filename);
        PANIC_COND(!fwDesc, "rad", "Somehow %s is missing", filename);
        callbackRAD->orgGcPfpUcode->addr = 0x1400;
        memmove(callbackRAD->orgGcPfpUcode->data, fwDesc->var, fwDesc->size);
        DBGLOG("rad", "Injected %s!", filename);

        snprintf(filename, 128, "%s_mec.bin", asicName);
        fwDesc = getFWDescByName(filename);
        PANIC_COND(!fwDesc, "rad", "Somehow %s is missing", filename);
        callbackRAD->orgGcMecUcode->addr = 0x0;
        memmove(callbackRAD->orgGcMecUcode->data, fwDesc->var, fwDesc->size);
        DBGLOG("rad", "Injected %s!", filename);

        snprintf(filename, 128, "%s_mec_jt.bin", asicName);
        fwDesc = getFWDescByName(filename);
        PANIC_COND(!fwDesc, "rad", "Somehow %s is missing", filename);
        callbackRAD->orgGcMecJtUcode->addr = 0x104A4;
        memmove(callbackRAD->orgGcMecJtUcode->data, fwDesc->var, fwDesc->size);
        DBGLOG("rad", "Injected %s!", filename);

        snprintf(filename, 128, "%s_sdma.bin", asicName);
        fwDesc = getFWDescByName(filename);
        PANIC_COND(!fwDesc, "rad", "Somehow %s is missing", filename);
        memmove(callbackRAD->orgSdmaUcode->data, fwDesc->var, fwDesc->size);
        DBGLOG("rad", "Injected %s!", filename);
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
        DBGLOG("rad", "Warning! Using Fallback Init Caps mechanism");
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

uint32_t RAD::wrapGetVideoMemoryType() { return 4; }    // DDR4
uint32_t RAD::wrapGetVideoMemoryBitWidth() { return 64; }
IOReturn RAD::wrapPopulateVramInfo() { return kIOReturnSuccess; }

bool RAD::wrapGFX10AcceleratorStart() {
    /**
     * We don't want the `AMDRadeonX6000` personality defined in the `Info.plist` to do anything.
     * We only use it to force-load `AMDRadeonX6000` and snatch the VCN symbols.
     */
    return false;
}

bool RAD::sdma1AllocateAndInitHWRingsHack(void *that) {
    NETLOG("rad", "sdma1AllocateAndInitHWRings: this = %p", that);
    getMember<void *>(that, 0x50) = getMember<void *>(getMember<void *>(that, 0x20), 0x28);    // Copy ring
    return true;
}

bool RAD::wrapAllocateHWEngines(void *that) {
    auto *pm4 = callbackRAD->orgGFX9PM4EngineNew(0x1E8);
    callbackRAD->orgGFX9PM4EngineConstructor(pm4);
    getMember<void *>(that, 0x3B8) = pm4;

    auto *sdma0 = callbackRAD->orgGFX9SDMAEngineNew(0x128);
    callbackRAD->orgGFX9SDMAEngineConstructor(sdma0);
    getMember<void *>(that, 0x3C0) = sdma0;
    callbackRAD->sdma0HWEngine = sdma0;

    auto *sdma1 = callbackRAD->orgGFX9SDMAEngineNew(0x128);
    callbackRAD->orgGFX9SDMAEngineConstructor(sdma1);
    getMember<bool>(sdma1, 0x10) = true;    // Set this->enabled to true, as this isn't a real engine.
    getMember<void *>(that, 0x3C8) = sdma1;

    auto *&oldVtable = getMember<mach_vm_address_t *>(sdma1, 0);
    auto *vtable = new mach_vm_address_t[0x48];
    memcpy(vtable, oldVtable, 0x48 * sizeof(mach_vm_address_t));
    oldVtable = vtable;
    vtable[0x200] = reinterpret_cast<mach_vm_address_t>(sdma1AllocateAndInitHWRingsHack);

    auto *vcn2 = callbackRAD->orgGFX10VCN2EngineNew(0x198);
    callbackRAD->orgGFX10VCN2EngineConstructor(vcn2);
    getMember<void *>(that, 0x3F8) = vcn2;
    return true;
}

void RAD::wrapSetupAndInitializeHWCapabilities(void *that) {
    NETLOG("rad", "wrapSetupAndInitializeCapabilities: this = %p", that);
    FunctionCast(wrapSetupAndInitializeHWCapabilities, callbackRAD->orgSetupAndInitializeHWCapabilities)(that);
    FunctionCast(wrapSetupAndInitializeHWCapabilities, callbackRAD->orgGFX10SetupAndInitializeHWCapabilities)(that);
    getMember<uint32_t>(that, 0xC0) = 0;    // Raven ASICs do not have an SDMA Page Queue
    NETLOG("rad", "wrapSetupAndInitializeCapabilities: done");
}

void RAD::wrapDumpASICHangStateCold(uint64_t param1) {
    NETLOG("rad", "dumpASICHangStateCold: param1 = 0x%llX", param1);
    IOSleep(3600000);
    FunctionCast(wrapDumpASICHangStateCold, callbackRAD->orgDumpASICHangStateCold)(param1);
    NETLOG("rad", "dumpASICHangStateCold finished");
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
            panic("rad: Unknown ASIC type");
    }
}

using t_pspLoadExtended = uint32_t (*)(void *, uint64_t, uint64_t, const void *, size_t);

uint32_t RAD::wrapPspAsdLoad(void *pspData) {
    /**
     * Hack: Add custom param 4 and 5 (pointer to firmware and size)
     * aka RCX and R8 registers
     * Complementary to `_psp_asd_load` patch-set.
     */
    auto *filename = new char[128];
    snprintf(filename, 128, "%s_asd.bin", getASICName());
    NETLOG("rad", "injecting %s!", filename);
    auto *org = reinterpret_cast<t_pspLoadExtended>(callbackRAD->orgPspAsdLoad);
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
    auto *org = reinterpret_cast<t_pspLoadExtended>(callbackRAD->orgPspDtmLoad);
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
    auto *org = reinterpret_cast<t_pspLoadExtended>(callbackRAD->orgPspHdcpLoad);
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
    return ret;
}

bool RAD::wrapPowerUpHW(void *that) {    // TODO: Remove this
    NETLOG("rad", "powerUpHW: this = %p", that);
    auto ret = FunctionCast(wrapPowerUpHW, callbackRAD->orgPowerUpHW)(that);
    NETLOG("rad", "powerUpHW returned %d", ret);
    return ret;
}

void RAD::wrapHWsetMemoryAllocationsEnabled(void *that, bool param1) {    // TODO: Remove this
    NETLOG("rad", "HWsetMemoryAllocationsEnabled: this = %p param1 = %d", that, param1);
    FunctionCast(wrapHWsetMemoryAllocationsEnabled, callbackRAD->orgHWsetMemoryAllocationsEnabled)(that, param1);
    NETLOG("rad", "HWsetMemoryAllocationsEnabled finished");
}

static bool sdma1Hacked = false;

bool RAD::sdma1IsIdleHack([[maybe_unused]] void *that) {
    return FunctionCast(sdma1IsIdleHack, getMember<mach_vm_address_t *>(callbackRAD->sdma0HWChannel, 0)[0x2E])(
        callbackRAD->sdma0HWChannel);
}

void *RAD::wrapRTGetHWChannel(void *that, uint32_t param1, uint32_t param2, uint32_t param3) {
    auto ret = FunctionCast(wrapRTGetHWChannel, callbackRAD->orgRTGetHWChannel)(that, param1, param2, param3);
    if (!sdma1Hacked && param1 == 2 && param2 == 0 && param3 == 0) {
        sdma1Hacked = true;
        NETLOG("rad", "RTGetHWChannel: SDMA1 HWChannel detected. Hacking it");

        auto sdma0HWChannel = FunctionCast(wrapRTGetHWChannel, callbackRAD->orgRTGetHWChannel)(that, param1, 2, param3);
        callbackRAD->sdma0HWChannel = sdma0HWChannel;

        auto *&oldVtable = getMember<mach_vm_address_t *>(ret, 0);
        auto *vtable = new mach_vm_address_t[0x6E];
        memcpy(vtable, oldVtable, 0x6E * sizeof(mach_vm_address_t));
        oldVtable = vtable;

        vtable[0x2E] = reinterpret_cast<mach_vm_address_t>(sdma1IsIdleHack);

        /* Swap ring with SDMA0's */
        getMember<void *>(ret, 0x28) = getMember<void *>(sdma0HWChannel, 0x28);
    }
    return ret;
}

void RAD::wrapCosDebugAssert(void *param1, uint8_t *param2, uint8_t *param3, uint32_t param4, uint8_t *param5) {
    NETLOG("rad", "cosDebugAssert: param1 = %p param2 = %p param3 = %p param4 = 0x%X param5 = %p", param1, param2,
        param3, param4, param5);
    FunctionCast(wrapCosDebugAssert, callbackRAD->orgCosDebugAssert)(param1, param2, param3, param4, param5);
}

uint32_t RAD::wrapHwReadReg32(void *that, uint32_t reg) {
    if (reg == 0xD31) {
        /**
         * NBIO 7.4 -> NBIO 7.0
         * reg = SOC15_OFFSET(NBIO_BASE, 0, mmRCC_DEV0_EPF0_STRAP0);
         */
        reg = 0xD2F;
        NETLOG("rad", "hwReadReg32: redirecting reg 0xD31 to 0x%X", reg);
    }
    auto ret = FunctionCast(wrapHwReadReg32, callbackRAD->orgHwReadReg32)(that, reg);
    return ret;
}

constexpr uint32_t PPSMC_MSG_PowerUpSdma = 0xE;

uint32_t RAD::wrapSmuRavenInitialize(void *smumData, uint32_t param2) {
    NETLOG("rad", "_SmuRaven_Initialize: param1 = %p param2 = 0x%X", smumData, param2);
    auto ret = FunctionCast(wrapSmuRavenInitialize, callbackRAD->orgSmuRavenInitialize)(smumData, param2);
    NETLOG("rad", "_SmuRaven_Initialize returned 0x%X", ret);
    callbackRAD->orgRavenSendMsgToSmcWithParam(smumData, PPSMC_MSG_PowerUpSdma, 0);
    return ret;
}

uint32_t RAD::wrapSmuRenoirInitialize(void *smumData, uint32_t param2) {
    NETLOG("rad", "_SmuRenoir_Initialize: param1 = %p param2 = 0x%X", smumData, param2);
    auto ret = FunctionCast(wrapSmuRenoirInitialize, callbackRAD->orgSmuRenoirInitialize)(smumData, param2);
    NETLOG("rad", "_SmuRenoir_Initialize returned 0x%X", ret);
    callbackRAD->orgRenoirSendMsgToSmcWithParam(smumData, PPSMC_MSG_PowerUpSdma, 0);
    return ret;
}

uint64_t RAD::wrapMapVA(void *that, uint64_t param1, void *memory, uint64_t param3, uint64_t sizeToMap,
    uint64_t flags) {
    NETLOG("rad", "mapVA: this = %p param1 = 0x%llX memory = %p param3 = 0x%llX sizeToMap = 0x%llX flags = 0x%llX",
        that, param1, memory, param3, sizeToMap, flags);
    auto ret = FunctionCast(wrapMapVA, callbackRAD->orgMapVA)(that, param1, memory, param3, sizeToMap, flags);
    NETLOG("rad", "mapVA returned 0x%llX", ret);
    return ret;
}

uint64_t RAD::wrapMapVMPT(void *that, void *vmptCtl, uint64_t vmptLevel, uint32_t param3, uint64_t param4,
    uint64_t param5, uint64_t sizeToMap) {
    NETLOG("rad",
        "mapVMPT: this = %p vmptCtl = %p vmptLevel = 0x%llX param3 = 0x%X param4 = 0x%llX param5 = 0x%llX sizeToMap = "
        "0x%llX",
        that, vmptCtl, vmptLevel, param3, param4, param5, sizeToMap);
    auto ret =
        FunctionCast(wrapMapVMPT, callbackRAD->orgMapVMPT)(that, vmptCtl, vmptLevel, param3, param4, param5, sizeToMap);
    NETLOG("rad", "mapVMPT returned 0x%llX", ret);
    return ret;
}

static uint64_t vmptConfig3Level[][3] = {
    {0x10000000, 0x200, 0x1000},
    {0x10000, 0x1000, 0x8000},
};

static uint64_t vmptConfig2Level[][3] = {
    {0x10000000, 0x200, 0x1000},
    {0x1000, 0x10000, 0x80000},
};

bool RAD::wrapVMMInit(void *that, void *param1) {
    NETLOG("rad", "VMMInit: this = %p param1 = %p", that, param1);

    auto vmptConfig = callbackRAD->isThreeLevelVMPT ? vmptConfig3Level : vmptConfig2Level;
    for (size_t level = 0; level < 2; level++) {
        getMember<uint64_t>(that, 0xAB0 + 0x20 * level) = vmptConfig[level][0];
        getMember<uint32_t>(that, 0xAB0 + 0x20 * level + 0xC) = static_cast<uint32_t>(vmptConfig[level][1]);
        getMember<uint32_t>(that, 0xAB0 + 0x20 * level + 0x10) = static_cast<uint32_t>(vmptConfig[level][2]);
    }
    auto ret = FunctionCast(wrapVMMInit, callbackRAD->orgVMMInit)(that, param1);
    if (!callbackRAD->isThreeLevelVMPT) {
        getMember<uint32_t>(that, 0xB30) = 2;
        memset(reinterpret_cast<void *>(reinterpret_cast<uint64_t>(that) + 0xAB0 + 0x20 * 2), 0,
            0x20);    // Only 2 levels
    }
    NETLOG("rad", "VMMInit returned %d", ret);
    return ret;
}

uint32_t RAD::wrapWriteWritePTEPDECommand(void *that, uint32_t *buf, uint64_t pe, uint32_t count, uint64_t flags,
    uint64_t addr, uint64_t incr) {
    NETLOG("rad",
        "writeWritePTEPDECommand: this = %p buf = %p pe = 0x%llX count = 0x%X flags = 0x%llX addr = 0x%llX incr = "
        "0x%llX",
        that, buf, pe, count, flags, addr, incr);
    auto ret = FunctionCast(wrapWriteWritePTEPDECommand, callbackRAD->orgWriteWritePTEPDECommand)(that, buf, pe, count,
        flags, addr, incr);
    NETLOG("rad", "writeWritePTEPDECommand returned 0x%X", ret);
    return ret;
}

uint64_t RAD::wrapGetPDEValue(void *that, uint64_t param1, uint64_t param2) {
    NETLOG("rad", "getPDEValue: this = %p param1 = 0x%llX param2 = 0x%llX", that, param1, param2);
    auto ret = FunctionCast(wrapGetPDEValue, callbackRAD->orgGetPDEValue)(that, param1, param2);
    NETLOG("rad", "getPDEValue returned 0x%llX", ret);
    return ret;
}

uint64_t RAD::wrapGetPTEValue(void *that, uint64_t param1, uint64_t param2, uint64_t param3, uint32_t param4) {
    NETLOG("rad", "getPTEValue: this = %p param1 = 0x%llX param2 = 0x%llX param3 = 0x%llX param4 = 0x%X", that, param1,
        param2, param3, param4);
    auto ret = FunctionCast(wrapGetPTEValue, callbackRAD->orgGetPTEValue)(that, param1, param2, param3, param4);
    NETLOG("rad", "getPTEValue returned 0x%llX", ret);
    return ret;
}

void RAD::wrapUpdateContiguousPTEsWithDMAUsingAddr(void *that, uint64_t param1, uint64_t param2, uint64_t param3,
    uint64_t param4, uint64_t param5) {
    NETLOG("rad",
        "updateContiguousPTEsWithDMAUsingAddr: this = %p param1 = 0x%llX param2 = 0x%llX param3 = 0x%llX param4 = "
        "0x%llX param5 = 0x%llX",
        that, param1, param2, param3, param4, param5);
    FunctionCast(wrapUpdateContiguousPTEsWithDMAUsingAddr, callbackRAD->orgUpdateContiguousPTEsWithDMAUsingAddr)(that,
        param1, param2, param3, param4, param5);
    NETLOG("rad", "updateContiguousPTEsWithDMAUsingAddr finished");
}

void RAD::wrapInitializeFamilyType(void *that) { getMember<uint32_t>(that, 0x308) = 0x8E; }

uint32_t RAD::pspFeatureUnsupported() { return 4; }

IOReturn RAD::wrapQueryHwBlockRegisterBase(void *that, uint32_t blockType, uint8_t param2, uint32_t param3,
    uint32_t *retPtr) {
    NETLOG("rad", "queryHwBlockRegisterBase: this = %p blockType = 0x%X param2 = 0x%hhX param3 = 0x%X retPtr = %p",
        that, blockType, param2, param3, retPtr);
    auto ret = FunctionCast(wrapQueryHwBlockRegisterBase, callbackRAD->orgQueryHwBlockRegisterBase)(that, blockType,
        param2, param3, retPtr);
    NETLOG("rad", "Register base is set to 0x%X", *retPtr);
    return ret;
}

void RAD::wrapHwWriteReg(void *that, uint32_t regIndex, uint32_t regVal) {
    NETLOG("rad", "hwWriteReg: this = %p regIndex = 0x%X regVal = 0x%X", that, regIndex, regVal);
    FunctionCast(wrapHwWriteReg, callbackRAD->orgHwWriteReg)(that, regIndex, regVal);
    NETLOG("rad", "hwWriteReg finished");
}

void RAD::wrapPrepareVMInvalidateRequest(void* that, void* param1, void* param2, bool param3) {
    NETLOG("rad", "prepareVMInvalidateRequest: this = %p param1 = %p param2 = %p param3 = %d", that, param1, param2, param3);
    FunctionCast(wrapPrepareVMInvalidateRequest, callbackRAD->orgPrepareVMInvalidateRequest)(that, param1, param2, param3);
    NETLOG("rad", "prepareVMInvalidateRequest finished");
}

void RAD::wrapInvalidateVM(void* that, void* param2, uint32_t* param3, uint32_t param4) {
    NETLOG("rad", "invalidateVM: this = %p param2 = %p param3 = %p param4 = 0x%X", that, param2, param3, param4);
    FunctionCast(wrapInvalidateVM, callbackRAD->orgInvalidateVM)(that, param2, param3, param4);
    NETLOG("rad", "invalidateVM finished");
}

void RAD::wrapFlushAndInvalidateCaches(void* that, uint64_t param1, uint64_t param2) {
    NETLOG("rad", "flushAndInvalidateCaches: this = %p param1 = 0x%llX param2 = 0x%llX", that, param1, param2);
    FunctionCast(wrapFlushAndInvalidateCaches, callbackRAD->orgFlushAndInvalidateCaches)(that, param1, param2);
    NETLOG("rad", "flushAndInvalidateCaches finished");
}

bool RAD::processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
    if (kextRadeonSupport.loadIndex == index) {
        KernelPatcher::RouteRequest requests[] = {
            {"__ZN16AtiDeviceControl16notifyLinkChangeE31kAGDCRegisterLinkControlEvent_tmj", wrapNotifyLinkChange,
                orgNotifyLinkChange},

        };
        if (!patcher.routeMultipleLong(index, requests, address, size)) {
            panic("RAD: Failed to route AMDSupport symbols");
        }

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
            {"_gc_9_0_rlc_ucode", orgGcRlcUcode},
            {"_gc_9_0_me_ucode", orgGcMeUcode},
            {"_gc_9_0_ce_ucode", orgGcCeUcode},
            {"_gc_9_0_pfp_ucode", orgGcPfpUcode},
            {"_gc_9_0_mec_ucode", orgGcMecUcode},
            {"_gc_9_0_mec_jt_ucode", orgGcMecJtUcode},
            {"_sdma_4_1_ucode", orgSdmaUcode},
            {"_Raven_SendMsgToSmcWithParameter", orgRavenSendMsgToSmcWithParam},
            {"_Renoir_SendMsgToSmcWithParameter", orgRenoirSendMsgToSmcWithParam},
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
            {"_psp_xgmi_is_support", pspFeatureUnsupported},
            {"_psp_rap_is_supported", pspFeatureUnsupported},
            {"__ZN14AmdTtlServices24queryHwBlockRegisterBaseE12hwblock_typehjPj", wrapQueryHwBlockRegisterBase,
                orgQueryHwBlockRegisterBase},
        };
        if (!patcher.routeMultipleLong(index, requests, address, size)) {
            panic("RAD: Failed to route AMDRadeonX5000HWLibs symbols");
        }

        constexpr uint8_t find_asic_reset[] = {0x55, 0x48, 0x89, 0xE5, 0x8B, 0x56, 0x04, 0xBE, 0x3B, 0x00, 0x00, 0x00,
            0x5D, 0xE9, 0x51, 0xFE, 0xFF, 0xFF};
        constexpr uint8_t repl_asic_reset[] = {0x55, 0x48, 0x89, 0xE5, 0x8B, 0x56, 0x04, 0xBE, 0x1E, 0x00, 0x00, 0x00,
            0x5D, 0xE9, 0x51, 0xFE, 0xFF, 0xFF};
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
            {"__ZNK34AMDRadeonX6000_AmdBiosParserHelper18getVideoMemoryTypeEv", wrapGetVideoMemoryType},
            {"__ZNK34AMDRadeonX6000_AmdBiosParserHelper22getVideoMemoryBitWidthEv", wrapGetVideoMemoryBitWidth},
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

        constexpr uint8_t find_null_check1[] = {0x48, 0x89, 0x83, 0x90, 0x00, 0x00, 0x00, 0x48, 0x85, 0xC0, 0x0F, 0x84,
            0x89, 0x00, 0x00, 0x00, 0x48, 0x8B, 0x7B, 0x18};
        constexpr uint8_t repl_null_check1[] = {0x48, 0x89, 0x83, 0x90, 0x00, 0x00, 0x00, 0x90, 0x90, 0x90, 0x90, 0x90,
            0x90, 0x90, 0x90, 0x90, 0x48, 0x8B, 0x7B, 0x18};
        static_assert(arrsize(find_null_check1) == arrsize(repl_null_check1), "Find/replace patch size mismatch");

        constexpr uint8_t find_null_check2[] = {0x48, 0x89, 0x83, 0x88, 0x00, 0x00, 0x00, 0x48, 0x85, 0xC0, 0x0F, 0x84,
            0xA1, 0x00, 0x00, 0x00, 0x48, 0x8B, 0x7B, 0x18};
        constexpr uint8_t repl_null_check2[] = {0x48, 0x89, 0x83, 0x88, 0x00, 0x00, 0x00, 0x90, 0x90, 0x90, 0x90, 0x90,
            0x90, 0x90, 0x90, 0x90, 0x48, 0x8B, 0x7B, 0x18};
        static_assert(arrsize(find_null_check2) == arrsize(repl_null_check2), "Find/replace patch size mismatch");

        constexpr uint8_t find_null_check3[] = {0x48, 0x83, 0xBB, 0x90, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x84, 0x90, 0x00,
            0x00, 0x00, 0x49, 0x89, 0xF7, 0xBA, 0x60, 0x00, 0x00, 0x00};
        constexpr uint8_t repl_null_check3[] = {0x48, 0x83, 0xBB, 0x90, 0x00, 0x00, 0x00, 0x00, 0x90, 0x90, 0x90, 0x90,
            0x90, 0x90, 0x49, 0x89, 0xF7, 0xBA, 0x60, 0x00, 0x00, 0x00};
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
        };
        if (!patcher.solveMultiple(index, solveRequests, address, size)) {
            panic("RAD: Failed to resolve AMDRadeonX5000 symbols");
        }

        KernelPatcher::RouteRequest requests[] = {
            {"__ZN32AMDRadeonX5000_AMDVega10Hardware17allocateHWEnginesEv", wrapAllocateHWEngines},
            {"__ZN32AMDRadeonX5000_AMDVega10Hardware32setupAndInitializeHWCapabilitiesEv",
                wrapSetupAndInitializeHWCapabilities, orgSetupAndInitializeHWCapabilities},
            {"__ZN26AMDRadeonX5000_AMDHardware17dumpASICHangStateEb.cold.1", wrapDumpASICHangStateCold,
                orgDumpASICHangStateCold},
            {"__ZN34AMDRadeonX5000_AMDAccelDisplayPipe20writeDiagnosisReportERPcRj",
                wrapAccelDisplayPipeWriteDiagnosisReport, orgAccelDisplayPipeWriteDiagnosisReport},
            {"__ZN23AMDRadeonX5000_AMDHWVMM27setMemoryAllocationsEnabledEb", wrapSetMemoryAllocationsEnabled,
                orgSetMemoryAllocationsEnabled},
            {"__ZN37AMDRadeonX5000_AMDGraphicsAccelerator9powerUpHWEv", wrapPowerUpHW, orgPowerUpHW},
            {"__ZN26AMDRadeonX5000_AMDHardware27setMemoryAllocationsEnabledEb", wrapHWsetMemoryAllocationsEnabled,
                orgHWsetMemoryAllocationsEnabled},
            {"__ZN28AMDRadeonX5000_AMDRTHardware12getHWChannelE18_eAMD_CHANNEL_TYPE11SS_PRIORITYj", wrapRTGetHWChannel,
                orgRTGetHWChannel},
            {"__ZN29AMDRadeonX5000_AMDHWVMContext5mapVAEyP13IOAccelMemoryyyN24AMDRadeonX5000_IAMDHWVMM10VmMapFlagsE",
                wrapMapVA, orgMapVA},
            {"__ZN29AMDRadeonX5000_AMDHWVMContext7mapVMPTEP12AMD_VMPT_CTL15eAMD_VMPT_LEVELjyyy", wrapMapVMPT,
                orgMapVMPT},
            {"__ZN25AMDRadeonX5000_AMDGFX9VMM4initEP30AMDRadeonX5000_IAMDHWInterface", wrapVMMInit, orgVMMInit},
            {"__ZN33AMDRadeonX5000_AMDGFX9SDMAChannel23writeWritePTEPDECommandEPjyjyyy", wrapWriteWritePTEPDECommand,
                orgWriteWritePTEPDECommand},
            {"__ZN29AMDRadeonX5000_AMDHWRegisters5writeEjj", wrapHwWriteReg, orgHwWriteReg},
            {"__ZN25AMDRadeonX5000_AMDGFX9VMM11getPDEValueE15eAMD_VMPT_LEVELy", wrapGetPDEValue, orgGetPDEValue},
            {"__ZN25AMDRadeonX5000_AMDGFX9VMM11getPTEValueE15eAMD_VMPT_LEVELyN24AMDRadeonX5000_IAMDHWVMM10VmMapFlagsEj",
                wrapGetPTEValue, orgGetPTEValue},
            {"__ZN29AMDRadeonX5000_AMDHWVMContext36updateContiguousPTEsWithDMAUsingAddrEyyyyy",
                wrapUpdateContiguousPTEsWithDMAUsingAddr, orgUpdateContiguousPTEsWithDMAUsingAddr},
            {"__ZN30AMDRadeonX5000_AMDGFX9Hardware20initializeFamilyTypeEv", wrapInitializeFamilyType},
            {"__ZN25AMDRadeonX5000_AMDGFX9VMM26prepareVMInvalidateRequestEP25AMD_VM_INVALIDATE_REQUESTPK22AMD_VM_INVALIDATE_INFOb", wrapPrepareVMInvalidateRequest, orgPrepareVMInvalidateRequest},
            {"__ZN23AMDRadeonX5000_AMDHWVMM12invalidateVMEPK28AMD_VM_INVALIDATE_RANGE_INFOPKjj", wrapInvalidateVM, orgInvalidateVM},
            {"__ZN24AMDRadeonX5000_AMDHWGart24flushAndInvalidateCachesEyy", wrapFlushAndInvalidateCaches, orgFlushAndInvalidateCaches},
        };
        if (!patcher.routeMultipleLong(index, requests, address, size)) {
            panic("RAD: Failed to route AMDRadeonX5000 symbols");
        }

        constexpr uint8_t find_startHWEngines[] = {0x49, 0x89, 0xFE, 0x31, 0xDB, 0x48, 0x83, 0xFB, 0x02, 0x74, 0x50};
        constexpr uint8_t repl_startHWEngines[] = {0x49, 0x89, 0xFE, 0x31, 0xDB, 0x48, 0x83, 0xFB, 0x01, 0x74, 0x50};
        static_assert(sizeof(find_startHWEngines) == sizeof(repl_startHWEngines), "Find/replace size mismatch");

        constexpr uint8_t find_VMMInit[] = {0x48, 0x89, 0x84, 0x0B, 0xD0, 0x0A, 0x00, 0x00, 0x89, 0x94, 0x0B, 0xDC,
            0x0A, 0x00, 0x00, 0x89, 0xD6, 0xC1, 0xE2, 0x03, 0x89, 0x94, 0x0B, 0xE0, 0x0A, 0x00, 0x00};
        constexpr uint8_t repl_VMMInit[] = {0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90,
            0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x90};
        static_assert(sizeof(find_VMMInit) == sizeof(repl_VMMInit), "Find/replace size mismatch");

        KernelPatcher::LookupPatch patches[] = {
            /**
             * `AMDRadeonX5000_AMDHardware::startHWEngines`
             * Make for loop stop at 1 instead of 2 in order to skip starting SDMA1 engine.
             */
            {&kextRadeonX5000, find_startHWEngines, repl_startHWEngines, arrsize(find_startHWEngines), 2},
            /**
             * `AMDRadeonX5000_AMDGFX9VMM::init`
             * NOP out part of the vmptConfig setting logic, in order not to override the value set in wrapVMMInit.
             */
            {&kextRadeonX5000, find_VMMInit, repl_VMMInit, arrsize(find_VMMInit), 2},
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
        };
        if (!patcher.routeMultipleLong(index, requests, address, size)) {
            panic("RAD: Failed to route AMDRadeonX6000 symbols");
        }

        constexpr uint8_t find_hwchannel_init1[] = {0x74, 0x54, 0x49, 0x8B, 0x7C, 0x24, 0x18, 0x48, 0x8B, 0x07, 0xFF,
            0x90, 0xB8, 0x03, 0x00, 0x00};
        constexpr uint8_t repl_hwchannel_init1[] = {0x74, 0x54, 0x49, 0x8B, 0x7C, 0x24, 0x18, 0x48, 0x8B, 0x07, 0xFF,
            0x90, 0xC0, 0x03, 0x00, 0x00};
        static_assert(sizeof(find_hwchannel_init1) == sizeof(repl_hwchannel_init1), "Find/replace size mismatch");

        constexpr uint8_t find_hwchannel_init2[] = {0xFF, 0x90, 0xC0, 0x03, 0x00, 0x00, 0xA8, 0x01, 0x74, 0x12, 0x49,
            0x8B, 0x04, 0x24, 0x4C, 0x89, 0xE7, 0xBE, 0x01, 0x00, 0x00, 0x00, 0xFF, 0x90, 0x18, 0x02, 0x00, 0x00, 0x49,
            0x8B, 0x7C, 0x24, 0x18, 0x48, 0x8B, 0x07, 0xFF, 0x90, 0xC0, 0x03, 0x00, 0x00, 0xA8, 0x02, 0x74, 0x12, 0x49,
            0x8B, 0x04, 0x24, 0x4C, 0x89, 0xE7, 0xBE, 0x02, 0x00, 0x00, 0x00, 0xFF, 0x90, 0x18, 0x02, 0x00, 0x00, 0x49,
            0x8B, 0x7C, 0x24, 0x18, 0x48, 0x8B, 0x07, 0xFF, 0x90, 0xC0, 0x03, 0x00, 0x00, 0x0F, 0xBA, 0xE0, 0x0B, 0x73,
            0x12, 0x49, 0x8B, 0x04, 0x24, 0x4C, 0x89, 0xE7, 0xBE, 0x08, 0x00, 0x00, 0x00, 0xFF, 0x90, 0x18, 0x02, 0x00,
            0x00, 0x49, 0x8B, 0x7C, 0x24, 0x18, 0x48, 0x8B, 0x07, 0xFF, 0x90, 0xC0, 0x03, 0x00, 0x00, 0x0F, 0xBA, 0xE0,
            0x0A, 0x73, 0x12, 0x49, 0x8B, 0x04, 0x24, 0x4C, 0x89, 0xE7, 0xBE, 0x10, 0x00, 0x00, 0x00, 0xFF, 0x90, 0x18,
            0x02, 0x00, 0x00, 0x49, 0x8B, 0x7C, 0x24, 0x18, 0x48, 0x8B, 0x07, 0xFF, 0x90, 0xC0, 0x03, 0x00, 0x00};
        constexpr uint8_t repl_hwchannel_init2[] = {0xFF, 0x90, 0xC8, 0x03, 0x00, 0x00, 0xA8, 0x01, 0x74, 0x12, 0x49,
            0x8B, 0x04, 0x24, 0x4C, 0x89, 0xE7, 0xBE, 0x01, 0x00, 0x00, 0x00, 0xFF, 0x90, 0x18, 0x02, 0x00, 0x00, 0x49,
            0x8B, 0x7C, 0x24, 0x18, 0x48, 0x8B, 0x07, 0xFF, 0x90, 0xC8, 0x03, 0x00, 0x00, 0xA8, 0x02, 0x74, 0x12, 0x49,
            0x8B, 0x04, 0x24, 0x4C, 0x89, 0xE7, 0xBE, 0x02, 0x00, 0x00, 0x00, 0xFF, 0x90, 0x18, 0x02, 0x00, 0x00, 0x49,
            0x8B, 0x7C, 0x24, 0x18, 0x48, 0x8B, 0x07, 0xFF, 0x90, 0xC8, 0x03, 0x00, 0x00, 0x0F, 0xBA, 0xE0, 0x0B, 0x73,
            0x12, 0x49, 0x8B, 0x04, 0x24, 0x4C, 0x89, 0xE7, 0xBE, 0x08, 0x00, 0x00, 0x00, 0xFF, 0x90, 0x18, 0x02, 0x00,
            0x00, 0x49, 0x8B, 0x7C, 0x24, 0x18, 0x48, 0x8B, 0x07, 0xFF, 0x90, 0xC8, 0x03, 0x00, 0x00, 0x0F, 0xBA, 0xE0,
            0x0A, 0x73, 0x12, 0x49, 0x8B, 0x04, 0x24, 0x4C, 0x89, 0xE7, 0xBE, 0x10, 0x00, 0x00, 0x00, 0xFF, 0x90, 0x18,
            0x02, 0x00, 0x00, 0x49, 0x8B, 0x7C, 0x24, 0x18, 0x48, 0x8B, 0x07, 0xFF, 0x90, 0xC8, 0x03, 0x00, 0x00};
        static_assert(sizeof(find_hwchannel_init2) == sizeof(repl_hwchannel_init2), "Find/replace size mismatch");

        constexpr uint8_t find_hwchannel_submitCommandBuffer[] = {0x48, 0x8B, 0x7B, 0x18, 0x48, 0x8B, 0x07, 0xFF, 0x90,
            0x30, 0x02, 0x00, 0x00, 0x48, 0x8B, 0x43, 0x50};
        constexpr uint8_t repl_hwchannel_submitCommandBuffer[] = {0x48, 0x8B, 0x7B, 0x18, 0x48, 0x8B, 0x07, 0x90, 0x90,
            0x90, 0x90, 0x90, 0x90, 0x48, 0x8B, 0x43, 0x50};
        static_assert(sizeof(find_hwchannel_submitCommandBuffer) == sizeof(repl_hwchannel_submitCommandBuffer),
            "Find/replace size mismatch");

        constexpr uint8_t find_hwchannel_waitForHwStamp[] = {0x49, 0x8B, 0x7D, 0x18, 0x48, 0x8B, 0x07, 0xFF, 0x90, 0xA0,
            0x02, 0x00, 0x00, 0x84, 0xC0, 0x74, 0x2E, 0x44, 0x39, 0xFB};
        constexpr uint8_t repl_hwchannel_waitForHwStamp[] = {0x49, 0x8B, 0x7D, 0x18, 0x48, 0x8B, 0x07, 0xFF, 0x90, 0x98,
            0x02, 0x00, 0x00, 0x84, 0xC0, 0x74, 0x2E, 0x44, 0x39, 0xFB};
        static_assert(sizeof(find_hwchannel_waitForHwStamp) == sizeof(repl_hwchannel_waitForHwStamp),
            "Find/replace size mismatch");

        constexpr uint8_t find_hwchannel_reset[] = {0x48, 0x8B, 0x7B, 0x18, 0x48, 0x8B, 0x07, 0xFF, 0x90, 0xB8, 0x03,
            0x00, 0x00, 0x49, 0x89, 0xC6, 0x48, 0x8B, 0x03};
        constexpr uint8_t repl_hwchannel_reset[] = {0x48, 0x8B, 0x7B, 0x18, 0x48, 0x8B, 0x07, 0xFF, 0x90, 0xC0, 0x03,
            0x00, 0x00, 0x49, 0x89, 0xC6, 0x48, 0x8B, 0x03};
        static_assert(sizeof(find_hwchannel_reset) == sizeof(repl_hwchannel_reset), "Find/replace size mismatch");

        constexpr uint8_t find_hwchannel_timestampUpdated1[] = {0x74, 0x20, 0x48, 0x8B, 0x7B, 0x18, 0x48, 0x8B, 0x07,
            0xFF, 0x90, 0xB8, 0x03, 0x00, 0x00, 0x48, 0x8B, 0xB3, 0xC8, 0x00, 0x00, 0x00};
        constexpr uint8_t repl_hwchannel_timestampUpdated1[] = {0x74, 0x20, 0x48, 0x8B, 0x7B, 0x18, 0x48, 0x8B, 0x07,
            0xFF, 0x90, 0xC0, 0x03, 0x00, 0x00, 0x48, 0x8B, 0xB3, 0xC8, 0x00, 0x00, 0x00};
        static_assert(sizeof(find_hwchannel_timestampUpdated1) == sizeof(repl_hwchannel_timestampUpdated1),
            "Find/replace size mismatch");

        constexpr uint8_t find_hwchannel_timestampUpdated2[] = {0x48, 0x8B, 0x7B, 0x18, 0x48, 0x8B, 0x07, 0xFF, 0x90,
            0xB8, 0x03, 0x00, 0x00, 0x49, 0x8B, 0xB6, 0x50, 0x03, 0x00, 0x00, 0x48, 0x89, 0xC7};
        constexpr uint8_t repl_hwchannel_timestampUpdated2[] = {0x48, 0x8B, 0x7B, 0x18, 0x48, 0x8B, 0x07, 0xFF, 0x90,
            0xC0, 0x03, 0x00, 0x00, 0x49, 0x8B, 0xB6, 0x50, 0x03, 0x00, 0x00, 0x48, 0x89, 0xC7};
        static_assert(sizeof(find_hwchannel_timestampUpdated2) == sizeof(repl_hwchannel_timestampUpdated2),
            "Find/replace size mismatch");

        constexpr uint8_t find_hwchannel_enableTimestampInterrupt[] = {0x85, 0xC0, 0x74, 0x14, 0x48, 0x8B, 0x7B, 0x18,
            0x48, 0x8B, 0x07, 0xFF, 0x90, 0xA0, 0x02, 0x00, 0x00, 0x41, 0x89, 0xC6, 0x41, 0x80, 0xF6, 0x01};
        constexpr uint8_t repl_hwchannel_enableTimestampInterrupt[] = {0x85, 0xC0, 0x74, 0x14, 0x48, 0x8B, 0x7B, 0x18,
            0x48, 0x8B, 0x07, 0xFF, 0x90, 0x98, 0x02, 0x00, 0x00, 0x41, 0x89, 0xC6, 0x41, 0x80, 0xF6, 0x01};
        static_assert(sizeof(find_hwchannel_enableTimestampInterrupt) ==
                          sizeof(repl_hwchannel_enableTimestampInterrupt),
            "Find/replace size mismatch");

        constexpr uint8_t find_hwchannel_writeDiagnosisReport[] = {0x49, 0x8B, 0x7C, 0x24, 0x18, 0x48, 0x8B, 0x07, 0xFF,
            0x90, 0xB8, 0x03, 0x00, 0x00, 0x49, 0x8B, 0xB4, 0x24, 0xC8, 0x00, 0x00, 0x00, 0xB9, 0x01, 0x00, 0x00, 0x00};
        constexpr uint8_t repl_hwchannel_writeDiagnosisReport[] = {0x49, 0x8B, 0x7C, 0x24, 0x18, 0x48, 0x8B, 0x07, 0xFF,
            0x90, 0xC0, 0x03, 0x00, 0x00, 0x49, 0x8B, 0xB4, 0x24, 0xC8, 0x00, 0x00, 0x00, 0xB9, 0x01, 0x00, 0x00, 0x00};
        static_assert(sizeof(find_hwchannel_writeDiagnosisReport) == sizeof(repl_hwchannel_writeDiagnosisReport),
            "Find/replace size mismatch");

        constexpr uint8_t find_setupAndInitializeHWCapabilities_pt1[] = {0x4C, 0x89, 0xF7, 0xFF, 0x90, 0xA0, 0x02, 0x00,
            0x00, 0x84, 0xC0, 0x0F, 0x84, 0x6E, 0x02, 0x00, 0x00};
        constexpr uint8_t repl_setupAndInitializeHWCapabilities_pt1[] = {0x4C, 0x89, 0xF7, 0xFF, 0x90, 0x98, 0x02, 0x00,
            0x00, 0x84, 0xC0, 0x0F, 0x84, 0x6E, 0x02, 0x00, 0x00};
        static_assert(sizeof(find_setupAndInitializeHWCapabilities_pt1) ==
                          sizeof(repl_setupAndInitializeHWCapabilities_pt1),
            "Find/replace size mismatch");

        constexpr uint8_t find_setupAndInitializeHWCapabilities_pt2[] = {0xFF, 0x50, 0x70, 0x85, 0xC0, 0x74, 0x0A, 0x41,
            0xC6, 0x46, 0x28, 0x00, 0xE9, 0xB0, 0x01, 0x00, 0x00};
        constexpr uint8_t repl_setupAndInitializeHWCapabilities_pt2[] = {0x66, 0x90, 0x90, 0x85, 0xC0, 0x66, 0x90, 0x41,
            0xC6, 0x46, 0x28, 0x00, 0xE9, 0xB0, 0x01, 0x00, 0x00};
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
            /** Mismatched VTable Call to getScheduler. */
            {&kextRadeonX6000, find_hwchannel_init1, repl_hwchannel_init1, arrsize(find_hwchannel_init1), 1},
            /** Mismatched VTable Calls to getGpuDebugPolicy. */
            {&kextRadeonX6000, find_hwchannel_init2, repl_hwchannel_init2, arrsize(find_hwchannel_init2), 1},
            /**
             * VTable Call to signalGPUWorkSubmitted.
             * Doesn't exist on X5000, but looks like it isn't necessary,
             * so we just NO-OP it.
             */
            {&kextRadeonX6000, find_hwchannel_submitCommandBuffer, repl_hwchannel_submitCommandBuffer,
                arrsize(find_hwchannel_submitCommandBuffer), 1},
            /** Mismatched VTable Call to isDeviceValid. */
            {&kextRadeonX6000, find_hwchannel_waitForHwStamp, repl_hwchannel_waitForHwStamp,
                arrsize(find_hwchannel_waitForHwStamp), 1},
            /** Mismatched VTable Call to getScheduler. */
            {&kextRadeonX6000, find_hwchannel_reset, repl_hwchannel_reset, arrsize(find_hwchannel_reset), 1},
            /** Mismatched VTable Calls to getScheduler. */
            {&kextRadeonX6000, find_hwchannel_timestampUpdated1, repl_hwchannel_timestampUpdated1,
                arrsize(find_hwchannel_timestampUpdated1), 1},
            {&kextRadeonX6000, find_hwchannel_timestampUpdated2, repl_hwchannel_timestampUpdated2,
                arrsize(find_hwchannel_timestampUpdated2), 1},
            /** Mismatched VTable Call to isDeviceValid. */
            {&kextRadeonX6000, find_hwchannel_enableTimestampInterrupt, repl_hwchannel_enableTimestampInterrupt,
                arrsize(find_hwchannel_enableTimestampInterrupt), 1},
            /** Mismatched VTable Call to getScheduler. */
            {&kextRadeonX6000, find_hwchannel_writeDiagnosisReport, repl_hwchannel_writeDiagnosisReport,
                arrsize(find_hwchannel_writeDiagnosisReport), 1},
            /** Mismatched VTable Call to isDeviceValid. */
            {&kextRadeonX6000, find_setupAndInitializeHWCapabilities_pt1, repl_setupAndInitializeHWCapabilities_pt1,
                arrsize(find_setupAndInitializeHWCapabilities_pt1), 1},
            /** Remove call to TTL. */
            {&kextRadeonX6000, find_setupAndInitializeHWCapabilities_pt2, repl_setupAndInitializeHWCapabilities_pt2,
                arrsize(find_setupAndInitializeHWCapabilities_pt2), 1},
        };
        for (auto &patch : patches) {
            patcher.applyLookupPatch(&patch);
            patcher.clearError();
        }

        return true;
    } else if (kextIOAcceleratorFamily2.loadIndex == index) {
        // KernelPatcher::RouteRequest requests[] = {};

        // if (!patcher.routeMultipleLong(index, requests, address, size)) {
        //     panic("RAD: Failed to route IOAcceleratorFamily2 symbols");
        // }

        return true;
    }

    return false;
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
                    else {
                        DBGLOG("rad", "prop %s was not merged due to no value", name);
                    }
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
