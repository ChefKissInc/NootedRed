// Copyright © 2022-2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#include <Headers/kern_api.hpp>
#include <Headers/kern_util.hpp>
#include <PrivateHeaders/Firmware.hpp>
#include <PrivateHeaders/GPUDriversAMD/Family.hpp>
#include <PrivateHeaders/GPUDriversAMD/Linux.hpp>
#include <PrivateHeaders/NRed.hpp>
#include <PrivateHeaders/PatcherPlus.hpp>
#include <PrivateHeaders/iVega/X5000.hpp>
#include <PrivateHeaders/iVega/X6000.hpp>

//------ Target Kexts ------//

static const char *pathRadeonX5000 = "/System/Library/Extensions/AMDRadeonX5000.kext/Contents/MacOS/AMDRadeonX5000";

static KernelPatcher::KextInfo kextRadeonX5000 {
    "com.apple.kext.AMDRadeonX5000",
    &pathRadeonX5000,
    1,
    {true},
    {},
    KernelPatcher::KextInfo::Unloaded,
};

//------ Patterns ------//

static const UInt8 kChannelTypesPattern[] = {0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00};

static const UInt8 kHwlConvertChipFamilyPattern[] = {0x81, 0xFE, 0x8D, 0x00, 0x00, 0x00, 0x0F};

//------ Patches ------//

// Make for loop stop after one SDMA engine.
static const UInt8 kStartHWEnginesOriginal[] = {0x40, 0x83, 0xF0, 0x02};
static const UInt8 kStartHWEnginesMask[] = {0xF0, 0xFF, 0xF0, 0xFF};
static const UInt8 kStartHWEnginesPatched[] = {0x40, 0x83, 0xF0, 0x01};

// The check in Addr::Lib::Create on 10.15 & 13.4+ is `familyId == 0x8D` instead of `familyId - 0x8D < 2`.
// So, change the 0x8D (AI) to 0x8E (RV).
static const UInt8 kAddrLibCreateOriginal[] = {0x41, 0x81, 0x7D, 0x08, 0x8D, 0x00, 0x00, 0x00};
static const UInt8 kAddrLibCreatePatched[] = {0x41, 0x81, 0x7D, 0x08, 0x8E, 0x00, 0x00, 0x00};

// For some reason Navi (`0x8F`) was added in 14.4 here. Lazy copy pasting?
static const UInt8 kAddrLibCreateOriginal1404[] = {0x41, 0x8B, 0x46, 0x08, 0x3D, 0x8F, 0x00, 0x00, 0x00, 0x74, 0x00,
    0x3D, 0x8D, 0x00, 0x00, 0x00, 0x0F, 0x85, 0x00, 0x00, 0x00, 0x00};
static const UInt8 kAddrLibCreateOriginalMask1404[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00};
static const UInt8 kAddrLibCreatePatched1404[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x8E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const UInt8 kAddrLibCreatePatchedMask1404[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// Catalina only. Change loop condition to skip SDMA1_HP.
static const UInt8 kCreateAccelChannelsOriginal[] = {0x8D, 0x44, 0x09, 0x02};
static const UInt8 kCreateAccelChannelsPatched[] = {0x8D, 0x44, 0x09, 0x01};

//------ Module Logic ------//

static iVega::X5000 instance {};

iVega::X5000 &iVega::X5000::singleton() { return instance; }

void iVega::X5000::init() {
    PANIC_COND(this->initialised, "X5000", "Attempted to initialise module twice!");
    this->initialised = true;

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
        case KernelVersion::Tahoe:
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

    SYSLOG("X5000", "Module initialised.");

    lilu.onKextLoadForce(&kextRadeonX5000);

    lilu.onKextLoadForce(
        nullptr, 0,
        [](void *user, KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size) {
            static_cast<iVega::X5000 *>(user)->processKext(patcher, id, slide, size);
        },
        this);
}

void iVega::X5000::processKext(KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size) {
    if (kextRadeonX5000.loadIndex != id) { return; }

    NRed::singleton().hwLateInit();

    UInt32 *orgChannelTypes = nullptr;
    mach_vm_address_t orgStartHWEngines = 0;

    PatcherPlus::PatternSolveRequest solveRequests[] = {
        {NRed::singleton().getAttributes().isCatalina() ?
                "__ZZN37AMDRadeonX5000_AMDGraphicsAccelerator22getAdditionalQueueListEPPK18_"
                "AMDQueueSpecifierE27additionalQueueList_Default" :
                "__ZZN37AMDRadeonX5000_AMDGraphicsAccelerator19createAccelChannelsEbE12channelTypes",
            orgChannelTypes, kChannelTypesPattern},
        {"__ZN31AMDRadeonX5000_AMDGFX9PM4EngineC1Ev", this->orgGFX9PM4EngineConstructor},
        {"__ZN32AMDRadeonX5000_AMDGFX9SDMAEngineC1Ev", this->orgGFX9SDMAEngineConstructor},
        {"__ZN35AMDRadeonX5000_AMDAccelVideoContext10gMetaClassE", NRed::singleton().metaClassMap[0][0]},
        {"__ZN37AMDRadeonX5000_AMDAccelDisplayMachine10gMetaClassE", NRed::singleton().metaClassMap[1][0]},
        {"__ZN34AMDRadeonX5000_AMDAccelDisplayPipe10gMetaClassE", NRed::singleton().metaClassMap[2][0]},
        {"__ZN30AMDRadeonX5000_AMDAccelChannel10gMetaClassE", NRed::singleton().metaClassMap[3][1]},
        {"__ZN28AMDRadeonX5000_IAMDHWChannel10gMetaClassE", NRed::singleton().metaClassMap[4][0]},
        {"__ZN26AMDRadeonX5000_AMDHardware14startHWEnginesEv", orgStartHWEngines},
    };
    PANIC_COND(!PatcherPlus::PatternSolveRequest::solveAll(patcher, id, solveRequests, slide, size), "X5000",
        "Failed to resolve symbols");

    PatcherPlus::PatternRouteRequest requests[] = {
        {"__ZN32AMDRadeonX5000_AMDVega10Hardware17allocateHWEnginesEv", allocateHWEngines},
        {"__ZN32AMDRadeonX5000_AMDVega10Hardware32setupAndInitializeHWCapabilitiesEv",
            wrapSetupAndInitializeHWCapabilities, this->orgSetupAndInitializeHWCapabilities},
        {"__ZN30AMDRadeonX5000_AMDGFX9Hardware32setupAndInitializeHWCapabilitiesEv",
            wrapGFX9SetupAndInitializeHWCapabilities, this->orgGFX9SetupAndInitializeHWCapabilities},
        {"__ZN26AMDRadeonX5000_AMDHardware12getHWChannelE20_eAMD_HW_ENGINE_TYPE18_eAMD_HW_RING_TYPE", wrapGetHWChannel,
            this->orgGetHWChannel},
        {"__ZN30AMDRadeonX5000_AMDGFX9Hardware20initializeFamilyTypeEv", initializeFamilyType},
        {"__ZN30AMDRadeonX5000_AMDGFX9Hardware20allocateAMDHWDisplayEv", allocateAMDHWDisplay},
        {"__ZN26AMDRadeonX5000_AMDHWMemory17adjustVRAMAddressEy", wrapAdjustVRAMAddress, this->orgAdjustVRAMAddress},
        {"__ZN30AMDRadeonX5000_AMDGFX9Hardware25allocateAMDHWAlignManagerEv", wrapAllocateAMDHWAlignManager,
            this->orgAllocateAMDHWAlignManager},
        {"__ZN43AMDRadeonX5000_AMDVega10GraphicsAccelerator13getDeviceTypeEP11IOPCIDevice", getDeviceType},
        {"__ZN30AMDRadeonX5000_AMDGFX9Hardware20writeASICHangLogInfoEPPv", returnZero},
        {"__ZN4Addr2V27Gfx9Lib20HwlConvertChipFamilyEjj", wrapHwlConvertChipFamily, this->orgHwlConvertChipFamily,
            kHwlConvertChipFamilyPattern},
    };
    PANIC_COND(!PatcherPlus::PatternRouteRequest::routeAll(patcher, id, requests, slide, size), "X5000",
        "Failed to route symbols");

    if (NRed::singleton().getAttributes().isVentura1304AndLater()) {
        PatcherPlus::PatternRouteRequest request {
            "__ZN37AMDRadeonX5000_AMDGraphicsAccelerator23obtainAccelChannelGroupE11SS_"
            "PRIORITYP27AMDRadeonX5000_AMDAccelTask",
            wrapObtainAccelChannelGroup1304, this->orgObtainAccelChannelGroup};
        PANIC_COND(!request.route(patcher, id, slide, size), "X5000", "Failed to route obtainAccelChannelGroup");
    } else if (NRed::singleton().getAttributes().isBigSurAndLater()) {
        PatcherPlus::PatternRouteRequest request {
            "__ZN37AMDRadeonX5000_AMDGraphicsAccelerator23obtainAccelChannelGroupE11SS_PRIORITY",
            wrapObtainAccelChannelGroup, this->orgObtainAccelChannelGroup};
        PANIC_COND(!request.route(patcher, id, slide, size), "X5000", "Failed to route obtainAccelChannelGroup");
    }

    if (NRed::singleton().getAttributes().isSonoma1404AndLater()) {
        const PatcherPlus::MaskedLookupPatch patch {&kextRadeonX5000, kAddrLibCreateOriginal1404,
            kAddrLibCreateOriginalMask1404, kAddrLibCreatePatched1404, kAddrLibCreatePatchedMask1404, 1};
        PANIC_COND(!patch.apply(patcher, slide, size), "X5000", "Failed to apply 14.4+ Addr::Lib::Create patch");
    } else if (NRed::singleton().getAttributes().isCatalina() ||
               NRed::singleton().getAttributes().isVentura1304AndLater()) {
        const PatcherPlus::MaskedLookupPatch patch {&kextRadeonX5000, kAddrLibCreateOriginal, kAddrLibCreatePatched, 1};
        PANIC_COND(!patch.apply(patcher, slide, size), "X5000",
            "Failed to apply Catalina & Ventura 13.4+ Addr::Lib::Create patch");
    }

    if (NRed::singleton().getAttributes().isCatalina()) {
        const PatcherPlus::MaskedLookupPatch patch {&kextRadeonX5000, kCreateAccelChannelsOriginal,
            kCreateAccelChannelsPatched, 2};
        PANIC_COND(!patch.apply(patcher, slide, size), "X5000", "Failed to patch createAccelChannels");

        if (NRed::singleton().getAttributes().isRenoir()) {
            UInt32 findNonBpp64 = Dcn1NonBpp64SwModeMask1015;
            UInt32 replNonBpp64 = Dcn2NonBpp64SwModeMask1015;
            UInt32 findBpp64 = Dcn1NonBpp64SwModeMask1015 ^ Dcn1Bpp64SwModeMask1015;
            UInt32 replBpp64 = Dcn2NonBpp64SwModeMask1015 ^ Dcn2Bpp64SwModeMask1015;
            UInt32 findBpp64Pt2 = Dcn1Bpp64SwModeMask1015;
            UInt32 replBpp64Pt2 = Dcn2Bpp64SwModeMask1015;
            const PatcherPlus::MaskedLookupPatch patches[] = {
                {&kextRadeonX5000, reinterpret_cast<const UInt8 *>(&findNonBpp64),
                    reinterpret_cast<const UInt8 *>(&replNonBpp64), sizeof(UInt32), 2},
                {&kextRadeonX5000, reinterpret_cast<const UInt8 *>(&findBpp64),
                    reinterpret_cast<const UInt8 *>(&replBpp64), sizeof(UInt32), 1},
                {&kextRadeonX5000, reinterpret_cast<const UInt8 *>(&findBpp64Pt2),
                    reinterpret_cast<const UInt8 *>(&replBpp64Pt2), sizeof(UInt32), 1},
            };
            PANIC_COND(!PatcherPlus::MaskedLookupPatch::applyAll(patcher, patches, slide, size), "X5000",
                "Failed to patch swizzle mode");
        }

        PANIC_COND(MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) != KERN_SUCCESS, "X5000",
            "Failed to enable kernel writing");
        *orgChannelTypes = 1;    // Make VMPT use SDMA0 instead of SDMA1
        MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);
        DBGLOG("X5000", "Applied SDMA1 patches");
    } else {
        const PatcherPlus::MaskedLookupPatch patch {&kextRadeonX5000, kStartHWEnginesOriginal, kStartHWEnginesMask,
            kStartHWEnginesPatched, kStartHWEnginesMask,
            NRed::singleton().getAttributes().isVenturaAndLater() ? 2U : 1};
        PANIC_COND(!patch.apply(patcher, orgStartHWEngines, PAGE_SIZE), "X5000", "Failed to patch startHWEngines");

        if (NRed::singleton().getAttributes().isRenoir()) {
            UInt32 findBpp64 = Dcn1Bpp64SwModeMask, replBpp64 = Dcn2Bpp64SwModeMask;
            UInt32 findNonBpp64 = Dcn1NonBpp64SwModeMask, replNonBpp64 = Dcn2NonBpp64SwModeMask;
            const PatcherPlus::MaskedLookupPatch patches[] = {
                {&kextRadeonX5000, reinterpret_cast<const UInt8 *>(&findBpp64),
                    reinterpret_cast<const UInt8 *>(&replBpp64), sizeof(UInt32),
                    NRed::singleton().getAttributes().isVentura1304AndLater() ? 2U : 4},
                {&kextRadeonX5000, reinterpret_cast<const UInt8 *>(&findNonBpp64),
                    reinterpret_cast<const UInt8 *>(&replNonBpp64), sizeof(UInt32),
                    NRed::singleton().getAttributes().isVentura1304AndLater() ? 2U : 4},
            };
            PANIC_COND(!PatcherPlus::MaskedLookupPatch::applyAll(patcher, patches, slide, size), "X5000",
                "Failed to patch swizzle mode");
        }

        PANIC_COND(MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) != KERN_SUCCESS, "X5000",
            "Failed to enable kernel writing");
        // createAccelChannels: stop at SDMA0
        orgChannelTypes[5] = 1;
        // getPagingChannel: get only SDMA0
        orgChannelTypes[NRed::singleton().getAttributes().isMontereyAndLater() ? 12 : 11] = 0;
        MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);
        DBGLOG("X5000", "Applied SDMA1 patches");
    }
}

bool iVega::X5000::allocateHWEngines(void *that) {
    DBGLOG("X5000", "allocateHWEngines << (that: %p)", that);
    auto *pm4 = OSObject::operator new(0x340);
    singleton().orgGFX9PM4EngineConstructor(pm4);
    singleton().pm4EngineField.set(that, pm4);

    auto *sdma0 = OSObject::operator new(0x250);
    singleton().orgGFX9SDMAEngineConstructor(sdma0);
    singleton().sdma0EngineField.set(that, sdma0);

    DBGLOG("X5000", "allocateHWEngines >> true");
    return true;
}

void iVega::X5000::wrapSetupAndInitializeHWCapabilities(void *that) {
    DBGLOG("X5000", "setupAndInitializeHWCapabilities << (that: %p)", that);
    FunctionCast(wrapSetupAndInitializeHWCapabilities, singleton().orgSetupAndInitializeHWCapabilities)(that);

    singleton().displayPipeCountField.set(that, NRed::singleton().getAttributes().isRenoir() ? 4 : 6);
    singleton().hasUVD0Field.set(that, false);
    singleton().hasVCEField.set(that, false);
    singleton().hasVCN0Field.set(that, false);
    singleton().hasSDMAPagingQueueField.set(that, false);
    DBGLOG("X5000", "setupAndInitializeHWCapabilities >>");
}

void iVega::X5000::wrapGFX9SetupAndInitializeHWCapabilities(void *that) {
    DBGLOG("X5000", "GFX9::setupAndInitializeHWCapabilities << (that: %p)", that);
    char filename[128] = {0};
    snprintf(filename, arrsize(filename), "%s_gpu_info.bin",
        NRed::singleton().getAttributes().isRenoir() ? "renoir" : NRed::singleton().getAttributes().getChipName());
    const auto &gpuInfoBin = getFirmwareNamed(filename);
    auto *header = reinterpret_cast<const CommonFirmwareHeader *>(gpuInfoBin.data);
    auto *gpuInfo = reinterpret_cast<const GPUInfoFirmware *>(gpuInfoBin.data + header->ucodeOff);

    singleton().seCountField.set(that, gpuInfo->gcNumSe);
    singleton().shPerSEField.set(that, gpuInfo->gcNumShPerSe);
    singleton().cuPerSHField.set(that, gpuInfo->gcNumCuPerSh);

    FunctionCast(wrapGFX9SetupAndInitializeHWCapabilities, singleton().orgGFX9SetupAndInitializeHWCapabilities)(that);
    DBGLOG("X5000", "GFX9::setupAndInitializeHWCapabilities >>");
}

#ifdef DEBUG
static const char *hwEngineToString(AMDHWEngineType ty) {
    if (ty >= kAMDHWEngineTypeMax) { return "Unknown"; }
    static const char *names[11] = {"PM4", "SDMA0", "SDMA1", "SDMA2", "SDMA3", "UVD0", "UVD1", "VCE", "VCN0", "VCN1",
        "SAMU"};
    return names[ty];
}
#endif

void *iVega::X5000::wrapGetHWChannel(void *that, AMDHWEngineType engineType, UInt32 ringId) {
    DBGLOG_COND(checkKernelArgument("-CKInternal"), "X5000", "getHWChannel << (that: %p, engineType: %s, ringId: 0x%X)",
        that, hwEngineToString(engineType), ringId);
    if (engineType == kAMDHWEngineTypeSDMA1) { engineType = kAMDHWEngineTypeSDMA0; }
    return FunctionCast(wrapGetHWChannel, singleton().orgGetHWChannel)(that, engineType, ringId);
}

void iVega::X5000::initializeFamilyType(void *that) {
    DBGLOG("X5000", "initializeFamilyType << (that: %p)", that);
    singleton().familyTypeField.set(that, AMD_FAMILY_RAVEN);
    DBGLOG("X5000", "initializeFamilyType >>");
}

void *iVega::X5000::allocateAMDHWDisplay(void *that) {
    DBGLOG("X5000", "allocateAMDHWDisplay << (that: %p)", that);
    auto *ret = FunctionCast(allocateAMDHWDisplay, X6000::singleton().orgAllocateAMDHWDisplay)(that);
    DBGLOG("X5000", "allocateAMDHWDisplay >> %p", ret);
    return ret;
}

UInt64 iVega::X5000::wrapAdjustVRAMAddress(void *that, UInt64 addr) {
    auto ret = FunctionCast(wrapAdjustVRAMAddress, singleton().orgAdjustVRAMAddress)(that, addr);
    if (addr == ret) { return ret; }
    return ret + NRed::singleton().getFbOffset();
}

static UInt32 fakeGetPreferredSwizzleMode2(void *, void *pIn) { return getMember<UInt32>(pIn, 0x10); }

void *iVega::X5000::wrapAllocateAMDHWAlignManager(void *that) {
    DBGLOG("X5000", "allocateAMDHWAlignManager << (that: %p)", that);
    auto *hwAlignManager = FunctionCast(wrapAllocateAMDHWAlignManager, singleton().orgAllocateAMDHWAlignManager)(that);
    auto *vtableNew = IOMalloc(0x238);
    auto *vtableOriginal = getMember<void *>(hwAlignManager, 0);
    memcpy(vtableNew, vtableOriginal, 0x230);
    getMember<mach_vm_address_t>(vtableNew, 0x230) = reinterpret_cast<mach_vm_address_t>(fakeGetPreferredSwizzleMode2);
    getMember<void *>(hwAlignManager, 0) = vtableNew;
    return hwAlignManager;
}

UInt32 iVega::X5000::getDeviceType() { return NRed::singleton().getAttributes().isRenoir() ? 9 : 0; }

UInt32 iVega::X5000::returnZero() { return 0; }

static void fixAccelGroup(void *that) {
    auto *&sdma1 = getMember<void *>(that, 0x18);
    // Replace field with SDMA0 because we don't have SDMA1
    if (sdma1 == nullptr) { sdma1 = getMember<void *>(that, 0x10); }
}

void *iVega::X5000::wrapObtainAccelChannelGroup(void *that, UInt32 priority) {
    auto ret = FunctionCast(wrapObtainAccelChannelGroup, singleton().orgObtainAccelChannelGroup)(that, priority);
    if (ret == nullptr) { return nullptr; }
    fixAccelGroup(ret);
    return ret;
}

void *iVega::X5000::wrapObtainAccelChannelGroup1304(void *that, UInt32 priority, void *task) {
    auto ret =
        FunctionCast(wrapObtainAccelChannelGroup1304, singleton().orgObtainAccelChannelGroup)(that, priority, task);
    if (ret == nullptr) { return nullptr; }
    fixAccelGroup(ret);
    return ret;
}

UInt32 iVega::X5000::wrapHwlConvertChipFamily(void *that, UInt32 family, UInt32 revision) {
    DBGLOG("X5000", "HwlConvertChipFamily >> (that: %p family: 0x%X revision: 0x%X)", that, family, revision);
    if (family == AMD_FAMILY_RAVEN) {
        auto &settings = singleton().chipSettingsField.get(that);
        settings.isArcticIsland = 1;
        settings.isRaven = 1;
        if (NRed::singleton().getAttributes().isRenoir()) {
            settings.htileAlignFix = 1;
            settings.applyAliasFix = 1;
        } else if (NRed::singleton().getAttributes().isRaven()) {
            settings.depthPipeXorDisable = 1;
        }
        settings.isDcn1 = 1;
        settings.metaBaseAlignFix = 1;
        return ADDR_CHIP_FAMILY_AI;
    }
    return FunctionCast(wrapHwlConvertChipFamily, singleton().orgHwlConvertChipFamily)(that, family, revision);
}
