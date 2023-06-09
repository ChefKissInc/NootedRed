//  Copyright © 2022-2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.0. See LICENSE for
//  details.

#include "kern_x5000.hpp"
#include "kern_nred.hpp"
#include "kern_patcherplus.hpp"
#include "kern_patches.hpp"
#include "kern_patterns.hpp"
#include "kern_x6000.hpp"
#include <Headers/kern_api.hpp>

static const char *pathRadeonX5000 = "/System/Library/Extensions/AMDRadeonX5000.kext/Contents/MacOS/AMDRadeonX5000";

static KernelPatcher::KextInfo kextRadeonX5000 {"com.apple.kext.AMDRadeonX5000", &pathRadeonX5000, 1, {}, {},
    KernelPatcher::KextInfo::Unloaded};

X5000 *X5000::callback = nullptr;

void X5000::init() {
    callback = this;
    lilu.onKextLoadForce(&kextRadeonX5000);
}

bool X5000::processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
    if (kextRadeonX5000.loadIndex == index) {
        NRed::callback->setRMMIOIfNecessary();

        uint32_t *orgChannelTypes = nullptr;
        mach_vm_address_t startHWEngines = 0;

        auto catalina = getKernelVersion() == KernelVersion::Catalina;
        SolveRequestPlus solveRequests[] = {
            {catalina ? "__ZZN37AMDRadeonX5000_AMDGraphicsAccelerator22getAdditionalQueueListEPPK18_"
                        "AMDQueueSpecifierE27additionalQueueList_Default" :
                        "__ZZN37AMDRadeonX5000_AMDGraphicsAccelerator19createAccelChannelsEbE12channelTypes",
                orgChannelTypes, kChannelTypesPattern},
            {"__ZN31AMDRadeonX5000_AMDGFX9PM4EngineC1Ev", this->orgGFX9PM4EngineConstructor},
            {"__ZN32AMDRadeonX5000_AMDGFX9SDMAEngineC1Ev", this->orgGFX9SDMAEngineConstructor},
            {"__ZN39AMDRadeonX5000_AMDAccelSharedUserClient5startEP9IOService", this->orgAccelSharedUCStart},
            {"__ZN39AMDRadeonX5000_AMDAccelSharedUserClient4stopEP9IOService", this->orgAccelSharedUCStop},
            {"__ZN35AMDRadeonX5000_AMDAccelVideoContext10gMetaClassE", NRed::callback->metaClassMap[0][0]},
            {"__ZN37AMDRadeonX5000_AMDAccelDisplayMachine10gMetaClassE", NRed::callback->metaClassMap[1][0]},
            {"__ZN34AMDRadeonX5000_AMDAccelDisplayPipe10gMetaClassE", NRed::callback->metaClassMap[2][0]},
            {"__ZN30AMDRadeonX5000_AMDAccelChannel10gMetaClassE", NRed::callback->metaClassMap[3][1]},
            {"__ZN30AMDRadeonX5000_AMDGFX9Hardware32setupAndInitializeHWCapabilitiesEv",
                this->orgSetupAndInitializeHWCapabilities},
            {"__ZN26AMDRadeonX5000_AMDHardware14startHWEnginesEv", startHWEngines},
        };
        PANIC_COND(!SolveRequestPlus::solveAll(&patcher, index, solveRequests, address, size), "x5000",
            "Failed to resolve symbols");

        auto ventura = getKernelVersion() >= KernelVersion::Ventura;
        auto ventura1304 = (ventura && getKernelMinorVersion() >= 5) || getKernelVersion() > KernelVersion::Ventura;
        RouteRequestPlus requests[] = {
            {"__ZN32AMDRadeonX5000_AMDVega10Hardware17allocateHWEnginesEv", wrapAllocateHWEngines},
            {"__ZN32AMDRadeonX5000_AMDVega10Hardware32setupAndInitializeHWCapabilitiesEv",
                wrapSetupAndInitializeHWCapabilities},
            {"__ZN26AMDRadeonX5000_AMDHardware12getHWChannelE20_eAMD_HW_ENGINE_TYPE18_eAMD_HW_RING_TYPE",
                wrapGetHWChannel, this->orgGetHWChannel},
            {"__ZN30AMDRadeonX5000_AMDGFX9Hardware20initializeFamilyTypeEv", wrapInitializeFamilyType},
            {"__ZN30AMDRadeonX5000_AMDGFX9Hardware20allocateAMDHWDisplayEv", wrapAllocateAMDHWDisplay},
            {"__ZN41AMDRadeonX5000_AMDGFX9GraphicsAccelerator15newVideoContextEv", wrapNewVideoContext},
            {"__ZN31AMDRadeonX5000_IAMDSMLInterface18createSMLInterfaceEj", wrapCreateSMLInterface},
            {"__ZN26AMDRadeonX5000_AMDHWMemory17adjustVRAMAddressEy", wrapAdjustVRAMAddress,
                this->orgAdjustVRAMAddress},
            {"__ZN37AMDRadeonX5000_AMDGraphicsAccelerator9newSharedEv", wrapNewShared, !catalina},
            {"__ZN37AMDRadeonX5000_AMDGraphicsAccelerator19newSharedUserClientEv", wrapNewSharedUserClient, !catalina},
            {"__ZN30AMDRadeonX5000_AMDGFX9Hardware25allocateAMDHWAlignManagerEv", wrapAllocateAMDHWAlignManager,
                this->orgAllocateAMDHWAlignManager},
            {"__ZN43AMDRadeonX5000_AMDVega10GraphicsAccelerator13getDeviceTypeEP11IOPCIDevice", wrapGetDeviceType},
            {"__ZN30AMDRadeonX5000_AMDGFX9Hardware20writeASICHangLogInfoEPPv", wrapReturnZero},
            {"__ZN37AMDRadeonX5000_AMDGraphicsAccelerator23obtainAccelChannelGroupE11SS_PRIORITY",
                wrapObtainAccelChannelGroup, this->orgObtainAccelChannelGroup, !catalina && !ventura1304},
            {"__ZN37AMDRadeonX5000_AMDGraphicsAccelerator23obtainAccelChannelGroupE11SS_PRIORITYP27AMDRadeonX5000_"
             "AMDAccelTask",
                wrapObtainAccelChannelGroup1304, this->orgObtainAccelChannelGroup, ventura1304},
            {"__ZN4Addr2V27Gfx9Lib20HwlConvertChipFamilyEjj", wrapHwlConvertChipFamily, kHwlConvertChipFamilyPattern},
        };
        PANIC_COND(!RouteRequestPlus::routeAll(patcher, index, requests, address, size), "x5000",
            "Failed to route symbols");

        LookupPatchPlus const addrLibPatch {&kextRadeonX5000, kAddrLibCreateOriginal, kAddrLibCreatePatched, 1,
            ventura1304};
        PANIC_COND(!addrLibPatch.apply(&patcher, address, size), "x5000",
            "Failed to apply Ventura 13.4+ Addr::Lib::Create patch: %d", patcher.getError());

        LookupPatchPlus const patch {&kextRadeonX5000, kStartHWEnginesOriginal, kStartHWEnginesMask,
            kStartHWEnginesPatched, kStartHWEnginesMask, ventura ? 2U : 1, !catalina};
        PANIC_COND(!patch.apply(&patcher, startHWEngines, PAGE_SIZE), "x5000", "Failed to patch startHWEngines");

        LookupPatchPlus const createAccelChannelsPatch {&kextRadeonX5000, kCreateAccelChannelsOriginal,
            kCreateAccelChannelsPatched, 2, catalina};
        PANIC_COND(!createAccelChannelsPatch.apply(&patcher, address, size), "x5000",
            "Failed to patch createAccelChannels");

        if (!catalina) {
            uint32_t findBpp64 = Dcn1Bpp64SwModeMask, replBpp64 = Dcn2Bpp64SwModeMask;
            uint32_t findNonBpp64 = Dcn1NonBpp64SwModeMask, replNonBpp64 = Dcn2NonBpp64SwModeMask;
            auto dcn2 = NRed::callback->chipType >= ChipType::Renoir;
            LookupPatchPlus const swizzleModePatches[] = {
                {&kextRadeonX5000, reinterpret_cast<const uint8_t *>(&findBpp64),
                    reinterpret_cast<const uint8_t *>(&replBpp64), sizeof(uint32_t), ventura1304 ? 2U : 4, dcn2},
                {&kextRadeonX5000, reinterpret_cast<const uint8_t *>(&findNonBpp64),
                    reinterpret_cast<const uint8_t *>(&replNonBpp64), sizeof(uint32_t), ventura1304 ? 2U : 4, dcn2},
            };
            PANIC_COND(!LookupPatchPlus::applyAll(&patcher, swizzleModePatches, address, size), "x5000",
                "Failed to patch swizzle mode");

            PANIC_COND(MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) != KERN_SUCCESS, "x5000",
                "Failed to enable kernel writing");
            orgChannelTypes[5] = 1;    // Fix createAccelChannels so that it only starts SDMA0
            orgChannelTypes[(getKernelVersion() >= KernelVersion::Monterey) ? 12 : 11] =
                0;    // Fix getPagingChannel so that it gets SDMA0
            MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);
            DBGLOG("x5000", "Applied SDMA1 patches");
        } else {
            auto dcn2 = NRed::callback->chipType >= ChipType::Renoir;
            uint32_t findNonBpp64 = 0x22222221;
            uint32_t replNonBpp64 = dcn2 ? Dcn2NonBpp64SwModeMask : Dcn1NonBpp64SwModeMask;
            uint32_t findBpp64 = 0x44444440;
            uint32_t replBpp64Pt2 = dcn2 ? Dcn2Bpp64SwModeMask : Dcn1Bpp64SwModeMask;
            uint32_t replBpp64 = replNonBpp64 ^ replBpp64Pt2;
            uint32_t findBpp64Pt2 = 0x66666661;
            LookupPatchPlus const swizzleModePatches[] = {
                {&kextRadeonX5000, reinterpret_cast<const uint8_t *>(&findNonBpp64),
                    reinterpret_cast<const uint8_t *>(&replNonBpp64), sizeof(uint32_t), 2},
                {&kextRadeonX5000, reinterpret_cast<const uint8_t *>(&findBpp64),
                    reinterpret_cast<const uint8_t *>(&replBpp64), sizeof(uint32_t), 1},
                {&kextRadeonX5000, reinterpret_cast<const uint8_t *>(&findBpp64Pt2),
                    reinterpret_cast<const uint8_t *>(&replBpp64Pt2), sizeof(uint32_t), 1},
            };
            PANIC_COND(!LookupPatchPlus::applyAll(&patcher, swizzleModePatches, address, size), "x5000",
                "Failed to patch swizzle mode");
            *orgChannelTypes = 1;    // Make VMPT use SDMA0 instead of SDMA1
        }

        return true;
    }

    return false;
}

bool X5000::wrapAllocateHWEngines(void *that) {
    auto catalina = getKernelVersion() == KernelVersion::Catalina;
    auto fieldBase = catalina ? 0x348 : 0x3B8;
    callback->orgGFX9PM4EngineConstructor(getMember<void *>(that, fieldBase) = IOMallocZero(0x340));
    callback->orgGFX9SDMAEngineConstructor(getMember<void *>(that, fieldBase + 0x8) = IOMallocZero(0x250));
    X6000::callback->orgVCN2EngineConstructor(
        getMember<void *>(that, fieldBase + (catalina ? 0x30 : 0x40)) = IOMallocZero(0x2D8));

    return true;
}

struct HWCapability {
    enum : uint64_t {
        DisplayPipeCount = 0x04,    // uint32_t
        SECount = 0x34,             // uint32_t
        SHPerSE = 0x3C,             // uint32_t
        CUPerSH = 0x70,             // uint32_t
        HasUVD0 = 0x84,             // bool
        HasVCE = 0x86,              // bool
        HasVCN0 = 0x87,             // bool
    };
};

struct HWCapabilityCatalina {
    enum : uint64_t {
        DisplayPipeCount = 0x04,    // uint32_t
        SECount = 0x30,             // uint32_t
        SHPerSE = 0x34,             // uint32_t
        CUPerSH = 0x58,             // uint32_t
        HasUVD0 = 0x68,             // bool
        HasVCE = 0x6A,              // bool
        HasVCN0 = 0x6B,             // bool
    };
};

template<typename T>
static inline void setHWCapability(void *that, uint64_t capability, T value) {
    getMember<T>(that, (getKernelVersion() >= KernelVersion::Ventura ? 0x30 : 0x28) + capability) = value;
}

void X5000::wrapSetupAndInitializeHWCapabilities(void *that) {
    auto isRavenDerivative = NRed::callback->chipType < ChipType::Renoir;
    char filename[128] = {0};
    snprintf(filename, arrsize(filename), "%s_gpu_info.bin", isRavenDerivative ? NRed::getChipName() : "renoir");
    auto &fwDesc = getFWDescByName(filename);
    auto *header = reinterpret_cast<const CommonFirmwareHeader *>(fwDesc.data);
    auto *gpuInfo = reinterpret_cast<const GPUInfoFirmware *>(fwDesc.data + header->ucodeOff);

    auto catalina = getKernelVersion() == KernelVersion::Catalina;
    setHWCapability<uint32_t>(that, catalina ? HWCapabilityCatalina::SECount : HWCapability::SECount, gpuInfo->gcNumSe);
    setHWCapability<uint32_t>(that, catalina ? HWCapabilityCatalina::SHPerSE : HWCapability::SHPerSE,
        gpuInfo->gcNumShPerSe);
    setHWCapability<uint32_t>(that, catalina ? HWCapabilityCatalina::CUPerSH : HWCapability::CUPerSH,
        gpuInfo->gcNumCuPerSh);

    FunctionCast(wrapSetupAndInitializeHWCapabilities, callback->orgSetupAndInitializeHWCapabilities)(that);

    setHWCapability<uint32_t>(that, catalina ? HWCapabilityCatalina::DisplayPipeCount : HWCapability::DisplayPipeCount,
        isRavenDerivative ? 4 : 6);
    setHWCapability<bool>(that, catalina ? HWCapabilityCatalina::HasUVD0 : HWCapability::HasUVD0, false);
    setHWCapability<bool>(that, catalina ? HWCapabilityCatalina::HasVCE : HWCapability::HasVCE, false);
    setHWCapability<bool>(that, catalina ? HWCapabilityCatalina::HasVCN0 : HWCapability::HasVCN0, true);
}

void *X5000::wrapGetHWChannel(void *that, uint32_t engineType, uint32_t ringId) {
    /** Redirect SDMA1 engine type to SDMA0 */
    return FunctionCast(wrapGetHWChannel, callback->orgGetHWChannel)(that, (engineType == 2) ? 1 : engineType, ringId);
}

void X5000::wrapInitializeFamilyType(void *that) {
    getMember<uint32_t>(that, getKernelVersion() == KernelVersion::Catalina ? 0x2B4 : 0x308) = AMDGPU_FAMILY_RAVEN;
}

void *X5000::wrapAllocateAMDHWDisplay(void *that) {
    return FunctionCast(wrapAllocateAMDHWDisplay, X6000::callback->orgAllocateAMDHWDisplay)(that);
}

void *X5000::wrapNewVideoContext(void *that) {
    return FunctionCast(wrapNewVideoContext, X6000::callback->orgNewVideoContext)(that);
}

void *X5000::wrapCreateSMLInterface(uint32_t configBit) {
    return FunctionCast(wrapCreateSMLInterface, X6000::callback->orgCreateSMLInterface)(configBit);
}

uint64_t X5000::wrapAdjustVRAMAddress(void *that, uint64_t addr) {
    auto ret = FunctionCast(wrapAdjustVRAMAddress, callback->orgAdjustVRAMAddress)(that, addr);
    return ret != addr ? (ret + NRed::callback->fbOffset) : ret;
}

void *X5000::wrapNewShared() { return FunctionCast(wrapNewShared, X6000::callback->orgNewShared)(); }

void *X5000::wrapNewSharedUserClient() {
    return FunctionCast(wrapNewSharedUserClient, X6000::callback->orgNewSharedUserClient)();
}

void *X5000::wrapAllocateAMDHWAlignManager() {
    auto ret = FunctionCast(wrapAllocateAMDHWAlignManager, callback->orgAllocateAMDHWAlignManager)();
    callback->hwAlignMgr = ret;

    callback->hwAlignMgrVtX5000 = getMember<uint8_t *>(ret, 0);
    callback->hwAlignMgrVtX6000 = IONewZero(uint8_t, 0x238);

    memcpy(callback->hwAlignMgrVtX6000, callback->hwAlignMgrVtX5000, 0x128);
    *reinterpret_cast<mach_vm_address_t *>(callback->hwAlignMgrVtX6000 + 0x128) =
        X6000::callback->orgGetPreferredSwizzleMode2;
    memcpy(callback->hwAlignMgrVtX6000 + 0x130, callback->hwAlignMgrVtX5000 + 0x128, 0x230 - 0x128);
    return ret;
}

uint32_t X5000::wrapGetDeviceType() { return NRed::callback->chipType < ChipType::Renoir ? 0 : 9; }

uint32_t X5000::wrapReturnZero() { return 0; }

static void fixAccelGroup(void *that) {
    auto *&sdma1 = getMember<void *>(that, 0x18);
    sdma1 = sdma1 ?: getMember<void *>(that, 0x10);    // Replace field with SDMA0, as we have no SDMA1
}

void *X5000::wrapObtainAccelChannelGroup(void *that, uint32_t priority) {
    auto ret = FunctionCast(wrapObtainAccelChannelGroup, callback->orgObtainAccelChannelGroup)(that, priority);
    if (ret) { fixAccelGroup(ret); }
    return ret;
}

void *X5000::wrapObtainAccelChannelGroup1304(void *that, uint32_t priority, void *task) {
    auto ret =
        FunctionCast(wrapObtainAccelChannelGroup1304, callback->orgObtainAccelChannelGroup)(that, priority, task);
    if (ret) { fixAccelGroup(ret); }
    return ret;
}

uint32_t X5000::wrapHwlConvertChipFamily(void *that, uint32_t, uint32_t) {
    auto &settings = getMember<Gfx9ChipSettings>(that, getKernelVersion() == KernelVersion::Catalina ? 0x5B18 : 0x5B10);
    auto renoir = NRed::callback->chipType >= ChipType::Renoir;
    settings.isArcticIsland = 1;
    settings.isRaven = 1;
    settings.depthPipeXorDisable = NRed::callback->chipType < ChipType::Raven2;
    settings.htileAlignFix = renoir;
    settings.applyAliasFix = renoir;
    settings.isDcn1 = 1;
    settings.metaBaseAlignFix = 1;
    return ADDR_CHIP_FAMILY_AI;
}
