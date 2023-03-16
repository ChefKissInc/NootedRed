//  Copyright © 2022-2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.0. See LICENSE for
//  details.

#include "kern_hwlibs.hpp"
#include "kern_nred.hpp"
#include "kern_patches.hpp"
#include <Headers/kern_api.hpp>

static const char *pathRadeonX5000HWLibs = "/System/Library/Extensions/AMDRadeonX5000HWServices.kext/Contents/PlugIns/"
                                           "AMDRadeonX5000HWLibs.kext/Contents/MacOS/AMDRadeonX5000HWLibs";

static KernelPatcher::KextInfo kextRadeonX5000HWLibs {"com.apple.kext.AMDRadeonX5000HWLibs", &pathRadeonX5000HWLibs, 1,
    {}, {}, KernelPatcher::KextInfo::Unloaded};

X5000HWLibs *X5000HWLibs::callback = nullptr;

void X5000HWLibs::init() {
    callback = this;
    lilu.onKextLoad(&kextRadeonX5000HWLibs);
    DBGLOG("hwlibs", "Initialised");
}

bool X5000HWLibs::processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
    if (kextRadeonX5000HWLibs.loadIndex == index) {
        CailAsicCapEntry *orgAsicCapsTable = nullptr;
        CailInitAsicCapEntry *orgAsicInitCapsTable = nullptr;
        uint32_t *ddiCapsRaven = nullptr, *ddiCapsRaven2 = nullptr;
        uint32_t *ddiCapsPicasso = nullptr, *ddiCapsRenoir = nullptr;
        void *goldenSettingsRaven = nullptr, *goldenSettingsRaven2 = nullptr;
        void *goldenSettingsPicasso = nullptr, *goldenSettingsRenoir = nullptr;
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
        PANIC_COND(!patcher.solveMultiple(index, solveRequests, address, size), "hwlibs", "Failed to resolve symbols");

        PANIC_COND(MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) != KERN_SUCCESS, "hwlibs",
            "Failed to enable kernel writing");
        orgDeviceTypeTable[0] = NRed::callback->deviceId;
        orgDeviceTypeTable[1] = 0;
        orgAsicInitCapsTable->familyId = orgAsicCapsTable->familyId = AMDGPU_FAMILY_RV;
        orgAsicInitCapsTable->deviceId = orgAsicCapsTable->deviceId = NRed::callback->deviceId;
        orgAsicInitCapsTable->revision = orgAsicCapsTable->revision = NRed::callback->revision;
        orgAsicInitCapsTable->emulatedRev = orgAsicCapsTable->emulatedRev =
            NRed::callback->enumeratedRevision + NRed::callback->revision;
        orgAsicInitCapsTable->pciRev = orgAsicCapsTable->pciRev = 0xFFFFFFFF;
        switch (NRed::callback->chipType) {
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
                MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);
                PANIC("hwlibs", "Unknown ASIC type %d", NRed::callback->chipType);
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
        PANIC_COND(!patcher.routeMultiple(index, requests, address, size), "hwlibs", "Failed to route symbols");

        KernelPatcher::LookupPatch patch = {&kextRadeonX5000HWLibs, kFullAsicResetPatched, kFullAsicResetOriginal,
            arrsize(kFullAsicResetPatched), 1};
        patcher.applyLookupPatch(&patch);
        patcher.clearError();

        return true;
    }

    return false;
}

uint32_t X5000HWLibs::wrapSmuGetHwVersion() { return 0x1; }

uint32_t X5000HWLibs::wrapPspSwInit(uint32_t *inputData, void *outputData) {
    if (NRed::callback->chipType < ChipType::Renoir) {
        inputData[3] = 0x9;
        inputData[4] = 0x0;
        inputData[5] = 0x2;

    } else {
        inputData[3] = 0xB;
        inputData[4] = 0x0;
        inputData[5] = 0x0;
    }
    auto ret = FunctionCast(wrapPspSwInit, callback->orgPspSwInit)(inputData, outputData);
    DBGLOG("hwlibs", "_psp_sw_init >> 0x%X", ret);
    return ret;
}

uint32_t X5000HWLibs::wrapGcGetHwVersion() { return 0x090400; }

void X5000HWLibs::wrapPopulateFirmwareDirectory(void *that) {
    auto *fwDir = IOMallocZero(0xD8);
    callback->orgAMDFirmwareDirectoryConstructor(fwDir, 3);
    getMember<void *>(that, 0xB8) = fwDir;

    auto *chipName = NRed::getChipName();
    char filename[128];
    snprintf(filename, 128, "%s_vcn.bin", chipName);
    auto *targetFilename = NRed::callback->chipType >= ChipType::Renoir ? "ativvaxy_nv.dat" : "ativvaxy_rv.dat";
    DBGLOG("hwlibs", "%s => %s", filename, targetFilename);

    auto &fwDesc = getFWDescByName(filename);
    auto *fwHeader = reinterpret_cast<const CommonFirmwareHeader *>(fwDesc.data);
    auto *fw =
        callback->orgCreateFirmware(fwDesc.data + fwHeader->ucodeOff, fwHeader->ucodeSize, 0x200, targetFilename);
    PANIC_COND(!fw, "hwlibs", "Failed to create '%s' firmware", targetFilename);
    DBGLOG("hwlibs", "Inserting %s!", targetFilename);
    PANIC_COND(!callback->orgPutFirmware(fwDir, 0, fw), "hwlibs", "Failed to inject ativvaxy_rv.dat firmware");

    if (NRed::callback->chipType >= ChipType::Renoir) {
        snprintf(filename, 128, "%s_dmcub.bin", chipName);
        DBGLOG("hwlibs", "%s => atidmcub_0.dat", filename);
        auto &fwDesc = getFWDescByName(filename);
        auto *fwHeader = reinterpret_cast<const CommonFirmwareHeader *>(fwDesc.data);
        fw =
            callback->orgCreateFirmware(fwDesc.data + fwHeader->ucodeOff, fwHeader->ucodeSize, 0x200, "atidmcub_0.dat");
        PANIC_COND(!fw, "hwlibs", "Failed to create atidmcub_0.dat firmware");
        DBGLOG("hwlibs", "Inserting atidmcub_0.dat!");
        PANIC_COND(!callback->orgPutFirmware(fwDir, 0, fw), "hwlibs", "Failed to inject atidmcub_0.dat firmware");
    }
}

void *X5000HWLibs::wrapCreatePowerTuneServices(void *that, void *param2) {
    auto *ret = IOMallocZero(0x18);
    if (NRed::callback->chipType >= ChipType::Renoir) {
        callback->orgVega20PowerTuneConstructor(ret, that, param2);
    } else {
        callback->orgVega10PowerTuneConstructor(ret, that, param2);
    }
    return ret;
}

uint32_t X5000HWLibs::hwLibsNoop() { return 0; }           // Always return success
uint32_t X5000HWLibs::hwLibsUnsupported() { return 4; }    // Always return unsupported

uint32_t X5000HWLibs::wrapSmuRavenInitialize(void *smum, uint32_t param2) {
    auto ret = FunctionCast(wrapSmuRavenInitialize, callback->orgSmuRavenInitialize)(smum, param2);
    callback->orgRavenSendMsgToSmc(smum, PPSMC_MSG_PowerUpSdma);
    return ret;
}

uint32_t X5000HWLibs::wrapSmuRenoirInitialize(void *smum, uint32_t param2) {
    auto ret = FunctionCast(wrapSmuRenoirInitialize, callback->orgSmuRenoirInitialize)(smum, param2);
    callback->orgRenoirSendMsgToSmc(smum, PPSMC_MSG_PowerUpSdma);
    return ret;
}

uint32_t X5000HWLibs::wrapPspCmdKmSubmit(void *psp, void *ctx, void *param3, void *param4) {
    // Skip loading of MEC2 FW on Renoir devices due to it being unsupported
    // See also: https://github.com/torvalds/linux/commit/f8f70c1371d304f42d4a1242d8abcbda807d0bed
    if (NRed::callback->chipType >= ChipType::Renoir) {
        static bool didMec1 = false;
        switch (getMember<uint32_t>(ctx, 16)) {
            case GFX_FW_TYPE_CP_MEC:
                if (!didMec1) {
                    didMec1 = true;
                    break;
                }
                DBGLOG("hwlibs", "Skipping MEC2 FW");
                return 0;
            case GFX_FW_TYPE_CP_MEC_ME2:
                DBGLOG("hwlibs", "Skipping MEC2 JT FW");
                return 0;
            default:
                break;
        }
    }

    return FunctionCast(wrapPspCmdKmSubmit, callback->orgPspCmdKmSubmit)(psp, ctx, param3, param4);
}
