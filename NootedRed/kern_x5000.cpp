//  Copyright © 2022-2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.0. See LICENSE for
//  details.

#include "kern_x5000.hpp"
#include "kern_nred.hpp"
#include "kern_patches.hpp"
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

        KernelPatcher::SolveRequest solveRequests[] = {
            {"__ZN31AMDRadeonX5000_AMDGFX9PM4EngineC1Ev", this->orgGFX9PM4EngineConstructor},
            {"__ZN32AMDRadeonX5000_AMDGFX9SDMAEngineC1Ev", this->orgGFX9SDMAEngineConstructor},
            {"__ZZN37AMDRadeonX5000_AMDGraphicsAccelerator19createAccelChannelsEbE12channelTypes", orgChannelTypes},
            {"__ZN39AMDRadeonX5000_AMDAccelSharedUserClient5startEP9IOService", this->orgAccelSharedUCStart},
            {"__ZN39AMDRadeonX5000_AMDAccelSharedUserClient4stopEP9IOService", this->orgAccelSharedUCStop},
            {"__ZN35AMDRadeonX5000_AMDAccelVideoContext10gMetaClassE", NRed::callback->metaClassMap[0][0]},
            {"__ZN37AMDRadeonX5000_AMDAccelDisplayMachine10gMetaClassE", NRed::callback->metaClassMap[1][0]},
            {"__ZN34AMDRadeonX5000_AMDAccelDisplayPipe10gMetaClassE", NRed::callback->metaClassMap[2][0]},
            {"__ZN30AMDRadeonX5000_AMDAccelChannel10gMetaClassE", NRed::callback->metaClassMap[3][1]},
            {"__ZN30AMDRadeonX5000_AMDGFX9Hardware32setupAndInitializeHWCapabilitiesEv",
                this->orgSetupAndInitializeHWCapabilities},
        };
        PANIC_COND(!patcher.solveMultiple(index, solveRequests, address, size), "x5000", "Failed to resolve symbols");

        KernelPatcher::RouteRequest requests[] = {
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
            {"__ZN37AMDRadeonX5000_AMDGraphicsAccelerator9newSharedEv", wrapNewShared},
            {"__ZN37AMDRadeonX5000_AMDGraphicsAccelerator19newSharedUserClientEv", wrapNewSharedUserClient},
            {"__ZN30AMDRadeonX5000_AMDGFX9Hardware25allocateAMDHWAlignManagerEv", wrapAllocateAMDHWAlignManager,
                this->orgAllocateAMDHWAlignManager},
        };
        PANIC_COND(!patcher.routeMultiple(index, requests, address, size), "x5000", "Failed to route symbols");

        PANIC_COND(MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) != KERN_SUCCESS, "x5000",
            "Failed to enable kernel writing");
        orgChannelTypes[5] = 1;     // Fix createAccelChannels so that it only starts SDMA0
        orgChannelTypes[11] = 0;    // Fix getPagingChannel so that it gets SDMA0
        MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);
        DBGLOG("x5000", "Applied SDMA1 patches");

        KernelPatcher::LookupPatch patch = {&kextRadeonX5000, kStartHWEnginesOriginal, kStartHWEnginesPatched,
            arrsize(kStartHWEnginesOriginal), 1};
        patcher.applyLookupPatch(&patch);
        patcher.clearError();

        return true;
    }

    return false;
}

bool X5000::wrapAllocateHWEngines(void *that) {
    callback->orgGFX9PM4EngineConstructor(getMember<void *>(that, 0x3B8) = IOMallocZero(0x1E8));
    callback->orgGFX9SDMAEngineConstructor(getMember<void *>(that, 0x3C0) = IOMallocZero(0x128));
    X6000::callback->orgVCN2EngineConstructor(getMember<void *>(that, 0x3F8) = IOMallocZero(0x198));

    return true;
}

enum HWCapability : uint64_t {
    DisplayPipeCount = 0x04,    // uint32_t
    SECount = 0x34,             // uint32_t
    SHPerSE = 0x3C,             // uint32_t
    CUPerSH = 0x70,             // uint32_t
    HasUVD0 = 0x84,             // bool
    HasUVD1 = 0x85,             // bool
    HasVCE = 0x86,              // bool
    HasVCN0 = 0x87,             // bool
    HasVCN1 = 0x88,             // bool
    HasSDMAPageQueue = 0x98,    // bool
};

template<typename T>
static inline void setHWCapability(void *that, HWCapability capability, T value) {
    getMember<T>(that, 0x28 + capability) = value;
}

void X5000::wrapSetupAndInitializeHWCapabilities(void *that) {
    auto isRavenDerivative = NRed::callback->chipType < ChipType::Renoir;

    auto *chipName = isRavenDerivative ? NRed::getChipName() : "renoir";
    char filename[128] = {0};
    snprintf(filename, arrsize(filename), "%s_gpu_info.bin", chipName);
    auto &fwDesc = getFWDescByName(filename);
    auto *header = reinterpret_cast<const CommonFirmwareHeader *>(fwDesc.data);
    auto *gpuInfo = reinterpret_cast<const GPUInfoFirmware *>(fwDesc.data + header->ucodeOff);

    setHWCapability<uint32_t>(that, HWCapability::SECount, gpuInfo->gcNumSe);
    setHWCapability<uint32_t>(that, HWCapability::SHPerSE, gpuInfo->gcNumShPerSe);
    setHWCapability<uint32_t>(that, HWCapability::CUPerSH, gpuInfo->gcNumCuPerSh);

    FunctionCast(wrapSetupAndInitializeHWCapabilities, callback->orgSetupAndInitializeHWCapabilities)(that);

    setHWCapability<uint32_t>(that, HWCapability::DisplayPipeCount, isRavenDerivative ? 4 : 6);
    setHWCapability<bool>(that, HWCapability::HasUVD0, false);
    setHWCapability<bool>(that, HWCapability::HasUVD1, false);
    setHWCapability<bool>(that, HWCapability::HasVCE, false);
    setHWCapability<bool>(that, HWCapability::HasVCN0, true);
    setHWCapability<bool>(that, HWCapability::HasVCN1, false);
    setHWCapability<bool>(that, HWCapability::HasSDMAPageQueue, false);
}

void *X5000::wrapGetHWChannel(void *that, uint32_t engineType, uint32_t ringId) {
    /** Redirect SDMA1 engine type to SDMA0 */
    return FunctionCast(wrapGetHWChannel, callback->orgGetHWChannel)(that, (engineType == 2) ? 1 : engineType, ringId);
}

void X5000::wrapInitializeFamilyType(void *that) { getMember<uint32_t>(that, 0x308) = AMDGPU_FAMILY_RAVEN; }

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
    callback->hwAlignMgrVtX6000 = static_cast<uint8_t *>(IOMallocZero(0x238));

    memcpy(callback->hwAlignMgrVtX6000, callback->hwAlignMgrVtX5000, 0x128);
    *reinterpret_cast<mach_vm_address_t *>(callback->hwAlignMgrVtX6000 + 0x128) =
        X6000::callback->orgGetPreferredSwizzleMode2;
    memcpy(callback->hwAlignMgrVtX6000 + 0x130, callback->hwAlignMgrVtX5000 + 0x128, 0x230 - 0x128);
    return ret;
}
