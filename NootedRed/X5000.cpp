// Copyright © 2022-2024 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#include "X5000.hpp"
#include "Firmware.hpp"
#include "Headers/kern_util.hpp"
#include "NRed.hpp"
#include "PatcherPlus.hpp"
#include "X6000.hpp"
#include <Headers/kern_api.hpp>

static const char *pathRadeonX5000 = "/System/Library/Extensions/AMDRadeonX5000.kext/Contents/MacOS/AMDRadeonX5000";

static KernelPatcher::KextInfo kextRadeonX5000 {
    "com.apple.kext.AMDRadeonX5000",
    &pathRadeonX5000,
    1,
    {true},
    {},
    KernelPatcher::KextInfo::Unloaded,
};

X5000 *X5000::callback = nullptr;

void X5000::init() {
    switch (getKernelVersion()) {
        case KernelVersion::Catalina:
            this->pm4EngineField = 0x348;
            this->sdma0EngineField = 0x350;
            this->displayPipeCountField = 0x2C;
            this->seCountField = 0x58;
            this->shPerSEField = 0x5C;
            this->cuPerSHField = 0x80;
            this->hasUVD0Field = 0x90;
            this->hasVCEField = 0x92;
            this->hasVCN0Field = 0x93;
            this->hasSDMAPagingQueueField = 0xA4;
            this->familyTypeField = 0x2B4;
            this->chipSettingsField = 0x5B18;
            break;
        case KernelVersion::BigSur:
        case KernelVersion::Monterey:
            this->pm4EngineField = 0x3B8;
            this->sdma0EngineField = 0x3C0;
            this->displayPipeCountField = 0x2C;
            this->seCountField = 0x5C;
            this->shPerSEField = 0x64;
            this->cuPerSHField = 0x98;
            this->hasUVD0Field = 0xAC;
            this->hasVCEField = 0xAE;
            this->hasVCN0Field = 0xAF;
            this->hasSDMAPagingQueueField = 0xC0;
            this->familyTypeField = 0x308;
            this->chipSettingsField = 0x5B10;
            break;
        case KernelVersion::Ventura:
        case KernelVersion::Sonoma:
        case KernelVersion::Sequoia:
            this->pm4EngineField = 0x3B8;
            this->sdma0EngineField = 0x3C0;
            this->displayPipeCountField = 0x34;
            this->seCountField = 0x64;
            this->shPerSEField = 0x6C;
            this->cuPerSHField = 0xA0;
            this->hasUVD0Field = 0xB4;
            this->hasVCEField = 0xB6;
            this->hasVCN0Field = 0xB7;
            this->hasSDMAPagingQueueField = 0xBF;
            this->familyTypeField = 0x308;
            this->chipSettingsField = 0x5B10;
            break;
        default:
            PANIC("X5000", "Unknown kernel version %d", getKernelVersion());
    }

    SYSLOG("X5000", "Module initialised");

    callback = this;
    lilu.onKextLoadForce(&kextRadeonX5000);
}

bool X5000::processKext(KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size) {
    if (kextRadeonX5000.loadIndex == id) {
        SYSLOG_COND(ADDPR(debugEnabled), "X5000", "slide is 0x%llx", slide);
        NRed::callback->hwLateInit();

        UInt32 *orgChannelTypes;
        mach_vm_address_t startHWEngines;

        SolveRequestPlus solveRequests[] = {
            {NRed::callback->attributes.isCatalina() ?
                    "__ZZN37AMDRadeonX5000_AMDGraphicsAccelerator22getAdditionalQueueListEPPK18_"
                    "AMDQueueSpecifierE27additionalQueueList_Default" :
                    "__ZZN37AMDRadeonX5000_AMDGraphicsAccelerator19createAccelChannelsEbE12channelTypes",
                orgChannelTypes, kChannelTypesPattern},
            {"__ZN31AMDRadeonX5000_AMDGFX9PM4EngineC1Ev", this->orgGFX9PM4EngineConstructor},
            {"__ZN32AMDRadeonX5000_AMDGFX9SDMAEngineC1Ev", this->orgGFX9SDMAEngineConstructor},
            {"__ZN35AMDRadeonX5000_AMDAccelVideoContext10gMetaClassE", NRed::callback->metaClassMap[0][0]},
            {"__ZN37AMDRadeonX5000_AMDAccelDisplayMachine10gMetaClassE", NRed::callback->metaClassMap[1][0]},
            {"__ZN34AMDRadeonX5000_AMDAccelDisplayPipe10gMetaClassE", NRed::callback->metaClassMap[2][0]},
            {"__ZN30AMDRadeonX5000_AMDAccelChannel10gMetaClassE", NRed::callback->metaClassMap[3][1]},
            {"__ZN28AMDRadeonX5000_IAMDHWChannel10gMetaClassE", NRed::callback->metaClassMap[4][0]},
            {"__ZN26AMDRadeonX5000_AMDHardware14startHWEnginesEv", startHWEngines},
        };
        PANIC_COND(!SolveRequestPlus::solveAll(patcher, id, solveRequests, slide, size), "X5000",
            "Failed to resolve symbols");

        RouteRequestPlus requests[] = {
            {"__ZN32AMDRadeonX5000_AMDVega10Hardware17allocateHWEnginesEv", wrapAllocateHWEngines},
            {"__ZN32AMDRadeonX5000_AMDVega10Hardware32setupAndInitializeHWCapabilitiesEv",
                wrapSetupAndInitializeHWCapabilities, this->orgSetupAndInitializeHWCapabilities},
            {"__ZN30AMDRadeonX5000_AMDGFX9Hardware32setupAndInitializeHWCapabilitiesEv",
                wrapGFX9SetupAndInitializeHWCapabilities, this->orgGFX9SetupAndInitializeHWCapabilities},
            {"__ZN26AMDRadeonX5000_AMDHardware12getHWChannelE20_eAMD_HW_ENGINE_TYPE18_eAMD_HW_RING_TYPE",
                wrapGetHWChannel, this->orgGetHWChannel},
            {"__ZN30AMDRadeonX5000_AMDGFX9Hardware20initializeFamilyTypeEv", wrapInitializeFamilyType},
            {"__ZN30AMDRadeonX5000_AMDGFX9Hardware20allocateAMDHWDisplayEv", wrapAllocateAMDHWDisplay},
            {"__ZN26AMDRadeonX5000_AMDHWMemory17adjustVRAMAddressEy", wrapAdjustVRAMAddress,
                this->orgAdjustVRAMAddress},
            {"__ZN30AMDRadeonX5000_AMDGFX9Hardware25allocateAMDHWAlignManagerEv", wrapAllocateAMDHWAlignManager,
                this->orgAllocateAMDHWAlignManager},
            {"__ZN43AMDRadeonX5000_AMDVega10GraphicsAccelerator13getDeviceTypeEP11IOPCIDevice", wrapGetDeviceType},
            {"__ZN30AMDRadeonX5000_AMDGFX9Hardware20writeASICHangLogInfoEPPv", wrapReturnZero},
            {"__ZN4Addr2V27Gfx9Lib20HwlConvertChipFamilyEjj", wrapHwlConvertChipFamily, this->orgHwlConvertChipFamily,
                kHwlConvertChipFamilyPattern},
        };
        PANIC_COND(!RouteRequestPlus::routeAll(patcher, id, requests, slide, size), "X5000", "Failed to route symbols");

        if (NRed::callback->attributes.isVentura1304AndLater()) {
            RouteRequestPlus request {"__ZN37AMDRadeonX5000_AMDGraphicsAccelerator23obtainAccelChannelGroupE11SS_"
                                      "PRIORITYP27AMDRadeonX5000_AMDAccelTask",
                wrapObtainAccelChannelGroup1304, this->orgObtainAccelChannelGroup};
            PANIC_COND(!request.route(patcher, id, slide, size), "X5000", "Failed to route obtainAccelChannelGroup");
        } else if (!NRed::callback->attributes.isCatalina()) {
            RouteRequestPlus request {
                "__ZN37AMDRadeonX5000_AMDGraphicsAccelerator23obtainAccelChannelGroupE11SS_PRIORITY",
                wrapObtainAccelChannelGroup, this->orgObtainAccelChannelGroup};
            PANIC_COND(!request.route(patcher, id, slide, size), "X5000", "Failed to route obtainAccelChannelGroup");
        }

        if (NRed::callback->attributes.isSonoma1404AndLater()) {
            const LookupPatchPlus patch {&kextRadeonX5000, kAddrLibCreateOriginal14_4, kAddrLibCreateOriginalMask14_4,
                kAddrLibCreatePatched14_4, kAddrLibCreatePatchedMask14_4, 1};
            PANIC_COND(!patch.apply(patcher, slide, size), "X5000", "Failed to apply 14.4+ Addr::Lib::Create patch");
        } else if (NRed::callback->attributes.isCatalina() || NRed::callback->attributes.isVentura1304AndLater()) {
            const LookupPatchPlus patch {&kextRadeonX5000, kAddrLibCreateOriginal, kAddrLibCreatePatched, 1};
            PANIC_COND(!patch.apply(patcher, slide, size), "X5000",
                "Failed to apply Catalina & Ventura 13.4+ Addr::Lib::Create patch");
        }

        if (NRed::callback->attributes.isCatalina()) {
            const LookupPatchPlus patch {&kextRadeonX5000, kCreateAccelChannelsOriginal, kCreateAccelChannelsPatched,
                2};
            PANIC_COND(!patch.apply(patcher, slide, size), "X5000", "Failed to patch createAccelChannels");

            UInt32 findNonBpp64 = 0x22222221;
            UInt32 replNonBpp64 =
                NRed::callback->attributes.isRenoir() ? Dcn2NonBpp64SwModeMask : Dcn1NonBpp64SwModeMask;
            UInt32 findBpp64 = 0x44444440;
            UInt32 replBpp64Pt2 = NRed::callback->attributes.isRenoir() ? Dcn2Bpp64SwModeMask : Dcn1Bpp64SwModeMask;
            UInt32 replBpp64 = replNonBpp64 ^ replBpp64Pt2;
            UInt32 findBpp64Pt2 = 0x66666661;
            const LookupPatchPlus patches[] = {
                {&kextRadeonX5000, reinterpret_cast<const UInt8 *>(&findNonBpp64),
                    reinterpret_cast<const UInt8 *>(&replNonBpp64), sizeof(UInt32), 2},
                {&kextRadeonX5000, reinterpret_cast<const UInt8 *>(&findBpp64),
                    reinterpret_cast<const UInt8 *>(&replBpp64), sizeof(UInt32), 1},
                {&kextRadeonX5000, reinterpret_cast<const UInt8 *>(&findBpp64Pt2),
                    reinterpret_cast<const UInt8 *>(&replBpp64Pt2), sizeof(UInt32), 1},
            };
            PANIC_COND(!LookupPatchPlus::applyAll(patcher, patches, slide, size), "X5000",
                "Failed to patch swizzle mode");

            PANIC_COND(MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) != KERN_SUCCESS, "X5000",
                "Failed to enable kernel writing");
            *orgChannelTypes = 1;    // Make VMPT use SDMA0 instead of SDMA1
            MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);
            DBGLOG("X5000", "Applied SDMA1 patches");
        } else {
            const LookupPatchPlus patch {&kextRadeonX5000, kStartHWEnginesOriginal, kStartHWEnginesMask,
                kStartHWEnginesPatched, kStartHWEnginesMask, NRed::callback->attributes.isVenturaAndLater() ? 2U : 1};
            PANIC_COND(!patch.apply(patcher, startHWEngines, PAGE_SIZE), "X5000", "Failed to patch startHWEngines");

            if (NRed::callback->attributes.isRenoir()) {
                UInt32 findBpp64 = Dcn1Bpp64SwModeMask, replBpp64 = Dcn2Bpp64SwModeMask;
                UInt32 findNonBpp64 = Dcn1NonBpp64SwModeMask, replNonBpp64 = Dcn2NonBpp64SwModeMask;
                const LookupPatchPlus patches[] = {
                    {&kextRadeonX5000, reinterpret_cast<const UInt8 *>(&findBpp64),
                        reinterpret_cast<const UInt8 *>(&replBpp64), sizeof(UInt32),
                        NRed::callback->attributes.isVentura1304AndLater() ? 2U : 4},
                    {&kextRadeonX5000, reinterpret_cast<const UInt8 *>(&findNonBpp64),
                        reinterpret_cast<const UInt8 *>(&replNonBpp64), sizeof(UInt32),
                        NRed::callback->attributes.isVentura1304AndLater() ? 2U : 4},
                };
                PANIC_COND(!LookupPatchPlus::applyAll(patcher, patches, slide, size), "X5000",
                    "Failed to patch swizzle mode");
            }

            PANIC_COND(MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) != KERN_SUCCESS, "X5000",
                "Failed to enable kernel writing");
            // createAccelChannels: stop at SDMA0
            orgChannelTypes[5] = 1;
            // getPagingChannel: get only SDMA0
            orgChannelTypes[NRed::callback->attributes.isMontereyAndLater() ? 12 : 11] = 0;
            MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);
            DBGLOG("X5000", "Applied SDMA1 patches");
        }

        return true;
    }

    return false;
}

bool X5000::wrapAllocateHWEngines(void *that) {
    auto *pm4 = OSObject::operator new(0x340);
    callback->orgGFX9PM4EngineConstructor(pm4);
    callback->pm4EngineField.set(that, pm4);

    auto *sdma0 = OSObject::operator new(0x250);
    callback->orgGFX9SDMAEngineConstructor(sdma0);
    callback->sdma0EngineField.set(that, sdma0);

    return true;
}

void X5000::wrapSetupAndInitializeHWCapabilities(void *that) {
    FunctionCast(wrapSetupAndInitializeHWCapabilities, callback->orgSetupAndInitializeHWCapabilities)(that);

    callback->displayPipeCountField.set(that, NRed::callback->attributes.isRenoir() ? 4 : 6);
    callback->hasUVD0Field.set(that, false);
    callback->hasVCEField.set(that, false);
    callback->hasVCN0Field.set(that, false);
    callback->hasSDMAPagingQueueField.set(that, false);
}

void X5000::wrapGFX9SetupAndInitializeHWCapabilities(void *that) {
    char filename[128] = {0};
    snprintf(filename, arrsize(filename), "%s_gpu_info.bin",
        NRed::callback->attributes.isRenoir() ? "renoir" : NRed::callback->getChipName());
    const auto &gpuInfoBin = getFWByName(filename);
    auto *header = reinterpret_cast<const CommonFirmwareHeader *>(gpuInfoBin.data);
    auto *gpuInfo = reinterpret_cast<const GPUInfoFirmware *>(gpuInfoBin.data + header->ucodeOff);

    callback->seCountField.set(that, gpuInfo->gcNumSe);
    callback->shPerSEField.set(that, gpuInfo->gcNumShPerSe);
    callback->cuPerSHField.set(that, gpuInfo->gcNumCuPerSh);

    FunctionCast(wrapGFX9SetupAndInitializeHWCapabilities, callback->orgGFX9SetupAndInitializeHWCapabilities)(that);
}

// Redirect SDMA1 to SDMA0
void *X5000::wrapGetHWChannel(void *that, UInt32 engineType, UInt32 ringId) {
    return FunctionCast(wrapGetHWChannel, callback->orgGetHWChannel)(that, (engineType == 2) ? 1 : engineType, ringId);
}

void X5000::wrapInitializeFamilyType(void *that) { callback->familyTypeField.set(that, AMDGPU_FAMILY_RAVEN); }

void *X5000::wrapAllocateAMDHWDisplay(void *that) {
    return FunctionCast(wrapAllocateAMDHWDisplay, X6000::callback->orgAllocateAMDHWDisplay)(that);
}

UInt64 X5000::wrapAdjustVRAMAddress(void *that, UInt64 addr) {
    auto ret = FunctionCast(wrapAdjustVRAMAddress, callback->orgAdjustVRAMAddress)(that, addr);
    return ret != addr ? (ret + NRed::callback->fbOffset) : ret;
}

static UInt32 fakeGetPreferredSwizzleMode2(void *, void *pIn) { return getMember<UInt32>(pIn, 0x10); }

void *X5000::wrapAllocateAMDHWAlignManager() {
    auto *hwAlignManager = FunctionCast(wrapAllocateAMDHWAlignManager, callback->orgAllocateAMDHWAlignManager)();
    auto *vtableNew = IOMalloc(0x238);
    auto *vtableOriginal = getMember<void *>(hwAlignManager, 0);
    memcpy(vtableNew, vtableOriginal, 0x230);
    getMember<mach_vm_address_t>(vtableNew, 0x230) = reinterpret_cast<mach_vm_address_t>(fakeGetPreferredSwizzleMode2);
    getMember<void *>(hwAlignManager, 0) = vtableNew;
    return hwAlignManager;
}

UInt32 X5000::wrapGetDeviceType() { return NRed::callback->attributes.isRenoir() ? 9 : 0; }

UInt32 X5000::wrapReturnZero() { return 0; }

static void fixAccelGroup(void *that) {
    auto *&sdma1 = getMember<void *>(that, 0x18);
    // Replace field with SDMA0 because we don't have SDMA1
    if (sdma1 == nullptr) { sdma1 = getMember<void *>(that, 0x10); }
}

void *X5000::wrapObtainAccelChannelGroup(void *that, UInt32 priority) {
    auto ret = FunctionCast(wrapObtainAccelChannelGroup, callback->orgObtainAccelChannelGroup)(that, priority);
    if (ret == nullptr) { return nullptr; }
    fixAccelGroup(ret);
    return ret;
}

void *X5000::wrapObtainAccelChannelGroup1304(void *that, UInt32 priority, void *task) {
    auto ret =
        FunctionCast(wrapObtainAccelChannelGroup1304, callback->orgObtainAccelChannelGroup)(that, priority, task);
    if (ret == nullptr) { return nullptr; }
    fixAccelGroup(ret);
    return ret;
}

UInt32 X5000::wrapHwlConvertChipFamily(void *that, UInt32 family, UInt32 revision) {
    DBGLOG("X5000", "HwlConvertChipFamily >> (that: %p family: 0x%X revision: 0x%X)", that, family, revision);
    if (family == AMDGPU_FAMILY_RAVEN) {
        auto &settings = callback->chipSettingsField.getRef(that);
        settings.isArcticIsland = 1;
        settings.isRaven = 1;
        if (NRed::callback->attributes.isRenoir()) {
            settings.htileAlignFix = 1;
            settings.applyAliasFix = 1;
        } else if (NRed::callback->attributes.isRaven()) {
            settings.depthPipeXorDisable = 1;
        }
        settings.isDcn1 = 1;
        settings.metaBaseAlignFix = 1;
        return ADDR_CHIP_FAMILY_AI;
    }
    return FunctionCast(wrapHwlConvertChipFamily, callback->orgHwlConvertChipFamily)(that, family, revision);
}
