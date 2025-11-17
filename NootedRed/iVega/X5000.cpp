// AMDRadeonX5000 patches for Vega iGPUs
//
// Copyright © 2022-2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#include <GPUDriversAMD/Family.hpp>
#include <Headers/kern_api.hpp>
#include <Headers/kern_util.hpp>
#include <Kexts.hpp>
#include <NRed.hpp>
#include <PenguinWizardry/KernelVersion.hpp>
#include <PenguinWizardry/PatcherPlus.hpp>
#include <iVega/AMDGFX9DCN1Display.hpp>
#include <iVega/AMDGFX9DCN2Display.hpp>
#include <iVega/X5000.hpp>

static const UInt8 kChannelTypesPattern[] = {0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00};

static const UInt8 kHwlConvertChipFamilyPattern[] = {0x81, 0xFE, 0x8D, 0x00, 0x00, 0x00, 0x0F};

// Make for loop stop after one SDMA engine.
static const UInt8 kStartHWEnginesOriginal[] = {0x40, 0x83, 0xF0, 0x02};
static const UInt8 kStartHWEnginesMask[] = {0xF0, 0xFF, 0xF0, 0xFF};
static const UInt8 kStartHWEnginesPatched[] = {0x40, 0x83, 0xF0, 0x01};

// The check in `Addr::Lib::Create` on 10.15 and 13.4+ is `familyId == 0x8D` instead of `familyId - 0x8D < 2`.
// Change the 0x8D (AI) to 0x8E (RV).
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

static iVega::X5000 instance {};

iVega::X5000 &iVega::X5000::singleton() { return instance; }

iVega::X5000::X5000() {
    switch (getKernelVersion()) {
        case KernelVersion::Catalina: {
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
        } break;
        case KernelVersion::BigSur:
        case KernelVersion::Monterey: {
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
        } break;
        default: {
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
        } break;
    }
}

void iVega::X5000::processKext(KernelPatcher &patcher, const size_t id, const mach_vm_address_t slide,
    const size_t size) {
    if (kextRadeonX5000.loadIndex != id) { return; }

    NRed::singleton().hwLateInit();

    UInt32 *orgChannelTypes;
    mach_vm_address_t orgStartHWEngines;

    PenguinWizardry::PatternSolveRequest solveRequests[] = {
        {currentKernelVersion() == MACOS_10_15 ?
                "__ZZN37AMDRadeonX5000_AMDGraphicsAccelerator22getAdditionalQueueListEPPK18_"
                "AMDQueueSpecifierE27additionalQueueList_Default" :
                "__ZZN37AMDRadeonX5000_AMDGraphicsAccelerator19createAccelChannelsEbE12channelTypes",
            orgChannelTypes, kChannelTypesPattern},
        {"__ZN31AMDRadeonX5000_AMDGFX9PM4EngineC1Ev", this->pm4EngineConstructor},
        {"__ZN32AMDRadeonX5000_AMDGFX9SDMAEngineC1Ev", this->sdmaEngineConstructor},
        {"__ZN26AMDRadeonX5000_AMDHardware14startHWEnginesEv", orgStartHWEngines},
    };
    PANIC_COND(!PenguinWizardry::PatternSolveRequest::solveAll(patcher, id, solveRequests, slide, size), "X5000",
        "Failed to resolve symbols");

    AMDRadeonX5000_AMDHWDisplay::resolve(patcher, id, slide, size);
    AMDRadeonX5000_AMDGFX9DCNDisplay::registerMC(kextRadeonX5000.id, patcher, id, slide, size);
    AMDRadeonX5000_AMDGFX9DCN1Display::resolve(kextRadeonX5000.id);
    AMDRadeonX5000_AMDGFX9DCN2Display::resolve(kextRadeonX5000.id);

    PenguinWizardry::PatternRouteRequest requests[] = {
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
        {"__ZN30AMDRadeonX5000_AMDGFX9Hardware20writeASICHangLogInfoEPPv", returnZero},
        {"__ZN4Addr2V27Gfx9Lib20HwlConvertChipFamilyEjj", wrapHwlConvertChipFamily, this->orgHwlConvertChipFamily,
            kHwlConvertChipFamilyPattern},
    };
    PANIC_COND(!PenguinWizardry::PatternRouteRequest::routeAll(patcher, id, requests, slide, size), "X5000",
        "Failed to route symbols");

    if (currentKernelVersion() >= MACOS_13_4) {
        PenguinWizardry::PatternRouteRequest request {
            "__ZN37AMDRadeonX5000_AMDGraphicsAccelerator23obtainAccelChannelGroupE11SS_"
            "PRIORITYP27AMDRadeonX5000_AMDAccelTask",
            wrapObtainAccelChannelGroup1304, this->orgObtainAccelChannelGroup};
        PANIC_COND(!request.route(patcher, id, slide, size), "X5000", "Failed to route obtainAccelChannelGroup");
    } else if (currentKernelVersion() >= MACOS_11) {
        PenguinWizardry::PatternRouteRequest request {
            "__ZN37AMDRadeonX5000_AMDGraphicsAccelerator23obtainAccelChannelGroupE11SS_PRIORITY",
            wrapObtainAccelChannelGroup, this->orgObtainAccelChannelGroup};
        PANIC_COND(!request.route(patcher, id, slide, size), "X5000", "Failed to route obtainAccelChannelGroup");
    }

    if (currentKernelVersion() >= MACOS_14_4) {
        const PenguinWizardry::MaskedLookupPatch patch {&kextRadeonX5000, kAddrLibCreateOriginal1404,
            kAddrLibCreateOriginalMask1404, kAddrLibCreatePatched1404, kAddrLibCreatePatchedMask1404, 1};
        PANIC_COND(!patch.apply(patcher, slide, size), "X5000", "Failed to apply 14.4+ Addr::Lib::Create patch");
    } else if (currentKernelVersion() == MACOS_10_15 || currentKernelVersion() >= MACOS_13_4) {
        const PenguinWizardry::MaskedLookupPatch patch {&kextRadeonX5000, kAddrLibCreateOriginal, kAddrLibCreatePatched,
            1};
        PANIC_COND(!patch.apply(patcher, slide, size), "X5000",
            "Failed to apply Catalina & Ventura 13.4+ Addr::Lib::Create patch");
    }

    if (currentKernelVersion() == MACOS_10_15) {
        const PenguinWizardry::MaskedLookupPatch patch {&kextRadeonX5000, kCreateAccelChannelsOriginal,
            kCreateAccelChannelsPatched, 2};
        PANIC_COND(!patch.apply(patcher, slide, size), "X5000", "Failed to patch createAccelChannels");

        // TODO: wait, what is this doing again?
        if (NRed::singleton().getAttributes().isRenoir()) {
            UInt32 findNonBpp64 = Dcn1NonBpp64SwModeMask1015;
            UInt32 replNonBpp64 = Dcn2NonBpp64SwModeMask1015;
            UInt32 findBpp64 = Dcn1NonBpp64SwModeMask1015 ^ Dcn1Bpp64SwModeMask1015;
            UInt32 replBpp64 = Dcn2NonBpp64SwModeMask1015 ^ Dcn2Bpp64SwModeMask1015;
            UInt32 findBpp64Pt2 = Dcn1Bpp64SwModeMask1015;
            UInt32 replBpp64Pt2 = Dcn2Bpp64SwModeMask1015;
            const PenguinWizardry::MaskedLookupPatch patches[] = {
                {&kextRadeonX5000, reinterpret_cast<const UInt8 *>(&findNonBpp64),
                    reinterpret_cast<const UInt8 *>(&replNonBpp64), sizeof(UInt32), 2},
                {&kextRadeonX5000, reinterpret_cast<const UInt8 *>(&findBpp64),
                    reinterpret_cast<const UInt8 *>(&replBpp64), sizeof(UInt32), 1},
                {&kextRadeonX5000, reinterpret_cast<const UInt8 *>(&findBpp64Pt2),
                    reinterpret_cast<const UInt8 *>(&replBpp64Pt2), sizeof(UInt32), 1},
            };
            PANIC_COND(!PenguinWizardry::MaskedLookupPatch::applyAll(patcher, patches, slide, size), "X5000",
                "Failed to patch swizzle mode");
        }

        PANIC_COND(MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) != KERN_SUCCESS, "X5000",
            "Failed to enable kernel writing");
        *orgChannelTypes = 1;    // Make VMPT use SDMA0 instead of SDMA1
        MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);
        DBGLOG("X5000", "Applied SDMA1 patches");
    } else {
        const PenguinWizardry::MaskedLookupPatch patch {&kextRadeonX5000, kStartHWEnginesOriginal, kStartHWEnginesMask,
            kStartHWEnginesPatched, kStartHWEnginesMask, currentKernelVersion() >= MACOS_13 ? 2U : 1};
        PANIC_COND(!patch.apply(patcher, orgStartHWEngines, PAGE_SIZE), "X5000", "Failed to patch startHWEngines");

        if (NRed::singleton().getAttributes().isRenoir()) {
            UInt32 findBpp64 = Dcn1Bpp64SwModeMask, replBpp64 = Dcn2Bpp64SwModeMask;
            UInt32 findNonBpp64 = Dcn1NonBpp64SwModeMask, replNonBpp64 = Dcn2NonBpp64SwModeMask;
            const PenguinWizardry::MaskedLookupPatch patches[] = {
                {&kextRadeonX5000, reinterpret_cast<const UInt8 *>(&findBpp64),
                    reinterpret_cast<const UInt8 *>(&replBpp64), sizeof(UInt32),
                    currentKernelVersion() >= MACOS_13_4 ? 2U : 4},
                {&kextRadeonX5000, reinterpret_cast<const UInt8 *>(&findNonBpp64),
                    reinterpret_cast<const UInt8 *>(&replNonBpp64), sizeof(UInt32),
                    currentKernelVersion() >= MACOS_13_4 ? 2U : 4},
            };
            PANIC_COND(!PenguinWizardry::MaskedLookupPatch::applyAll(patcher, patches, slide, size), "X5000",
                "Failed to patch swizzle mode");
        }

        PANIC_COND(MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) != KERN_SUCCESS, "X5000",
            "Failed to enable kernel writing");
        // createAccelChannels: stop at SDMA0
        orgChannelTypes[5] = 1;
        // getPagingChannel: get only SDMA0
        orgChannelTypes[currentKernelVersion() >= MACOS_12 ? 12 : 11] = 0;
        MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);
        DBGLOG("X5000", "Applied SDMA1 patches");
    }
}

bool iVega::X5000::allocateHWEngines(void *const self) {
    [[clang::suppress]]
    auto *const pm4 = OSObject::operator new(0x340);
    singleton().pm4EngineConstructor(pm4);
    singleton().pm4EngineField(self) = pm4;

    [[clang::suppress]]
    auto *const sdma0 = OSObject::operator new(0x250);
    singleton().sdmaEngineConstructor(sdma0);
    singleton().sdma0EngineField(self) = sdma0;

    // No VCN? :-(

    return true;
}

void iVega::X5000::wrapSetupAndInitializeHWCapabilities(void *const self) {
    FunctionCast(wrapSetupAndInitializeHWCapabilities, singleton().orgSetupAndInitializeHWCapabilities)(self);

    singleton().displayPipeCountField(self) = NRed::singleton().getAttributes().isRenoir() ? 6 : 4;
    singleton().hasUVD0Field(self) = false;
    singleton().hasVCEField(self) = false;
    singleton().hasVCN0Field(self) = false;
    singleton().hasSDMAPagingQueueField(self) = false;
}

// TODO: Replace with IP Discovery
void iVega::X5000::wrapGFX9SetupAndInitializeHWCapabilities(void *const self) {
    auto &seCount = singleton().seCountField(self);
    auto &shPerSE = singleton().shPerSEField(self);
    auto &cuPerSH = singleton().cuPerSHField(self);

    seCount = 1;
    shPerSE = 1;
    if (NRed::singleton().getAttributes().isRenoir()) {
        cuPerSH = 8;
    } else if (NRed::singleton().getAttributes().isRaven2()) {
        cuPerSH = 3;
    } else {
        cuPerSH = 11;
    }

    FunctionCast(wrapGFX9SetupAndInitializeHWCapabilities, singleton().orgGFX9SetupAndInitializeHWCapabilities)(self);
}

#ifdef DEBUG
static const char *hwEngineToString(const AMDHWEngineType ty) {
    if (ty >= kAMDHWEngineTypeMax) { return "Unknown"; }
    static const char *names[11] = {"PM4", "SDMA0", "SDMA1", "SDMA2", "SDMA3", "UVD0", "UVD1", "VCE", "VCN0", "VCN1",
        "SAMU"};
    return names[ty];
}
#endif

// TODO: Investigate why this is needed.
void *iVega::X5000::wrapGetHWChannel(void *const self, AMDHWEngineType engineType, const UInt32 ringId) {
    DBGLOG_COND(checkKernelArgument("-CKInternal"), "X5000", "getHWChannel << (self: %p, engineType: %s, ringId: 0x%X)",
        self, hwEngineToString(engineType), ringId);
    if (engineType == kAMDHWEngineTypeSDMA1) { engineType = kAMDHWEngineTypeSDMA0; }
    return FunctionCast(wrapGetHWChannel, singleton().orgGetHWChannel)(self, engineType, ringId);
}

void iVega::X5000::initializeFamilyType(void *const self) { singleton().familyTypeField(self) = AMD_FAMILY_RAVEN; }

void *iVega::X5000::allocateAMDHWDisplay(void *const) {
    if (NRed::singleton().getAttributes().isRenoir()) {
        return AMDRadeonX5000_AMDGFX9DCN2Display::gRTMetaClass.alloc();
    }
    return AMDRadeonX5000_AMDGFX9DCN1Display::gRTMetaClass.alloc();
}

UInt64 iVega::X5000::wrapAdjustVRAMAddress(void *const self, const UInt64 addr) {
    auto ret = FunctionCast(wrapAdjustVRAMAddress, singleton().orgAdjustVRAMAddress)(self, addr);
    if (ret != addr) { return ret + NRed::singleton().getFbOffset(); }
    return ret;
}

UInt32 iVega::X5000::returnZero() { return 0; }

// TODO: Investigate why this is needed.
static void fixAccelGroup(void *const self) {
    auto *&sdma1 = getMember<void *>(self, 0x18);
    // Replace field with SDMA0 because we don't have SDMA1
    if (sdma1 == nullptr) { sdma1 = getMember<void *>(self, 0x10); }
}

void *iVega::X5000::wrapObtainAccelChannelGroup(void *const self, const UInt32 priority) {
    auto ret = FunctionCast(wrapObtainAccelChannelGroup, singleton().orgObtainAccelChannelGroup)(self, priority);
    if (ret == nullptr) { return nullptr; }
    fixAccelGroup(ret);
    return ret;
}

void *iVega::X5000::wrapObtainAccelChannelGroup1304(void *const self, const UInt32 priority, void *const task) {
    auto ret =
        FunctionCast(wrapObtainAccelChannelGroup1304, singleton().orgObtainAccelChannelGroup)(self, priority, task);
    if (ret == nullptr) { return nullptr; }
    fixAccelGroup(ret);
    return ret;
}

UInt32 iVega::X5000::wrapHwlConvertChipFamily(void *const self, const UInt32 family, const UInt32 revision) {
    DBGLOG("X5000", "HwlConvertChipFamily >> (self: %p family: 0x%X revision: 0x%X)", self, family, revision);
    if (family == AMD_FAMILY_RAVEN) {
        auto &settings = singleton().chipSettingsField(self);
        settings.isArcticIsland = 1;
        settings.isRaven = 1;
        if (NRed::singleton().getAttributes().isRenoir()) {
            settings.htileAlignFix = 1;
            settings.applyAliasFix = 1;
        } else if (!NRed::singleton().getAttributes().isRaven2()) {
            settings.depthPipeXorDisable = 1;
        }
        settings.isDcn1 = 1;    // what to do about this?
        settings.metaBaseAlignFix = 1;
        return ADDR_CHIP_FAMILY_AI;
    }
    return FunctionCast(wrapHwlConvertChipFamily, singleton().orgHwlConvertChipFamily)(self, family, revision);
}
