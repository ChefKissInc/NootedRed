//  Copyright © 2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.0. See LICENSE for
//  details.

#include "kern_wred.hpp"
#include "kern_amd.hpp"
#include "kern_model.hpp"
#include <Headers/kern_api.hpp>
#include <Headers/kern_devinfo.hpp>

static const char *pathAGDP = "/System/Library/Extensions/AppleGraphicsControl.kext/Contents/PlugIns/"
                              "AppleGraphicsDevicePolicy.kext/Contents/MacOS/AppleGraphicsDevicePolicy";

static const char *pathRadeonX5000HWLibs = "/System/Library/Extensions/AMDRadeonX5000HWServices.kext/Contents/PlugIns/"
                                           "AMDRadeonX5000HWLibs.kext/Contents/MacOS/AMDRadeonX5000HWLibs";
static const char *pathRadeonX6000Framebuffer =
    "/System/Library/Extensions/AMDRadeonX6000Framebuffer.kext/Contents/MacOS/AMDRadeonX6000Framebuffer";
static const char *pathRadeonX6000 = "/System/Library/Extensions/AMDRadeonX6000.kext/Contents/MacOS/AMDRadeonX6000";
static const char *pathRadeonX5000 = "/System/Library/Extensions/AMDRadeonX5000.kext/Contents/MacOS/AMDRadeonX5000";

static KernelPatcher::KextInfo kextAGDP {"com.apple.driver.AppleGraphicsDevicePolicy", &pathAGDP, 1, {true}, {},
    KernelPatcher::KextInfo::Unloaded};

static KernelPatcher::KextInfo kextRadeonX5000HWLibs {"com.apple.kext.AMDRadeonX5000HWLibs", &pathRadeonX5000HWLibs, 1,
    {}, {}, KernelPatcher::KextInfo::Unloaded};

static KernelPatcher::KextInfo kextRadeonX6000Framebuffer {"com.apple.kext.AMDRadeonX6000Framebuffer",
    &pathRadeonX6000Framebuffer, 1, {}, {}, KernelPatcher::KextInfo::Unloaded};

static KernelPatcher::KextInfo kextRadeonX6000 = {"com.apple.kext.AMDRadeonX6000", &pathRadeonX6000, 1, {}, {},
    KernelPatcher::KextInfo::Unloaded};

static KernelPatcher::KextInfo kextRadeonX5000 {"com.apple.kext.AMDRadeonX5000", &pathRadeonX5000, 1, {}, {},
    KernelPatcher::KextInfo::Unloaded};

WRed *WRed::callbackWRed = nullptr;

void WRed::init() {
    SYSLOG(MODULE_SHORT, "Please don't support tonymacx86.com!");
    callbackWRed = this;

    lilu.onPatcherLoadForce(
        [](void *user, KernelPatcher &patcher) { static_cast<WRed *>(user)->processPatcher(patcher); }, this);
    lilu.onKextLoad(&kextAGDP);
    lilu.onKextLoadForce(&kextRadeonX5000HWLibs);
    lilu.onKextLoadForce(&kextRadeonX6000);
    lilu.onKextLoadForce(&kextRadeonX5000);
    lilu.onKextLoadForce(&kextRadeonX6000Framebuffer);
    lilu.onKextLoadForce(
        nullptr, 0,
        [](void *user, KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
            static_cast<WRed *>(user)->processKext(patcher, index, address, size);
        },
        this);
}

void WRed::deinit() { OSSafeReleaseNULL(this->vbiosData); }

void WRed::processPatcher(KernelPatcher &patcher) {
    auto *devInfo = DeviceInfo::create();
    if (!devInfo) {
        SYSLOG(MODULE_SHORT, "Failed to create DeviceInfo");
        return;
    }

    devInfo->processSwitchOff();

    auto *videoBuiltin = devInfo->videoBuiltin;
    if (!videoBuiltin) {
        SYSLOG(MODULE_SHORT, "videoBuiltin null");
        for (size_t i = 0; i < devInfo->videoExternal.size(); i++) {
            if (!OSDynamicCast(IOPCIDevice, devInfo->videoExternal[i].video)) { continue; }
            if (WIOKit::readPCIConfigValue(devInfo->videoExternal[i].video, WIOKit::kIOPCIConfigVendorID) ==
                WIOKit::VendorID::ATIAMD) {
                videoBuiltin = devInfo->videoExternal[i].video;
                break;
            }
        }
    }

    PANIC_COND(!videoBuiltin, MODULE_SHORT, "videoBuiltin null");
    auto *iGPU = OSDynamicCast(IOPCIDevice, videoBuiltin);
    PANIC_COND(!iGPU, MODULE_SHORT, "videoBuiltin is not IOPCIDevice");
    PANIC_COND(WIOKit::readPCIConfigValue(iGPU, WIOKit::kIOPCIConfigVendorID) != WIOKit::VendorID::ATIAMD, MODULE_SHORT,
        "videoBuiltin is not AMD");

    callbackWRed->iGPU = iGPU;

    WIOKit::renameDevice(iGPU, "IGPU");
    WIOKit::awaitPublishing(iGPU);

    static uint8_t builtin[] = {0x01};
    iGPU->setProperty("built-in", builtin, arrsize(builtin));
    callbackWRed->deviceId = WIOKit::readPCIConfigValue(iGPU, WIOKit::kIOPCIConfigDeviceID);
    auto *model = getBranding(callbackWRed->deviceId, WIOKit::readPCIConfigValue(iGPU, WIOKit::kIOPCIConfigRevisionID));
    if (model) { iGPU->setProperty("model", model); }

    if (UNLIKELY(iGPU->getProperty("ATY,bin_image"))) {
        DBGLOG(MODULE_SHORT, "VBIOS manually overridden");
    } else {
        if (!callbackWRed->getVBIOSFromVFCT(iGPU)) {
            SYSLOG(MODULE_SHORT, "Failed to get VBIOS from VFCT.");
            PANIC_COND(!callbackWRed->getVBIOSFromVRAM(iGPU), MODULE_SHORT, "Failed to get VBIOS from VRAM");
        }
    }

    callbackWRed->rmmio = iGPU->mapDeviceMemoryWithRegister(kIOPCIConfigBaseAddress5);
    PANIC_COND(!callbackWRed->rmmio || !callbackWRed->rmmio->getLength(), MODULE_SHORT, "Failed to map RMMIO");
    callbackWRed->rmmioPtr = reinterpret_cast<uint32_t *>(callbackWRed->rmmio->getVirtualAddress());

    callbackWRed->fbOffset = static_cast<uint64_t>(callbackWRed->readReg32(0x296B)) << 24;
    callbackWRed->revision = (callbackWRed->readReg32(0xD2F) & 0xF000000) >> 0x18;
    switch (callbackWRed->deviceId) {
        case 0x15D8:
            if (callbackWRed->revision >= 0x8) {
                callbackWRed->chipType = ChipType::Raven2;
                callbackWRed->enumeratedRevision = 0x79;
            } else {
                callbackWRed->chipType = ChipType::Picasso;
                callbackWRed->enumeratedRevision = 0x41;
            }
            break;
        case 0x15DD:
            if (callbackWRed->revision >= 0x8) {
                callbackWRed->chipType = ChipType::Raven2;
                callbackWRed->enumeratedRevision = 0x79;
            } else {
                callbackWRed->chipType = ChipType::Raven;
                callbackWRed->enumeratedRevision = 0x10;
            }
            break;
        case 0x164C:
            [[fallthrough]];
        case 0x1636:
            callbackWRed->chipType = ChipType::Renoir;
            callbackWRed->enumeratedRevision = 0x91;
            break;
        case 0x15E7:
            [[fallthrough]];
        case 0x1638:
            callbackWRed->chipType = ChipType::GreenSardine;
            callbackWRed->enumeratedRevision = 0xA1;
            break;
        default:
            PANIC(MODULE_SHORT, "Unknown device ID");
    }

    DeviceInfo::deleter(devInfo);

    KernelPatcher::RouteRequest requests[] = {
        {"__ZN15OSMetaClassBase12safeMetaCastEPKS_PK11OSMetaClass", wrapSafeMetaCast, orgSafeMetaCast},
    };
    PANIC_COND(!patcher.routeMultiple(KernelPatcher::KernelID, requests), MODULE_SHORT,
        "Failed to route OSMetaClassBase::safeMetaCast");
}

OSMetaClassBase *WRed::wrapSafeMetaCast(const OSMetaClassBase *anObject, const OSMetaClass *toMeta) {
    auto ret = FunctionCast(wrapSafeMetaCast, callbackWRed->orgSafeMetaCast)(anObject, toMeta);
    if (!ret) {
        for (const auto &ent : callbackWRed->metaClassMap) {
            if (ent[0] == toMeta) {
                return FunctionCast(wrapSafeMetaCast, callbackWRed->orgSafeMetaCast)(anObject, ent[1]);
            } else if (ent[1] == toMeta) {
                return FunctionCast(wrapSafeMetaCast, callbackWRed->orgSafeMetaCast)(anObject, ent[0]);
            }
        }
    }
    return ret;
}

void WRed::processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
    if (kextAGDP.loadIndex == index) {
        const uint8_t find[] = {0x83, 0xF8, 0x02};
        const uint8_t repl[] = {0x83, 0xF8, 0x00};
        KernelPatcher::LookupPatch patches[] = {
            {&kextAGDP, reinterpret_cast<const uint8_t *>("board-id"), reinterpret_cast<const uint8_t *>("board-ix"),
                sizeof("board-id"), 1},
            {&kextAGDP, find, repl, arrsize(find), 1},
        };
        for (auto &patch : patches) {
            patcher.applyLookupPatch(&patch);
            SYSLOG_COND(patcher.getError() != KernelPatcher::Error::NoError, MODULE_SHORT,
                "Failed to apply AGDP patch: %d", patcher.getError());
            patcher.clearError();
        }
    } else if (kextRadeonX5000HWLibs.loadIndex == index) {
        CailAsicCapEntry *orgAsicCapsTable = nullptr;
        CailInitAsicCapEntry *orgAsicInitCapsTable = nullptr;
        uint32_t *ddiCapsRaven = nullptr;
        uint32_t *ddiCapsRaven2 = nullptr;
        uint32_t *ddiCapsPicasso = nullptr;
        uint32_t *ddiCapsRenoir = nullptr;
        void *goldenSettingsRaven = nullptr;
        void *goldenSettingsRaven2 = nullptr;
        void *goldenSettingsPicasso = nullptr;
        void *goldenSettingsRenoir = nullptr;
        uint32_t *orgDeviceTypeTable = nullptr;

        KernelPatcher::SolveRequest solveRequests[] = {
            {"__ZL15deviceTypeTable", orgDeviceTypeTable},
            {"__ZN11AMDFirmware14createFirmwareEPhjjPKc", orgCreateFirmware},
            {"__ZN20AMDFirmwareDirectory11putFirmwareE16_AMD_DEVICE_TYPEP11AMDFirmware", orgPutFirmware},
            {"__ZN31AtiAppleVega10PowerTuneServicesC1EP11PP_InstanceP18PowerPlayCallbacks",
                orgVega10PowerTuneConstructor},
            {"__ZN31AtiAppleVega20PowerTuneServicesC1EP11PP_InstanceP18PowerPlayCallbacks",
                orgVega20PowerTuneConstructor},
            {"__ZL20CAIL_ASIC_CAPS_TABLE", orgAsicCapsTable},
            {"_CAILAsicCapsInitTable", orgAsicInitCapsTable},
            {"_Raven_SendMsgToSmc", orgRavenSendMsgToSmc},
            {"_Renoir_SendMsgToSmc", orgRenoirSendMsgToSmc},
            {"__ZN20AMDFirmwareDirectoryC1Ej", orgAMDFirmwareDirectoryConstructor},
            {"_RAVEN1_GoldenSettings_A0", goldenSettingsRaven},
            {"_RAVEN2_GoldenSettings_A0", goldenSettingsRaven2},
            {"_PICASSO_GoldenSettings_A0", goldenSettingsPicasso},
            {"_RENOIR_GoldenSettings_A0", goldenSettingsRenoir},
            {"_CAIL_DDI_CAPS_RAVEN_A0", ddiCapsRaven},
            {"_CAIL_DDI_CAPS_RAVEN2_A0", ddiCapsRaven2},
            {"_CAIL_DDI_CAPS_PICASSO_A0", ddiCapsPicasso},
            {"_CAIL_DDI_CAPS_RENOIR_A0", ddiCapsRenoir},
        };
        PANIC_COND(!patcher.solveMultiple(index, solveRequests, address, size), MODULE_SHORT,
            "Failed to resolve AMDRadeonX5000HWLibs symbols");

        PANIC_COND(MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) != KERN_SUCCESS, MODULE_SHORT,
            "Failed to enable kernel writing");
        DBGLOG(MODULE_SHORT, "Patching device type table");
        orgDeviceTypeTable[0] = deviceId;
        orgDeviceTypeTable[1] = 0;

        DBGLOG(MODULE_SHORT, "Patching HWLibs caps tables");
        orgAsicInitCapsTable->familyId = orgAsicCapsTable->familyId = AMDGPU_FAMILY_RV;
        orgAsicInitCapsTable->deviceId = orgAsicCapsTable->deviceId = callbackWRed->deviceId;
        orgAsicInitCapsTable->revision = orgAsicCapsTable->revision = callbackWRed->revision;
        orgAsicInitCapsTable->emulatedRev = orgAsicCapsTable->emulatedRev =
            callbackWRed->enumeratedRevision + callbackWRed->revision;
        orgAsicInitCapsTable->pciRev = orgAsicCapsTable->pciRev = 0xFFFFFFFF;
        switch (callbackWRed->chipType) {
            case ChipType::Raven:
                orgAsicInitCapsTable->caps = orgAsicCapsTable->caps = ddiCapsRaven;
                orgAsicInitCapsTable->goldenCaps = goldenSettingsRaven;
                break;
            case ChipType::Raven2:
                orgAsicInitCapsTable->caps = orgAsicCapsTable->caps = ddiCapsRaven2;
                orgAsicInitCapsTable->goldenCaps = goldenSettingsRaven2;
                break;
            case ChipType::Picasso:
                orgAsicInitCapsTable->caps = orgAsicCapsTable->caps = ddiCapsPicasso;
                orgAsicInitCapsTable->goldenCaps = goldenSettingsPicasso;
                break;
            case ChipType::Renoir:
                [[fallthrough]];
            case ChipType::GreenSardine:
                orgAsicInitCapsTable->caps = orgAsicCapsTable->caps = ddiCapsRenoir;
                orgAsicInitCapsTable->goldenCaps = goldenSettingsRenoir;
                break;
            default:
                PANIC(MODULE_SHORT, "Unknown ASIC type %d", callbackWRed->chipType);
        }
        MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);

        KernelPatcher::RouteRequest requests[] = {
            {"__ZN35AMDRadeonX5000_AMDRadeonHWLibsX500025populateFirmwareDirectoryEv", wrapPopulateFirmwareDirectory},
            {"__ZN25AtiApplePowerTuneServices23createPowerTuneServicesEP11PP_InstanceP18PowerPlayCallbacks",
                wrapCreatePowerTuneServices},
            {"_gc_get_hw_version", wrapGcGetHwVersion},
            {"_smu_get_hw_version", wrapSmuGetHwVersion},
            {"_smu_get_fw_constants", hwLibsNoop},
            {"_smu_9_0_1_check_fw_status", hwLibsNoop},
            {"_smu_9_0_1_unload_smu", hwLibsNoop},
            {"_psp_sw_init", wrapPspSwInit, orgPspSwInit},
            {"_psp_bootloader_is_sos_running", hwLibsNoop},
            {"_psp_bootloader_load_sos", hwLibsNoop},
            {"_psp_bootloader_load_sysdrv_3_1", hwLibsNoop},
            {"_psp_xgmi_is_support", hwLibsUnsupported},
            {"_psp_rap_is_supported", hwLibsUnsupported},
            {"_psp_cmd_km_submit", wrapPspCmdKmSubmit, orgPspCmdKmSubmit},
            {"_SmuRaven_Initialize", wrapSmuRavenInitialize, orgSmuRavenInitialize},
            {"_SmuRenoir_Initialize", wrapSmuRenoirInitialize, orgSmuRenoirInitialize},
        };
        PANIC_COND(!patcher.routeMultiple(index, requests, address, size), MODULE_SHORT,
            "Failed to route AMDRadeonX5000HWLibs symbols");

        /**
         * Patch for `_smu_9_0_1_full_asic_reset`
         * Correct sent message to `0x1E` as the original code sends `0x3B` which is wrong for SMU 10/12.
         */
        const uint8_t find[] = {0x55, 0x48, 0x89, 0xE5, 0x8B, 0x56, 0x04, 0xBE, 0x3B, 0x00, 0x00, 0x00, 0x5D, 0xE9,
            0x51, 0xFE, 0xFF, 0xFF};
        const uint8_t repl[] = {0x55, 0x48, 0x89, 0xE5, 0x8B, 0x56, 0x04, 0xBE, 0x1E, 0x00, 0x00, 0x00, 0x5D, 0xE9,
            0x51, 0xFE, 0xFF, 0xFF};
        static_assert(arrsize(find) == arrsize(repl));
        KernelPatcher::LookupPatch patch = {&kextRadeonX5000HWLibs, find, repl, arrsize(find), 1};
        patcher.applyLookupPatch(&patch);
        patcher.clearError();
    } else if (kextRadeonX6000Framebuffer.loadIndex == index) {
        static const uint32_t ddiCapsRaven[16] = {0x800005U, 0x500011FEU, 0x80000U, 0x11001000U, 0x200U, 0x68000001U,
            0x20000000, 0x4002U, 0x22420001U, 0x9E20E10U, 0x2000120U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U};
        static const uint32_t ddiCapsRenoir[16] = {0x800005U, 0x500011FEU, 0x80000U, 0x11001000U, 0x200U, 0x68000001U,
            0x20000000, 0x4002U, 0x22420001U, 0x9E20E18U, 0x2000120U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U};

        CailAsicCapEntry *orgAsicCapsTable = nullptr;

        KernelPatcher::SolveRequest solveRequests[] = {
            {"__ZL20CAIL_ASIC_CAPS_TABLE", orgAsicCapsTable},
        };
        PANIC_COND(!patcher.solveMultiple(index, solveRequests, address, size, true), MODULE_SHORT,
            "Failed to resolve AMDRadeonX6000Framebuffer symbols");

        PANIC_COND(MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) != KERN_SUCCESS, MODULE_SHORT,
            "Failed to enable kernel writing");
        DBGLOG(MODULE_SHORT, "Patching X6000FB caps table");
        orgAsicCapsTable->familyId = AMDGPU_FAMILY_RV;
        orgAsicCapsTable->caps = callbackWRed->chipType < ChipType::Renoir ? ddiCapsRaven : ddiCapsRenoir;
        orgAsicCapsTable->deviceId = callbackWRed->deviceId;
        orgAsicCapsTable->revision = callbackWRed->revision;
        orgAsicCapsTable->emulatedRev = callbackWRed->enumeratedRevision + callbackWRed->revision;
        orgAsicCapsTable->pciRev = 0xFFFFFFFF;
        MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);

        KernelPatcher::RouteRequest requests[] = {
            {"__ZNK15AmdAtomVramInfo16populateVramInfoER16AtomFirmwareInfo", wrapPopulateVramInfo},
            {"__ZN30AMDRadeonX6000_AmdAsicInfoNavi18populateDeviceInfoEv", wrapPopulateDeviceInfo,
                orgPopulateDeviceInfo},
            {"__ZNK32AMDRadeonX6000_AmdAsicInfoNavi1027getEnumeratedRevisionNumberEv", wrapGetEnumeratedRevision},
            {"__ZN32AMDRadeonX6000_AmdRegisterAccess11hwReadReg32Ej", wrapHwReadReg32, orgHwReadReg32},
            {"__ZN24AMDRadeonX6000_AmdLogger15initWithPciInfoEP11IOPCIDevice", wrapInitWithPciInfo, orgInitWithPciInfo},
            {"__ZN34AMDRadeonX6000_AmdRadeonController10doGPUPanicEPKcz", wrapDoGPUPanic},
        };
        PANIC_COND(!patcher.routeMultiple(index, requests, address, size), MODULE_SHORT,
            "Failed to route AMDRadeonX6000Framebuffer symbols");

        /** Neutralise VRAM Info creation null check to proceed with Controller Core Services initialisation. */
        const uint8_t find_null_check1[] = {0x48, 0x89, 0x83, 0x90, 0x00, 0x00, 0x00, 0x48, 0x85, 0xC0, 0x0F, 0x84,
            0x89, 0x00, 0x00, 0x00, 0x48, 0x8B, 0x7B, 0x18};
        const uint8_t repl_null_check1[] = {0x48, 0x89, 0x83, 0x90, 0x00, 0x00, 0x00, 0x90, 0x90, 0x90, 0x90, 0x90,
            0x90, 0x90, 0x90, 0x90, 0x48, 0x8B, 0x7B, 0x18};
        static_assert(arrsize(find_null_check1) == arrsize(repl_null_check1));

        /** Neutralise PSP Firmware Info creation null check to proceed with Controller Core Services
           initialisation. */
        const uint8_t find_null_check2[] = {0x48, 0x89, 0x83, 0x88, 0x00, 0x00, 0x00, 0x48, 0x85, 0xC0, 0x0F, 0x84,
            0xA1, 0x00, 0x00, 0x00, 0x48, 0x8B, 0x7B, 0x18};
        const uint8_t repl_null_check2[] = {0x48, 0x89, 0x83, 0x88, 0x00, 0x00, 0x00, 0x90, 0x90, 0x90, 0x90, 0x90,
            0x90, 0x90, 0x90, 0x90, 0x48, 0x8B, 0x7B, 0x18};
        static_assert(arrsize(find_null_check2) == arrsize(repl_null_check2));

        /** Neutralise VRAM Info null check inside `AmdAtomFwServices::getFirmwareInfo`. */
        const uint8_t find_null_check3[] = {0x48, 0x83, 0xBB, 0x90, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x84, 0x90, 0x00,
            0x00, 0x00, 0x49, 0x89, 0xF7, 0xBA, 0x60, 0x00, 0x00, 0x00};
        const uint8_t repl_null_check3[] = {0x48, 0x83, 0xBB, 0x90, 0x00, 0x00, 0x00, 0x00, 0x90, 0x90, 0x90, 0x90,
            0x90, 0x90, 0x49, 0x89, 0xF7, 0xBA, 0x60, 0x00, 0x00, 0x00};
        static_assert(arrsize(find_null_check3) == arrsize(repl_null_check3));

        /** Tell AGDC that we're an iGPU */
        const uint8_t find_getVendorInfo[] = {0xC7, 0x03, 0x00, 0x00, 0x03, 0x00, 0x48, 0xB8, 0x02, 0x10, 0x00, 0x00,
            0x02, 0x00, 0x00, 0x00};
        const uint8_t repl_getVendorInfo[] = {0xC7, 0x03, 0x00, 0x00, 0x03, 0x00, 0x48, 0xB8, 0x02, 0x10, 0x00, 0x00,
            0x01, 0x00, 0x00, 0x00};
        static_assert(arrsize(find_getVendorInfo) == arrsize(repl_getVendorInfo));

        KernelPatcher::LookupPatch patches[] = {
            {&kextRadeonX6000Framebuffer, find_null_check1, repl_null_check1, arrsize(find_null_check1), 1},
            {&kextRadeonX6000Framebuffer, find_null_check2, repl_null_check2, arrsize(find_null_check2), 1},
            {&kextRadeonX6000Framebuffer, find_null_check3, repl_null_check3, arrsize(find_null_check3), 1},
            {&kextRadeonX6000Framebuffer, find_getVendorInfo, repl_getVendorInfo, arrsize(find_getVendorInfo), 1},
        };
        for (auto &patch : patches) {
            patcher.applyLookupPatch(&patch);
            patcher.clearError();
        }
    } else if (kextRadeonX5000.loadIndex == index) {
        uint32_t *orgChannelTypes = nullptr;

        KernelPatcher::SolveRequest solveRequests[] = {
            {"__ZN31AMDRadeonX5000_AMDGFX9PM4EngineC1Ev", orgGFX9PM4EngineConstructor},
            {"__ZN32AMDRadeonX5000_AMDGFX9SDMAEngineC1Ev", orgGFX9SDMAEngineConstructor},
            {"__ZZN37AMDRadeonX5000_AMDGraphicsAccelerator19createAccelChannelsEbE12channelTypes", orgChannelTypes},
            {"__ZN39AMDRadeonX5000_AMDAccelSharedUserClient5startEP9IOService", orgAccelSharedUCStart},
            {"__ZN39AMDRadeonX5000_AMDAccelSharedUserClient4stopEP9IOService", orgAccelSharedUCStop},
            {"__ZN35AMDRadeonX5000_AMDAccelVideoContext10gMetaClassE", metaClassMap[0][0]},
            {"__ZN37AMDRadeonX5000_AMDAccelDisplayMachine10gMetaClassE", metaClassMap[1][0]},
            {"__ZN34AMDRadeonX5000_AMDAccelDisplayPipe10gMetaClassE", metaClassMap[2][0]},
            {"__ZN30AMDRadeonX5000_AMDAccelChannel10gMetaClassE", metaClassMap[3][1]},
        };
        PANIC_COND(!patcher.solveMultiple(index, solveRequests, address, size), MODULE_SHORT,
            "Failed to resolve AMDRadeonX5000 symbols");

        /** Patch the data so that it only starts SDMA0. */
        PANIC_COND(MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) != KERN_SUCCESS, MODULE_SHORT,
            "Failed to enable kernel writing");
        orgChannelTypes[5] = 1;     // Fix createAccelChannels
        orgChannelTypes[11] = 0;    // Fix getPagingChannel
        MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);

        KernelPatcher::RouteRequest requests[] = {
            {"__ZN32AMDRadeonX5000_AMDVega10Hardware17allocateHWEnginesEv", wrapAllocateHWEngines},
            {"__ZN32AMDRadeonX5000_AMDVega10Hardware32setupAndInitializeHWCapabilitiesEv",
                wrapSetupAndInitializeHWCapabilities, orgSetupAndInitializeHWCapabilities},
            {"__ZN32AMDRadeonX5000_AMDVega20Hardware17allocateHWEnginesEv", wrapAllocateHWEngines},
            {"__ZN32AMDRadeonX5000_AMDVega20Hardware32setupAndInitializeHWCapabilitiesEv",
                wrapSetupAndInitializeHWCapabilities, orgSetupAndInitializeHWCapabilitiesVega20},
            {"__ZN28AMDRadeonX5000_AMDRTHardware12getHWChannelE18_eAMD_CHANNEL_TYPE11SS_PRIORITYj", wrapRTGetHWChannel,
                orgRTGetHWChannel},
            {"__ZN30AMDRadeonX5000_AMDGFX9Hardware20initializeFamilyTypeEv", wrapInitializeFamilyType},
            {"__ZN30AMDRadeonX5000_AMDGFX9Hardware20allocateAMDHWDisplayEv", wrapAllocateAMDHWDisplay},
            {"__ZN41AMDRadeonX5000_AMDGFX9GraphicsAccelerator15newVideoContextEv", wrapNewVideoContext},
            {"__ZN31AMDRadeonX5000_IAMDSMLInterface18createSMLInterfaceEj", wrapCreateSMLInterface},
            {"__ZN26AMDRadeonX5000_AMDHWMemory17adjustVRAMAddressEy", wrapAdjustVRAMAddress, orgAdjustVRAMAddress},
            {"__ZN37AMDRadeonX5000_AMDGraphicsAccelerator9newSharedEv", wrapNewShared},
            {"__ZN37AMDRadeonX5000_AMDGraphicsAccelerator19newSharedUserClientEv", wrapNewSharedUserClient},
            {"__ZN30AMDRadeonX5000_AMDGFX9Hardware25allocateAMDHWAlignManagerEv", wrapAllocateAMDHWAlignManager,
                orgAllocateAMDHWAlignManager},
        };
        PANIC_COND(!patcher.routeMultiple(index, requests, address, size), MODULE_SHORT,
            "Failed to route AMDRadeonX5000 symbols");

        /**
         * `AMDRadeonX5000_AMDHardware::startHWEngines`
         * Make for loop stop at 1 instead of 2 since we only have one SDMA engine.
         */
        const uint8_t find[] = {0x49, 0x89, 0xFE, 0x31, 0xDB, 0x48, 0x83, 0xFB, 0x02, 0x74, 0x50};
        const uint8_t repl[] = {0x49, 0x89, 0xFE, 0x31, 0xDB, 0x48, 0x83, 0xFB, 0x01, 0x74, 0x50};
        static_assert(arrsize(find) == arrsize(repl));
        KernelPatcher::LookupPatch patch = {&kextRadeonX5000, find, repl, arrsize(find), 1};
        patcher.applyLookupPatch(&patch);
        patcher.clearError();
    } else if (kextRadeonX6000.loadIndex == index) {
        KernelPatcher::SolveRequest solveRequests[] = {
            {"__ZN30AMDRadeonX6000_AMDVCN2HWEngineC1Ev", orgVCN2EngineConstructorX6000},
            {"__ZN31AMDRadeonX6000_AMDGFX10Hardware20allocateAMDHWDisplayEv", orgAllocateAMDHWDisplayX6000},
            {"__ZN42AMDRadeonX6000_AMDGFX10GraphicsAccelerator15newVideoContextEv", orgNewVideoContextX6000},
            {"__ZN31AMDRadeonX6000_IAMDSMLInterface18createSMLInterfaceEj", orgCreateSMLInterfaceX6000},
            {"__ZN37AMDRadeonX6000_AMDGraphicsAccelerator9newSharedEv", orgNewSharedX6000},
            {"__ZN37AMDRadeonX6000_AMDGraphicsAccelerator19newSharedUserClientEv", orgNewSharedUserClientX6000},
            {"__ZN35AMDRadeonX6000_AMDAccelVideoContext10gMetaClassE", metaClassMap[0][1]},
            {"__ZN37AMDRadeonX6000_AMDAccelDisplayMachine10gMetaClassE", metaClassMap[1][1]},
            {"__ZN34AMDRadeonX6000_AMDAccelDisplayPipe10gMetaClassE", metaClassMap[2][1]},
            {"__ZN30AMDRadeonX6000_AMDAccelChannel10gMetaClassE", metaClassMap[3][0]},
            {"__ZN33AMDRadeonX6000_AMDHWAlignManager224getPreferredSwizzleMode2EP33_ADDR2_COMPUTE_SURFACE_INFO_INPUT",
                orgGetPreferredSwizzleMode2},
        };
        PANIC_COND(!patcher.solveMultiple(index, solveRequests, address, size), MODULE_SHORT,
            "Failed to resolve AMDRadeonX6000 symbols");

        KernelPatcher::RouteRequest requests[] = {
            {"__ZN37AMDRadeonX6000_AMDGraphicsAccelerator5startEP9IOService", wrapAccelStartX6000},
            {"__ZN39AMDRadeonX6000_AMDAccelSharedUserClient5startEP9IOService", wrapAccelSharedUCStartX6000},
            {"__ZN39AMDRadeonX6000_AMDAccelSharedUserClient4stopEP9IOService", wrapAccelSharedUCStopX6000},
            {"__ZN30AMDRadeonX6000_AMDGFX10Display23initDCNRegistersOffsetsEv", wrapInitDCNRegistersOffsets,
                orgInitDCNRegistersOffsets},
            {"__ZN29AMDRadeonX6000_AMDAccelShared11SurfaceCopyEPjyP12IOAccelEvent", wrapAccelSharedSurfaceCopy,
                orgAccelSharedSurfaceCopy},
            {"__ZN27AMDRadeonX6000_AMDHWDisplay17allocateScanoutFBEjP16IOAccelResource2S1_Py", wrapAllocateScanoutFB,
                orgAllocateScanoutFB},
            {"__ZN27AMDRadeonX6000_AMDHWDisplay14fillUBMSurfaceEjP17_FRAMEBUFFER_INFOP13_UBM_SURFINFO",
                wrapFillUBMSurface, orgFillUBMSurface},
            {"__ZN27AMDRadeonX6000_AMDHWDisplay16configureDisplayEjjP17_FRAMEBUFFER_INFOP16IOAccelResource2",
                wrapConfigureDisplay, orgConfigureDisplay},
            {"__ZN27AMDRadeonX6000_AMDHWDisplay14getDisplayInfoEjbbPvP17_FRAMEBUFFER_INFO", wrapGetDisplayInfo,
                orgGetDisplayInfo},
        };
        PANIC_COND(!patcher.routeMultiple(index, requests, address, size), MODULE_SHORT,
            "Failed to route AMDRadeonX6000 symbols");

        /** Mismatched VTable Calls to getGpuDebugPolicy. */
        const uint8_t find_getGpuDebugPolicy[] = {0x48, 0x8B, 0x07, 0xFF, 0x90, 0xC0, 0x03, 0x00, 0x00};
        const uint8_t repl_getGpuDebugPolicy[] = {0x48, 0x8B, 0x07, 0xFF, 0x90, 0xC8, 0x03, 0x00, 0x00};
        static_assert(arrsize(find_getGpuDebugPolicy) == arrsize(repl_getGpuDebugPolicy));

        /** VTable Call to signalGPUWorkSubmitted. Doesn't exist on X5000, but looks like it isn't necessary, so we
           just NO-OP it. */
        const uint8_t find_HWChannel_submitCommandBuffer[] = {0x48, 0x8B, 0x7B, 0x18, 0x48, 0x8B, 0x07, 0xFF, 0x90,
            0x30, 0x02, 0x00, 0x00, 0x48, 0x8B, 0x43, 0x50};
        const uint8_t repl_HWChannel_submitCommandBuffer[] = {0x48, 0x8B, 0x7B, 0x18, 0x48, 0x8B, 0x07, 0x90, 0x90,
            0x90, 0x90, 0x90, 0x90, 0x48, 0x8B, 0x43, 0x50};
        static_assert(arrsize(find_HWChannel_submitCommandBuffer) == arrsize(repl_HWChannel_submitCommandBuffer));

        /** Mismatched VTable Call to isDeviceValid in enableTimestampInterrupt. */
        const uint8_t find_enableTimestampInterrupt[] = {0x48, 0x8B, 0x07, 0xFF, 0x90, 0xA0, 0x02, 0x00, 0x00};
        const uint8_t repl_enableTimestampInterrupt[] = {0x48, 0x8B, 0x07, 0xFF, 0x90, 0x98, 0x02, 0x00, 0x00};
        static_assert(arrsize(find_enableTimestampInterrupt) == arrsize(repl_enableTimestampInterrupt));

        /** Mismatched VTable Calls to getScheduler. */
        const uint8_t find_getScheduler[] = {0x48, 0x8B, 0x07, 0xFF, 0x90, 0xB8, 0x03, 0x00, 0x00};
        const uint8_t repl_getScheduler[] = {0x48, 0x8B, 0x07, 0xFF, 0x90, 0xC0, 0x03, 0x00, 0x00};
        static_assert(arrsize(find_getScheduler) == arrsize(repl_getScheduler));

        /** Mismatched VTable Calls to isDeviceValid. */
        const uint8_t find_isDeviceValid[] = {0x48, 0x8B, 0x07, 0xFF, 0x90, 0xA0, 0x02, 0x00, 0x00, 0x84, 0xC0};
        const uint8_t repl_isDeviceValid[] = {0x48, 0x8B, 0x07, 0xFF, 0x90, 0x98, 0x02, 0x00, 0x00, 0x84, 0xC0};
        static_assert(arrsize(find_isDeviceValid) == arrsize(repl_isDeviceValid));

        KernelPatcher::LookupPatch patches[] = {
            {&kextRadeonX6000, find_getGpuDebugPolicy, repl_getGpuDebugPolicy, arrsize(find_getGpuDebugPolicy), 28},
            {&kextRadeonX6000, find_HWChannel_submitCommandBuffer, repl_HWChannel_submitCommandBuffer,
                arrsize(find_HWChannel_submitCommandBuffer), 1},
            {&kextRadeonX6000, find_enableTimestampInterrupt, repl_enableTimestampInterrupt,
                arrsize(find_enableTimestampInterrupt), 1},
            {&kextRadeonX6000, find_getScheduler, repl_getScheduler, arrsize(find_getScheduler), 22},
            {&kextRadeonX6000, find_isDeviceValid, repl_isDeviceValid, arrsize(find_isDeviceValid), 14},
        };
        for (auto &patch : patches) {
            patcher.applyLookupPatch(&patch);
            patcher.clearError();
        }
    }
}

uint32_t WRed::wrapSmuGetHwVersion() { return 0x1; }

uint32_t WRed::wrapPspSwInit(uint32_t *inputData, void *outputData) {
    if (callbackWRed->chipType < ChipType::Renoir) {
        inputData[3] = 0x9;
        inputData[4] = 0x0;
        inputData[5] = 0x2;

    } else {
        inputData[3] = 0xB;
        inputData[4] = 0x0;
        inputData[5] = 0x0;
    }
    auto ret = FunctionCast(wrapPspSwInit, callbackWRed->orgPspSwInit)(inputData, outputData);
    DBGLOG(MODULE_SHORT, "_psp_sw_init >> 0x%X", ret);
    return ret;
}

uint32_t WRed::wrapGcGetHwVersion() { return 0x090400; }

void WRed::wrapPopulateFirmwareDirectory(void *that) {
    auto *fwDir = IOMallocZero(0xD8);
    callbackWRed->orgAMDFirmwareDirectoryConstructor(fwDir, 3);
    getMember<void *>(that, 0xB8) = fwDir;

    auto *asicName = getASICName();
    char filename[128];
    snprintf(filename, 128, "%s_vcn.bin", asicName);
    auto *targetFilename = callbackWRed->chipType >= ChipType::Renoir ? "ativvaxy_nv.dat" : "ativvaxy_rv.dat";
    DBGLOG(MODULE_SHORT, "%s => %s", filename, targetFilename);

    auto &fwDesc = getFWDescByName(filename);
    auto *fwHeader = reinterpret_cast<const GfxFwHeaderV1 *>(fwDesc.data);
    auto *fw =
        callbackWRed->orgCreateFirmware(fwDesc.data + fwHeader->ucodeOff, fwHeader->ucodeSize, 0x200, targetFilename);
    PANIC_COND(!fw, MODULE_SHORT, "Failed to create '%s' firmware", targetFilename);
    DBGLOG(MODULE_SHORT, "Inserting %s!", targetFilename);
    PANIC_COND(!callbackWRed->orgPutFirmware(fwDir, 0, fw), MODULE_SHORT, "Failed to inject ativvaxy_rv.dat firmware");

    if (callbackWRed->chipType >= ChipType::Renoir) {
        snprintf(filename, 128, "%s_dmcub.bin", asicName);
        DBGLOG(MODULE_SHORT, "%s => atidmcub_0.dat", filename);
        auto &fwDesc = getFWDescByName(filename);
        auto *fwHeader = reinterpret_cast<const CommonFirmwareHeader *>(fwDesc.data);
        fw = callbackWRed->orgCreateFirmware(fwDesc.data + fwHeader->ucodeOff, fwHeader->ucodeSize, 0x200,
            "atidmcub_0.dat");
        PANIC_COND(!fw, MODULE_SHORT, "Failed to create atidmcub_0.dat firmware");
        DBGLOG(MODULE_SHORT, "Inserting atidmcub_0.dat!");
        PANIC_COND(!callbackWRed->orgPutFirmware(fwDir, 0, fw), MODULE_SHORT,
            "Failed to inject atidmcub_0.dat firmware");
    }
}

void *WRed::wrapCreatePowerTuneServices(void *that, void *param2) {
    auto *ret = IOMallocZero(0x18);
    if (callbackWRed->chipType >= ChipType::Renoir) {
        callbackWRed->orgVega20PowerTuneConstructor(ret, that, param2);
    } else {
        callbackWRed->orgVega10PowerTuneConstructor(ret, that, param2);
    }
    return ret;
}

uint16_t WRed::wrapGetEnumeratedRevision() { return callbackWRed->enumeratedRevision; }

IOReturn WRed::wrapPopulateDeviceInfo(void *that) {
    auto ret = FunctionCast(wrapPopulateDeviceInfo, callbackWRed->orgPopulateDeviceInfo)(that);
    getMember<uint32_t>(that, 0x60) = AMDGPU_FAMILY_RV;
    return ret;
}

uint32_t WRed::hwLibsNoop() { return 0; }           // Always return success
uint32_t WRed::hwLibsUnsupported() { return 4; }    // Always return unsupported

IOReturn WRed::wrapPopulateVramInfo([[maybe_unused]] void *that, void *fwInfo) {
    auto *vbios = static_cast<const uint8_t *>(callbackWRed->vbiosData->getBytesNoCopy());
    auto base = *reinterpret_cast<const uint16_t *>(vbios + ATOM_ROM_TABLE_PTR);
    auto dataTable = *reinterpret_cast<const uint16_t *>(vbios + base + ATOM_ROM_DATA_PTR);
    auto *mdt = reinterpret_cast<const uint16_t *>(vbios + dataTable + 4);
    uint32_t channelCount = 1;
    if (mdt[0x1E]) {
        DBGLOG(MODULE_SHORT, "Fetching VRAM info from iGPU System Info");
        uint32_t offset = 0x1E * 2 + 4;
        auto index = *reinterpret_cast<const uint16_t *>(vbios + dataTable + offset);
        auto *table = reinterpret_cast<const IgpSystemInfo *>(vbios + index);
        switch (table->header.formatRev) {
            case 1:
                switch (table->header.contentRev) {
                    case 11:
                        [[fallthrough]];
                    case 12:
                        if (table->infoV11.umaChannelCount) channelCount = table->infoV11.umaChannelCount;
                        break;
                    default:
                        DBGLOG(MODULE_SHORT, "Unsupported contentRev %d", table->header.contentRev);
                        break;
                }
                break;
            case 2:
                switch (table->header.contentRev) {
                    case 1:
                        [[fallthrough]];
                    case 2:
                        if (table->infoV2.umaChannelCount) channelCount = table->infoV2.umaChannelCount;
                        break;
                    default:
                        DBGLOG(MODULE_SHORT, "Unsupported contentRev %d", table->header.contentRev);
                        break;
                }
                break;
            default:
                DBGLOG(MODULE_SHORT, "Unsupported formatRev %d", table->header.formatRev);
                break;
        }
    } else {
        DBGLOG(MODULE_SHORT, "No iGPU System Info in Master Data Table");
    }
    getMember<uint32_t>(fwInfo, 0x1C) = 4;                    // VRAM Type (DDR4)
    getMember<uint32_t>(fwInfo, 0x20) = channelCount * 64;    // VRAM Width (64-bit channels)
    return kIOReturnSuccess;
}

/**
 * We don't want the `AMDRadeonX6000` personality defined in the `Info.plist` to do anything.
 * We only use it to force-load `AMDRadeonX6000` and snatch the VCN/DCN symbols.
 */
bool WRed::wrapAccelStartX6000() { return false; }

bool WRed::wrapAllocateHWEngines(void *that) {
    auto *pm4 = IOMallocZero(0x1E8);
    callbackWRed->orgGFX9PM4EngineConstructor(pm4);
    getMember<void *>(that, 0x3B8) = pm4;

    auto *sdma0 = IOMallocZero(0x128);
    callbackWRed->orgGFX9SDMAEngineConstructor(sdma0);
    getMember<void *>(that, 0x3C0) = sdma0;

    auto *vcn2 = IOMallocZero(0x198);
    callbackWRed->orgVCN2EngineConstructorX6000(vcn2);
    getMember<void *>(that, 0x3F8) = vcn2;
    return true;
}

void WRed::wrapSetupAndInitializeHWCapabilities(void *that) {
    FunctionCast(wrapSetupAndInitializeHWCapabilities, callbackWRed->chipType >= ChipType::Renoir ?
                                                           callbackWRed->orgSetupAndInitializeHWCapabilitiesVega20 :
                                                           callbackWRed->orgSetupAndInitializeHWCapabilities)(that);
    if (callbackWRed->chipType < ChipType::Renoir) {
        getMember<uint32_t>(that, 0x2C) = 4;    // Surface Count (?)
    }
    getMember<bool>(that, 0xC0) = false;    // SDMA Page Queue
    getMember<bool>(that, 0xAC) = false;    // VCE
    getMember<bool>(that, 0xAE) = false;    // VCE-related
}

void *WRed::wrapRTGetHWChannel(void *that, uint32_t param1, uint32_t param2, uint32_t param3) {
    if (param1 == 2 && param2 == 0 && param3 == 0) { param2 = 2; }    // Redirect SDMA1 retrieval to SDMA0
    return FunctionCast(wrapRTGetHWChannel, callbackWRed->orgRTGetHWChannel)(that, param1, param2, param3);
}

uint32_t WRed::wrapHwReadReg32(void *that, uint32_t reg) {
    return FunctionCast(wrapHwReadReg32, callbackWRed->orgHwReadReg32)(that, reg == 0xD31 ? 0xD2F : reg);
}

uint32_t WRed::wrapSmuRavenInitialize(void *smum, uint32_t param2) {
    auto ret = FunctionCast(wrapSmuRavenInitialize, callbackWRed->orgSmuRavenInitialize)(smum, param2);
    callbackWRed->orgRavenSendMsgToSmc(smum, PPSMC_MSG_PowerUpSdma);
    return ret;
}

uint32_t WRed::wrapSmuRenoirInitialize(void *smum, uint32_t param2) {
    auto ret = FunctionCast(wrapSmuRenoirInitialize, callbackWRed->orgSmuRenoirInitialize)(smum, param2);
    callbackWRed->orgRenoirSendMsgToSmc(smum, PPSMC_MSG_PowerUpSdma);
    return ret;
}

void WRed::wrapInitializeFamilyType(void *that) { getMember<uint32_t>(that, 0x308) = AMDGPU_FAMILY_RV; }

void *WRed::wrapAllocateAMDHWDisplay(void *that) {
    return FunctionCast(wrapAllocateAMDHWDisplay, callbackWRed->orgAllocateAMDHWDisplayX6000)(that);
}

uint32_t WRed::wrapPspCmdKmSubmit(void *psp, void *ctx, void *param3, void *param4) {
    // Skip loading of MEC2 FW on Renoir devices due to it being unsupported
    // See also: https://github.com/torvalds/linux/commit/f8f70c1371d304f42d4a1242d8abcbda807d0bed
    if (callbackWRed->chipType >= ChipType::Renoir) {
        static bool didMec1 = false;
        switch (getMember<uint32_t>(ctx, 16)) {
            case GFX_FW_TYPE_CP_MEC:
                if (!didMec1) {
                    didMec1 = true;
                    break;
                }
                DBGLOG(MODULE_SHORT, "Skipping MEC2 FW");
                return 0;
            case GFX_FW_TYPE_CP_MEC_ME2:
                DBGLOG(MODULE_SHORT, "Skipping MEC2 JT FW");
                return 0;
            default:
                break;
        }
    }

    return FunctionCast(wrapPspCmdKmSubmit, callbackWRed->orgPspCmdKmSubmit)(psp, ctx, param3, param4);
}

bool WRed::wrapInitWithPciInfo(void *that, void *param1) {
    auto ret = FunctionCast(wrapInitWithPciInfo, callbackWRed->orgInitWithPciInfo)(that, param1);
    // Hack AMDRadeonX6000_AmdLogger to log everything
    getMember<uint64_t>(that, 0x28) = ~0ULL;
    getMember<uint32_t>(that, 0x30) = 0xFF;
    return ret;
}

void *WRed::wrapNewVideoContext(void *that) {
    return FunctionCast(wrapNewVideoContext, callbackWRed->orgNewVideoContextX6000)(that);
}

void *WRed::wrapCreateSMLInterface(uint32_t configBit) {
    return FunctionCast(wrapCreateSMLInterface, callbackWRed->orgCreateSMLInterfaceX6000)(configBit);
}

uint64_t WRed::wrapAdjustVRAMAddress(void *that, uint64_t addr) {
    auto ret = FunctionCast(wrapAdjustVRAMAddress, callbackWRed->orgAdjustVRAMAddress)(that, addr);
    return ret != addr ? (ret + callbackWRed->fbOffset) : ret;
}

void *WRed::wrapNewShared() { return FunctionCast(wrapNewShared, callbackWRed->orgNewSharedX6000)(); }

void *WRed::wrapNewSharedUserClient() {
    return FunctionCast(wrapNewSharedUserClient, callbackWRed->orgNewSharedUserClientX6000)();
}

bool WRed::wrapAccelSharedUCStartX6000(void *that, void *provider) {
    return FunctionCast(wrapAccelSharedUCStartX6000, callbackWRed->orgAccelSharedUCStart)(that, provider);
}

bool WRed::wrapAccelSharedUCStopX6000(void *that, void *provider) {
    return FunctionCast(wrapAccelSharedUCStopX6000, callbackWRed->orgAccelSharedUCStop)(that, provider);
}

void WRed::wrapInitDCNRegistersOffsets(void *that) {
    FunctionCast(wrapInitDCNRegistersOffsets, callbackWRed->orgInitDCNRegistersOffsets)(that);
    if (callbackWRed->chipType < ChipType::Renoir) {
        DBGLOG(MODULE_SHORT, "initDCNRegistersOffsets !! PATCHING REGISTERS FOR DCN 1.0 !!");
        auto base = getMember<uint32_t>(that, 0x4830);
        getMember<uint32_t>(that, 0x4840) = base + mmHUBPREQ0_DCSURF_PRIMARY_SURFACE_ADDRESS;
        getMember<uint32_t>(that, 0x4878) = base + mmHUBPREQ1_DCSURF_PRIMARY_SURFACE_ADDRESS;
        getMember<uint32_t>(that, 0x48B0) = base + mmHUBPREQ2_DCSURF_PRIMARY_SURFACE_ADDRESS;
        getMember<uint32_t>(that, 0x48E8) = base + mmHUBPREQ3_DCSURF_PRIMARY_SURFACE_ADDRESS;
        getMember<uint32_t>(that, 0x4844) = base + mmHUBPREQ0_DCSURF_PRIMARY_SURFACE_ADDRESS_HIGH;
        getMember<uint32_t>(that, 0x487C) = base + mmHUBPREQ1_DCSURF_PRIMARY_SURFACE_ADDRESS_HIGH;
        getMember<uint32_t>(that, 0x48B4) = base + mmHUBPREQ2_DCSURF_PRIMARY_SURFACE_ADDRESS_HIGH;
        getMember<uint32_t>(that, 0x48EC) = base + mmHUBPREQ3_DCSURF_PRIMARY_SURFACE_ADDRESS_HIGH;
        getMember<uint32_t>(that, 0x4848) = base + mmHUBP0_DCSURF_SURFACE_CONFIG;
        getMember<uint32_t>(that, 0x4880) = base + mmHUBP1_DCSURF_SURFACE_CONFIG;
        getMember<uint32_t>(that, 0x48B8) = base + mmHUBP2_DCSURF_SURFACE_CONFIG;
        getMember<uint32_t>(that, 0x48F0) = base + mmHUBP3_DCSURF_SURFACE_CONFIG;
        getMember<uint32_t>(that, 0x484C) = base + mmHUBPREQ0_DCSURF_SURFACE_PITCH;
        getMember<uint32_t>(that, 0x4884) = base + mmHUBPREQ1_DCSURF_SURFACE_PITCH;
        getMember<uint32_t>(that, 0x48BC) = base + mmHUBPREQ2_DCSURF_SURFACE_PITCH;
        getMember<uint32_t>(that, 0x48F4) = base + mmHUBPREQ3_DCSURF_SURFACE_PITCH;
        getMember<uint32_t>(that, 0x4850) = base + mmHUBP0_DCSURF_ADDR_CONFIG;
        getMember<uint32_t>(that, 0x4888) = base + mmHUBP1_DCSURF_ADDR_CONFIG;
        getMember<uint32_t>(that, 0x48C0) = base + mmHUBP2_DCSURF_ADDR_CONFIG;
        getMember<uint32_t>(that, 0x48F8) = base + mmHUBP3_DCSURF_ADDR_CONFIG;
        getMember<uint32_t>(that, 0x4854) = base + mmHUBP0_DCSURF_TILING_CONFIG;
        getMember<uint32_t>(that, 0x488C) = base + mmHUBP1_DCSURF_TILING_CONFIG;
        getMember<uint32_t>(that, 0x48C4) = base + mmHUBP2_DCSURF_TILING_CONFIG;
        getMember<uint32_t>(that, 0x48FC) = base + mmHUBP3_DCSURF_TILING_CONFIG;
        getMember<uint32_t>(that, 0x4858) = base + mmHUBP0_DCSURF_PRI_VIEWPORT_START;
        getMember<uint32_t>(that, 0x4890) = base + mmHUBP1_DCSURF_PRI_VIEWPORT_START;
        getMember<uint32_t>(that, 0x48C8) = base + mmHUBP2_DCSURF_PRI_VIEWPORT_START;
        getMember<uint32_t>(that, 0x4900) = base + mmHUBP3_DCSURF_PRI_VIEWPORT_START;
        getMember<uint32_t>(that, 0x485C) = base + mmHUBP0_DCSURF_PRI_VIEWPORT_DIMENSION;
        getMember<uint32_t>(that, 0x4894) = base + mmHUBP1_DCSURF_PRI_VIEWPORT_DIMENSION;
        getMember<uint32_t>(that, 0x48CC) = base + mmHUBP2_DCSURF_PRI_VIEWPORT_DIMENSION;
        getMember<uint32_t>(that, 0x4904) = base + mmHUBP3_DCSURF_PRI_VIEWPORT_DIMENSION;
        getMember<uint32_t>(that, 0x4860) = base + mmOTG0_OTG_CONTROL;
        getMember<uint32_t>(that, 0x4898) = base + mmOTG1_OTG_CONTROL;
        getMember<uint32_t>(that, 0x48D0) = base + mmOTG2_OTG_CONTROL;
        getMember<uint32_t>(that, 0x4908) = base + mmOTG3_OTG_CONTROL;
        getMember<uint32_t>(that, 0x4940) = base + mmOTG4_OTG_CONTROL;
        getMember<uint32_t>(that, 0x4978) = base + mmOTG5_OTG_CONTROL;
        getMember<uint32_t>(that, 0x4864) = base + mmOTG0_OTG_INTERLACE_CONTROL;
        getMember<uint32_t>(that, 0x489C) = base + mmOTG1_OTG_INTERLACE_CONTROL;
        getMember<uint32_t>(that, 0x48D4) = base + mmOTG2_OTG_INTERLACE_CONTROL;
        getMember<uint32_t>(that, 0x490C) = base + mmOTG3_OTG_INTERLACE_CONTROL;
        getMember<uint32_t>(that, 0x4944) = base + mmOTG4_OTG_INTERLACE_CONTROL;
        getMember<uint32_t>(that, 0x497C) = base + mmOTG5_OTG_INTERLACE_CONTROL;
        getMember<uint32_t>(that, 0x4868) = base + mmHUBPREQ0_DCSURF_FLIP_CONTROL;
        getMember<uint32_t>(that, 0x48A0) = base + mmHUBPREQ1_DCSURF_FLIP_CONTROL;
        getMember<uint32_t>(that, 0x48D8) = base + mmHUBPREQ2_DCSURF_FLIP_CONTROL;
        getMember<uint32_t>(that, 0x4910) = base + mmHUBPREQ3_DCSURF_FLIP_CONTROL;
        getMember<uint32_t>(that, 0x486C) = base + mmHUBPRET0_HUBPRET_CONTROL;
        getMember<uint32_t>(that, 0x48A4) = base + mmHUBPRET1_HUBPRET_CONTROL;
        getMember<uint32_t>(that, 0x48DC) = base + mmHUBPRET2_HUBPRET_CONTROL;
        getMember<uint32_t>(that, 0x4914) = base + mmHUBPRET3_HUBPRET_CONTROL;
        getMember<uint32_t>(that, 0x4870) = base + mmHUBPREQ0_DCSURF_SURFACE_EARLIEST_INUSE;
        getMember<uint32_t>(that, 0x48A8) = base + mmHUBPREQ1_DCSURF_SURFACE_EARLIEST_INUSE;
        getMember<uint32_t>(that, 0x48E0) = base + mmHUBPREQ2_DCSURF_SURFACE_EARLIEST_INUSE;
        getMember<uint32_t>(that, 0x4918) = base + mmHUBPREQ3_DCSURF_SURFACE_EARLIEST_INUSE;
        getMember<uint32_t>(that, 0x4874) = base + mmHUBPREQ0_DCSURF_SURFACE_EARLIEST_INUSE_HIGH;
        getMember<uint32_t>(that, 0x48AC) = base + mmHUBPREQ1_DCSURF_SURFACE_EARLIEST_INUSE_HIGH;
        getMember<uint32_t>(that, 0x48E4) = base + mmHUBPREQ2_DCSURF_SURFACE_EARLIEST_INUSE_HIGH;
        getMember<uint32_t>(that, 0x491C) = base + mmHUBPREQ3_DCSURF_SURFACE_EARLIEST_INUSE_HIGH;
    }
}

void *WRed::wrapAllocateAMDHWAlignManager() {
    auto ret = FunctionCast(wrapAllocateAMDHWAlignManager, callbackWRed->orgAllocateAMDHWAlignManager)();
    callbackWRed->hwAlignMgr = ret;

    callbackWRed->hwAlignMgrVtX5000 = getMember<uint8_t *>(ret, 0);
    callbackWRed->hwAlignMgrVtX6000 = static_cast<uint8_t *>(IOMallocZero(0x238));

    memcpy(callbackWRed->hwAlignMgrVtX6000, callbackWRed->hwAlignMgrVtX5000, 0x128);
    *reinterpret_cast<mach_vm_address_t *>(callbackWRed->hwAlignMgrVtX6000 + 0x128) =
        callbackWRed->orgGetPreferredSwizzleMode2;
    memcpy(callbackWRed->hwAlignMgrVtX6000 + 0x130, callbackWRed->hwAlignMgrVtX5000 + 0x128, 0x230 - 0x128);
    return ret;
}

#define HWALIGNMGR_ADJUST getMember<void *>(callbackWRed->hwAlignMgr, 0) = callbackWRed->hwAlignMgrVtX6000;
#define HWALIGNMGR_REVERT getMember<void *>(callbackWRed->hwAlignMgr, 0) = callbackWRed->hwAlignMgrVtX5000;

uint64_t WRed::wrapAccelSharedSurfaceCopy(void *that, void *param1, uint64_t param2, void *param3) {
    HWALIGNMGR_ADJUST
    auto ret =
        FunctionCast(wrapAccelSharedSurfaceCopy, callbackWRed->orgAccelSharedSurfaceCopy)(that, param1, param2, param3);
    HWALIGNMGR_REVERT
    return ret;
}

uint64_t WRed::wrapAllocateScanoutFB(void *that, uint32_t param1, void *param2, void *param3, void *param4) {
    HWALIGNMGR_ADJUST
    auto ret =
        FunctionCast(wrapAllocateScanoutFB, callbackWRed->orgAllocateScanoutFB)(that, param1, param2, param3, param4);
    HWALIGNMGR_REVERT
    return ret;
}

uint64_t WRed::wrapFillUBMSurface(void *that, uint32_t param1, void *param2, void *param3) {
    HWALIGNMGR_ADJUST
    auto ret = FunctionCast(wrapFillUBMSurface, callbackWRed->orgFillUBMSurface)(that, param1, param2, param3);
    HWALIGNMGR_REVERT
    return ret;
}

bool WRed::wrapConfigureDisplay(void *that, uint32_t param1, uint32_t param2, void *param3, void *param4) {
    HWALIGNMGR_ADJUST
    auto ret =
        FunctionCast(wrapConfigureDisplay, callbackWRed->orgConfigureDisplay)(that, param1, param2, param3, param4);
    HWALIGNMGR_REVERT
    return ret;
}

uint64_t WRed::wrapGetDisplayInfo(void *that, uint32_t param1, bool param2, bool param3, void *param4, void *param5) {
    HWALIGNMGR_ADJUST
    auto ret =
        FunctionCast(wrapGetDisplayInfo, callbackWRed->orgGetDisplayInfo)(that, param1, param2, param3, param4, param5);
    HWALIGNMGR_REVERT
    return ret;
}

void WRed::wrapDoGPUPanic() {
    DBGLOG(MODULE_SHORT, "doGPUPanic << ()");
    while (true) { IOSleep(3600000); }
}
