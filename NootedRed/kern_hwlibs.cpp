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

        CailAsicCapEntry *orgCapsTbl = nullptr;
        CailDeviceTypeEntry *orgDeviceTypeTable = nullptr;

        KernelPatcher::SolveRequest solveRequests[] = {
            {"__ZL15deviceTypeTable", orgDeviceTypeTable},
            {"__ZN11AMDFirmware14createFirmwareEPhjjPKc", this->orgCreateFirmware},
            {"__ZN20AMDFirmwareDirectory11putFirmwareE16_AMD_DEVICE_TYPEP11AMDFirmware", this->orgPutFirmware},
            {"__ZL20CAIL_ASIC_CAPS_TABLE", orgCapsTbl},
            {"__ZN20AMDFirmwareDirectoryC1Ej", this->orgAMDFirmwareDirectoryConstructor},
        };
        PANIC_COND(!patcher.solveMultiple(index, solveRequests, address, size), "hwlibs", "Failed to resolve symbols");

        KernelPatcher::RouteRequest requests[] = {
            {"__ZN35AMDRadeonX5000_AMDRadeonHWLibsX500025populateFirmwareDirectoryEv", wrapPopulateFirmwareDirectory},
            {"_gc_get_hw_version", wrapGcGetHwVersion},
            {"_smu_get_hw_version", wrapSmuGetHwVersion},
            {"_smu_get_fw_constants", hwLibsNoop},
            {"_smu_9_0_1_check_fw_status", hwLibsNoop},
            {"_smu_9_0_1_unload_smu", hwLibsNoop},
            {"_psp_sw_init", wrapPspSwInit, this->orgPspSwInit},
            {"_psp_cmd_km_submit", wrapPspCmdKmSubmit, this->orgPspCmdKmSubmit},
            {NRed::callback->chipType < ChipType::Renoir ? "_SmuRaven_Initialize" : "_SmuRenoir_Initialize",
                wrapSmuInitialize, this->orgSmuInitialize},
        };
        PANIC_COND(!patcher.routeMultiple(index, requests, address, size), "hwlibs", "Failed to route symbols");

        PANIC_COND(MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) != KERN_SUCCESS, "hwlibs",
            "Failed to enable kernel writing");
        *orgDeviceTypeTable = {
            .deviceId = NRed::callback->deviceId,
            .deviceType = 6,
        };
        *orgCapsTbl = {
            .familyId = AMDGPU_FAMILY_RAVEN,
            .deviceId = NRed::callback->deviceId,
            .revision = NRed::callback->revision,
            .extRevision = NRed::callback->extRevision,
            .pciRevision = NRed::callback->pciRevision,
            .caps = NRed::callback->chipType < ChipType::Renoir ? ddiCapsRaven : ddiCapsRenoir,
        };
        MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);
        DBGLOG("hwlibs", "Applied DDI Caps patches");

        KernelPatcher::LookupPatch patches[] = {
            {&kextRadeonX5000HWLibs, kFullAsicResetOriginal, kFullAsicResetPatched, arrsize(kFullAsicResetOriginal), 1},
            {&kextRadeonX5000HWLibs, kCreatePowerTuneServicesOriginal1, kCreatePowerTuneServicesPatched1,
                arrsize(kCreatePowerTuneServicesOriginal1), 1},
        };
        for (auto &patch : patches) {
            patcher.applyLookupPatch(&patch);
            patcher.clearError();
        }
        PANIC_COND(!KernelPatcher::findAndReplaceWithMask(reinterpret_cast<uint8_t *>(address), size,
                       kCreatePowerTuneServicesOriginal2, arrsize(kCreatePowerTuneServicesOriginal2),
                       kCreatePowerTuneServicesMask2, arrsize(kCreatePowerTuneServicesMask2),
                       kCreatePowerTuneServicesPatched2, arrsize(kCreatePowerTuneServicesPatched2), nullptr, 0, 1, 0),
            "hwlibs", "Failed to apply PowerTune services patch part 2");

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

AMDReturn X5000HWLibs::hwLibsNoop() { return kAMDReturnSuccess; }

AMDReturn X5000HWLibs::wrapSmuInitialize(void *smum, uint32_t param2) {
    auto ret = FunctionCast(wrapSmuInitialize, callback->orgSmuInitialize)(smum, param2);
    NRed::callback->sendMsgToSmc(PPSMC_MSG_PowerUpSdma);
    return ret;
}

AMDReturn X5000HWLibs::wrapPspCmdKmSubmit(void *psp, void *ctx, void *param3, void *param4) {
    // Upstream patch: https://github.com/torvalds/linux/commit/f8f70c1371d304f42d4a1242d8abcbda807d0bed
    if (NRed::callback->chipType >= ChipType::Renoir && getMember<uint32_t>(ctx, 0x10) == 6) {
        DBGLOG("hwlibs", "Skipping MEC2 JT FW");
        return kAMDReturnSuccess;
    }

    return FunctionCast(wrapPspCmdKmSubmit, callback->orgPspCmdKmSubmit)(psp, ctx, param3, param4);
}
