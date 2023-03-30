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
    lilu.onKextLoadForce(&kextRadeonX5000HWLibs);
}

bool X5000HWLibs::processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
    if (kextRadeonX5000HWLibs.loadIndex == index) {
        CailAsicCapEntry *orgAsicCapsTable = nullptr;
        CailInitAsicCapEntry *orgAsicInitCapsTable = nullptr;
        const void *goldenSettings[static_cast<uint32_t>(ChipType::Unknown)] = {nullptr};
        const uint32_t *ddiCaps[static_cast<uint32_t>(ChipType::Unknown)] = {nullptr};
        CailDeviceTypeEntry *orgDeviceTypeTable = nullptr;

        KernelPatcher::SolveRequest solveRequests[] = {
            {"__ZL15deviceTypeTable", orgDeviceTypeTable},
            {"__ZN11AMDFirmware14createFirmwareEPhjjPKc", this->orgCreateFirmware},
            {"__ZN20AMDFirmwareDirectory11putFirmwareE16_AMD_DEVICE_TYPEP11AMDFirmware", this->orgPutFirmware},
            {"__ZN31AtiAppleVega10PowerTuneServicesC1EP11PP_InstanceP18PowerPlayCallbacks",
                this->orgVega10PowerTuneConstructor},
            {"__ZN31AtiAppleVega20PowerTuneServicesC1EP11PP_InstanceP18PowerPlayCallbacks",
                this->orgVega20PowerTuneConstructor},
            {"__ZL20CAIL_ASIC_CAPS_TABLE", orgAsicCapsTable},
            {"_CAILAsicCapsInitTable", orgAsicInitCapsTable},
            {"_Raven_SendMsgToSmc", this->orgRavenSendMsgToSmc},
            {"_Renoir_SendMsgToSmc", this->orgRenoirSendMsgToSmc},
            {"__ZN20AMDFirmwareDirectoryC1Ej", this->orgAMDFirmwareDirectoryConstructor},
            {"_CAIL_DDI_CAPS_RAVEN_A0", ddiCaps[static_cast<uint32_t>(ChipType::Raven)]},
            {"_CAIL_DDI_CAPS_RAVEN2_A0", ddiCaps[static_cast<uint32_t>(ChipType::Raven2)]},
            {"_CAIL_DDI_CAPS_PICASSO_A0", ddiCaps[static_cast<uint32_t>(ChipType::Picasso)]},
            {"_CAIL_DDI_CAPS_RENOIR_A0", ddiCaps[static_cast<uint32_t>(ChipType::Renoir)]},
            {"_RAVEN1_GoldenSettings_A0", goldenSettings[static_cast<uint32_t>(ChipType::Raven)]},
            {"_RAVEN2_GoldenSettings_A0", goldenSettings[static_cast<uint32_t>(ChipType::Raven2)]},
            {"_PICASSO_GoldenSettings_A0", goldenSettings[static_cast<uint32_t>(ChipType::Picasso)]},
            {"_RENOIR_GoldenSettings_A0", goldenSettings[static_cast<uint32_t>(ChipType::Renoir)]},
        };
        PANIC_COND(!patcher.solveMultiple(index, solveRequests, address, size), "hwlibs", "Failed to resolve symbols");

        KernelPatcher::RouteRequest requests[] = {
            {"__ZN35AMDRadeonX5000_AMDRadeonHWLibsX500025populateFirmwareDirectoryEv", wrapPopulateFirmwareDirectory},
            {"__ZN25AtiApplePowerTuneServices23createPowerTuneServicesEP11PP_InstanceP18PowerPlayCallbacks",
                wrapCreatePowerTuneServices},
            {"_gc_get_hw_version", wrapGcGetHwVersion},
            {"_smu_get_hw_version", wrapSmuGetHwVersion},
            {"_smu_get_fw_constants", hwLibsNoop},
            {"_smu_9_0_1_check_fw_status", hwLibsNoop},
            {"_smu_9_0_1_unload_smu", hwLibsNoop},
            {"_psp_sw_init", wrapPspSwInit, this->orgPspSwInit},
            {"_psp_bootloader_is_sos_running", hwLibsNoop},
            {"_psp_bootloader_load_sos", hwLibsNoop},
            {"_psp_bootloader_load_sysdrv_3_1", hwLibsNoop},
            {"_psp_xgmi_is_support", hwLibsUnsupported},
            {"_psp_rap_is_supported", hwLibsUnsupported},
            {"_psp_cmd_km_submit", wrapPspCmdKmSubmit, this->orgPspCmdKmSubmit},
            {"_SmuRaven_Initialize", wrapSmuRavenInitialize, this->orgSmuRavenInitialize},
            {"_SmuRenoir_Initialize", wrapSmuRenoirInitialize, this->orgSmuRenoirInitialize},
        };
        PANIC_COND(!patcher.routeMultiple(index, requests, address, size), "hwlibs", "Failed to route symbols");

        ddiCaps[static_cast<uint32_t>(ChipType::GreenSardine)] = ddiCaps[static_cast<uint32_t>(ChipType::Renoir)];
        goldenSettings[static_cast<uint32_t>(ChipType::GreenSardine)] =
            goldenSettings[static_cast<uint32_t>(ChipType::Renoir)];

        PANIC_COND(MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) != KERN_SUCCESS, "hwlibs",
            "Failed to enable kernel writing");
        *orgDeviceTypeTable = {.deviceId = NRed::callback->deviceId, .deviceType = 0};
        orgAsicInitCapsTable->familyId = orgAsicCapsTable->familyId = AMDGPU_FAMILY_RAVEN;
        orgAsicInitCapsTable->deviceId = orgAsicCapsTable->deviceId = NRed::callback->deviceId;
        orgAsicInitCapsTable->revision = orgAsicCapsTable->revision = NRed::callback->revision;
        orgAsicInitCapsTable->emulatedRev = orgAsicCapsTable->emulatedRev =
            static_cast<uint32_t>(NRed::callback->enumeratedRevision) + NRed::callback->revision;
        orgAsicInitCapsTable->pciRev = orgAsicCapsTable->pciRev = 0xFFFFFFFF;
        orgAsicInitCapsTable->caps = orgAsicCapsTable->caps = ddiCaps[static_cast<uint32_t>(NRed::callback->chipType)];
        orgAsicInitCapsTable->goldenCaps = goldenSettings[static_cast<uint32_t>(NRed::callback->chipType)];
        MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);
        DBGLOG("hwlibs", "Applied DDI Caps patches");

        KernelPatcher::LookupPatch patch = {&kextRadeonX5000HWLibs, kFullAsicResetPatched, kFullAsicResetOriginal,
            arrsize(kFullAsicResetPatched), 1};
        patcher.applyLookupPatch(&patch);
        patcher.clearError();

        return true;
    }

    return false;
}

uint32_t X5000HWLibs::wrapSmuGetHwVersion() { return 0x1; }

AMDReturn X5000HWLibs::wrapPspSwInit(uint32_t *inputData, void *outputData) {
    if (NRed::callback->chipType < ChipType::Renoir) {
        inputData[3] = 0x9;
        inputData[5] = 0x2;

    } else {
        inputData[3] = 0xB;
        inputData[5] = 0x0;
    }
    inputData[4] = 0x0;
    auto ret = FunctionCast(wrapPspSwInit, callback->orgPspSwInit)(inputData, outputData);
    DBGLOG("hwlibs", "_psp_sw_init >> 0x%X", ret);
    return ret;
}

uint32_t X5000HWLibs::wrapGcGetHwVersion() { return 0x090400; }

void X5000HWLibs::wrapPopulateFirmwareDirectory(void *that) {
    auto *fwDir = getMember<void *>(that, 0xB8) = IOMallocZero(0xD8);
    callback->orgAMDFirmwareDirectoryConstructor(fwDir, 3);

    auto *filename = NRed::callback->chipType >= ChipType::Renoir ? "ativvaxy_nv.dat" : "ativvaxy_rv.dat";
    auto &fwDesc = getFWDescByName(filename);
    auto *fw = callback->orgCreateFirmware(fwDesc.data, fwDesc.size, 0x200, filename);
    PANIC_COND(!fw, "hwlibs", "Failed to create '%s' firmware", filename);
    DBGLOG("hwlibs", "Inserting %s!", filename);
    PANIC_COND(!callback->orgPutFirmware(fwDir, 0, fw), "hwlibs", "Failed to inject %s firmware", filename);
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

AMDReturn X5000HWLibs::hwLibsNoop() { return kAMDReturnSuccess; }
AMDReturn X5000HWLibs::hwLibsUnsupported() { return kAMDReturnUnsupported; }

AMDReturn X5000HWLibs::wrapSmuRavenInitialize(void *smum, uint32_t param2) {
    auto ret = FunctionCast(wrapSmuRavenInitialize, callback->orgSmuRavenInitialize)(smum, param2);
    callback->orgRavenSendMsgToSmc(smum, PPSMC_MSG_PowerUpSdma);
    return ret;
}

AMDReturn X5000HWLibs::wrapSmuRenoirInitialize(void *smum, uint32_t param2) {
    auto ret = FunctionCast(wrapSmuRenoirInitialize, callback->orgSmuRenoirInitialize)(smum, param2);
    callback->orgRenoirSendMsgToSmc(smum, PPSMC_MSG_PowerUpSdma);
    return ret;
}

AMDReturn X5000HWLibs::wrapPspCmdKmSubmit(void *psp, void *ctx, void *param3, void *param4) {
    // Upstream patch: https://github.com/torvalds/linux/commit/f8f70c1371d304f42d4a1242d8abcbda807d0bed
    if (NRed::callback->chipType >= ChipType::Renoir) {
        static bool didMec1 = false;
        switch (getMember<uint32_t>(ctx, 16)) {
            case GFX_FW_TYPE_CP_MEC:
                if (!didMec1) {
                    didMec1 = true;
                    break;
                }
                DBGLOG("hwlibs", "Skipping MEC2 FW");
                return kAMDReturnSuccess;
            case GFX_FW_TYPE_CP_MEC_ME2:
                DBGLOG("hwlibs", "Skipping MEC2 JT FW");
                return kAMDReturnSuccess;
            default:
                break;
        }
    }

    return FunctionCast(wrapPspCmdKmSubmit, callback->orgPspCmdKmSubmit)(psp, ctx, param3, param4);
}
