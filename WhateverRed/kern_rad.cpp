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
static const char *pathAMD10000Controller = "/System/Library/Extensions/AMD10000Controller.kext/Contents/MacOS/"
                                            "AMD10000Controller";

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
static KernelPatcher::KextInfo kextAMD10000Controller {
    "com.apple.kext.AMD10000Controller",
    &pathAMD10000Controller,
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

/**
 *  Power-gating flags
 *  Each symbol corresponds to a bit provided in a radpg argument mask
 */
static const char *powerGatingFlags[] = {
    "CAIL_DisableDrmdmaPowerGating",
    "CAIL_DisableGfxCGPowerGating",
    "CAIL_DisableUVDPowerGating",
    "CAIL_DisableVCEPowerGating",
    "CAIL_DisableDynamicGfxMGPowerGating",
    "CAIL_DisableGmcPowerGating",
    "CAIL_DisableAcpPowerGating",
    "CAIL_DisableSAMUPowerGating",
};

RAD *RAD::callbackRAD;

void RAD::init() {
    callbackRAD = this;

    currentPropProvider.init();

    force24BppMode = checkKernelArgument("-rad24");

    if (force24BppMode) lilu.onKextLoadForce(&kextRadeonFramebuffer);

    dviSingleLink = checkKernelArgument("-raddvi");

    lilu.onKextLoadForce(&kextAMD10000Controller);
    lilu.onKextLoadForce(&kextRadeonSupport);
    lilu.onKextLoadForce(&kextRadeonX5000HWLibs);
    lilu.onKextLoadForce(&kextRadeonX5000);
    lilu.onKextLoadForce(&kextRadeonX6000);

    // FIXME: autodetect?
    uint32_t powerGatingMask = 0;
    PE_parse_boot_argn("radpg", &powerGatingMask, sizeof(powerGatingMask));
    for (size_t i = 0; i < arrsize(powerGatingFlags); i++) {
        if (!(powerGatingMask & (1 << i))) {
            DBGLOG("rad", "not enabling %s", powerGatingFlags[i]);
            powerGatingFlags[i] = nullptr;
        } else {
            DBGLOG("rad", "enabling %s", powerGatingFlags[i]);
        }
    }
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
    uint8_t revision;
    uint8_t checksum;
    char oemId[6];
    char oemTableId[8];
    uint32_t oemRevision;
    char creatorId[4];
    uint32_t creatorRevision;
    char tableUUID[16];
    uint32_t vbiosImageOffset;
    uint32_t lib1ImageOffset;
    uint32_t reserved[4];
};

struct GOPVideoBIOSHeader {
    uint32_t pciBus;
    uint32_t pciDevice;
    uint32_t pciFunction;
    uint16_t vendorID;
    uint16_t deviceID;
    uint16_t ssvId;
    uint16_t ssId;
    uint32_t revision;
    uint32_t imageLength;
};
#pragma pack(pop)

// Hack
class AppleACPIPlatformExpert : IOACPIPlatformExpert {
    friend class RAD;
};

void RAD::wrapAmdTtlServicesConstructor(IOService *that, IOPCIDevice *provider) {
    NETDBG::enabled = true;
    NETLOG("rad", "patching device type table");
    WIOKit::renameDevice(provider, "GFX0");
    if (MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) != KERN_SUCCESS) {
        panic("Failed to enable kernel writing.");
    }
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

uint64_t RAD::wrapIpiSmuSwInit(void *tlsInstance) {
    NETLOG("rad", "_ipi_smu_sw_init: tlsInstance = %p", tlsInstance);
    auto ret = FunctionCast(wrapIpiSmuSwInit, callbackRAD->orgIpiSmuSwInit)(tlsInstance);
    NETLOG("rad", "_ipi_smu_sw_init returned 0x%llX", ret);
    return ret;
}

uint64_t RAD::wrapSmuSwInit(void *input, uint64_t *output) {
    NETLOG("rad", "_smu_sw_init: input = %p output = %p", input, output);
    auto ret = FunctionCast(wrapSmuSwInit, callbackRAD->orgSmuSwInit)(input, output);
    NETLOG("rad", "_smu_sw_init: output 0:0x%llX 1:0x%llX", output[0], output[1]);
    NETLOG("rad", "_smu_sw_init returned 0x%llX", ret);
    return ret;
}

uint32_t RAD::wrapSmuInternalSwInit(uint64_t param1, uint64_t param2, void *param3) {
    NETLOG("rad", "_smu_internal_sw_init: param1 = 0x%llX param2 = 0x%llX param3 = %p", param1, param2, param3);
    auto ret = FunctionCast(wrapSmuInternalSwInit, callbackRAD->orgSmuInternalSwInit)(param1, param2, param3);
    NETLOG("rad", "_smu_internal_sw_init returned 0x%X", ret);
    return ret;
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
            NETLOG("rad", "Spoofing PSP version v10.x.x to v9.0.2");
            param1[3] = 0x9;
            param1[4] = 0x0;
            param1[5] = 0x2;
            break;
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
        case 0x90000:
            NETLOG("rad", "Spoofing GC version v9.x.x to v9.2.1");
            return 0x90201;
        default:
            return ret;
    }
}

uint32_t RAD::wrapInternalCosReadFw(uint64_t param1, uint64_t *param2) {
    NETLOG("rad", "_internal_cos_read_fw: param1 = 0x%llX param2 = %p", param1, param2);
    auto ret = FunctionCast(wrapInternalCosReadFw, callbackRAD->orgInternalCosReadFw)(param1, param2);
    NETLOG("rad", "_internal_cos_read_fw returned 0x%X", ret);
    return ret;
}

void RAD::wrapPopulateFirmwareDirectory(uint64_t that) {
    NETLOG("rad", "AMDRadeonX5000_AMDRadeonHWLibsX5000::populateFirmwareDirectory this = 0x%llX", that);
    FunctionCast(wrapPopulateFirmwareDirectory, callbackRAD->orgPopulateFirmwareDirectory)(that);
    auto *fwDesc = getFWDescByName("ativvaxy_rv.dat");
    if (!fwDesc) { panic("Somehow ativvaxy_rv.dat is missing"); }
    auto *fwDescBackdoor = getFWDescByName("atidmcub_0.dat");
    if (!fwDescBackdoor) { panic("Somehow atidmcub_0.dat is missing"); }

    auto *fw = callbackRAD->orgCreateFirmware(fwDesc->getBytesNoCopy(), fwDesc->getLength(), 0x200, "ativvaxy_rv.dat");
    auto *fwBackdoor =
        callbackRAD->orgCreateFirmware(fwDesc->getBytesNoCopy(), fwDesc->getLength(), 0x200, "atidmcub_0.dat");
    auto *fwDir = *reinterpret_cast<void **>(that + 0xB8);
    NETLOG("rad", "fwDir = %p", fwDir);
    NETLOG("rad", "inserting ativvaxy_rv.dat!");
    if (!callbackRAD->orgPutFirmware(fwDir, 6, fw)) { panic("Failed to inject ativvaxy_rv.dat firmware"); }
    NETLOG("rad", "inserting atidmcub_0.dat!");
    if (!callbackRAD->orgPutFirmware(fwDir, 6, fwBackdoor)) { panic("Failed to inject atidmcub_0.dat firmware"); }
}

void *RAD::wrapCreateAtomBiosProxy(void *param1) {
    NETLOG("rad", "createAtomBiosProxy: param1 = %p", param1);
    auto ret = FunctionCast(wrapCreateAtomBiosProxy, callbackRAD->orgCreateAtomBiosProxy)(param1);
    NETLOG("rad", "createAtomBiosProxy returned %p", ret);
    return ret;
}

IOReturn RAD::wrapPopulateDeviceMemory(void *that, uint32_t reg) {
    DBGLOG("rad", "populateDeviceMemory: this = %p reg = 0x%X", that, reg);
    auto ret = FunctionCast(wrapPopulateDeviceMemory, callbackRAD->orgPopulateDeviceMemory)(that, reg);
    DBGLOG("rad", "populateDeviceMemory returned 0x%X", ret);
    return kIOReturnSuccess;
}

uint64_t RAD::wrapMCILUpdateGfxCGPG(void *param1) {
    NETLOG("rad", "_Cail_MCILUpdateGfxCGPG: param1 = %p", param1);
    auto ret = FunctionCast(wrapMCILUpdateGfxCGPG, callbackRAD->orgMCILUpdateGfxCGPG)(param1);
    NETLOG("rad", "_Cail_MCILUpdateGfxCGPG returned 0x%llX", ret);
    return ret;
}

IOReturn RAD::wrapQueryEngineRunningState(void *that, void *param1, void *param2) {
    NETLOG("rad", "queryEngineRunningState: this = %p param1 = %p param2 = %p", that, param1, param2);
    NETLOG("rad", "queryEngineRunningState: *param2 = 0x%X", *static_cast<uint32_t *>(param2));
    auto ret = FunctionCast(wrapQueryEngineRunningState, callbackRAD->orgQueryEngineRunningState)(that, param1, param2);
    NETLOG("rad", "queryEngineRunningState: after *param2 = 0x%X", *static_cast<uint32_t *>(param2));
    NETLOG("rad", "queryEngineRunningState returned 0x%X", ret);
    return ret;
}

IOReturn RAD::wrapQueryComputeQueueIsIdle(void *that, uint64_t param1) {
    NETLOG("rad", "QueryComputeQueueIsIdle: this = %p param1 = 0x%llX", that, param1);
    auto ret = FunctionCast(wrapQueryComputeQueueIsIdle, callbackRAD->orgQueryComputeQueueIsIdle)(that, param1);
    NETLOG("rad", "QueryComputeQueueIsIdle returned 0x%X", ret);
    return ret;
}

uint64_t RAD::wrapCAILQueryEngineRunningState(void *param1, uint32_t *param2, uint64_t param3) {
    NETLOG("rad", "_CAILQueryEngineRunningState: param1 = %p param2 = %p param3 = %llX", param1, param2, param3);
    NETLOG("rad", "_CAILQueryEngineRunningState: *param2 = 0x%X", *param2);
    auto ret = FunctionCast(wrapCAILQueryEngineRunningState, callbackRAD->orgCAILQueryEngineRunningState)(param1,
        param2, param3);
    NETLOG("rad", "_CAILQueryEngineRunningState: after *param2 = 0x%X", *param2);
    NETLOG("rad", "_CAILQueryEngineRunningState returned 0x%llX", ret);
    return ret;
}

uint64_t RAD::wrapCailMonitorEngineInternalState(void *that, uint32_t param1, uint32_t *param2) {
    NETLOG("rad", "_CailMonitorEngineInternalState: this = %p param1 = 0x%X param2 = %p", that, param1, param2);
    NETLOG("rad", "_CailMonitorEngineInternalState: *param2 = 0x%X", *param2);
    auto ret = FunctionCast(wrapCailMonitorEngineInternalState, callbackRAD->orgCailMonitorEngineInternalState)(that,
        param1, param2);
    NETLOG("rad", "_CailMonitorEngineInternalState: after *param2 = 0x%X", *param2);
    NETLOG("rad", "_CailMonitorEngineInternalState returned 0x%llX", ret);
    return ret;
}

uint64_t RAD::wrapCailMonitorPerformanceCounter(void *that, uint32_t *param1) {
    NETLOG("rad", "_CailMonitorPerformanceCounter: this = %p param1 = %p", that, param1);
    NETLOG("rad", "_CailMonitorPerformanceCounter: *param1 = 0x%X", *param1);
    auto ret =
        FunctionCast(wrapCailMonitorPerformanceCounter, callbackRAD->orgCailMonitorPerformanceCounter)(that, param1);
    NETLOG("rad", "_CailMonitorPerformanceCounter: after *param1 = 0x%X", *param1);
    NETLOG("rad", "_CailMonitorPerformanceCounter returned 0x%llX", ret);
    return ret;
}

bool RAD::wrapAMDHWChannelWaitForIdle(void *that, uint64_t param1) {
    NETLOG("rad", "AMDRadeonX5000_AMDHWChannel::waitForIdle: this = %p param1 = 0x%llx", that, param1);
    auto ret = FunctionCast(wrapAMDHWChannelWaitForIdle, callbackRAD->orgAMDHWChannelWaitForIdle)(that, param1);
    NETLOG("rad", "AMDRadeonX5000_AMDHWChannel::waitForIdle returned %d", ret);
    return ret;
}

uint64_t RAD::wrapSMUMInitialize(uint64_t param1, uint32_t *param2, uint64_t param3) {
    NETLOG("rad", "_SMUM_Initialize: param1 = 0x%llX param2 = %p param3 = 0x%llX", param1, param2, param3);
    auto ret = FunctionCast(wrapSMUMInitialize, callbackRAD->orgSMUMInitialize)(param1, param2, param3);
    NETLOG("rad", "_SMUM_Initialize returned 0x%llX", ret);
    return ret;
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
    return 0x8e;
}

static uint16_t emulatedRevisionOff(uint16_t revision, uint16_t deviceId) {
    /**
     * Emulated Revision = Revision + Enumerated Revision.
     */
    switch (deviceId) {
        case 0x15d8:
            return 0x41;
        case 0x15dd:
            if (revision >= 0x8) { return 0x79; }
            return 0x10;
        case 0x15E7:
            [[fallthrough]];
        case 0x164C:
            [[fallthrough]];
        case 0x1636:
            [[fallthrough]];
        case 0x1638:
            return 0x91;
        default:
            if (revision == 1) { return 0x20; }
            return 0x10;
    }
}

IOReturn RAD::wrapPopulateDeviceInfo(uint64_t that) {
    NETLOG("rad", "AMDRadeonX5000_AmdAsicInfoNavi::populateDeviceInfo: this = 0x%llX", that);
    auto ret = FunctionCast(wrapPopulateDeviceInfo, callbackRAD->orgPopulateDeviceInfo)(that);
    auto *pciDev = *reinterpret_cast<IOPCIDevice **>(that + 0x18);
    auto *familyId = reinterpret_cast<uint32_t *>(that + 0x44);
    auto deviceId = pciDev->configRead16(kIOPCIConfigDeviceID);
    auto *revision = reinterpret_cast<uint32_t *>(that + 0x48);
    auto *emulatedRevision = reinterpret_cast<uint32_t *>(that + 0x4c);
    *emulatedRevision = *revision + emulatedRevisionOff(*revision, deviceId);
    NETLOG("rad", "deviceId = 0x%X revision = 0x%X emulatedRevision = 0x%X", deviceId, *revision, *emulatedRevision);
    *familyId = 0x8e;
    NETLOG("rad", "locating Init Caps entry");
    if (MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) != KERN_SUCCESS) {
        panic("Failed to enable kernel writing");
    }
    CailInitAsicCapEntry *initCaps = nullptr;
    for (size_t i = 0; i < 789; i++) {
        auto *temp = callbackRAD->orgAsicInitCapsTable + i;
        if (temp->familyId == 0x8e && temp->deviceId == deviceId && temp->emulatedRev == *emulatedRevision) {
            initCaps = temp;
            break;
        }
    }
    if (!initCaps) {
        NETLOG("rad", "Warning! Using Fallback Init Caps mechanism");
        for (size_t i = 0; i < 789; i++) {
            auto *temp = callbackRAD->orgAsicInitCapsTable + i;
            if (temp->familyId == 0x8e && temp->deviceId == deviceId &&
                (temp->emulatedRev >= emulatedRevisionOff(*revision, deviceId) ||
                    temp->emulatedRev <= *emulatedRevision)) {
                initCaps = temp;
                break;
            }
        }
        if (!initCaps) { panic("rad: Failed to find Init Caps entry for device ID 0x%X", deviceId); }
    }
    callbackRAD->orgAsicCapsTable->familyId = 0x8e;
    callbackRAD->orgAsicCapsTable->deviceId = deviceId;
    callbackRAD->orgAsicCapsTable->revision = *revision;
    callbackRAD->orgAsicCapsTable->pciRev = 0xFFFFFFFF;
    callbackRAD->orgAsicCapsTable->emulatedRev = *emulatedRevision;
    memmove(callbackRAD->orgAsicCapsTable->caps, initCaps->caps, 0x40);
    MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);
    NETLOG("rad", "AMDRadeonX5000_AmdAsicInfoNavi::populateDeviceInfo returned 0x%X", ret);
    return ret;
}

uint64_t RAD::wrapSmuGetFwConstants() {
    /**
     * According to Linux AMDGPU source code,
     * on APUs, the System BIOS is the one that loads the SMC Firmware, and
     * therefore, we must not load any firmware ourselves.
     * We fix that by making _smu_get_fw_constants a no-op.
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

bool RAD::wrapGFX10AcceleratorStart() {
    /**
     * The `Info.plist` contains a personality for the
     * AMD X6000 Accelerator, but we don't want it to do
     * anything. We have it there only so it force-loads
     * the kext and we can resolve the symbols we need.
     */
    return false;
}

bool RAD::wrapAllocateHWEngines(uint64_t that) {
    NETLOG("rad", "allocateHWEngines: this = 0x%llX", that);
    auto *vtable = *reinterpret_cast<mach_vm_address_t **>(that);
    vtable[0x62] = reinterpret_cast<mach_vm_address_t>(wrapGetHWEngine);

    auto *pm4Engine = callbackRAD->orgGFX9PM4EngineNew(0x1e8);
    callbackRAD->orgGFX9PM4EngineConstructor(pm4Engine);
    *reinterpret_cast<void **>(that + 0x3b8) = pm4Engine;

    auto *sdmaEngine = callbackRAD->orgGFX9SDMAEngineNew(0x128);
    callbackRAD->orgGFX9SDMAEngineConstructor(sdmaEngine);
    *reinterpret_cast<void **>(that + 0x3c0) = sdmaEngine;

    auto *sdma1Engine = callbackRAD->orgGFX9SDMAEngineNew(0x128);
    callbackRAD->orgGFX9SDMAEngineConstructor(sdma1Engine);
    *reinterpret_cast<void **>(that + 0x3c8) = sdma1Engine;

    auto *vcn2Engine = callbackRAD->orgGFX10VCN2EngineNew(0x198);
    callbackRAD->orgGFX10VCN2EngineConstructor(vcn2Engine);
    *reinterpret_cast<void **>(that + 0x3f8) = vcn2Engine;
    NETLOG("rad", "allocateHWEngines: returning true");
    return true;
}

void *RAD::wrapGetHWEngine(void *that, uint32_t engineType) {
    NETLOG("rad", "getHWEngine: this = %p engineType = 0x%X", that, engineType);
    auto ret = FunctionCast(wrapGetHWEngine, callbackRAD->orgGetHWEngine)(that, engineType);
    NETLOG("rad", "getHWEngine returned %p", ret);
    return ret;
}

void RAD::wrapSetupAndInitializeHWCapabilities(uint64_t that) {
    NETLOG("rad", "wrapSetupAndInitializeCapabilities: that = 0x%llX", that);
    FunctionCast(wrapSetupAndInitializeHWCapabilities, callbackRAD->orgSetupAndInitializeHWCapabilities)(that);
    *reinterpret_cast<uint32_t *>(that + 0xAC) = 0x0;
    *reinterpret_cast<uint32_t *>(that + 0xAF) = 0x100001;
    NETLOG("rad", "wrapSetupAndInitializeCapabilities: done");
}

bool RAD::wrapPM4EnginePowerUp(void *that) {
    NETLOG("rad", "PM4EnginePowerUp: this = %p", that);
    auto ret = FunctionCast(wrapPM4EnginePowerUp, callbackRAD->orgPM4EnginePowerUp)(that);
    NETLOG("rad", "PM4EnginePowerUp returned %d", ret);
    auto *buf = (char *)IOMallocZero(0x100000);
    auto *bufPtr = buf;
    size_t size = 0xfffff;
    callbackRAD->orgWriteDiagnosisReport(callbackRAD->callbackAccelerator, &bufPtr, &size);
    NETDBG::nprint(buf, strnlen(buf, 0xfffff));
    NETDBG::printf("\n");
    IOFree(buf, 0x100000);
    NETLOG("rad", "PM4EnginePowerUp done");
    IOSleep(3600000);
    return ret;
}

void RAD::wrapDumpASICHangStateCold(uint64_t param1) {
    NETLOG("rad", "dumpASICHangStateCold: param1 = 0x%llX", param1);
    IOSleep(3600000);
    FunctionCast(wrapDumpASICHangStateCold, callbackRAD->orgDumpASICHangStateCold)(param1);
    NETLOG("rad", "dumpASICHangStateCold finished");
}

bool RAD::wrapAccelStart(void *that, IOService *provider) {
    NETLOG("rad", "accelStart: this = %p provider = %p", that, provider);
    callbackRAD->callbackAccelerator = that;
    auto ret = FunctionCast(wrapAccelStart, callbackRAD->orgAccelStart)(that, provider);
    NETLOG("rad", "accelStart returned %d", ret);
    return ret;
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
    auto ret = FunctionCast(wrapHwRegRead, callbackRAD->orgHwRegRead)(that, addr);
    NETLOG("rad", "hwRegRead: this = %p addr = 0x%llX", that, addr);
    NETLOG("rad", "hwRegRead returned 0x%llX", ret);
    return ret;
}

uint64_t RAD::wrapHwRegWrite(void *that, uint64_t addr, uint64_t val) {
    NETLOG("rad", "hwRegWrite: this = %p addr = 0x%llX val = 0x%llX", that, addr, val);
    auto ret = FunctionCast(wrapHwRegWrite, callbackRAD->orgHwRegWrite)(that, addr, val);
    NETLOG("rad", "hwRegWrite returned 0x%llX", ret);
    return ret;
}

bool RAD::wrapCailInitCSBCommandBuffer(void* cailData) {
    panic("__AMDSucksAtCoding");
    NETLOG("rad", "\n\n----------------------------------------------------------------------\n\n");
    NETLOG("rad", "_CailInitCSBCommandBuffer: cailData = %p", cailData);
    auto ret = FunctionCast(wrapCailInitCSBCommandBuffier, callbackRAD->orgCailInitCSBCommandBuffer)(cailData);
    NETLOG("rad", "_CailInitCSBCommandBuffer returned %d", ret);
    NETLOG("rad", "\n\n----------------------------------------------------------------------\n\n");
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
            {"__ZN13AtomBiosProxy19createAtomBiosProxyER16AtomBiosInitData", wrapCreateAtomBiosProxy,
                orgCreateAtomBiosProxy},
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
        KernelPatcher::LookupPatch patch {&kextRadeonSupport, find, repl, arrsize(find), 2};
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
            {"__ZL20CAIL_ASIC_CAPS_TABLE", orgAsicCapsTable},
            {"_CAILAsicCapsInitTable", orgAsicInitCapsTable},
        };
        if (!patcher.solveMultiple(index, solveRequests, address, size)) {
            panic("RAD: Failed to resolve AMDRadeonX5000HWLibs symbols");
        }

        KernelPatcher::RouteRequest requests[] = {
            {"__ZN14AmdTtlServicesC2EP11IOPCIDevice", wrapAmdTtlServicesConstructor, orgAmdTtlServicesConstructor},
            {"_ipi_smu_sw_init", wrapIpiSmuSwInit, orgIpiSmuSwInit},
            {"_smu_sw_init", wrapSmuSwInit, orgSmuSwInit},
            {"_smu_internal_sw_init", wrapSmuInternalSwInit, orgSmuInternalSwInit},
            {"_smu_get_hw_version", wrapSmuGetHwVersion, orgSmuGetHwVersion},
            {"_psp_sw_init", wrapPspSwInit, orgPspSwInit},
            {"_gc_get_hw_version", wrapGcGetHwVersion, orgGcGetHwVersion},
            {"_internal_cos_read_fw", wrapInternalCosReadFw, orgInternalCosReadFw},
            {"__ZN35AMDRadeonX5000_AMDRadeonHWLibsX500025populateFirmwareDirectoryEv", wrapPopulateFirmwareDirectory,
                orgPopulateFirmwareDirectory},
            {"__ZN15AmdCailServices23queryEngineRunningStateEP17CailHwEngineQueueP22CailEngineRunningState",
                wrapQueryEngineRunningState, orgQueryEngineRunningState},
            {"_CAILQueryEngineRunningState", wrapCAILQueryEngineRunningState, orgCAILQueryEngineRunningState},
            {"_CailMonitorEngineInternalState", wrapCailMonitorEngineInternalState, orgCailMonitorEngineInternalState},
            {"_CailMonitorPerformanceCounter", wrapCailMonitorPerformanceCounter, orgCailMonitorPerformanceCounter},
            {"_SMUM_Initialize", wrapSMUMInitialize, orgSMUMInitialize},
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
            {"_CailInitCSBCommandBuffer", wrapCailInitCSBCommandBuffer, orgCailInitCSBCommandBuffer},
        };
        if (!patcher.routeMultipleLong(index, requests, address, size)) {
            panic("RAD: Failed to route AMDRadeonX5000HWLibs symbols");
        }

        uint8_t find_asic_reset[] = {0x55, 0x48, 0x89, 0xe5, 0x8b, 0x56, 0x04, 0xbe, 0x3b, 0x00, 0x00, 0x00, 0x5d, 0xe9,
            0x51, 0xfe, 0xff, 0xff};
        uint8_t repl_asic_reset[] = {0x55, 0x48, 0x89, 0xe5, 0x8b, 0x56, 0x04, 0xbe, 0x1e, 0x00, 0x00, 0x00, 0x5d, 0xe9,
            0x51, 0xfe, 0xff, 0xff};

        uint8_t find_phm_uselegacystatemanager[] = {0x48, 0x89, 0xe5, 0x8b, 0x4f, 0x08, 0x83, 0xc1, 0x92, 0x83, 0xf9,
            0x20, 0x77, 0x21, 0xb8, 0x01, 0x00, 0x00, 0x00};
        uint8_t repl_phm_uselegacystatemanager[] = {0x48, 0x89, 0xe5, 0x8b, 0x4f, 0x08, 0x83, 0xc1, 0x92, 0x83, 0xf9,
            0x20, 0xeb, 0x21, 0xb8, 0x01, 0x00, 0x00, 0x00};

        KernelPatcher::LookupPatch patches[] = {
            /**
             * Patch for _smu_9_0_1_full_asic_reset
             * This function performs a full ASIC reset.
             * The patch corrects the sent message to 0x1E;
             * the original code sends 0x3B, which is wrong for SMU 10.
             */
            {&kextRadeonX5000HWLibs, find_asic_reset, repl_asic_reset, arrsize(find_asic_reset), 2},
            /**
             * Patch for _phm_use_legacy_state_manager
             * This function checks if the legacy state manager should be used.
             * Because the family ID for Raven is 0x8e, the subtraction of 0x6e
             * results in 0x20, and the result is not less than 0x20, it returns true.
             */
            {&kextRadeonX5000HWLibs, find_phm_uselegacystatemanager, repl_phm_uselegacystatemanager,
                arrsize(find_phm_uselegacystatemanager), 2},
        };
        for (auto &patch : patches) {
            patcher.applyLookupPatch(&patch);
            patcher.clearError();
        }

        return true;
    } else if (kextAMD10000Controller.loadIndex == index) {
        DBGLOG("rad", "Hooking AMD10000Controller");

        KernelPatcher::RouteRequest requests[] = {
            {"__ZNK22Vega10SharedController11getFamilyIdEv", wrapGetFamilyId},
            {"__ZN17ASIC_INFO__VEGA1218populateDeviceInfoEv", wrapPopulateDeviceInfo, orgPopulateDeviceInfo},
        };
        if (!patcher.routeMultipleLong(index, requests, address, size)) {
            panic("RAD: Failed to route AMD10000Controller symbols");
        }

        /**
         * Patch for DEVICE_COMPONENT_FACTORY::createAsicInfo
         * Eliminates if statement to allow creation of ASIC Info.
         * Uses Vega 12 branch.
         */
        uint8_t find[] = {0x81, 0xf9, 0xa0, 0x66, 0x00, 0x00, 0x0f, 0x8c, 0x32, 0x00, 0x00, 0x00, 0x0f, 0xb7, 0x45,
            0xe6, 0x3d, 0xbf, 0x66, 0x00, 0x00, 0x0f, 0x8f, 0x23, 0x00, 0x00, 0x00, 0xbf, 0x90, 0x00, 0x00, 0x00, 0xe8,
            0x2e, 0x3d, 0xff, 0xff};
        uint8_t repl[] = {0x81, 0xf9, 0xa0, 0x66, 0x00, 0x00, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x0f, 0xb7, 0x45,
            0xe6, 0x3d, 0xbf, 0x66, 0x00, 0x00, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0xbf, 0x90, 0x00, 0x00, 0x00, 0xe8,
            0x2e, 0x3d, 0xff, 0xff};
        KernelPatcher::LookupPatch patch {&kextAMD10000Controller, find, repl, arrsize(find), 2};
        patcher.applyLookupPatch(&patch);
        patcher.clearError();

        return true;
    } else if (kextRadeonX5000.loadIndex == index) {
        KernelPatcher::SolveRequest solveRequests[] = {
            {"__ZN31AMDRadeonX5000_AMDGFX9PM4EnginenwEm", orgGFX9PM4EngineNew},
            {"__ZN31AMDRadeonX5000_AMDGFX9PM4EngineC1Ev", orgGFX9PM4EngineConstructor},
            {"__ZN32AMDRadeonX5000_AMDGFX9SDMAEnginenwEm", orgGFX9SDMAEngineNew},
            {"__ZN32AMDRadeonX5000_AMDGFX9SDMAEngineC1Ev", orgGFX9SDMAEngineConstructor},
            {"__ZN37AMDRadeonX5000_AMDGraphicsAccelerator20writeDiagnosisReportERPcRj", orgWriteDiagnosisReport},
        };
        if (!patcher.solveMultiple(index, solveRequests, address, size)) {
            panic("RAD: Failed to resolve AMDRadeonX5000 symbols");
        }

        KernelPatcher::RouteRequest requests[] = {
            {"__ZN27AMDRadeonX5000_AMDHWHandler8getStateEv", wrapGetState, orgGetState},
            {"__ZN28AMDRadeonX5000_AMDRTHardware13initializeTtlEP16_GART_PARAMETERS", wrapInitializeTtl,
                orgInitializeTtl},
            {"__ZN31AMDRadeonX5000_AMDGFX9PM4Engine23QueryComputeQueueIsIdleE18_eAMD_HW_RING_TYPE",
                wrapQueryComputeQueueIsIdle, orgQueryComputeQueueIsIdle},
            {"__ZN27AMDRadeonX5000_AMDHWChannel11waitForIdleEj", wrapAMDHWChannelWaitForIdle,
                orgAMDHWChannelWaitForIdle},
            {"__ZN32AMDRadeonX5000_AMDVega12Hardware17allocateHWEnginesEv", wrapAllocateHWEngines},
            {"__ZN26AMDRadeonX5000_AMDHardware11getHWEngineE20_eAMD_HW_ENGINE_TYPE", wrapGetHWEngine, orgGetHWEngine},
            {"__ZN32AMDRadeonX5000_AMDVega12Hardware32setupAndInitializeHWCapabilitiesEv",
                wrapSetupAndInitializeHWCapabilities, orgSetupAndInitializeHWCapabilities},
            {"__ZN31AMDRadeonX5000_AMDGFX9PM4Engine7powerUpEv", wrapPM4EnginePowerUp, orgPM4EnginePowerUp},
            {"__ZN26AMDRadeonX5000_AMDHardware17dumpASICHangStateEb.cold.1", wrapDumpASICHangStateCold,
                orgDumpASICHangStateCold},
            {"__ZN37AMDRadeonX5000_AMDGraphicsAccelerator5startEP9IOService", wrapAccelStart, orgAccelStart},
            {"__ZN24AMDRadeonX5000_AMDRTRing7getHeadEv", wrapGFX9RTRingGetHead, orgGFX9RTRingGetHead},
            {"__ZN29AMDRadeonX5000_AMDHWRegisters4readEj", wrapHwRegRead, orgHwRegRead},
            {"__ZN29AMDRadeonX5000_AMDHWRegisters5writeEjj", wrapHwRegWrite, orgHwRegWrite},
        };
        if (!patcher.routeMultipleLong(index, requests, address, size)) {
            panic("RAD: Failed to route AMDRadeonX5000 symbols");
        }

        return true;
    } else if (kextRadeonX6000.loadIndex == index) {
        KernelPatcher::SolveRequest solveRequests[] = {
            {"__ZN30AMDRadeonX6000_AMDVCN2HWEnginenwEm", orgGFX10VCN2EngineNew},
            {"__ZN30AMDRadeonX6000_AMDVCN2HWEngineC1Ev", orgGFX10VCN2EngineConstructor},
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

        uint8_t find_hwchannel_submitCommandBuffer[] = {0x48, 0x8b, 0x7b, 0x18, 0x48, 0x8b, 0x07, 0xff, 0x90, 0x30,
            0x02, 0x00, 0x00, 0x48, 0x8b, 0x43, 0x50};
        uint8_t repl_hwchannel_submitCommandBuffer[] = {0x48, 0x8b, 0x7b, 0x18, 0x48, 0x8b, 0x07, 0x90, 0x90, 0x90,
            0x90, 0x90, 0x90, 0x48, 0x8b, 0x43, 0x50};

        uint8_t find_hwchannel_waitForHwStamp[] = {0x49, 0x8b, 0x7d, 0x18, 0x48, 0x8b, 0x07, 0xff, 0x90, 0xa0, 0x02,
            0x00, 0x00, 0x84, 0xc0, 0x74, 0x2e, 0x44, 0x39, 0xfb};
        uint8_t repl_hwchannel_waitForHwStamp[] = {0x49, 0x8b, 0x7d, 0x18, 0x48, 0x8b, 0x07, 0xff, 0x90, 0x98, 0x02,
            0x00, 0x00, 0x84, 0xc0, 0x74, 0x2e, 0x44, 0x39, 0xfb};

        uint8_t find_hwchannel_reset[] = {0x48, 0x8b, 0x7b, 0x18, 0x48, 0x8b, 0x07, 0xff, 0x90, 0xb8, 0x03, 0x00, 0x00,
            0x49, 0x89, 0xc6, 0x48, 0x8b, 0x03};
        uint8_t repl_hwchannel_reset[] = {0x48, 0x8b, 0x7b, 0x18, 0x48, 0x8b, 0x07, 0xff, 0x90, 0xc0, 0x03, 0x00, 0x00,
            0x49, 0x89, 0xc6, 0x48, 0x8b, 0x03};

        uint8_t find_hwchannel_timestampUpdated1[] = {0x74, 0x20, 0x48, 0x8b, 0x7b, 0x18, 0x48, 0x8b, 0x07, 0xff, 0x90,
            0xb8, 0x03, 0x00, 0x00, 0x48, 0x8b, 0xb3, 0xc8, 0x00, 0x00, 0x00};
        uint8_t repl_hwchannel_timestampUpdated1[] = {0x74, 0x20, 0x48, 0x8b, 0x7b, 0x18, 0x48, 0x8b, 0x07, 0xff, 0x90,
            0xc0, 0x03, 0x00, 0x00, 0x48, 0x8b, 0xb3, 0xc8, 0x00, 0x00, 0x00};
        uint8_t find_hwchannel_timestampUpdated2[] = {0x48, 0x8b, 0x7b, 0x18, 0x48, 0x8b, 0x07, 0xff, 0x90, 0xb8, 0x03,
            0x00, 0x00, 0x49, 0x8b, 0xb6, 0x50, 0x03, 0x00, 0x00, 0x48, 0x89, 0xc7};
        uint8_t repl_hwchannel_timestampUpdated2[] = {0x48, 0x8b, 0x7b, 0x18, 0x48, 0x8b, 0x07, 0xff, 0x90, 0xc0, 0x03,
            0x00, 0x00, 0x49, 0x8b, 0xb6, 0x50, 0x03, 0x00, 0x00, 0x48, 0x89, 0xc7};

        uint8_t find_hwchannel_enableTimestampInterrupt[] = {0x85, 0xc0, 0x74, 0x14, 0x48, 0x8b, 0x7b, 0x18, 0x48, 0x8b,
            0x07, 0xff, 0x90, 0xa0, 0x02, 0x00, 0x00, 0x41, 0x89, 0xc6, 0x41, 0x80, 0xf6, 0x01};
        uint8_t repl_hwchannel_enableTimestampInterrupt[] = {0x85, 0xc0, 0x74, 0x14, 0x48, 0x8b, 0x7b, 0x18, 0x48, 0x8b,
            0x07, 0xff, 0x90, 0x98, 0x02, 0x00, 0x00, 0x41, 0x89, 0xc6, 0x41, 0x80, 0xf6, 0x01};

        uint8_t find_hwchannel_writeDiagnosisReport[] = {0x49, 0x8b, 0x7c, 0x24, 0x18, 0x48, 0x8b, 0x07, 0xff, 0x90,
            0xb8, 0x03, 0x00, 0x00, 0x49, 0x8b, 0xb4, 0x24, 0xc8, 0x00, 0x00, 0x00, 0xb9, 0x01, 0x00, 0x00, 0x00};
        uint8_t repl_hwchannel_writeDiagnosisReport[] = {0x49, 0x8b, 0x7c, 0x24, 0x18, 0x48, 0x8b, 0x07, 0xff, 0x90,
            0xc0, 0x03, 0x00, 0x00, 0x49, 0x8b, 0xb4, 0x24, 0xc8, 0x00, 0x00, 0x00, 0xb9, 0x01, 0x00, 0x00, 0x00};

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
            {&kextRadeonX6000, find_hwchannel_init1, repl_hwchannel_init1, arrsize(find_hwchannel_init1), 2},
            /**
             * Mismatched VTable Calls to getGpuDebugPolicy.
             */
            {&kextRadeonX6000, find_hwchannel_init2, repl_hwchannel_init2, arrsize(find_hwchannel_init2), 2},
            /**
             * VTable Call to signalGPUWorkSubmitted.
             * Doesn't exist on X5000, but looks like it isn't necessary,
             * so we just NO-OP it.
             */
            {&kextRadeonX6000, find_hwchannel_submitCommandBuffer, repl_hwchannel_submitCommandBuffer,
                arrsize(find_hwchannel_submitCommandBuffer), 2},
            /**
             * Mismatched VTable Call to isDeviceValid.
             */
            {&kextRadeonX6000, find_hwchannel_waitForHwStamp, repl_hwchannel_waitForHwStamp,
                arrsize(find_hwchannel_waitForHwStamp), 2},
            /**
             * Mismatched VTable Call to getScheduler.
             */
            {&kextRadeonX6000, find_hwchannel_reset, repl_hwchannel_reset, arrsize(find_hwchannel_reset), 2},
            /**
             * Mismatched VTable Calls to getScheduler.
             */
            {&kextRadeonX6000, find_hwchannel_timestampUpdated1, repl_hwchannel_timestampUpdated1,
                arrsize(find_hwchannel_timestampUpdated1), 2},
            {&kextRadeonX6000, find_hwchannel_timestampUpdated2, repl_hwchannel_timestampUpdated2,
                arrsize(find_hwchannel_timestampUpdated2), 2},
            /**
             * Mismatched VTable Call to isDeviceValid.
             */
            {&kextRadeonX6000, find_hwchannel_enableTimestampInterrupt, repl_hwchannel_enableTimestampInterrupt,
                arrsize(find_hwchannel_enableTimestampInterrupt), 2},
            /**
             * Mismatched VTable Call to getScheduler.
             */
            {&kextRadeonX6000, find_hwchannel_writeDiagnosisReport, repl_hwchannel_writeDiagnosisReport,
                arrsize(find_hwchannel_writeDiagnosisReport), 2},
        };
        for (auto &patch : patches) {
            patcher.applyLookupPatch(&patch);
            patcher.clearError();
        }

        return true;
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

uint64_t RAD::wrapGetState(void *that) {
    DBGLOG("rad", "getState this = %p", that);
    auto ret = FunctionCast(wrapGetState, callbackRAD->orgGetState)(that);
    DBGLOG("rad", "getState returned 0x%llX", ret);
    return ret;
}

bool RAD::wrapInitializeTtl(void *that, void *param1) {
    NETLOG("rad", "initializeTtl this = %p", that);
    auto ret = FunctionCast(wrapInitializeTtl, callbackRAD->orgInitializeTtl)(that, param1);
    NETLOG("rad", "initializeTtl returned %d", ret);
    return ret;
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

    if (!strcmp(prefix, "CAIL,")) {
        for (size_t i = 0; i < arrsize(powerGatingFlags); i++) {
            if (powerGatingFlags[i] && props->getObject(powerGatingFlags[i])) {
                DBGLOG("rad", "cail prop merge found %s, replacing", powerGatingFlags[i]);
                auto num = OSNumber::withNumber(1, 32);
                if (num) {
                    props->setObject(powerGatingFlags[i], num);
                    num->release();
                }
            }
        }
    }
}

void RAD::applyPropertyFixes(IOService *service, uint32_t connectorNum) {
    if (!service->getProperty("CFG,CFG_FB_LIMIT")) {
        DBGLOG("rad", "setting fb limit to %u", connectorNum);
        service->setProperty("CFG_FB_LIMIT", connectorNum, 32);
    }
}

uint32_t RAD::wrapGetConnectorsInfoV1(void *that, RADConnectors::Connector *connectors, uint8_t *sz) {
    NETLOG("rad", "getConnectorsInfoV1: that = %p connectors = %p sz = %p", that, connectors, sz);
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
    NETLOG("rad", "getConnectorsInfoV2: that = %p connectors = %p sz = %p", that, connectors, sz);
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
