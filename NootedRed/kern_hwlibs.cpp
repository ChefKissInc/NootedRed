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
        NRed::callback->setRMMIOIfNecessary();

        CailAsicCapEntry *orgAsicCapsTable = nullptr;
        CailInitAsicCapEntry *orgAsicInitCapsTable = nullptr;
        const void *goldenSettings[static_cast<uint32_t>(ChipType::Unknown) - 1] = {nullptr};
        CailDeviceTypeEntry *orgDeviceTypeTable = nullptr;
        uint8_t *orgSmuFullAsicReset = nullptr;
        DeviceCapabilityEntry *deviceCapabilityTbl = nullptr;
        const void *swipInfo[3] = {nullptr}, *goldenRegisterSettings[3] = {nullptr};
        const void *swipInfoMinimal = nullptr, *devDoorbellRangeNotSupported = nullptr;

        KernelPatcher::SolveRequest solveRequests[] = {
            {"__ZL15deviceTypeTable", orgDeviceTypeTable},
            {"__ZN11AMDFirmware14createFirmwareEPhjjPKc", this->orgCreateFirmware},
            {"__ZN20AMDFirmwareDirectory11putFirmwareE16_AMD_DEVICE_TYPEP11AMDFirmware", this->orgPutFirmware},
            {"__ZN31AtiAppleVega10PowerTuneServicesC1EP11PP_InstanceP18PowerPlayCallbacks",
                this->orgVega10PowerTuneConstructor},
            {"__ZL20CAIL_ASIC_CAPS_TABLE", orgAsicCapsTable},
            {"_CAILAsicCapsInitTable", orgAsicInitCapsTable},
            {"__ZN20AMDFirmwareDirectoryC1Ej", this->orgAMDFirmwareDirectoryConstructor},
            {"_RAVEN1_GoldenSettings_A0", goldenSettings[static_cast<uint32_t>(ChipType::Raven)]},
            {"_RAVEN2_GoldenSettings_A0", goldenSettings[static_cast<uint32_t>(ChipType::Raven2)]},
            {"_PICASSO_GoldenSettings_A0", goldenSettings[static_cast<uint32_t>(ChipType::Picasso)]},
            {"_RENOIR_GoldenSettings_A0", goldenSettings[static_cast<uint32_t>(ChipType::Renoir)]},
            {"_smu_9_0_1_full_asic_reset", orgSmuFullAsicReset},
            {"_DeviceCapabilityTbl", deviceCapabilityTbl},
            {"_swipInfoRaven", swipInfo[0]},
            {"_swipInfoRaven2", swipInfo[1]},
            {"_swipInfoRenoir", swipInfo[2]},
            {"_swipInfoMinimalInit", swipInfoMinimal},
            {"_devDoorbellRangeNotSupported", devDoorbellRangeNotSupported},
            {"_ravenGoldenRegisterSettings", goldenRegisterSettings[0]},
            {"_raven2GoldenRegisterSettings", goldenRegisterSettings[1]},
            {"_renoirGoldenRegisterSettings", goldenRegisterSettings[2]},
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
            {"_psp_cmd_km_submit", wrapPspCmdKmSubmit, this->orgPspCmdKmSubmit},
            {"_psp_cos_wait_for", wrapPspCosWaitFor, orgPspCosWaitFor},
            {"_update_sdma_power_gating", wrapUpdateSdmaPowerGating, orgUpdateSdmaPowerGating},
        };
        auto count = arrsize(requests);
        PANIC_COND(!patcher.routeMultiple(index, requests, count, address, size), "hwlibs", "Failed to route symbols");

        PANIC_COND(MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) != KERN_SUCCESS, "hwlibs",
            "Failed to enable kernel writing");
        *orgDeviceTypeTable = {.deviceId = NRed::callback->deviceId, .deviceType = 6};
        orgAsicInitCapsTable->familyId = orgAsicCapsTable->familyId = AMDGPU_FAMILY_RAVEN;
        orgAsicInitCapsTable->deviceId = orgAsicCapsTable->deviceId = NRed::callback->deviceId;
        orgAsicInitCapsTable->revision = orgAsicCapsTable->revision = NRed::callback->revision;
        orgAsicInitCapsTable->emulatedRev = orgAsicCapsTable->emulatedRev =
            static_cast<uint32_t>(NRed::callback->enumeratedRevision) + NRed::callback->revision;
        orgAsicInitCapsTable->pciRev = orgAsicCapsTable->pciRev = NRed::callback->pciRevision;
        auto isRavenDerivative = NRed::callback->chipType < ChipType::Renoir;
        orgAsicInitCapsTable->caps = orgAsicCapsTable->caps = isRavenDerivative ? ddiCapsRaven : ddiCapsRenoir;
        orgAsicInitCapsTable->goldenCaps =
            goldenSettings[static_cast<uint32_t>(isRavenDerivative ? NRed::callback->chipType : ChipType::Renoir)];

        deviceCapabilityTbl->familyId = AMDGPU_FAMILY_RAVEN;
        deviceCapabilityTbl->deviceId = NRed::callback->deviceId;
        deviceCapabilityTbl->internalRevision = NRed::callback->revision;
        deviceCapabilityTbl->externalRevision = orgAsicInitCapsTable->emulatedRev;
        auto capTblIndex = NRed::callback->chipType < ChipType::Raven2 ? 0 :
                           NRed::callback->chipType < ChipType::Renoir ? 1 :
                                                                         2;
        deviceCapabilityTbl->swipInfo = swipInfo[capTblIndex];
        deviceCapabilityTbl->swipInfoMinimal = swipInfoMinimal;
        deviceCapabilityTbl->devAttrFlags = &ravenDevAttrFlags;
        deviceCapabilityTbl->goldenRegisterSetings = goldenRegisterSettings[capTblIndex];
        deviceCapabilityTbl->doorbellRange = devDoorbellRangeNotSupported;
        MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);
        DBGLOG("hwlibs", "Applied DDI Caps patches");

        PANIC_COND(!isRavenDerivative && !KernelPatcher::findAndReplace(orgSmuFullAsicReset, PAGE_SIZE,
                                             kFullAsicResetOriginal, kFullAsicResetPatched),
            "hwlibs", "Failed to patch _smu_9_0_1_full_asic_reset");

        return true;
    }

    return false;
}

uint32_t X5000HWLibs::wrapSmuGetHwVersion() { return 0x1; }

AMDReturn X5000HWLibs::wrapPspSwInit(uint32_t *inputData, void *outputData) {
    if (inputData[3] >= 0xA) {
        inputData[3] = 0xB;
        inputData[4] = inputData[5] = 0x0;
    }
    auto ret = FunctionCast(wrapPspSwInit, callback->orgPspSwInit)(inputData, outputData);
    DBGLOG("hwlibs", "_psp_sw_init >> 0x%X", ret);
    return ret;
}

uint32_t X5000HWLibs::wrapGcGetHwVersion() { return 0x090400; }

void X5000HWLibs::wrapPopulateFirmwareDirectory(void *that) {
    auto *fwDir = getMember<void *>(that, 0xB8) = IOMallocZero(0xD8);
    callback->orgAMDFirmwareDirectoryConstructor(fwDir, 3);

    auto isRenoirDerivative = NRed::callback->chipType >= ChipType::Renoir;
    auto *filename = isRenoirDerivative ? "ativvaxy_nv.dat" : "ativvaxy_rv.dat";
    auto &fwDesc = getFWDescByName(filename);
    /** VCN 2.2, VCN 1.0 */
    auto *fw = callback->orgCreateFirmware(fwDesc.data, fwDesc.size, isRenoirDerivative ? 0x0202 : 0x0100, filename);
    PANIC_COND(!fw, "hwlibs", "Failed to create '%s' firmware", filename);
    DBGLOG("hwlibs", "Inserting %s!", filename);
    PANIC_COND(!callback->orgPutFirmware(fwDir, 6, fw), "hwlibs", "Failed to inject %s firmware", filename);
}

void *X5000HWLibs::wrapCreatePowerTuneServices(void *that, void *param2) {
    auto *ret = IOMallocZero(0x18);
    callback->orgVega10PowerTuneConstructor(ret, that, param2);
    return ret;
}

AMDReturn X5000HWLibs::hwLibsNoop() { return kAMDReturnSuccess; }

AMDReturn X5000HWLibs::wrapPspCmdKmSubmit(void *psp, void *ctx, void *param3, void *param4) {
    // Upstream patch: https://github.com/torvalds/linux/commit/f8f70c1371d304f42d4a1242d8abcbda807d0bed
    if (NRed::callback->chipType >= ChipType::Renoir && getMember<uint32_t>(ctx, 0x10) == 6) {
        DBGLOG("hwlibs", "Skipping MEC2 JT FW");
        return kAMDReturnSuccess;
    }

    return FunctionCast(wrapPspCmdKmSubmit, callback->orgPspCmdKmSubmit)(psp, ctx, param3, param4);
}

AMDReturn X5000HWLibs::wrapPspCosWaitFor(void *cos, uint64_t param2, uint64_t param3, uint64_t param4) {
    IOSleep(20);    // There might be a handshake issue with the hardware, requiring delay
    return FunctionCast(wrapPspCosWaitFor, callback->orgPspCosWaitFor)(cos, param2, param3, param4);
}

void X5000HWLibs::wrapUpdateSdmaPowerGating(void * param1, uint32_t mode) {
    DBGLOG("hwlibs", "_update_sdma_power_gating << (param1: %p mode: 0x%X)", param1, mode);
    if (mode == 0 || mode == 3) {
        NRed::callback->sendMsgToSmc(PPSMC_MSG_PowerUpSdma);
    } else if (mode == 2) {
        NRed::callback->sendMsgToSmc(PPSMC_MSG_PowerDownSdma);
    }

    FunctionCast(wrapUpdateSdmaPowerGating, callback->orgUpdateSdmaPowerGating)(param1, mode);
}
