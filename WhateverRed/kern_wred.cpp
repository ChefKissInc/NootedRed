//  Copyright © 2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.0. See LICENSE for
//  details.

#include "kern_wred.hpp"
#include "kern_fw.hpp"
#include "kern_netdbg.hpp"
#include <Headers/kern_api.hpp>
#include <IOKit/acpi/IOACPIPlatformExpert.h>

static const char *pathRadeonX5000HWLibs = "/System/Library/Extensions/AMDRadeonX5000HWServices.kext/Contents/PlugIns/"
                                           "AMDRadeonX5000HWLibs.kext/Contents/MacOS/AMDRadeonX5000HWLibs";
static const char *pathRadeonX6000Framebuffer =
    "/System/Library/Extensions/AMDRadeonX6000Framebuffer.kext/Contents/MacOS/AMDRadeonX6000Framebuffer";
static const char *pathRadeonX6000 = "/System/Library/Extensions/AMDRadeonX6000.kext/Contents/MacOS/AMDRadeonX6000";
static const char *pathRadeonX5000 = "/System/Library/Extensions/AMDRadeonX5000.kext/Contents/MacOS/AMDRadeonX5000";

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
    callbackWRed = this;

    lilu.onKextLoadForce(&kextRadeonX5000HWLibs);
    lilu.onKextLoadForce(&kextRadeonX6000Framebuffer);
    lilu.onKextLoadForce(&kextRadeonX6000);
    lilu.onKextLoadForce(&kextRadeonX5000);

    lilu.onKextLoadForce(    // For compatibility
        nullptr, 0,
        [](void *user, KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
            static_cast<WRed *>(user)->processKext(patcher, index, address, size);
        },
        this);
}

void WRed::deinit() {
    if (this->vbiosData) { this->vbiosData->release(); }
}

void WRed::processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
    if (kextRadeonX5000HWLibs.loadIndex == index) {
        KernelPatcher::SolveRequest solveRequests[] = {
            {"__ZL15deviceTypeTable", orgDeviceTypeTable},
            {"__ZN11AMDFirmware14createFirmwareEPhjjPKc", orgCreateFirmware},
            {"__ZN20AMDFirmwareDirectory11putFirmwareE16_AMD_DEVICE_TYPEP11AMDFirmware", orgPutFirmware},
            {"__ZN31AtiAppleVega10PowerTuneServicesC1EP11PP_InstanceP18PowerPlayCallbacks",
                orgVega10PowerTuneConstructor},
            {"__ZL20CAIL_ASIC_CAPS_TABLE", orgAsicCapsTableHWLibs},
            {"_CAILAsicCapsInitTable", orgAsicInitCapsTable},
            {"_gc_9_2_1_rlc_ucode", orgGcRlcUcode},
            {"_gc_9_2_1_me_ucode", orgGcMeUcode},
            {"_gc_9_2_1_ce_ucode", orgGcCeUcode},
            {"_gc_9_2_1_pfp_ucode", orgGcPfpUcode},
            {"_gc_9_2_1_mec_ucode", orgGcMecUcode},
            {"_gc_9_2_1_mec_jt_ucode", orgGcMecJtUcode},
            {"_sdma_4_1_ucode", orgSdma41Ucode},
            {"_sdma_4_2_ucode", orgSdma42Ucode},
            {"_Raven_SendMsgToSmcWithParameter", orgRavenSendMsgToSmcWithParam},
            {"_Renoir_SendMsgToSmcWithParameter", orgRenoirSendMsgToSmcWithParam},
        };
        PANIC_COND(!patcher.solveMultiple(index, solveRequests, address, size), "wred",
            "Failed to resolve AMDRadeonX5000HWLibs symbols");

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
            {"_psp_asd_load", wrapPspAsdLoad, orgPspAsdLoad},
            {"_psp_dtm_load", wrapPspDtmLoad, orgPspDtmLoad},
            {"_psp_hdcp_load", wrapPspHdcpLoad, orgPspHdcpLoad},
            {"_SmuRaven_Initialize", wrapSmuRavenInitialize, orgSmuRavenInitialize},
            {"_SmuRenoir_Initialize", wrapSmuRenoirInitialize, orgSmuRenoirInitialize},
            {"_psp_xgmi_is_support", pspFeatureUnsupported},
            {"_psp_rap_is_supported", pspFeatureUnsupported},
            {"_psp_cos_log", wrapPspCosLog, orgPspCosLog},
            {"_psp_cmd_km_submit", wrapPspCmdKmSubmit, orgPspCmdKmSubmit},
            {"__ZN20AtiPowerPlayServicesC2EP18PowerPlayCallbacks", wrapAtiPowerPlayServicesConstructor,
                orgAtiPowerPlayServicesConstructor},
            {"_gvm_get_ip_function", wrapGvmGetIpFunction, orgGvmGetIpFunction},
        };
        PANIC_COND(!patcher.routeMultipleLong(index, requests, address, size), "wred",
            "Failed to route AMDRadeonX5000HWLibs symbols");

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
            {&kextRadeonX5000HWLibs, find_asic_reset, repl_asic_reset, arrsize(find_asic_reset), 1},
        };
        for (auto &patch : patches) {
            patcher.applyLookupPatch(&patch);
            patcher.clearError();
        }
    } else if (kextRadeonX6000Framebuffer.loadIndex == index) {
        KernelPatcher::SolveRequest solveRequests[] = {
            {"__ZL20CAIL_ASIC_CAPS_TABLE", orgAsicCapsTable},
        };
        PANIC_COND(!patcher.solveMultiple(index, solveRequests, address, size, true), "wred",
            "Failed to resolve AMDRadeonX6000Framebuffer symbols");

        KernelPatcher::RouteRequest requests[] = {
            {"__ZNK15AmdAtomVramInfo16populateVramInfoER16AtomFirmwareInfo", wrapPopulateVramInfo},
            {"__ZNK26AMDRadeonX6000_AmdAsicInfo11getFamilyIdEv", wrapGetFamilyId},
            {"__ZN30AMDRadeonX6000_AmdAsicInfoNavi18populateDeviceInfoEv", wrapPopulateDeviceInfo,
                orgPopulateDeviceInfo},
            {"__ZNK32AMDRadeonX6000_AmdAsicInfoNavi1027getEnumeratedRevisionNumberEv", wrapGetEnumeratedRevision},
            {"__ZN32AMDRadeonX6000_AmdRegisterAccess11hwReadReg32Ej", wrapHwReadReg32, orgHwReadReg32},
            {"__ZN24AMDRadeonX6000_AmdLogger15initWithPciInfoEP11IOPCIDevice", wrapInitWithPciInfo, orgInitWithPciInfo},
            {"__ZN35AMDRadeonX6000_AmdRadeonFramebuffer16enableControllerEv", wrapEnableController,
                orgEnableController},
            {"__ZN34AMDRadeonX6000_AmdRadeonController7powerUpEv", wrapControllerPowerUp, orgControllerPowerUp},
            {"__ZN33AMDRadeonX6000_AmdPowerPlayHelper7powerUpEv", wrapPpPowerUp, orgPpPowerUp},
            {"__ZN27AMDRadeonX6000_AmdDalHelper7powerUpEv", wrapDalHelperPowerUp, orgDalHelperPowerUp},
            {"__ZN38AMDRadeonX6000_AmdRadeonControllerNavi19setupBootWatermarksEv", wrapSetupBootWatermarks,
                orgSetupBootWatermarks},
            {"__ZN34AMDRadeonX6000_AmdRadeonController10setupLinksEv", wrapSetupLinks, orgSetupLinks},
            {"__ZNK34AMDRadeonX6000_AmdRadeonController18messageAcceleratorE25_eAMDAccelIOFBRequestTypePvS1_S1_",
                wrapMessageAccelerator, orgMessageAccelerator},
        };
        PANIC_COND(!patcher.routeMultiple(index, requests, address, size, true), "wred",
            "Failed to route AMDRadeonX6000Framebuffer symbols");

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
            /** Neutralise VRAM Info creation null check to proceed with Controller Core Services initialisation. */
            {&kextRadeonX6000Framebuffer, find_null_check1, repl_null_check1, arrsize(find_null_check1), 1},

            /** Neutralise PSP Firmware Info creation null check to proceed with Controller Core Services
               initialisation. */
            {&kextRadeonX6000Framebuffer, find_null_check2, repl_null_check2, arrsize(find_null_check2), 1},

            /** Neutralise VRAM Info null check inside `AmdAtomFwServices::getFirmwareInfo`. */
            {&kextRadeonX6000Framebuffer, find_null_check3, repl_null_check3, arrsize(find_null_check3), 1},
        };
        for (auto &patch : patches) {
            patcher.applyLookupPatch(&patch);
            patcher.clearError();
        }
    } else if (kextRadeonX5000.loadIndex == index) {
        uint32_t *orgChannelTypes = nullptr;

        KernelPatcher::SolveRequest solveRequests[] = {
            {"__ZN31AMDRadeonX5000_AMDGFX9PM4EnginenwEm", orgGFX9PM4EngineNew},
            {"__ZN31AMDRadeonX5000_AMDGFX9PM4EngineC1Ev", orgGFX9PM4EngineConstructor},
            {"__ZN32AMDRadeonX5000_AMDGFX9SDMAEnginenwEm", orgGFX9SDMAEngineNew},
            {"__ZN32AMDRadeonX5000_AMDGFX9SDMAEngineC1Ev", orgGFX9SDMAEngineConstructor},
            {"__ZZN37AMDRadeonX5000_AMDGraphicsAccelerator19createAccelChannelsEbE12channelTypes", orgChannelTypes},
        };
        PANIC_COND(!patcher.solveMultiple(index, solveRequests, address, size), "wred",
            "Failed to resolve AMDRadeonX5000 symbols");

        /** Patch the data so that it only starts SDMA0. */
        PANIC_COND(MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) != KERN_SUCCESS, "wred",
            "Failed to enable kernel writing");
        orgChannelTypes[5] = 1;
        MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);

        KernelPatcher::RouteRequest requests[] = {
            {"__ZN32AMDRadeonX5000_AMDVega10Hardware17allocateHWEnginesEv", wrapAllocateHWEngines},
            {"__ZN32AMDRadeonX5000_AMDVega10Hardware32setupAndInitializeHWCapabilitiesEv",
                wrapSetupAndInitializeHWCapabilities, orgSetupAndInitializeHWCapabilities},
            {"__ZN28AMDRadeonX5000_AMDRTHardware12getHWChannelE18_eAMD_CHANNEL_TYPE11SS_PRIORITYj", wrapRTGetHWChannel,
                orgRTGetHWChannel},
            {"__ZN29AMDRadeonX5000_AMDHWVMContext5mapVAEyP13IOAccelMemoryyyN24AMDRadeonX5000_IAMDHWVMM10VmMapFlagsE",
                wrapMapVA, orgMapVA},
            {"__ZN29AMDRadeonX5000_AMDHWVMContext7mapVMPTEP12AMD_VMPT_CTL15eAMD_VMPT_LEVELjyyy", wrapMapVMPT,
                orgMapVMPT},
            {"__ZN33AMDRadeonX5000_AMDGFX9SDMAChannel23writeWritePTEPDECommandEPjyjyyy", wrapWriteWritePTEPDECommand,
                orgWriteWritePTEPDECommand},
            {"__ZN25AMDRadeonX5000_AMDGFX9VMM11getPDEValueE15eAMD_VMPT_LEVELy", wrapGetPDEValue, orgGetPDEValue},
            {"__ZN25AMDRadeonX5000_AMDGFX9VMM11getPTEValueE15eAMD_VMPT_LEVELyN24AMDRadeonX5000_IAMDHWVMM10VmMapFlagsEj",
                wrapGetPTEValue, orgGetPTEValue},
            {"__ZN29AMDRadeonX5000_AMDHWVMContext36updateContiguousPTEsWithDMAUsingAddrEyyyyy",
                wrapUpdateContiguousPTEsWithDMAUsingAddr, orgUpdateContiguousPTEsWithDMAUsingAddr},
            {"__ZN30AMDRadeonX5000_AMDGFX9Hardware20initializeFamilyTypeEv", wrapInitializeFamilyType},
            {"__ZN30AMDRadeonX5000_AMDGFX9Hardware20allocateAMDHWDisplayEv", wrapAllocateAMDHWDisplay},
            {"__ZN25AMDRadeonX5000_AMDGFX9VMM4initEP30AMDRadeonX5000_IAMDHWInterface", wrapVMMInit, orgVMMInit},
            {"__ZN23AMDRadeonX5000_AMDHWVMM16getVMPTBCoverageEv", wrapGetVMPTBCoverage, orgGetVMPTBCoverage},
            {"__ZN29AMDRadeonX5000_AMDHWRegisters5writeEjj", wrapHwRegWrite, orgHwRegWrite},
        };
        PANIC_COND(!patcher.routeMultipleLong(index, requests, address, size), "wred",
            "Failed to route AMDRadeonX5000 symbols");

        constexpr uint8_t find_startHWEngines[] = {0x49, 0x89, 0xFE, 0x31, 0xDB, 0x48, 0x83, 0xFB, 0x02, 0x74, 0x50};
        constexpr uint8_t repl_startHWEngines[] = {0x49, 0x89, 0xFE, 0x31, 0xDB, 0x48, 0x83, 0xFB, 0x01, 0x74, 0x50};
        static_assert(sizeof(find_startHWEngines) == sizeof(repl_startHWEngines), "Find/replace size mismatch");

        constexpr uint8_t find_sdmachannel_init[] = {0x83, 0xf8, 0x01, 0xb8, 0x21, 0x01, 0x00, 0xff, 0xb9, 0x27, 0x01,
            0x00, 0xff, 0x0f, 0x44, 0xc8};
        constexpr uint8_t repl_sdmachannel_init[] = {0x83, 0xf8, 0x02, 0xb8, 0x21, 0x01, 0x00, 0xff, 0xb9, 0x27, 0x01,
            0x00, 0xff, 0x0f, 0x44, 0xc8};
        static_assert(sizeof(find_sdmachannel_init) == sizeof(repl_sdmachannel_init), "Find/replace size mismatch");

        constexpr uint8_t find_VMMInit[] = {0x48, 0x89, 0x84, 0x0B, 0xD0, 0x0A, 0x00, 0x00, 0x89, 0x94, 0x0B, 0xDC,
            0x0A, 0x00, 0x00, 0x89, 0xD6, 0xC1, 0xE2, 0x03, 0x89, 0x94, 0x0B, 0xE0, 0x0A, 0x00, 0x00};
        constexpr uint8_t repl_VMMInit[] = {0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90,
            0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x90};
        static_assert(sizeof(find_VMMInit) == sizeof(repl_VMMInit), "Find/replace size mismatch");

        constexpr uint8_t find_VMMInit2[] = {0x75, 0xd2, 0x48, 0xc7, 0x83, 0xf0, 0x0a, 0x00, 0x00, 0x00, 0x10, 0x00,
            0x00, 0x48, 0xb8, 0x10, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x48, 0x89, 0x83, 0xfc, 0x0a, 0x00, 0x00,
            0x4c, 0x8d, 0x05, 0xc1, 0x89, 0x08, 0x00};
        constexpr uint8_t repl_VMMInit2[] = {0x75, 0xd2, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90,
            0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90,
            0x4c, 0x8d, 0x05, 0xc1, 0x89, 0x08, 0x00};
        static_assert(sizeof(find_VMMInit2) == sizeof(repl_VMMInit2), "Find/replace size mismatch");

        KernelPatcher::LookupPatch patches[] = {
            /**
             * `AMDRadeonX5000_AMDHardware::startHWEngines`
             * Make for loop stop at 1 instead of 2 in order to skip starting SDMA1 engine.
             */
            {&kextRadeonX5000, find_startHWEngines, repl_startHWEngines, arrsize(find_startHWEngines), 1},

            /**
             * `AMDRadeonX5000_AMDGFX9SDMAChannel::init`
             * Field 0x98 somehow tells the scheduler to wait for VMPT before sending user SDMA commands.
             * Invert the check to set the SDMA1 value when on SDMA0.
             */
            {&kextRadeonX5000, find_sdmachannel_init, repl_sdmachannel_init, arrsize(find_sdmachannel_init), 1},
            /**
             * `AMDRadeonX5000_AMDGFX9VMM::init`
             * NOP out part of the vmptConfig setting logic, in order not to override the value set in wrapVMMInit.
             */
            {&kextRadeonX5000, find_VMMInit, repl_VMMInit, arrsize(find_VMMInit), 1},
            {&kextRadeonX5000, find_VMMInit2, repl_VMMInit2, arrsize(find_VMMInit2), 1},
        };
        for (auto &patch : patches) {
            patcher.applyLookupPatch(&patch);
            patcher.clearError();
        }
    } else if (kextRadeonX6000.loadIndex == index) {
        KernelPatcher::SolveRequest solveRequests[] = {
            {"__ZN30AMDRadeonX6000_AMDVCN2HWEnginenwEm", orgVCN2EngineNewX6000},
            {"__ZN30AMDRadeonX6000_AMDVCN2HWEngineC1Ev", orgVCN2EngineConstructorX6000},
            {"__ZN32AMDRadeonX6000_AMDNavi10Hardware32setupAndInitializeHWCapabilitiesEv",
                orgSetupAndInitializeHWCapabilitiesX6000},
            {"__ZN31AMDRadeonX6000_AMDGFX10Hardware20allocateAMDHWDisplayEv", orgAllocateAMDHWDisplayX6000},
        };
        PANIC_COND(!patcher.solveMultiple(index, solveRequests, address, size), "wred",
            "Failed to resolve AMDRadeonX6000 symbols");

        KernelPatcher::RouteRequest requests[] = {
            {"__ZN37AMDRadeonX6000_AMDGraphicsAccelerator5startEP9IOService", wrapAccelStartX6000},
        };
        PANIC_COND(!patcher.routeMultipleLong(index, requests, address, size), "wred",
            "Failed to route AMDRadeonX6000 symbols");

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

            /** VTable Call to signalGPUWorkSubmitted. Doesn't exist on X5000, but looks like it isn't necessary, so we
               just NO-OP it. */
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
    }
}

// Hack
class AppleACPIPlatformExpert : IOACPIPlatformExpert {
    friend class WRed;
};

void WRed::wrapAmdTtlServicesConstructor(void *that, IOPCIDevice *provider) {
    WIOKit::renameDevice(provider, "GFX0");

    static uint8_t builtBytes[] = {0x01};
    provider->setProperty("built-in", builtBytes, sizeof(builtBytes));

    NETDBG::enabled = true;
    NETLOG("wred", "Patching device type table");
    PANIC_COND(MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) != KERN_SUCCESS, "wred",
        "Failed to enable kernel writing");
    callbackWRed->orgDeviceTypeTable[0] = provider->extendedConfigRead16(kIOPCIConfigDeviceID);
    callbackWRed->orgDeviceTypeTable[1] = 6;
    MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);
    if (provider->getProperty("ATY,bin_image")) {
        NETLOG("wred", "VBIOS manually overridden");
    } else {
        NETLOG("wred", "Fetching VBIOS from VFCT table");
        auto *expert = reinterpret_cast<AppleACPIPlatformExpert *>(provider->getPlatform());
        PANIC_COND(!expert, "wred", "Failed to get AppleACPIPlatformExpert");

        auto *vfctData = expert->getACPITableData("VFCT", 0);
        PANIC_COND(!vfctData, "wred", "Failed to get VFCT from AppleACPIPlatformExpert");

        auto *vfct = static_cast<const VFCT *>(vfctData->getBytesNoCopy());
        PANIC_COND(!vfct, "wred", "VFCT OSData::getBytesNoCopy returned null");

        auto *vbiosContent = static_cast<const GOPVideoBIOSHeader *>(
            vfctData->getBytesNoCopy(vfct->vbiosImageOffset, sizeof(GOPVideoBIOSHeader)));
        PANIC_COND(!vfct->vbiosImageOffset || !vbiosContent, "wred", "No VBIOS contained in VFCT table");

        auto *vbiosPtr =
            vfctData->getBytesNoCopy(vfct->vbiosImageOffset + sizeof(GOPVideoBIOSHeader), vbiosContent->imageLength);
        PANIC_COND(!vbiosPtr, "wred", "Bad VFCT: Offset + Size not within buffer boundaries");

        callbackWRed->vbiosData = OSData::withBytes(vbiosPtr, vbiosContent->imageLength);
        PANIC_COND(!callbackWRed->vbiosData, "wred", "OSData::withBytes failed");
        provider->setProperty("ATY,bin_image", callbackWRed->vbiosData);
    }

    NETLOG("wred", "AmdTtlServices: Calling original constructor");
    FunctionCast(wrapAmdTtlServicesConstructor, callbackWRed->orgAmdTtlServicesConstructor)(that, provider);
}

uint32_t WRed::wrapSmuGetHwVersion(uint64_t param1, uint32_t param2) {
    auto ret = FunctionCast(wrapSmuGetHwVersion, callbackWRed->orgSmuGetHwVersion)(param1, param2);
    NETLOG("wred", "_smu_get_hw_version returned 0x%X", ret);
    switch (ret) {
        case 0x2:
            NETLOG("wred", "Spoofing SMU v10 to v9.0.1");
            return 0x1;
        case 0xB:
            [[fallthrough]];
        case 0xC:
            NETLOG("wred", "Spoofing SMU v11/v12 to v11");
            return 0x3;
        default:
            return ret;
    }
}

uint32_t WRed::wrapPspSwInit(uint32_t *param1, uint32_t *param2) {
    switch (param1[3]) {
        case 0xA:
            [[fallthrough]];
        case 0xB:
            [[fallthrough]];
        case 0xC:
            NETLOG("wred", "Spoofing PSP version v10/v11/v12 to v11");
            param1[3] = 0xB;
            param1[4] = 0x0;
            param1[5] = 0x0;
            break;
        default:
            break;
    }
    auto ret = FunctionCast(wrapPspSwInit, callbackWRed->orgPspSwInit)(param1, param2);
    NETLOG("wred", "_psp_sw_init returned 0x%X", ret);
    return ret;
}

uint32_t WRed::wrapGcGetHwVersion(uint32_t *param1) {
    auto ret = FunctionCast(wrapGcGetHwVersion, callbackWRed->orgGcGetHwVersion)(param1);
    switch (ret & 0xFFFF00) {
        case 0x090100:
            [[fallthrough]];
        case 0x090200:
            [[fallthrough]];
        case 0x090300:
            NETLOG("wred", "Spoofing GC version v9.1/v9.2/v9.3 to v9.2.1");
            return 0x090201;
        default:
            NETLOG("wred", "_gc_get_hw_version returned 0x%X", ret);
            return ret;
    }
}

void WRed::wrapPopulateFirmwareDirectory(void *that) {
    FunctionCast(wrapPopulateFirmwareDirectory, callbackWRed->orgPopulateFirmwareDirectory)(that);
    callbackWRed->callbackFirmwareDirectory = getMember<void *>(that, 0xB8);
    auto &fwDesc = getFWDescByName("renoir_dmcub.bin");
    NETLOG("wred", "renoir_dmcub.bin => atidmcub_0.dat");
    auto *fwBackdoor = callbackWRed->orgCreateFirmware(fwDesc.data, fwDesc.size, 0x200, "atidmcub_0.dat");
    NETLOG("wred", "inserting atidmcub_0.dat!");
    PANIC_COND(!callbackWRed->orgPutFirmware(callbackWRed->callbackFirmwareDirectory, 6, fwBackdoor), "wred",
        "Failed to inject atidmcub_0.dat firmware");
}

void *WRed::wrapCreatePowerTuneServices(void *that, void *param2) {
    auto *ret = IOMallocZero(0x18);
    callbackWRed->orgVega10PowerTuneConstructor(ret, that, param2);
    return ret;
}

uint16_t WRed::wrapGetFamilyId([[maybe_unused]] void *that) { return 0x8E; }    // 0x8F -> 0x8E

uint16_t WRed::wrapGetEnumeratedRevision(void *that) {
    auto *&pciDev = getMember<IOPCIDevice *>(that, 0x18);
    auto &revision = getMember<uint32_t>(that, 0x68);

    switch (pciDev->configRead16(kIOPCIConfigDeviceID)) {
        case 0x15D8:
            if (revision >= 0x8) {
                callbackWRed->asicType = ASICType::Raven2;
                return 0x79;
            }
            callbackWRed->asicType = ASICType::Picasso;
            return 0x41;
        case 0x15DD:
            if (revision >= 0x8) {
                callbackWRed->asicType = ASICType::Raven2;
                return 0x79;
            }
            callbackWRed->asicType = ASICType::Raven;
            return 0x10;
        case 0x15E7:
            [[fallthrough]];
        case 0x164C:
            [[fallthrough]];
        case 0x1636:
            [[fallthrough]];
        case 0x1638:
            callbackWRed->asicType = ASICType::Renoir;
            return 0x91;
        default:
            PANIC("wred", "Unknown device ID for iGPU");
    }
}

static bool injectedIPFirmware = false;

IOReturn WRed::wrapPopulateDeviceInfo(void *that) {
    auto ret = FunctionCast(wrapPopulateDeviceInfo, callbackWRed->orgPopulateDeviceInfo)(that);
    getMember<uint32_t>(that, 0x60) = 0x8E;
    auto deviceId = getMember<IOPCIDevice *>(that, 0x18)->configRead16(kIOPCIConfigDeviceID);
    auto &revision = getMember<uint32_t>(that, 0x68);
    auto &emulatedRevision = getMember<uint32_t>(that, 0x6c);

    NETLOG("wred", "Locating Init Caps entry");
    PANIC_COND(MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) != KERN_SUCCESS, "wred",
        "Failed to enable kernel writing");

    if (!injectedIPFirmware) {
        injectedIPFirmware = true;
        auto *asicName = getASICName();
        auto *filename = new char[128];

        snprintf(filename, 128, "%s_vcn.bin", asicName);
        auto *targetFilename = callbackWRed->asicType == ASICType::Renoir ? "ativvaxy_nv.dat" : "ativvaxy_rv.dat";
        DBGLOG("wred", "%s => %s", filename, targetFilename);

        auto *fwDesc = &getFWDescByName(filename);
        auto *fw = callbackWRed->orgCreateFirmware(fwDesc->data, fwDesc->size, 0x200, targetFilename);
        DBGLOG("wred", "Inserting %s!", targetFilename);
        PANIC_COND(!callbackWRed->orgPutFirmware(callbackWRed->callbackFirmwareDirectory, 6, fw), "wred",
            "Failed to inject ativvaxy_rv.dat firmware");

        snprintf(filename, 128, "%s_rlc.bin", asicName);
        callbackWRed->orgGcRlcUcode->addr = 0x0;
        fwDesc = &getFWDescByName(filename);
        memmove(callbackWRed->orgGcRlcUcode->data, fwDesc->data, fwDesc->size);
        DBGLOG("wred", "Injected %s!", filename);

        snprintf(filename, 128, "%s_me.bin", asicName);
        fwDesc = &getFWDescByName(filename);
        callbackWRed->orgGcMeUcode->addr = 0x1000;
        memmove(callbackWRed->orgGcMeUcode->data, fwDesc->data, fwDesc->size);
        DBGLOG("wred", "Injected %s!", filename);

        snprintf(filename, 128, "%s_ce.bin", asicName);
        fwDesc = &getFWDescByName(filename);
        callbackWRed->orgGcCeUcode->addr = 0x800;
        memmove(callbackWRed->orgGcCeUcode->data, fwDesc->data, fwDesc->size);
        DBGLOG("wred", "Injected %s!", filename);

        snprintf(filename, 128, "%s_pfp.bin", asicName);
        fwDesc = &getFWDescByName(filename);
        callbackWRed->orgGcPfpUcode->addr = 0x1400;
        memmove(callbackWRed->orgGcPfpUcode->data, fwDesc->data, fwDesc->size);
        DBGLOG("wred", "Injected %s!", filename);

        snprintf(filename, 128, "%s_mec.bin", asicName);
        fwDesc = &getFWDescByName(filename);
        callbackWRed->orgGcMecUcode->addr = 0x0;
        memmove(callbackWRed->orgGcMecUcode->data, fwDesc->data, fwDesc->size);
        DBGLOG("wred", "Injected %s!", filename);

        snprintf(filename, 128, "%s_mec_jt.bin", asicName);
        fwDesc = &getFWDescByName(filename);
        callbackWRed->orgGcMecJtUcode->addr = 0x104A4;
        memmove(callbackWRed->orgGcMecJtUcode->data, fwDesc->data, fwDesc->size);
        DBGLOG("wred", "Injected %s!", filename);

        snprintf(filename, 128, "%s_sdma.bin", asicName);
        fwDesc = &getFWDescByName(filename);
        memmove(callbackWRed->orgSdma41Ucode->data, fwDesc->data, fwDesc->size);
        memmove(callbackWRed->orgSdma42Ucode->data, fwDesc->data, fwDesc->size);
        DBGLOG("wred", "Injected %s!", filename);
        delete[] filename;
    }

    CailInitAsicCapEntry *initCaps = nullptr;
    for (size_t i = 0; i < 789; i++) {
        auto *temp = callbackWRed->orgAsicInitCapsTable + i;
        if (temp->familyId == 0x8e && temp->deviceId == deviceId && temp->emulatedRev == emulatedRevision) {
            initCaps = temp;
            break;
        }
    }
    if (!initCaps) {
        DBGLOG("wred", "Warning: Using fallback Init Caps search");
        for (size_t i = 0; i < 789; i++) {
            auto *temp = callbackWRed->orgAsicInitCapsTable + i;
            if (temp->familyId == 0x8e && temp->deviceId == deviceId &&
                (temp->emulatedRev >= wrapGetEnumeratedRevision(that) || temp->emulatedRev <= emulatedRevision)) {
                initCaps = temp;
                break;
            }
        }
        PANIC_COND(!initCaps, "wred", "Failed to find Init Caps entry");
    }

    callbackWRed->orgAsicCapsTable->familyId = callbackWRed->orgAsicCapsTableHWLibs->familyId = 0x8e;
    callbackWRed->orgAsicCapsTable->deviceId = callbackWRed->orgAsicCapsTableHWLibs->deviceId = deviceId;
    callbackWRed->orgAsicCapsTable->revision = callbackWRed->orgAsicCapsTableHWLibs->revision = revision;
    callbackWRed->orgAsicCapsTable->pciRev = callbackWRed->orgAsicCapsTableHWLibs->pciRev = 0xFFFFFFFF;
    callbackWRed->orgAsicCapsTable->emulatedRev = callbackWRed->orgAsicCapsTableHWLibs->emulatedRev = emulatedRevision;
    callbackWRed->orgAsicCapsTable->caps = callbackWRed->orgAsicCapsTableHWLibs->caps = initCaps->caps;
    MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);

    return ret;
}

uint32_t WRed::wrapSmuGetFwConstants([[maybe_unused]] void *param1) { return 0; }    // SMC firmware's already loaded
uint32_t WRed::wrapSmuInternalHwInit([[maybe_unused]] void *param1) { return 0; }    // Don't wait for firmware load
IOReturn WRed::wrapPopulateVramInfo([[maybe_unused]] void *that, void *fwInfo) {
    getMember<uint32_t>(fwInfo, 0x1C) = 4;      // DDR4
    getMember<uint32_t>(fwInfo, 0x20) = 128;    // 128-bit
    return kIOReturnSuccess;
}

/**
 * We don't want the `AMDRadeonX6000` personality defined in the `Info.plist` to do anything.
 * We only use it to force-load `AMDRadeonX6000` and snatch the VCN symbols.
 */
bool WRed::wrapAccelStartX6000() { return false; }

bool WRed::wrapAllocateHWEngines(void *that) {
    auto *pm4 = callbackWRed->orgGFX9PM4EngineNew(0x1E8);
    callbackWRed->orgGFX9PM4EngineConstructor(pm4);
    getMember<void *>(that, 0x3B8) = pm4;

    auto *sdma0 = callbackWRed->orgGFX9SDMAEngineNew(0x128);
    callbackWRed->orgGFX9SDMAEngineConstructor(sdma0);
    getMember<void *>(that, 0x3C0) = sdma0;

    auto *vcn2 = callbackWRed->orgVCN2EngineNewX6000(0x198);
    callbackWRed->orgVCN2EngineConstructorX6000(vcn2);
    getMember<void *>(that, 0x3F8) = vcn2;
    return true;
}

void WRed::wrapSetupAndInitializeHWCapabilities(void *that) {
    FunctionCast(wrapSetupAndInitializeHWCapabilities, callbackWRed->orgSetupAndInitializeHWCapabilities)(that);
    FunctionCast(wrapSetupAndInitializeHWCapabilities, callbackWRed->orgSetupAndInitializeHWCapabilitiesX6000)(that);
    getMember<uint32_t>(that, 0xC0) = 0;    // Raven ASICs do not have an SDMA Page Queue
}

/**
 * Hack: Add custom param 4 and 5 (pointer to firmware and size)
 * aka RCX and R8 registers
 * Complementary to `_psp_asd_load` patch-set.
 */
uint32_t WRed::wrapPspAsdLoad(void *pspData) {
    auto *filename = new char[128];
    snprintf(filename, 128, "%s_asd.bin", getASICName());
    NETLOG("wred", "injecting %s!", filename);
    auto &fwDesc = getFWDescByName(filename);
    delete[] filename;
    auto *org = reinterpret_cast<t_pspLoadExtended>(callbackWRed->orgPspAsdLoad);
    auto ret = org(pspData, 0, 0, fwDesc.data, fwDesc.size);
    NETLOG("wred", "_psp_asd_load returned 0x%X", ret);
    return ret;
}

/** Same idea as `_psp_asd_load`. */
uint32_t WRed::wrapPspDtmLoad(void *pspData) {
    auto *filename = new char[128];
    snprintf(filename, 128, "%s_dtm.bin", getASICName());
    NETLOG("wred", "injecting %s!", filename);
    auto &fwDesc = getFWDescByName(filename);
    delete[] filename;
    auto *org = reinterpret_cast<t_pspLoadExtended>(callbackWRed->orgPspDtmLoad);
    auto ret = org(pspData, 0, 0, fwDesc.data, fwDesc.size);
    NETLOG("wred", "_psp_dtm_load returned 0x%X", ret);
    return 0;
}

/** Same idea as `_psp_asd_load`. */
uint32_t WRed::wrapPspHdcpLoad(void *pspData) {
    auto *filename = new char[128];
    snprintf(filename, 128, "%s_hdcp.bin", getASICName());
    NETLOG("wred", "injecting %s!", filename);
    auto &fwDesc = getFWDescByName(filename);
    delete[] filename;
    auto *org = reinterpret_cast<t_pspLoadExtended>(callbackWRed->orgPspHdcpLoad);
    auto ret = org(pspData, 0, 0, fwDesc.data, fwDesc.size);
    NETLOG("wred", "_psp_hdcp_load returned 0x%X", ret);
    return ret;
}

void *WRed::wrapRTGetHWChannel(void *that, uint32_t param1, uint32_t param2, uint32_t param3) {
    if (param1 == 2 && param2 == 0 && param3 == 0) { param2 = 2; }    // Redirect SDMA1 retrival to SDMA0
    return FunctionCast(wrapRTGetHWChannel, callbackWRed->orgRTGetHWChannel)(that, param1, param2, param3);
}

uint32_t WRed::wrapHwReadReg32(void *that, uint32_t reg) {
    if (reg == 0xD31) {
        /**
         * NBIO 7.4 -> NBIO 7.0
         * reg = SOC15_OFFSET(NBIO_BASE, 0, mmRCC_DEV0_EPF0_STRAP0);
         */
        reg = 0xD2F;
        NETLOG("wred", "hwReadReg32: redirecting reg 0xD31 to 0xD2F");
    }
    auto ret = FunctionCast(wrapHwReadReg32, callbackWRed->orgHwReadReg32)(that, reg);
    return ret;
}

uint32_t WRed::wrapSmuRavenInitialize(void *smumData, uint32_t param2) {
    NETLOG("wred", "_SmuRaven_Initialize: param1 = %p param2 = 0x%X", smumData, param2);
    auto ret = FunctionCast(wrapSmuRavenInitialize, callbackWRed->orgSmuRavenInitialize)(smumData, param2);
    NETLOG("wred", "_SmuRaven_Initialize returned 0x%X", ret);
    callbackWRed->orgRavenSendMsgToSmcWithParam(smumData, PPSMC_MSG_PowerUpSdma, 0);
    return ret;
}

uint32_t WRed::wrapSmuRenoirInitialize(void *smumData, uint32_t param2) {
    NETLOG("wred", "_SmuRenoir_Initialize: param1 = %p param2 = 0x%X", smumData, param2);
    auto ret = FunctionCast(wrapSmuRenoirInitialize, callbackWRed->orgSmuRenoirInitialize)(smumData, param2);
    NETLOG("wred", "_SmuRenoir_Initialize returned 0x%X", ret);
    callbackWRed->orgRenoirSendMsgToSmcWithParam(smumData, PPSMC_MSG_PowerUpSdma, 0);
    return ret;
}

uint64_t WRed::wrapMapVA(void *that, uint64_t param1, void *memory, uint64_t param3, uint64_t sizeToMap,
    uint64_t flags) {
    NETLOG("wred", "mapVA: this = %p param1 = 0x%llX memory = %p param3 = 0x%llX sizeToMap = 0x%llX flags = 0x%llX",
        that, param1, memory, param3, sizeToMap, flags);
    auto ret = FunctionCast(wrapMapVA, callbackWRed->orgMapVA)(that, param1, memory, param3, sizeToMap, flags);
    NETLOG("wred", "mapVA returned 0x%llX", ret);
    return ret;
}

uint64_t WRed::wrapMapVMPT(void *that, void *vmptCtl, uint64_t vmptLevel, uint32_t param3, uint64_t param4,
    uint64_t param5, uint64_t sizeToMap) {
    NETLOG("wred",
        "mapVMPT: this = %p vmptCtl = %p vmptLevel = 0x%llX param3 = 0x%X param4 = 0x%llX param5 = 0x%llX sizeToMap = "
        "0x%llX",
        that, vmptCtl, vmptLevel, param3, param4, param5, sizeToMap);
    auto ret = FunctionCast(wrapMapVMPT, callbackWRed->orgMapVMPT)(that, vmptCtl, vmptLevel, param3, param4, param5,
        sizeToMap);
    NETLOG("wred", "mapVMPT returned 0x%llX", ret);
    return ret;
}

uint32_t WRed::wrapWriteWritePTEPDECommand(void *that, uint32_t *buf, uint64_t pe, uint32_t count, uint64_t flags,
    uint64_t addr, uint64_t incr) {
    NETLOG("wred",
        "writeWritePTEPDECommand: this = %p buf = %p pe = 0x%llX count = 0x%X flags = 0x%llX addr = 0x%llX incr = "
        "0x%llX",
        that, buf, pe, count, flags, addr, incr);
    auto ret = FunctionCast(wrapWriteWritePTEPDECommand, callbackWRed->orgWriteWritePTEPDECommand)(that, buf, pe, count,
        flags, addr, incr);
    NETLOG("wred", "writeWritePTEPDECommand returned 0x%X", ret);
    return ret;
}

void WRed::wrapUpdateContiguousPTEsWithDMAUsingAddr(void *that, uint64_t param1, uint64_t param2, uint64_t param3,
    uint64_t param4, uint64_t param5) {
    NETLOG("wred",
        "updateContiguousPTEsWithDMAUsingAddr: this = %p param1 = 0x%llX param2 = 0x%llX param3 = 0x%llX param4 = "
        "0x%llX param5 = 0x%llX",
        that, param1, param2, param3, param4, param5);
    FunctionCast(wrapUpdateContiguousPTEsWithDMAUsingAddr, callbackWRed->orgUpdateContiguousPTEsWithDMAUsingAddr)(that,
        param1, param2, param3, param4, param5);
    NETLOG("wred", "updateContiguousPTEsWithDMAUsingAddr finished");
}

void WRed::wrapInitializeFamilyType(void *that) { getMember<uint32_t>(that, 0x308) = 0x8E; }    // 0x8D -> 0x8E

uint32_t WRed::pspFeatureUnsupported() { return 4; }    // PSP RAP and XGMI are unsupported

void *WRed::wrapAllocateAMDHWDisplay(void *that) {
    return FunctionCast(wrapAllocateAMDHWDisplay, callbackWRed->orgAllocateAMDHWDisplayX6000)(that);
}

void WRed::wrapPspCosLog(void *pspData, uint32_t param2, uint64_t param3, uint32_t param4, char *param5) {
    if (param5) {
        NETDBG::printf("AMD TTL COS: %s", param5);
        auto len = strlen(param5);
        if (!len || param5[len - 1] != '\n') { NETDBG::printf("\n"); }
    }
    FunctionCast(wrapPspCosLog, callbackWRed->orgPspCosLog)(pspData, param2, param3, param4, param5);
}

uint32_t WRed::wrapPspCmdKmSubmit(void *pspData, void *context, void *param3, void *param4) {
    uint32_t fwType = getMember<uint>(context, 16);
    // Skip loading of CP MEC JT2 FW on Renoir devices due to it being unsupported
    // See also: https://github.com/torvalds/linux/commit/f8f70c1371d304f42d4a1242d8abcbda807d0bed
    if (fwType == 6 && callbackWRed->asicType == ASICType::Renoir) {
        NETLOG("wred", "Skipping loading of fwType 6");
        return 0;
    }

    auto ret = FunctionCast(wrapPspCmdKmSubmit, callbackWRed->orgPspCmdKmSubmit)(pspData, context, param3, param4);
    NETLOG("wred", "_psp_cmd_km_submit returned 0x%X", ret);
    return ret;
}

void WRed::wrapAtiPowerPlayServicesConstructor(void *that, void *ppCallbacks) {
    getMember<uint32_t>(ppCallbacks, 0x60) = 0xFF;    // Set debugLevel in order to activate _MCILDebugPrint
    FunctionCast(wrapAtiPowerPlayServicesConstructor, callbackWRed->orgAtiPowerPlayServicesConstructor)(that,
        ppCallbacks);
}

bool WRed::wrapInitWithPciInfo(void *that, void *param1) {
    auto ret = FunctionCast(wrapInitWithPciInfo, callbackWRed->orgInitWithPciInfo)(that, param1);
    // Hack AMDRadeonX6000_AmdLogger to log everything
    getMember<uint64_t>(that, 0x28) = 0xFFFFFFFFFFFFFFFFULL;
    getMember<uint32_t>(that, 0x30) = 0xFF;
    return ret;
}

IOReturn WRed::wrapEnableController(void *that) {
    if (callbackWRed->asicType == ASICType::Renoir) { IOSleep(2000); }
    NETLOG("wred", "enableController: that = %p", that);
    auto ret = FunctionCast(wrapEnableController, callbackWRed->orgEnableController)(that);
    NETLOG("wred", "enableController returned 0x%X", ret);
    if (callbackWRed->asicType == ASICType::Renoir) { IOSleep(2000); }
    return ret;
}

uint64_t WRed::wrapControllerPowerUp(void *that) {
    NETLOG("wred", "controllerPowerUp: that = %p", that);
    auto ret = FunctionCast(wrapControllerPowerUp, callbackWRed->orgControllerPowerUp)(that);
    NETLOG("wred", "controllerPowerUp returned 0x%llX", ret);
    if (callbackWRed->asicType == ASICType::Renoir) { IOSleep(2000); }
    return ret;
}

uint64_t WRed::wrapPpPowerUp(void *that) {
    NETLOG("wred", "ppPowerUp: that = %p", that);
    auto ret = FunctionCast(wrapPpPowerUp, callbackWRed->orgPpPowerUp)(that);
    NETLOG("wred", "ppPowerUp returned 0x%llX", ret);
    if (callbackWRed->asicType == ASICType::Renoir) { IOSleep(2000); }
    return ret;
}

uint64_t WRed::wrapDalHelperPowerUp(void *that) {
    NETLOG("wred", "dalHelperPowerUp: that = %p", that);
    auto ret = FunctionCast(wrapDalHelperPowerUp, callbackWRed->orgDalHelperPowerUp)(that);
    NETLOG("wred", "dalHelperPowerUp returned 0x%llX", ret);
    if (callbackWRed->asicType == ASICType::Renoir) { IOSleep(2000); }
    return ret;
}

uint64_t WRed::wrapSetupBootWatermarks(void *that) {
    NETLOG("wred", "setupBootWatermarks: that = %p", that);
    auto ret = FunctionCast(wrapSetupBootWatermarks, callbackWRed->orgSetupBootWatermarks)(that);
    NETLOG("wred", "setupBootWatermarks returned 0x%llX", ret);
    if (callbackWRed->asicType == ASICType::Renoir) { IOSleep(2000); }
    return ret;
}

uint32_t WRed::wrapSetupLinks(void *that) {
    NETLOG("wred", "setupLinks: that = %p", that);
    auto ret = FunctionCast(wrapSetupLinks, callbackWRed->orgSetupLinks)(that);
    NETLOG("wred", "setupLinks returned 0x%X", ret);
    if (callbackWRed->asicType == ASICType::Renoir) { IOSleep(2000); }
    return ret;
}

uint64_t WRed::wrapMessageAccelerator(void *that, uint32_t param1, void *param2, void *param3, void *param4) {
    NETLOG("wred", "messageAccelerator: that = %p param1 = 0x%X param2 = %p param3 = %p param4 = %p", that, param1,
        param2, param3, param4);
    if (callbackWRed->asicType == ASICType::Renoir) { IOSleep(2000); }
    auto ret =
        FunctionCast(wrapMessageAccelerator, callbackWRed->orgMessageAccelerator)(that, param1, param2, param3, param4);
    NETLOG("wred", "messageAccelerator returned 0x%llX", ret);
    if (callbackWRed->asicType == ASICType::Renoir) { IOSleep(2000); }
    return ret;
}

// {incr, entryCount, vmBlockSize}
static uint64_t oneLevelVMPTConfig[][3] = {
    {0x1000, 0x400000, 0x2000000},
    {0x1000, 0x200, 0x1000},
    {0x1000, 0x200, 0x1000},
};

static uint64_t twoLevelVMPTConfig[][3] = {
    {0x10000000, 0x200, 0x1000},
    {0x1000, 0x10000, 0x80000},
    {0x1000, 0x200, 0x1000},
};

// Inferred from the Linux codebase
static uint64_t threeLevelVMPTConfig[][3] = {
    {0x40000000, 0x80, 0x1000},
    {0x200000, 0x200, 0x1000},
    {0x1000, 0x200, 0x1000},
};

bool WRed::wrapVMMInit(void *that, void *param1) {
    NETLOG("rad", "VMMInit: this = %p param1 = %p", that, param1);

    for (size_t level = 0; level < 3; level++) {
        getMember<uint64_t>(that, 0xAB0 + 0x20 * level) = oneLevelVMPTConfig[level][0];
        getMember<uint32_t>(that, 0xAB0 + 0x20 * level + 0xC) = static_cast<uint32_t>(oneLevelVMPTConfig[level][1]);
        getMember<uint32_t>(that, 0xAB0 + 0x20 * level + 0x10) = static_cast<uint32_t>(oneLevelVMPTConfig[level][2]);
    }

    auto ret = FunctionCast(wrapVMMInit, callbackWRed->orgVMMInit)(that, param1);
    getMember<uint64_t>(that, 0xAA8) = 0x800000000;    // rangeEnd
    getMember<uint32_t>(that, 0xB30) = 1;              // vmptLevels
    getMember<uint32_t>(that, 0xB34) = 0;              // vmptDepth

    NETLOG("rad", "VMMInit returned %d", ret);

    getMember<mach_vm_address_t *>(that, 0)[56] = reinterpret_cast<mach_vm_address_t>(wrapGetVMPTBCoverage);
    return ret;
}

uint64_t WRed::wrapGetPDEValue(void *that, uint64_t level, uint64_t param2) {
    NETLOG("wred", "getPDEValue: this = %p level = 0x%llX param2 = 0x%llX", that, level, param2);
    auto ret = FunctionCast(wrapGetPDEValue, callbackWRed->orgGetPDEValue)(that, level, param2);
    auto old_ret = ret;
    NETLOG("wred", "getPDEValue returned 0x%llX", old_ret);

    // Unset AMDGPU_PDE_BFS
    ret &= ~(0xFULL << 59);

    if (old_ret != ret) { NETLOG("wred", "getPDEValue: returning 0x%llX", ret); }

    return ret;
}

uint64_t WRed::wrapGetPTEValue(void *that, uint64_t level, uint64_t param2, uint64_t param3, uint32_t param4) {
    NETLOG("wred", "getPTEValue: this = %p level = 0x%llX param2 = 0x%llX param3 = 0x%llX param4 = 0x%X", that, level,
        param2, param3, param4);
    auto ret = FunctionCast(wrapGetPTEValue, callbackWRed->orgGetPTEValue)(that, level, param2, param3, param4);
    NETLOG("wred", "getPTEValue returned 0x%llX", ret);
    return ret;
}

uint64_t WRed::wrapGvmGetIpFunction(uint16_t major, uint16_t minor, uint16_t patch, uint32_t funcType, void *funcTable,
    uint32_t ipType) {
    NETLOG("wred",
        "_gvm_get_ip_function: major = 0x%hX minor = 0x%hX patch = 0x%hX funcType = 0x%X funcTable = %p ipType = 0x%X",
        major, minor, patch, funcType, funcTable, ipType);
    if (ipType == 0xF) {
        switch (callbackWRed->asicType) {
            case ASICType::Raven2:
                major = 0x6;
                minor = 0x1;
                patch = 0x0;
                NETLOG("wred", "_gvm_get_ip_function: Changing MC version to v6.1.0");
                break;
            case ASICType::Raven:
                major = 0x6;
                minor = 0x0;
                patch = 0x0;
                NETLOG("wred", "_gvm_get_ip_function: Changing MC version to v6.0.0");
                break;
            default:
                break;
        }
    }
    auto ret = FunctionCast(wrapGvmGetIpFunction, callbackWRed->orgGvmGetIpFunction)(major, minor, patch, funcType,
        funcTable, ipType);
    NETLOG("wred", "_gvm_get_ip_function returned 0x%llX", ret);
    return ret;
}

uint64_t WRed::wrapGetVMPTBCoverage(void *that) {
    static uint32_t callCount = 0;
    callCount++;
    if (callCount > 10) { PANIC("wred", "Meow"); }
    uint block_size = 0;
    uint64_t ret = 1ULL << (21 + block_size);
    NETLOG("wred", "getVMPTBCoverage: Returning 0x%llX", ret);
    return ret;
}

void WRed::wrapHwRegWrite(void *that, uint32_t regIndex, uint32_t regVal) {
    NETLOG("wred", "hwRegWrite: that = %p regIndex = 0x%X regVal = 0x%X", that, regIndex, regVal);
    FunctionCast(wrapHwRegWrite, callbackWRed->orgHwRegWrite)(that, regIndex, regVal);
}
