//  Copyright © 2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.0. See LICENSE for
//  details.

#include "kern_wred.hpp"
#include "kern_amd.hpp"
#include "kern_fw.hpp"
#include <Headers/kern_api.hpp>

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

    lilu.onPatcherLoadForce(
        [](void *user, KernelPatcher &patcher) { static_cast<WRed *>(user)->processPatcher(patcher); }, this);
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

void WRed::processPatcher(KernelPatcher &patcher) {
    KernelPatcher::RouteRequest requests[] = {
        {"__ZN15OSMetaClassBase12safeMetaCastEPKS_PK11OSMetaClass", wrapSafeMetaCast, orgSafeMetaCast},
    };
    PANIC_COND(!patcher.routeMultiple(KernelPatcher::KernelID, requests), "wred",
        "Failed to route OSMetaClassBase::safeMetaCast");
}

OSMetaClassBase *WRed::wrapSafeMetaCast(const OSMetaClassBase *anObject, const OSMetaClass *toMeta) {
    auto ret = FunctionCast(wrapSafeMetaCast, callbackWRed->orgSafeMetaCast)(anObject, toMeta);
    if (!ret) {
        for (auto &ent : callbackWRed->metaClassMap) {
            if (ent[0] == toMeta) {
                toMeta = ent[1];
            } else if (ent[1] == toMeta) {
                toMeta = ent[0];
            } else {
                continue;
            }
            return FunctionCast(wrapSafeMetaCast, callbackWRed->orgSafeMetaCast)(anObject, toMeta);
        }
    }
    return ret;
}

void WRed::processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
    if (kextRadeonX5000HWLibs.loadIndex == index) {
        KernelPatcher::SolveRequest solveRequests[] = {
            {"__ZL15deviceTypeTable", orgDeviceTypeTable},
            {"__ZN11AMDFirmware14createFirmwareEPhjjPKc", orgCreateFirmware},
            {"__ZN20AMDFirmwareDirectory11putFirmwareE16_AMD_DEVICE_TYPEP11AMDFirmware", orgPutFirmware},
            {"__ZN31AtiAppleVega10PowerTuneServicesC1EP11PP_InstanceP18PowerPlayCallbacks",
                orgVega10PowerTuneConstructor},
            {"__ZL20CAIL_ASIC_CAPS_TABLE", orgCapsTableHWLibs},
            {"_CAILAsicCapsInitTable", orgAsicInitCapsTable},
            {"_gc_9_2_1_rlc_ucode", orgGcRlcUcode},
            {"_gc_9_2_1_me_ucode", orgGcMeUcode},
            {"_gc_9_2_1_ce_ucode", orgGcCeUcode},
            {"_gc_9_2_1_pfp_ucode", orgGcPfpUcode},
            {"_gc_9_2_1_mec_ucode", orgGcMecUcode},
            {"_gc_9_2_1_mec_jt_ucode", orgGcMecJtUcode},
            {"_sdma_4_1_ucode", orgSdma41Ucode},
            {"_sdma_4_1_2_ucode", orgSdma412Ucode},
            {"_Raven_SendMsgToSmc", orgRavenSendMsgToSmc},
            {"_Renoir_SendMsgToSmc", orgRenoirSendMsgToSmc},
        };
        PANIC_COND(!patcher.solveMultiple(index, solveRequests, address, size), "wred",
            "Failed to resolve AMDRadeonX5000HWLibs symbols");

        KernelPatcher::RouteRequest requests[] = {
            {"__ZN14AmdTtlServicesC2EP11IOPCIDevice", wrapAmdTtlServicesConstructor, orgAmdTtlServicesConstructor},
            {"_smu_get_hw_version", wrapSmuGetHwVersion},
            {"_psp_sw_init", wrapPspSwInit, orgPspSwInit},
            {"_gc_get_hw_version", wrapGcGetHwVersion},
            {"__ZN35AMDRadeonX5000_AMDRadeonHWLibsX500025populateFirmwareDirectoryEv", wrapPopulateFirmwareDirectory,
                orgPopulateFirmwareDirectory},
            {"__ZN25AtiApplePowerTuneServices23createPowerTuneServicesEP11PP_InstanceP18PowerPlayCallbacks",
                wrapCreatePowerTuneServices},
            {"_smu_get_fw_constants", hwLibsNoop},
            {"_smu_9_0_1_check_fw_status", hwLibsNoop},
            {"_smu_9_0_1_unload_smu", hwLibsNoop},
            {"_psp_asd_load", wrapPspAsdLoad, orgPspAsdLoad},
            {"_psp_dtm_load", wrapPspDtmLoad, orgPspDtmLoad},
            {"_psp_hdcp_load", wrapPspHdcpLoad, orgPspHdcpLoad},
            {"_SmuRaven_Initialize", wrapSmuRavenInitialize, orgSmuRavenInitialize},
            {"_SmuRenoir_Initialize", wrapSmuRenoirInitialize, orgSmuRenoirInitialize},
            {"_psp_cmd_km_submit", wrapPspCmdKmSubmit, orgPspCmdKmSubmit},
        };
        PANIC_COND(!patcher.routeMultiple(index, requests, address, size), "wred",
            "Failed to route AMDRadeonX5000HWLibs symbols");

        /**
         * Patch for `_smu_9_0_1_full_asic_reset`
         * Correct sent message to `0x1E` as the original code sends `0x3B` which is wrong for SMU 10.
         */
        const uint8_t find_asic_reset[] = {0x55, 0x48, 0x89, 0xE5, 0x8B, 0x56, 0x04, 0xBE, 0x3B, 0x00, 0x00, 0x00, 0x5D,
            0xE9, 0x51, 0xFE, 0xFF, 0xFF};
        const uint8_t repl_asic_reset[] = {0x55, 0x48, 0x89, 0xE5, 0x8B, 0x56, 0x04, 0xBE, 0x1E, 0x00, 0x00, 0x00, 0x5D,
            0xE9, 0x51, 0xFE, 0xFF, 0xFF};
        static_assert(arrsize(find_asic_reset) == arrsize(repl_asic_reset));

        /**
         * Patches for `_psp_asd_load`.
         * `_psp_asd_load` loads a hardcoded ASD firmware binary
         * included in the kext as `_psp_asd_bin`.
         * The copied data isn't in a table, it is a single
         * binary copied over to the PSP private memory.
         * We can't replicate such logic in any AMDGPU kext function,
         * as the memory accesses to GPU data is inaccessible
         * from external kexts, therefore, we have to do a hack.
         * The hack is very straight forward; we have replaced the
         * assembly that loads hardcoded values from
         *     `lea rsi, [_psp_asd_bin]`
         *     `mov edx, 0x2c100`
         *     `mov rdi, r15`
         *     `call _memcpy`
         * to
         *     `mov rsi, rcx`
         *     `mov rdx, r8`
         *     `mov rdi, r15`
         *     `call _memcpy`
         * so that it gets the pointer and size from parameter 4 and 5.
         * Register choice was because the parameter 2 and 3 registers
         * get overwritten before this call to memcpy
         * The hack we came up with looks like terrible practice,
         * but this will have to do.
         * Pain.
         */
        const uint8_t find_load_asd_pt1[] = {0x0f, 0x85, 0x83, 0x00, 0x00, 0x00, 0x48, 0x8d, 0x35, 0xf7, 0x93, 0xf4,
            0x00, 0xba, 0x00, 0xc1, 0x02, 0x00, 0x4c, 0x89, 0xff, 0xe8, 0xf2, 0xa6, 0x56, 0x02};
        const uint8_t repl_load_asd_pt1[] = {0x0f, 0x85, 0x83, 0x00, 0x00, 0x00, 0x48, 0x8b, 0xf1, 0x4c, 0x89, 0xc2,
            0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x4c, 0x89, 0xff, 0xe8, 0xf2, 0xa6, 0x56, 0x02};
        static_assert(arrsize(find_load_asd_pt1) == arrsize(repl_load_asd_pt1));
        const uint8_t find_load_asd_pt2[] = {0x44, 0x89, 0x66, 0x08, 0x48, 0xc7, 0x46, 0x0c, 0x00, 0xc1, 0x02, 0x00,
            0x48, 0xc7, 0x46, 0x14, 0x00, 0x00, 0x00, 0x00};
        const uint8_t repl_load_asd_pt2[] = {0x44, 0x89, 0x66, 0x08, 0x4c, 0x89, 0x84, 0x26, 0x0c, 0x00, 0x00, 0x00,
            0x48, 0xc7, 0x46, 0x14, 0x00, 0x00, 0x00, 0x00};
        static_assert(arrsize(find_load_asd_pt2) == arrsize(repl_load_asd_pt2));

        /**
         * Patches for `_psp_dtm_load`.
         * Same idea as `_psp_asd_load`.
         */
        const uint8_t find_load_dtm_pt1[] = {0x48, 0x8b, 0xbb, 0xf8, 0x0a, 0x00, 0x00, 0x48, 0x8d, 0x35, 0x47, 0x4f,
            0xf7, 0x00, 0xba, 0x00, 0x21, 0x00, 0x00, 0xe8, 0x45, 0xa1, 0x56, 0x02, 0x48, 0x8d, 0xb5, 0x70, 0xfc, 0xff,
            0xff};
        const uint8_t repl_load_dtm_pt1[] = {0x48, 0x8b, 0xbb, 0xf8, 0x0a, 0x00, 0x00, 0x48, 0x8b, 0xf1, 0x49, 0x8b,
            0xd0, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0xe8, 0x45, 0xa1, 0x56, 0x02, 0x48, 0x8d, 0xb5, 0x70, 0xfc, 0xff,
            0xff};
        static_assert(arrsize(find_load_dtm_pt1) == arrsize(repl_load_dtm_pt1));
        const uint8_t find_load_dtm_pt2[] = {0x44, 0x89, 0x76, 0x08, 0xc7, 0x46, 0x0c, 0x00, 0x21, 0x00, 0x00, 0x48,
            0x8b, 0x83, 0xb8, 0x2d, 0x00, 0x00};
        const uint8_t repl_load_dtm_pt2[] = {0x44, 0x89, 0x76, 0x08, 0x44, 0x89, 0x86, 0x0c, 0x00, 0x00, 0x00, 0x48,
            0x8b, 0x83, 0xb8, 0x2d, 0x00, 0x00};
        static_assert(arrsize(find_load_dtm_pt2) == arrsize(repl_load_dtm_pt2));

        /**
         * Patch for `_psp_hdcp_load`.
         * Same idea as `_psp_asd_load`.
         */
        const uint8_t find_load_hdcp[] = {0x48, 0x8d, 0x35, 0x0d, 0xa4, 0xf7, 0x00, 0xba, 0x00, 0x61, 0x00, 0x00, 0xe8,
            0x0b, 0x26, 0x5a, 0x02, 0x48, 0x8d, 0xb5, 0x60, 0xfc, 0xff, 0xff, 0xc7, 0x06, 0x01, 0x00, 0x00, 0x00, 0x89,
            0x5e, 0x04, 0x48, 0xc1, 0xeb, 0x20, 0x89, 0x5e, 0x08, 0xc7, 0x46, 0x0c, 0x00, 0x61, 0x00, 0x00};
        const uint8_t repl_load_hdcp[] = {0x48, 0x89, 0xce, 0x66, 0x90, 0x66, 0x90, 0x41, 0x8b, 0xd0, 0x66, 0x90, 0xe8,
            0x0b, 0x26, 0x5a, 0x02, 0x48, 0x8d, 0xb5, 0x60, 0xfc, 0xff, 0xff, 0xc7, 0x06, 0x01, 0x00, 0x00, 0x00, 0x89,
            0x5e, 0x04, 0x48, 0xc1, 0xeb, 0x20, 0x89, 0x5e, 0x08, 0x44, 0x89, 0x86, 0x0c, 0x00, 0x00, 0x00};
        static_assert(arrsize(find_load_hdcp) == arrsize(repl_load_hdcp));

        KernelPatcher::LookupPatch patches[] = {
            {&kextRadeonX5000HWLibs, find_asic_reset, repl_asic_reset, arrsize(find_asic_reset), 1},
            {&kextRadeonX5000HWLibs, find_load_asd_pt1, repl_load_asd_pt1, arrsize(find_load_asd_pt1), 1},
            {&kextRadeonX5000HWLibs, find_load_asd_pt2, repl_load_asd_pt2, arrsize(find_load_asd_pt2), 1},
            {&kextRadeonX5000HWLibs, find_load_dtm_pt1, repl_load_dtm_pt1, arrsize(find_load_dtm_pt1), 1},
            {&kextRadeonX5000HWLibs, find_load_dtm_pt2, repl_load_dtm_pt2, arrsize(find_load_dtm_pt2), 1},
            {&kextRadeonX5000HWLibs, find_load_hdcp, repl_load_hdcp, arrsize(find_load_hdcp), 1},
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
            {"__ZN30AMDRadeonX6000_AmdAsicInfoNavi18populateDeviceInfoEv", wrapPopulateDeviceInfo,
                orgPopulateDeviceInfo},
            {"__ZNK32AMDRadeonX6000_AmdAsicInfoNavi1027getEnumeratedRevisionNumberEv", wrapGetEnumeratedRevision},
            {"__ZN32AMDRadeonX6000_AmdRegisterAccess11hwReadReg32Ej", wrapHwReadReg32, orgHwReadReg32},
            {"__ZN24AMDRadeonX6000_AmdLogger15initWithPciInfoEP11IOPCIDevice", wrapInitWithPciInfo, orgInitWithPciInfo},
        };
        PANIC_COND(!patcher.routeMultiple(index, requests, address, size), "wred",
            "Failed to route AMDRadeonX6000Framebuffer symbols");

        /** Neutralise VRAM Info creation null check to proceed with Controller Core Services initialisation. */
        const uint8_t find_null_check1[] = {0x48, 0x89, 0x83, 0x90, 0x00, 0x00, 0x00, 0x48, 0x85, 0xC0, 0x0F, 0x84,
            0x89, 0x00, 0x00, 0x00, 0x48, 0x8B, 0x7B, 0x18};
        const uint8_t repl_null_check1[] = {0x48, 0x89, 0x83, 0x90, 0x00, 0x00, 0x00, 0x90, 0x90, 0x90, 0x90, 0x90,
            0x90, 0x90, 0x90, 0x90, 0x48, 0x8B, 0x7B, 0x18};
        static_assert(arrsize(find_null_check1) == arrsize(repl_null_check1), "Find/replace patch size mismatch");

        /** Neutralise PSP Firmware Info creation null check to proceed with Controller Core Services
           initialisation. */
        const uint8_t find_null_check2[] = {0x48, 0x89, 0x83, 0x88, 0x00, 0x00, 0x00, 0x48, 0x85, 0xC0, 0x0F, 0x84,
            0xA1, 0x00, 0x00, 0x00, 0x48, 0x8B, 0x7B, 0x18};
        const uint8_t repl_null_check2[] = {0x48, 0x89, 0x83, 0x88, 0x00, 0x00, 0x00, 0x90, 0x90, 0x90, 0x90, 0x90,
            0x90, 0x90, 0x90, 0x90, 0x48, 0x8B, 0x7B, 0x18};
        static_assert(arrsize(find_null_check2) == arrsize(repl_null_check2), "Find/replace patch size mismatch");

        /** Neutralise VRAM Info null check inside `AmdAtomFwServices::getFirmwareInfo`. */
        const uint8_t find_null_check3[] = {0x48, 0x83, 0xBB, 0x90, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x84, 0x90, 0x00,
            0x00, 0x00, 0x49, 0x89, 0xF7, 0xBA, 0x60, 0x00, 0x00, 0x00};
        const uint8_t repl_null_check3[] = {0x48, 0x83, 0xBB, 0x90, 0x00, 0x00, 0x00, 0x00, 0x90, 0x90, 0x90, 0x90,
            0x90, 0x90, 0x49, 0x89, 0xF7, 0xBA, 0x60, 0x00, 0x00, 0x00};
        static_assert(arrsize(find_null_check3) == arrsize(repl_null_check3), "Find/replace patch size mismatch");

        KernelPatcher::LookupPatch patches[] = {
            {&kextRadeonX6000Framebuffer, find_null_check1, repl_null_check1, arrsize(find_null_check1), 1},
            {&kextRadeonX6000Framebuffer, find_null_check2, repl_null_check2, arrsize(find_null_check2), 1},
            {&kextRadeonX6000Framebuffer, find_null_check3, repl_null_check3, arrsize(find_null_check3), 1},
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
        PANIC_COND(!patcher.solveMultiple(index, solveRequests, address, size), "wred",
            "Failed to resolve AMDRadeonX5000 symbols");

        /** Patch the data so that it only starts SDMA0. */
        PANIC_COND(MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) != KERN_SUCCESS, "wred",
            "Failed to enable kernel writing");
        orgChannelTypes[5] = 1;     // Fix createAccelChannels
        orgChannelTypes[11] = 0;    // Fix getPagingChannel
        MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);

        KernelPatcher::RouteRequest requests[] = {
            {"__ZN32AMDRadeonX5000_AMDVega10Hardware17allocateHWEnginesEv", wrapAllocateHWEngines},
            {"__ZN32AMDRadeonX5000_AMDVega10Hardware32setupAndInitializeHWCapabilitiesEv",
                wrapSetupAndInitializeHWCapabilities, orgSetupAndInitializeHWCapabilities},
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
        PANIC_COND(!patcher.routeMultiple(index, requests, address, size), "wred",
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
        PANIC_COND(!patcher.solveMultiple(index, solveRequests, address, size), "wred",
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
        PANIC_COND(!patcher.routeMultiple(index, requests, address, size), "wred",
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

void WRed::wrapAmdTtlServicesConstructor(void *that, IOPCIDevice *provider) {
    SYSLOG("wred", "Please don't support tonymacx86.com!");

    DBGLOG("wred", "Patching device type table");
    PANIC_COND(MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) != KERN_SUCCESS, "wred",
        "Failed to enable kernel writing");
    callbackWRed->orgDeviceTypeTable[0] = provider->extendedConfigRead16(kIOPCIConfigDeviceID);
    callbackWRed->orgDeviceTypeTable[1] = 6;
    MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);
    if (provider->getProperty("ATY,bin_image")) {
        DBGLOG("wred", "VBIOS manually overridden");
    } else {
        if (!callbackWRed->getVBIOSFromVFCT(provider)) {
            DBGLOG("wred", "Failed to get VBIOS from VFCT, trying to get it from VRAM");
            PANIC_COND(!callbackWRed->getVBIOSFromVRAM(provider), "wred", "Failed to get VBIOS from VRAM");
        }
    }

    FunctionCast(wrapAmdTtlServicesConstructor, callbackWRed->orgAmdTtlServicesConstructor)(that, provider);
}

uint32_t WRed::wrapSmuGetHwVersion() { return 0x1; }

uint32_t WRed::wrapPspSwInit(uint32_t *param1, uint32_t *param2) {
    switch (param1[3]) {
        case 0xA:
            DBGLOG("wred", "Spoofing PSP version v10 to v9.0.2");
            param1[3] = 0x9;
            param1[4] = 0x0;
            param1[5] = 0x2;
            break;
        case 0xB:
            [[fallthrough]];
        case 0xC:
            DBGLOG("wred", "Spoofing PSP version v11/v12 to v11");
            param1[3] = 0xB;
            param1[4] = 0x0;
            param1[5] = 0x0;
            break;
        default:
            break;
    }
    auto ret = FunctionCast(wrapPspSwInit, callbackWRed->orgPspSwInit)(param1, param2);
    DBGLOG("wred", "_psp_sw_init >> 0x%X", ret);
    return ret;
}

uint32_t WRed::wrapGcGetHwVersion() { return 0x090201; }

void WRed::wrapPopulateFirmwareDirectory(void *that) {
    FunctionCast(wrapPopulateFirmwareDirectory, callbackWRed->orgPopulateFirmwareDirectory)(that);
    callbackWRed->callbackFirmwareDirectory = getMember<void *>(that, 0xB8);
}

void *WRed::wrapCreatePowerTuneServices(void *that, void *param2) {
    auto *ret = IOMallocZero(0x18);
    callbackWRed->orgVega10PowerTuneConstructor(ret, that, param2);
    return ret;
}

uint16_t WRed::wrapGetEnumeratedRevision(void *that) {
    auto revision = getMember<uint32_t>(that, 0x68);
    switch (getMember<IOPCIDevice *>(that, 0x18)->configRead16(kIOPCIConfigDeviceID)) {
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
        case 0x1638:
            callbackWRed->asicType = ASICType::Renoir;
            return 0x91;
        case 0x164C:
            [[fallthrough]];
        case 0x1636:
            callbackWRed->asicType = ASICType::GreenSardine;
            return 0xA1;
        default:
            PANIC("wred", "Unknown device ID");
    }
}

static bool injectedIPFirmware = false;

IOReturn WRed::wrapPopulateDeviceInfo(void *that) {
    auto ret = FunctionCast(wrapPopulateDeviceInfo, callbackWRed->orgPopulateDeviceInfo)(that);
    getMember<uint32_t>(that, 0x60) = AMDGPU_FAMILY_RV;
    auto deviceId = getMember<IOPCIDevice *>(that, 0x18)->configRead16(kIOPCIConfigDeviceID);
    auto revision = getMember<uint32_t>(that, 0x68);
    auto emulatedRevision = getMember<uint32_t>(that, 0x6c);

    DBGLOG("wred", "Locating Init Caps entry");
    PANIC_COND(MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) != KERN_SUCCESS, "wred",
        "Failed to enable kernel writing");

    if (!injectedIPFirmware) {
        injectedIPFirmware = true;
        auto *asicName = getASICName();
        auto *filename = new char[128];

        if (callbackWRed->asicType >= ASICType::Renoir) {
            snprintf(filename, 128, "%s_dmcub.bin", asicName);
            DBGLOG("wred", "%s => atidmcub_0.dat", filename);
            auto &fwDesc = getFWDescByName(filename);
            auto *fwHeader = reinterpret_cast<const GfxFwHeaderV1 *>(fwDesc.data);
            auto *fwDmcub = callbackWRed->orgCreateFirmware(fwDesc.data + fwHeader->ucodeOff, fwHeader->ucodeSize,
                0x200, "atidmcub_0.dat");
            PANIC_COND(!fwDmcub, "wred", "Failed to create atidmcub_0.dat firmware");
            DBGLOG("wred", "Inserting atidmcub_0.dat!");
            PANIC_COND(!callbackWRed->orgPutFirmware(callbackWRed->callbackFirmwareDirectory, 6, fwDmcub), "wred",
                "Failed to inject atidmcub_0.dat firmware");
        }

        snprintf(filename, 128, "%s_vcn.bin", asicName);
        auto *targetFilename = callbackWRed->asicType >= ASICType::Renoir ? "ativvaxy_nv.dat" : "ativvaxy_rv.dat";
        DBGLOG("wred", "%s => %s", filename, targetFilename);

        auto *fwDesc = &getFWDescByName(filename);
        auto *fwHeader = reinterpret_cast<const GfxFwHeaderV1 *>(fwDesc->data);
        auto *fw = callbackWRed->orgCreateFirmware(fwDesc->data + fwHeader->ucodeOff, fwHeader->ucodeSize, 0x200,
            targetFilename);
        PANIC_COND(!fw, "wred", "Failed to create '%s' firmware", targetFilename);
        DBGLOG("wred", "Inserting %s!", targetFilename);
        PANIC_COND(!callbackWRed->orgPutFirmware(callbackWRed->callbackFirmwareDirectory, 6, fw), "wred",
            "Failed to inject ativvaxy_rv.dat firmware");

        snprintf(filename, 128, "%s_rlc.bin", asicName);
        fwDesc = &getFWDescByName(filename);
        fwHeader = reinterpret_cast<const GfxFwHeaderV1 *>(fwDesc->data);
        callbackWRed->orgGcRlcUcode->addr = 0x0;
        callbackWRed->orgGcRlcUcode->data = fwDesc->data + fwHeader->ucodeOff;
        DBGLOG("wred", "Injected %s!", filename);

        snprintf(filename, 128, "%s_me.bin", asicName);
        fwDesc = &getFWDescByName(filename);
        fwHeader = reinterpret_cast<const GfxFwHeaderV1 *>(fwDesc->data);
        callbackWRed->orgGcMeUcode->addr = 0x0;
        callbackWRed->orgGcMeUcode->data = fwDesc->data + fwHeader->ucodeOff;
        DBGLOG("wred", "Injected %s!", filename);

        snprintf(filename, 128, "%s_ce.bin", asicName);
        fwDesc = &getFWDescByName(filename);
        fwHeader = reinterpret_cast<const GfxFwHeaderV1 *>(fwDesc->data);
        callbackWRed->orgGcCeUcode->addr = 0x0;
        callbackWRed->orgGcCeUcode->data = fwDesc->data + fwHeader->ucodeOff;
        DBGLOG("wred", "Injected %s!", filename);

        snprintf(filename, 128, "%s_pfp.bin", asicName);
        fwDesc = &getFWDescByName(filename);
        fwHeader = reinterpret_cast<const GfxFwHeaderV1 *>(fwDesc->data);
        callbackWRed->orgGcPfpUcode->addr = 0x0;
        callbackWRed->orgGcPfpUcode->data = fwDesc->data + fwHeader->ucodeOff;
        DBGLOG("wred", "Injected %s!", filename);

        snprintf(filename, 128, "%s_mec.bin", asicName);
        fwDesc = &getFWDescByName(filename);
        fwHeader = reinterpret_cast<const GfxFwHeaderV1 *>(fwDesc->data);
        callbackWRed->orgGcMecUcode->addr = 0x0;
        callbackWRed->orgGcMecUcode->data = fwDesc->data + fwHeader->ucodeOff;
        DBGLOG("wred", "Injected %s!", filename);
        callbackWRed->orgGcMecJtUcode->addr = fwHeader->jtOff;
        callbackWRed->orgGcMecJtUcode->data =
            fwDesc->data + fwHeader->ucodeOff + (fwHeader->ucodeSize - fwHeader->jtSize * 4);
        DBGLOG("wred", "Injected %s <jt>!", filename);

        snprintf(filename, 128, "%s_sdma.bin", asicName);
        fwDesc = &getFWDescByName(filename);
        auto *sdmaFwHeader = reinterpret_cast<const SdmaFwHeaderV1 *>(fwDesc->data);
        callbackWRed->orgSdma41Ucode->data = fwDesc->data + sdmaFwHeader->ucodeOff;
        callbackWRed->orgSdma412Ucode->data = fwDesc->data + sdmaFwHeader->ucodeOff;
        DBGLOG("wred", "Injected %s!", filename);

        delete[] filename;
    }

    CailInitAsicCapEntry *initCaps = nullptr;
    for (size_t i = 0; i < 789; i++) {
        auto *temp = callbackWRed->orgAsicInitCapsTable + i;
        if (temp->familyId == AMDGPU_FAMILY_RV && temp->deviceId == deviceId && temp->revision == revision) {
            initCaps = temp;
            break;
        }
    }

    if (!initCaps) {
        DBGLOG("wred", "Warning: Using fallback Init Caps search");
        for (size_t i = 0; i < 789; i++) {
            auto *temp = callbackWRed->orgAsicInitCapsTable + i;
            if (temp->familyId == AMDGPU_FAMILY_RV && temp->deviceId == deviceId) {
                initCaps = temp;
                initCaps->emulatedRev = emulatedRevision;
                initCaps->revision = revision;
                initCaps->pciRev = 0xFFFFFFFF;
                break;
            }
        }
        PANIC_COND(!initCaps, "wred", "Failed to find Init Caps entry");
    }

    callbackWRed->orgCapsTableHWLibs->familyId = callbackWRed->orgAsicCapsTable->familyId = AMDGPU_FAMILY_RV;
    callbackWRed->orgCapsTableHWLibs->deviceId = callbackWRed->orgAsicCapsTable->deviceId = deviceId;
    callbackWRed->orgCapsTableHWLibs->revision = callbackWRed->orgAsicCapsTable->revision = revision;
    callbackWRed->orgCapsTableHWLibs->emulatedRev = callbackWRed->orgAsicCapsTable->emulatedRev = emulatedRevision;
    callbackWRed->orgCapsTableHWLibs->pciRev = callbackWRed->orgAsicCapsTable->pciRev = 0xFFFFFFFF;
    callbackWRed->orgCapsTableHWLibs->caps = callbackWRed->orgAsicCapsTable->caps = initCaps->caps;
    MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);

    return ret;
}

uint32_t WRed::hwLibsNoop() { return 0; }    // Always return success
IOReturn WRed::wrapPopulateVramInfo([[maybe_unused]] void *that, void *fwInfo) {
    getMember<uint32_t>(fwInfo, 0x1C) = 4;      // DDR4
    getMember<uint32_t>(fwInfo, 0x20) = 128;    // 128-bit
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
    FunctionCast(wrapSetupAndInitializeHWCapabilities, callbackWRed->orgSetupAndInitializeHWCapabilities)(that);
    if (callbackWRed->asicType < ASICType::Renoir) {
        getMember<uint32_t>(that, 0x2C) = 4;    // Surface Count
    }
    getMember<bool>(that, 0xC0) = false;    // SDMA Page Queue
    getMember<bool>(that, 0xAC) = false;    // VCE
    getMember<bool>(that, 0xAE) = false;    // VCE-related
}

/**
 * Hack: Add custom param 4 and 5 (pointer to firmware and size)
 * aka RCX and R8 registers
 * Complementary to `_psp_asd_load` patch-set.
 */
uint32_t WRed::wrapPspAsdLoad(void *pspData) {
    auto *filename = new char[128];
    snprintf(filename, 128, "%s_asd.bin", getASICName());
    DBGLOG("wred", "injecting %s!", filename);
    auto &fwDesc = getFWDescByName(filename);
    auto *fwHeader = reinterpret_cast<const GfxFwHeaderV1 *>(fwDesc.data);
    delete[] filename;
    auto *org = reinterpret_cast<t_pspLoadExtended>(callbackWRed->orgPspAsdLoad);
    auto ret = org(pspData, 0, 0, fwDesc.data + fwHeader->ucodeOff, fwHeader->ucodeSize);
    DBGLOG("wred", "_psp_asd_load returned 0x%X", ret);
    return ret;
}

/** Same idea as `_psp_asd_load`. */
uint32_t WRed::wrapPspDtmLoad(void *pspData) {
    auto *filename = new char[128];
    snprintf(filename, 128, "%s_ta.bin", getASICName());
    DBGLOG("wred", "Injecting %s <dtm>!", filename);
    auto &fwDesc = getFWDescByName(filename);
    auto *fwHeader = reinterpret_cast<const PspTaFwHeaderV1 *>(fwDesc.data);
    delete[] filename;
    auto *org = reinterpret_cast<t_pspLoadExtended>(callbackWRed->orgPspDtmLoad);
    auto ret = org(pspData, 0, 0, fwDesc.data + fwHeader->ucodeOff + fwHeader->dtm.off, fwHeader->dtm.size);
    DBGLOG("wred", "_psp_dtm_load returned 0x%X", ret);
    return ret;
}

/** Same idea as `_psp_asd_load`. */
uint32_t WRed::wrapPspHdcpLoad(void *pspData) {
    auto *filename = new char[128];
    snprintf(filename, 128, "%s_ta.bin", getASICName());
    DBGLOG("wred", "Injecting %s <hdcp>!", filename);
    auto &fwDesc = getFWDescByName(filename);
    auto *fwHeader = reinterpret_cast<const PspTaFwHeaderV1 *>(fwDesc.data);
    delete[] filename;
    auto *org = reinterpret_cast<t_pspLoadExtended>(callbackWRed->orgPspHdcpLoad);
    auto ret = org(pspData, 0, 0, fwDesc.data + fwHeader->ucodeOff + fwHeader->hdcp.off, fwHeader->hdcp.size);
    DBGLOG("wred", "_psp_hdcp_load returned 0x%X", ret);
    return ret;
}

void *WRed::wrapRTGetHWChannel(void *that, uint32_t param1, uint32_t param2, uint32_t param3) {
    if (param1 == 2 && param2 == 0 && param3 == 0) { param2 = 2; }    // Redirect SDMA1 retrieval to SDMA0
    return FunctionCast(wrapRTGetHWChannel, callbackWRed->orgRTGetHWChannel)(that, param1, param2, param3);
}

uint32_t WRed::wrapHwReadReg32(void *that, uint32_t reg) {
    if (!callbackWRed->fbOffset) {
        callbackWRed->fbOffset =
            static_cast<uint64_t>(FunctionCast(wrapHwReadReg32, callbackWRed->orgHwReadReg32)(that, 0x296B)) << 24;
    }
    return FunctionCast(wrapHwReadReg32, callbackWRed->orgHwReadReg32)(that, reg == 0xD31 ? 0xD2F : reg);
}

uint32_t WRed::wrapSmuRavenInitialize(void *smum, uint32_t param2) {
    auto ret = FunctionCast(wrapSmuRavenInitialize, callbackWRed->orgSmuRavenInitialize)(smum, param2);
    callbackWRed->orgRavenSendMsgToSmc(smum, PPSMC_MSG_PowerUpSdma);
    callbackWRed->orgRavenSendMsgToSmc(smum, PPSMC_MSG_PowerGateMmHub);
    return ret;
}

uint32_t WRed::wrapSmuRenoirInitialize(void *smum, uint32_t param2) {
    auto ret = FunctionCast(wrapSmuRenoirInitialize, callbackWRed->orgSmuRenoirInitialize)(smum, param2);
    callbackWRed->orgRenoirSendMsgToSmc(smum, PPSMC_MSG_PowerUpSdma);
    callbackWRed->orgRenoirSendMsgToSmc(smum, PPSMC_MSG_PowerGateMmHub);
    return ret;
}

void WRed::wrapInitializeFamilyType(void *that) { getMember<uint32_t>(that, 0x308) = AMDGPU_FAMILY_RV; }

void *WRed::wrapAllocateAMDHWDisplay(void *that) {
    return FunctionCast(wrapAllocateAMDHWDisplay, callbackWRed->orgAllocateAMDHWDisplayX6000)(that);
}

uint32_t WRed::wrapPspCmdKmSubmit(void *psp, void *ctx, void *param3, void *param4) {
    // Skip loading of CP MEC JT2 FW on Renoir devices due to it being unsupported
    // See also: https://github.com/torvalds/linux/commit/f8f70c1371d304f42d4a1242d8abcbda807d0bed
    if ((callbackWRed->asicType >= ASICType::Renoir) && getMember<uint>(ctx, 16) == 6) {
        DBGLOG("wred", "Skipping loading of fwType 6");
        return 0;
    }

    return FunctionCast(wrapPspCmdKmSubmit, callbackWRed->orgPspCmdKmSubmit)(psp, ctx, param3, param4);
}

bool WRed::wrapInitWithPciInfo(void *that, void *param1) {
    auto ret = FunctionCast(wrapInitWithPciInfo, callbackWRed->orgInitWithPciInfo)(that, param1);
    // Hack AMDRadeonX6000_AmdLogger to log everything
    getMember<uint64_t>(that, 0x28) = ~0ULL;
    getMember<uint32_t>(that, 0x30) = ~0U;
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
    if (callbackWRed->asicType < ASICType::Renoir) {
        DBGLOG("wred", "initDCNRegistersOffsets !! PATCHING REGISTERS FOR DCN 1.0 !!");
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
