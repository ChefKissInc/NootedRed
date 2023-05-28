//  Copyright © 2022-2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.0. See LICENSE for
//  details.

#include "kern_hwlibs.hpp"
#include "kern_nred.hpp"
#include "kern_patcherplus.hpp"
#include "kern_patches.hpp"
#include "kern_patterns.hpp"
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

        SolveWithFallbackRequest solveRequests[] = {
            {"__ZL15deviceTypeTable", orgDeviceTypeTable, kDeviceTypeTablePattern},
            {"__ZN11AMDFirmware14createFirmwareEPhjjPKc", this->orgCreateFirmware, kCreateFirmwarePattern},
            {"__ZN20AMDFirmwareDirectory11putFirmwareE16_AMD_DEVICE_TYPEP11AMDFirmware", this->orgPutFirmware,
                kPutFirmwarePattern},
            {"__ZL20CAIL_ASIC_CAPS_TABLE", orgCapsTbl, kCailAsicCapsTableHWLibsPattern},
        };
        PANIC_COND(!SolveWithFallbackRequest::solveAll(patcher, index, solveRequests, address, size), "hwlibs",
            "Failed to resolve symbols");

        RouteWithFallbackRequest requests[] = {
            {"__ZN35AMDRadeonX5000_AMDRadeonHWLibsX500025populateFirmwareDirectoryEv", wrapPopulateFirmwareDirectory,
                this->orgPopulateFirmwareDirectory},
            {"_smu_get_hw_version", wrapSmuGetHwVersion, kSmuGetHwVersionPattern},
            {"_smu_get_fw_constants", hwLibsNoop, kSmuGetFwConstantsPattern, kSmuGetFwConstantsMask},
            {"_smu_9_0_1_check_fw_status", hwLibsNoop, kSmu901CheckFwStatusPattern, kSmu901CheckFwStatusMask},
            {"_smu_9_0_1_unload_smu", hwLibsNoop, kSmu901UnloadSmuPattern, kSmu901UnloadSmuMask},
            {"_psp_cmd_km_submit", wrapPspCmdKmSubmit, this->orgPspCmdKmSubmit, kPspCmdKmSubmitPattern,
                kPspCmdKmSubmitMask},
            {"_update_sdma_power_gating", wrapUpdateSdmaPowerGating, this->orgUpdateSdmaPowerGating,
                kUpdateSdmaPowerGatingPattern, kUpdateSdmaPowerGatingMask},
        };
        PANIC_COND(!RouteWithFallbackRequest::routeAll(patcher, index, requests, address, size), "hwlibs",
            "Failed to route symbols");

        PANIC_COND(MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) != KERN_SUCCESS, "hwlibs",
            "Failed to enable kernel writing");
        *orgDeviceTypeTable = {.deviceId = NRed::callback->deviceId, .deviceType = 6};
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

        auto adjustment = getKernelVersion() > KernelVersion::BigSur;
        KernelPatcher::LookupPatch patches[] = {
            {&kextRadeonX5000HWLibs, kPspSwInitOriginal1, kPspSwInitPatched1, arrsize(kPspSwInitOriginal1), 1},
            {&kextRadeonX5000HWLibs, kFullAsicResetOriginal, kFullAsicResetPatched, arrsize(kFullAsicResetOriginal), 1},
            {&kextRadeonX5000HWLibs,
                adjustment ? kCreatePowerTuneServicesOriginal1Monterey : kCreatePowerTuneServicesOriginal1,
                adjustment ? kCreatePowerTuneServicesPatched1Monterey : kCreatePowerTuneServicesPatched1,
                adjustment ? arrsize(kCreatePowerTuneServicesOriginal1Monterey) :
                             arrsize(kCreatePowerTuneServicesOriginal1),
                1},
        };
        for (size_t i = 0; i < arrsize(patches); i++) {
            patcher.applyLookupPatch(patches + i);
            SYSLOG_COND(patcher.getError() != KernelPatcher::Error::NoError, "hwlibs", "Failed to apply patches[%zu]",
                i);
            patcher.clearError();
        }

        PANIC_COND(!KernelPatcher::findAndReplaceWithMask(reinterpret_cast<uint8_t *>(address), size,
                       kCreatePowerTuneServicesOriginal2, arrsize(kCreatePowerTuneServicesOriginal2),
                       kCreatePowerTuneServicesMask2, arrsize(kCreatePowerTuneServicesMask2),
                       kCreatePowerTuneServicesPatched2, arrsize(kCreatePowerTuneServicesPatched2), nullptr, 0, 1, 0),
            "hwlibs", "Failed to apply PowerTune services patch part 2");

        PANIC_COND(!KernelPatcher::findAndReplaceWithMask(reinterpret_cast<uint8_t *>(address), size, kGcSwInitOriginal,
                       kGcSwInitOriginalMask, kGcSwInitPatched, kGcSwInitPatchedMask, 1, 0),
            "hwlibs", "Failed to apply _gc_sw_init version spoof patch");

        PANIC_COND(!KernelPatcher::findAndReplaceWithMask(reinterpret_cast<uint8_t *>(address), size,
                       kGcSetFwEntryInfoOriginal, arrsize(kGcSetFwEntryInfoOriginal), kGcSetFwEntryInfoMask,
                       arrsize(kGcSetFwEntryInfoMask), kGcSetFwEntryInfoPatched, arrsize(kGcSetFwEntryInfoPatched),
                       nullptr, 0, 1, 0),
            "hwlibs", "Failed to apply _gc_set_fw_entry_info version spoof patch");

        PANIC_COND(!KernelPatcher::findAndReplaceWithMask(reinterpret_cast<uint8_t *>(address), size,
                       kPspSwInitOriginal2, arrsize(kPspSwInitOriginal2), kPspSwInitMask2, arrsize(kPspSwInitMask2),
                       kPspSwInitPatched2, arrsize(kPspSwInitPatched2), nullptr, 0, 1, 0),
            "hwlibs", "Failed to apply _psp_sw_init patch part 2");

        return true;
    }

    return false;
}

uint32_t X5000HWLibs::wrapSmuGetHwVersion() { return 0x1; }

void X5000HWLibs::wrapPopulateFirmwareDirectory(void *that) {
    FunctionCast(wrapPopulateFirmwareDirectory, callback->orgPopulateFirmwareDirectory)(that);

    auto isRenoirDerivative = NRed::callback->chipType >= ChipType::Renoir;
    auto *filename = isRenoirDerivative ? "ativvaxy_nv.dat" : "ativvaxy_rv.dat";
    auto &fwDesc = getFWDescByName(filename);
    /** VCN 2.2, VCN 1.0 */
    auto *fw = callback->orgCreateFirmware(fwDesc.data, fwDesc.size, isRenoirDerivative ? 0x0202 : 0x0100, filename);
    PANIC_COND(!fw, "hwlibs", "Failed to create '%s' firmware", filename);
    DBGLOG("hwlibs", "Inserting %s!", filename);
    auto *fwDir = getMember<void *>(that, getKernelVersion() > KernelVersion::BigSur ? 0xB0 : 0xB8);
    PANIC_COND(!fwDir, "hwlibs", "Failed to get firmware directory");
    PANIC_COND(!callback->orgPutFirmware(fwDir, 6, fw), "hwlibs", "Failed to inject %s firmware", filename);
}

AMDReturn X5000HWLibs::hwLibsNoop() { return kAMDReturnSuccess; }

void X5000HWLibs::wrapUpdateSdmaPowerGating(void *cail, uint32_t mode) {
    if (mode == 0 || mode == 3) { NRed::callback->sendMsgToSmc(PPSMC_MSG_PowerUpSdma); }
    FunctionCast(wrapUpdateSdmaPowerGating, callback->orgUpdateSdmaPowerGating)(cail, mode);
}

AMDReturn X5000HWLibs::wrapPspCmdKmSubmit(void *psp, void *ctx, void *param3, void *param4) {
    // Upstream patch: https://github.com/torvalds/linux/commit/f8f70c1371d304f42d4a1242d8abcbda807d0bed
    if (NRed::callback->chipType >= ChipType::Renoir && getMember<uint32_t>(ctx, 0x10) == 6) {
        DBGLOG("hwlibs", "Skipping MEC2 JT FW");
        return kAMDReturnSuccess;
    }

    return FunctionCast(wrapPspCmdKmSubmit, callback->orgPspCmdKmSubmit)(psp, ctx, param3, param4);
}
