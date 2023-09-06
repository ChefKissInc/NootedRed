//  Copyright © 2022-2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5. See LICENSE for
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

bool X5000HWLibs::processKext(KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size) {
    if (kextRadeonX5000HWLibs.loadIndex == id) {
        NRed::callback->setRMMIOIfNecessary();

        CAILAsicCapsEntry *orgCapsTable = nullptr;
        CAILAsicCapsInitEntry *orgCapsInitTable = nullptr;
        CAILDeviceTypeEntry *orgDeviceTypeTable = nullptr;
        DeviceCapabilityEntry *orgDevCapTable = nullptr;

        auto catalina = getKernelVersion() == KernelVersion::Catalina;
        SolveRequestPlus solveRequests[] = {
            {"__ZL15deviceTypeTable", orgDeviceTypeTable, kDeviceTypeTablePattern, !catalina},
            {"__ZN11AMDFirmware14createFirmwareEPhjjPKc", this->orgCreateFirmware, kCreateFirmwarePattern, !catalina},
            {"__ZN20AMDFirmwareDirectory11putFirmwareE16_AMD_DEVICE_TYPEP11AMDFirmware", this->orgPutFirmware,
                kPutFirmwarePattern, !catalina},
            {"__ZL20CAIL_ASIC_CAPS_TABLE", orgCapsTable, kCailAsicCapsTableHWLibsPattern},
            {"_CAILAsicCapsInitTable", orgCapsInitTable, kCAILAsicCapsInitTablePattern},
            {"_DeviceCapabilityTbl", orgDevCapTable, kDeviceCapabilityTblPattern},
        };
        PANIC_COND(!SolveRequestPlus::solveAll(patcher, id, solveRequests, slide, size), "hwlibs",
            "Failed to resolve symbols");

        bool renoir = NRed::callback->chipType >= ChipType::Renoir;
        RouteRequestPlus requests[] = {
            {"__ZN35AMDRadeonX5000_AMDRadeonHWLibsX500025populateFirmwareDirectoryEv", wrapPopulateFirmwareDirectory,
                this->orgPopulateFirmwareDirectory, !catalina},
            {catalina ? "_smu_get_external_fw" : "_smu_get_fw_constants", hwLibsNoop, kSmuGetFwConstantsPattern,
                kSmuGetFwConstantsMask},
            {"_smu_9_0_1_check_fw_status", hwLibsNoop, kSmu901CheckFwStatusPattern, kSmu901CheckFwStatusMask},
            {"_smu_9_0_1_unload_smu", hwLibsNoop, kSmu901UnloadSmuPattern, kSmu901UnloadSmuMask},
            {"_psp_cmd_km_submit", wrapPspCmdKmSubmit, this->orgPspCmdKmSubmit, kPspCmdKmSubmitPattern,
                kPspCmdKmSubmitMask},
            {"_update_sdma_power_gating", wrapUpdateSdmaPowerGating, this->orgUpdateSdmaPowerGating,
                kUpdateSdmaPowerGatingPattern, kUpdateSdmaPowerGatingMask},
            {"__ZN16AmdTtlFwServices7getIpFwEjPKcP10_TtlFwInfo", wrapGetIpFw, this->orgGetIpFw, catalina},
        };
        PANIC_COND(!RouteRequestPlus::routeAll(patcher, id, requests, slide, size), "hwlibs",
            "Failed to route symbols");

        PANIC_COND(MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) != KERN_SUCCESS, "hwlibs",
            "Failed to enable kernel writing");
        if (!catalina) { *orgDeviceTypeTable = {.deviceId = NRed::callback->deviceId, .deviceType = 6}; }
        auto found = false;
        auto targetDeviceId = renoir && NRed::callback->deviceId != 0x1636 ? 0x1636 : NRed::callback->deviceId;
        while (orgCapsInitTable->deviceId != 0xFFFFFFFF) {
            if (orgCapsInitTable->familyId == AMDGPU_FAMILY_RAVEN && orgCapsInitTable->deviceId == targetDeviceId) {
                orgCapsInitTable->deviceId = NRed::callback->deviceId;
                orgCapsInitTable->revision = NRed::callback->revision;
                orgCapsInitTable->extRevision =
                    static_cast<uint64_t>(NRed::callback->enumRevision) + NRed::callback->revision;
                orgCapsInitTable->pciRevision = NRed::callback->pciRevision;
                *orgCapsTable = {
                    .familyId = AMDGPU_FAMILY_RAVEN,
                    .deviceId = NRed::callback->deviceId,
                    .revision = NRed::callback->revision,
                    .extRevision = static_cast<uint32_t>(NRed::callback->enumRevision) + NRed::callback->revision,
                    .pciRevision = NRed::callback->pciRevision,
                    .caps = orgCapsInitTable->caps,
                };
                found = true;
                break;
            }
            orgCapsInitTable++;
        }
        PANIC_COND(!found, "hwlibs", "Failed to find init caps table entry");
        found = false;
        while (orgDevCapTable->familyId) {
            if (orgDevCapTable->familyId == AMDGPU_FAMILY_RAVEN && orgDevCapTable->deviceId == targetDeviceId) {
                orgDevCapTable->deviceId = NRed::callback->deviceId;
                orgDevCapTable->extRevision =
                    static_cast<uint64_t>(NRed::callback->enumRevision) + NRed::callback->revision;
                orgDevCapTable->revision = DEVICE_CAP_ENTRY_REV_DONT_CARE;
                orgDevCapTable->enumRevision = DEVICE_CAP_ENTRY_REV_DONT_CARE;
                found = true;
                break;
            }
            orgDevCapTable++;
        }
        PANIC_COND(!found, "hwlibs", "Failed to find device capability table entry");
        MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);
        DBGLOG("hwlibs", "Applied DDI Caps patches");

        auto ventura = getKernelVersion() >= KernelVersion::Ventura;
        auto monterey = getKernelVersion() >= KernelVersion::Monterey;
        const LookupPatchPlus patches[] = {
            {&kextRadeonX5000HWLibs, kPspSwInitOriginal1, kPspSwInitPatched1, 1, !catalina},
            {&kextRadeonX5000HWLibs, kPspSwInitOriginal2, kPspSwInitOriginalMask2, kPspSwInitPatched2,
                kPspSwInitPatchedMask2, 1, !catalina},
            {&kextRadeonX5000HWLibs, kSmuInitFunctionPointerListOriginal, kSmuInitFunctionPointerListOriginalMask,
                kSmuInitFunctionPointerListPatched, kSmuInitFunctionPointerListPatchedMask, 1},
            {&kextRadeonX5000HWLibs, kFullAsicResetOriginal, kFullAsicResetPatched, 1},
            {&kextRadeonX5000HWLibs, kGcSwInitOriginal, kGcSwInitOriginalMask, kGcSwInitPatched, kGcSwInitPatchedMask,
                1, !catalina},
            {&kextRadeonX5000HWLibs, kGcSetFwEntryInfoOriginal, kGcSetFwEntryInfoOriginalMask, kGcSetFwEntryInfoPatched,
                kGcSetFwEntryInfoPatchedMask, 1, !catalina},
            {&kextRadeonX5000HWLibs, kCreatePowerTuneServicesOriginal1, kCreatePowerTuneServicesPatched1, 1, !monterey},
            {&kextRadeonX5000HWLibs, kCreatePowerTuneServicesMontereyOriginal1,
                kCreatePowerTuneServicesMontereyPatched1, 1, monterey},
            {&kextRadeonX5000HWLibs, kCreatePowerTuneServicesOriginal2, kCreatePowerTuneServicesMask2,
                kCreatePowerTuneServicesPatched2, 1},
            {&kextRadeonX5000HWLibs, kCailQueryAdapterInfoOriginal, kCailQueryAdapterInfoPatched, 1, ventura},
            {&kextRadeonX5000HWLibs, kSDMAInitFunctionPointerListOriginal, kSDMAInitFunctionPointerListPatched, 1,
                ventura},
        };
        PANIC_COND(!LookupPatchPlus::applyAll(patcher, patches, slide, size), "hwlibs", "Failed to apply patches: %d",
            patcher.getError());

        return true;
    }

    return false;
}

void X5000HWLibs::wrapPopulateFirmwareDirectory(void *that) {
    FunctionCast(wrapPopulateFirmwareDirectory, callback->orgPopulateFirmwareDirectory)(that);

    bool isRenoirDerivative = NRed::callback->chipType >= ChipType::Renoir;

    auto *filename = isRenoirDerivative ? "ativvaxy_nv.dat" : "ativvaxy_rv.dat";
    auto &fwDesc = getFWDescByName(filename);
    DBGLOG("hwlibs", "VCN firmware filename is %s", filename);

    /** VCN 2.2, VCN 1.0 */
    auto *fw = callback->orgCreateFirmware(fwDesc.data, fwDesc.size, isRenoirDerivative ? 0x0202 : 0x0100, filename);
    PANIC_COND(fw == nullptr, "hwlibs", "Failed to create '%s' firmware", filename);

    auto *fwDir = getMember<void *>(that, getKernelVersion() > KernelVersion::BigSur ? 0xB0 : 0xB8);
    PANIC_COND(fwDir == nullptr, "hwlibs", "Failed to get firmware directory");
    PANIC_COND(!callback->orgPutFirmware(fwDir, 6, fw), "hwlibs", "Failed to insert '%s' firmware", filename);
}

CAILResult X5000HWLibs::hwLibsNoop() { return kCAILResultSuccess; }

void X5000HWLibs::wrapUpdateSdmaPowerGating(void *cail, uint32_t mode) {
    FunctionCast(wrapUpdateSdmaPowerGating, callback->orgUpdateSdmaPowerGating)(cail, mode);
    switch (mode) {
        case 0:
            [[fallthrough]];
        case 3:
            NRed::callback->sendMsgToSmc(PPSMC_MSG_PowerUpSdma);
            break;
        case 2:
            NRed::callback->sendMsgToSmc(PPSMC_MSG_PowerDownSdma);
            break;
        default:
            break;
    }
}

CAILResult X5000HWLibs::wrapPspCmdKmSubmit(void *psp, void *ctx, void *param3, void *param4) {
    char filename[128] = {0};
    auto &size = getMember<uint32_t>(ctx, 0xC);
    auto cmdID = getMember<uint32_t>(ctx, 0x0);
    auto *data = getMember<uint8_t *>(psp, getKernelVersion() == KernelVersion::Catalina ? 0xB00 :
                                           getKernelVersion() >= KernelVersion::Ventura  ? 0xB48 :
                                                                                           0xAF8);

    switch (cmdID) {
        case kPSPCommandLoadTA: {
            const char *name = reinterpret_cast<char *>(data + 0x8DB);
            if (!strncmp(name, "AMD DTM Application", 20)) {
                DBGLOG("hwlibs", "DTM is being loaded (size: 0x%X)", size);
                strncpy(filename, "psp_dtm.bin", 12);
            } else if (!strncmp(name, "AMD HDCP Application", 21)) {
                DBGLOG("hwlibs", "HDCP is being loaded (size: 0x%X)", size);
                strncpy(filename, "psp_hdcp.bin", 13);
            } else if (!strncmp(name, "AMD AUC Application", 20)) {
                DBGLOG("hwlibs", "AUC is being loaded (size: 0x%X)", size);
                strncpy(filename, "psp_auc.bin", 12);
            } else if (!strncmp(name, "AMD FP Application", 19)) {
                DBGLOG("hwlibs", "FP is being loaded (size: 0x%X)", size);
                strncpy(filename, "psp_fp.bin", 11);
            } else {
                DBGLOG("hwlibs", "Other PSP TA is being loaded: (name: %s size: 0x%X)", data + 0x8DB, size);
                return FunctionCast(wrapPspCmdKmSubmit, callback->orgPspCmdKmSubmit)(psp, ctx, param3, param4);
            }
            break;
        }
        case kPSPCommandLoadASD: {
            DBGLOG("hwlibs", "ASD is being loaded (size: 0x%X)", size);
            strncpy(filename, "psp_asd.bin", 12);
            break;
        }
        case kPSPCommandLoadIPFW: {
            auto *prefix = NRed::getGCPrefix();
            auto uCodeID = getMember<uint32_t>(ctx, 0x10);
            switch (uCodeID) {
                case kUCodeCE:
                    DBGLOG("hwlibs", "CE is being loaded (size: 0x%X)", size);
                    snprintf(filename, 128, "%sce_ucode.bin", prefix);
                    break;
                case kUCodePFP:
                    DBGLOG("hwlibs", "PFP is being loaded (size: 0x%X)", size);
                    snprintf(filename, 128, "%spfp_ucode.bin", prefix);
                    break;
                case kUCodeME:
                    DBGLOG("hwlibs", "ME is being loaded (size: 0x%X)", size);
                    snprintf(filename, 128, "%sme_ucode.bin", prefix);
                    break;
                case kUCodeMEC1JT:
                    DBGLOG("hwlibs", "MEC1 JT is being loaded (size: 0x%X)", size);
                    snprintf(filename, 128, "%smec_jt_ucode.bin", prefix);
                    break;
                case kUCodeMEC2JT:
                    DBGLOG("hwlibs", "MEC2 JT is being loaded (size: 0x%X)", size);
                    if (NRed::callback->chipType >= ChipType::Renoir) {
                        DBGLOG("hwlibs", "Skipping MEC2 JT FW");
                        return kCAILResultSuccess;
                    }
                    snprintf(filename, 128, "%smec_jt_ucode.bin", prefix);
                    break;
                case kUCodeMEC1:
                    DBGLOG("hwlibs", "MEC1 is being loaded (size: 0x%X)", size);
                    snprintf(filename, 128, "%smec_ucode.bin", prefix);
                    break;
                case kUCodeMEC2:
                    DBGLOG("hwlibs", "MEC2 is being loaded (size: 0x%X)", size);
                    if (NRed::callback->chipType >= ChipType::Renoir) {
                        DBGLOG("hwlibs", "Skipping MEC2 FW");
                        return kCAILResultSuccess;
                    }
                    snprintf(filename, 128, "%smec_ucode.bin", prefix);
                    break;
                case kUCodeRLC:
                    DBGLOG("hwlibs", "RLC is being loaded (size: 0x%X)", size);
                    snprintf(filename, 128, "%srlc_ucode.bin", prefix);
                    break;
                case kUCodeSDMA0:
                    DBGLOG("hwlibs", "SDMA0 is being loaded (size: 0x%X)", size);
                    strncpy(filename, "sdma_4_1_ucode.bin", 19);
                    break;
                case kUCodeDMCUERAM:
                    DBGLOG("hwlibs", "DMCU ERAM is being loaded (size: 0x%X)", size);
                    strncpy(filename, "dmcu_eram_dcn10.bin", 20);
                    break;
                case kUCodeDMCUISR:
                    DBGLOG("hwlibs", "DMCU ISR is being loaded (size: 0x%X)", size);
                    strncpy(filename, "dmcu_intvectors_dcn10.bin", 26);
                    break;
                case kUCodeRLCV:
                    DBGLOG("hwlibs", "RLC V is being loaded (size: 0x%X)", size);
                    if (NRed::callback->chipType >= ChipType::Renoir) {
                        DBGLOG("hwlibs", "Skipping RLC V FW");
                        return kCAILResultSuccess;
                    }
                    snprintf(filename, 128, "%srlcv_ucode.bin", prefix);
                    break;
                case kUCodeRLCSRListGPM:
                    DBGLOG("hwlibs", "RLC SRList GPM is being loaded (size: 0x%X)", size);
                    snprintf(filename, 128, "%srlc_srlist_gpm_mem.bin", prefix);
                    break;
                case kUCodeRLCSRListSRM:
                    DBGLOG("hwlibs", "RLC SRList SRM is being loaded (size: 0x%X)", size);
                    snprintf(filename, 128, "%srlc_srlist_srm_mem.bin", prefix);
                    break;
                case kUCodeRLCSRListCntl:
                    DBGLOG("hwlibs", "RLC SRList Cntl is being loaded (size: 0x%X)", size);
                    snprintf(filename, 128, "%srlc_srlist_cntl.bin", prefix);
                    break;
                case kUCodeDMCUB:
                    DBGLOG("hwlibs", "DMCUB is being loaded (size: 0x%X)", size);
                    strncpy(filename, "atidmcub_instruction_dcn21.bin", 31);
                    break;
                default:
                    DBGLOG("hwlibs", "UCode ID 0x%X loading", uCodeID);
                    return FunctionCast(wrapPspCmdKmSubmit, callback->orgPspCmdKmSubmit)(psp, ctx, param3, param4);
            }
            break;
        }
        default:
            DBGLOG("hwlibs", "Not hijacking command id 0x%X", cmdID);
            return FunctionCast(wrapPspCmdKmSubmit, callback->orgPspCmdKmSubmit)(psp, ctx, param3, param4);
    }

    auto &fwDesc = getFWDescByName(filename);
    memcpy(data, fwDesc.data, fwDesc.size);
    size = fwDesc.size;

    return FunctionCast(wrapPspCmdKmSubmit, callback->orgPspCmdKmSubmit)(psp, ctx, param3, param4);
}

bool X5000HWLibs::wrapGetIpFw(void *that, uint32_t ipVersion, char *name, void *out) {
    if (!strncmp(name, "ativvaxy_rv.dat", 16) || !strncmp(name, "ativvaxy_nv.dat", 16)) {
        auto &fwDesc = getFWDescByName(name);
        getMember<const uint8_t *>(out, 0x0) = fwDesc.data;
        getMember<uint32_t>(out, 0x8) = fwDesc.size;
        return true;
    }
    return FunctionCast(wrapGetIpFw, callback->orgGetIpFw)(that, ipVersion, name, out);
}
