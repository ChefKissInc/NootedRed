//! Copyright © 2022-2024 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5.
//! See LICENSE for details.

#include "HWLibs.hpp"
#include "Firmware.hpp"
#include "NRed.hpp"
#include "PatcherPlus.hpp"
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
        if (!catalina) {
            SolveRequestPlus solveRequests[] = {
                {"__ZL15deviceTypeTable", orgDeviceTypeTable, kDeviceTypeTablePattern},
                {"__ZN11AMDFirmware14createFirmwareEPhjjPKc", this->orgCreateFirmware, kCreateFirmwarePattern,
                    kCreateFirmwarePatternMask},
                {"__ZN20AMDFirmwareDirectory11putFirmwareE16_AMD_DEVICE_TYPEP11AMDFirmware", this->orgPutFirmware,
                    kPutFirmwarePattern},
            };
            PANIC_COND(!SolveRequestPlus::solveAll(patcher, id, solveRequests, slide, size), "HWLibs",
                "Failed to resolve symbols");
        }

        SolveRequestPlus solveRequests[] = {
            {"__ZL20CAIL_ASIC_CAPS_TABLE", orgCapsTable, kCailAsicCapsTableHWLibsPattern},
            {"_CAILAsicCapsInitTable", orgCapsInitTable, kCAILAsicCapsInitTablePattern},
            {"_DeviceCapabilityTbl", orgDevCapTable, kDeviceCapabilityTblPattern},
        };
        PANIC_COND(!SolveRequestPlus::solveAll(patcher, id, solveRequests, slide, size), "HWLibs",
            "Failed to resolve symbols");

        bool ventura = getKernelVersion() >= KernelVersion::Ventura;
        bool renoir = NRed::callback->chipType >= ChipType::Renoir;
        bool sonoma144 = getKernelVersion() > KernelVersion::Sonoma ||
                         (getKernelVersion() == KernelVersion::Sonoma && getKernelMinorVersion() >= 4);
        if (catalina) {
            RouteRequestPlus request {"__ZN16AmdTtlFwServices7getIpFwEjPKcP10_TtlFwInfo", wrapGetIpFw,
                this->orgGetIpFw};
            PANIC_COND(!request.route(patcher, id, slide, size), "HWLibs", "Failed to route getIpFw");
        } else {
            RouteRequestPlus requests[] = {
                {"__ZN35AMDRadeonX5000_AMDRadeonHWLibsX500025populateFirmwareDirectoryEv",
                    wrapPopulateFirmwareDirectory, this->orgGetIpFw},
                {"_psp_bootloader_is_sos_running_3_1", hwLibsGeneralFailure, kPspBootloaderIsSosRunning31Pattern},
                {"_psp_security_feature_caps_set_3_1",
                    renoir ? pspSecurityFeatureCapsSet12 : pspSecurityFeatureCapsSet10,
                    ventura ? kPspSecurityFeatureCapsSet31VenturaPattern : kPspSecurityFeatureCapsSet31Pattern},
            };
            PANIC_COND(!RouteRequestPlus::routeAll(patcher, id, requests, slide, size), "HWLibs",
                "Failed to route symbols (>10.15)");
            if (sonoma144) {
                RouteRequestPlus requests[] = {
                    {"_psp_bootloader_load_sysdrv_3_1", hwLibsNoop, kPspBootloaderLoadSysdrv31Pattern14_4},
                    {"_psp_bootloader_set_ecc_mode_3_1", hwLibsNoop, kPspBootloaderSetEccMode31Pattern14_4},
                    {"_psp_bootloader_load_sos_3_1", pspBootloaderLoadSos10, kPspBootloaderLoadSos31Pattern14_4},
                    {"_psp_reset_3_1", hwLibsUnsupported, kPspReset31Pattern14_4},
                };
                PANIC_COND(!RouteRequestPlus::routeAll(patcher, id, requests, slide, size), "HWLibs",
                    "Failed to route symbols (>=14.4)");
            } else {
                RouteRequestPlus requests[] = {
                    {"_psp_bootloader_load_sysdrv_3_1", hwLibsNoop, kPspBootloaderLoadSysdrv31Pattern,
                        kPspBootloaderLoadSysdrv31Mask},
                    {"_psp_bootloader_set_ecc_mode_3_1", hwLibsNoop, kPspBootloaderSetEccMode31Pattern},
                    {"_psp_bootloader_load_sos_3_1", pspBootloaderLoadSos10, kPspBootloaderLoadSos31Pattern,
                        kPspBootloaderLoadSos31Mask},
                    {"_psp_reset_3_1", hwLibsUnsupported, kPspReset31Pattern},
                };
                PANIC_COND(!RouteRequestPlus::routeAll(patcher, id, requests, slide, size), "HWLibs",
                    "Failed to route symbols (<14.4)");
            }
        }

        if (sonoma144) {
            RouteRequestPlus request = {"_psp_cmd_km_submit", wrapPspCmdKmSubmit, this->orgPspCmdKmSubmit,
                kPspCmdKmSubmitPattern14_4, kPspCmdKmSubmitMask14_4};
            PANIC_COND(!request.route(patcher, id, slide, size), "HWLibs", "Failed to route psp_cmd_km_submit (14.4+)");
        } else {
            RouteRequestPlus request = {"_psp_cmd_km_submit", wrapPspCmdKmSubmit, this->orgPspCmdKmSubmit,
                kPspCmdKmSubmitPattern, kPspCmdKmSubmitMask};
            PANIC_COND(!request.route(patcher, id, slide, size), "HWLibs", "Failed to route psp_cmd_km_submit");
        }

        RouteRequestPlus request = {"_smu_9_0_1_create_function_pointer_list", wrapSmu901CreateFunctionPointerList,
            getKernelVersion() >= KernelVersion::Ventura ? kSmu901CreateFunctionPointerListVenturaPattern :
                                                           kSmu901CreateFunctionPointerListPattern,
            kSmu901CreateFunctionPointerListPatternMask};
        PANIC_COND(!request.route(patcher, id, slide, size), "HWLibs",
            "Failed to route smu_9_0_1_create_function_pointer_list");

        PANIC_COND(MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) != KERN_SUCCESS, "HWLibs",
            "Failed to enable kernel writing");
        if (orgDeviceTypeTable) { *orgDeviceTypeTable = {.deviceId = NRed::callback->deviceId, .deviceType = 6}; }

        auto targetDeviceId = renoir ? 0x1636 : NRed::callback->deviceId;
        for (; orgCapsInitTable->deviceId != 0xFFFFFFFF; orgCapsInitTable++) {
            if (orgCapsInitTable->familyId == AMDGPU_FAMILY_RAVEN && orgCapsInitTable->deviceId == targetDeviceId) {
                orgCapsInitTable->deviceId = NRed::callback->deviceId;
                orgCapsInitTable->revision = NRed::callback->revision;
                orgCapsInitTable->extRevision =
                    static_cast<UInt64>(NRed::callback->enumRevision) + NRed::callback->revision;
                orgCapsInitTable->pciRevision = NRed::callback->pciRevision;
                *orgCapsTable = {
                    .familyId = AMDGPU_FAMILY_RAVEN,
                    .deviceId = NRed::callback->deviceId,
                    .revision = NRed::callback->revision,
                    .extRevision = static_cast<UInt32>(NRed::callback->enumRevision) + NRed::callback->revision,
                    .pciRevision = NRed::callback->pciRevision,
                    .caps = orgCapsInitTable->caps,
                };
                break;
            }
        }
        PANIC_COND(orgCapsInitTable->deviceId == 0xFFFFFFFF, "HWLibs", "Failed to find init caps table entry");
        for (; orgDevCapTable->familyId; orgDevCapTable++) {
            if (orgDevCapTable->familyId == AMDGPU_FAMILY_RAVEN && orgDevCapTable->deviceId == targetDeviceId) {
                orgDevCapTable->deviceId = NRed::callback->deviceId;
                orgDevCapTable->extRevision =
                    static_cast<UInt64>(NRed::callback->enumRevision) + NRed::callback->revision;
                orgDevCapTable->revision = DEVICE_CAP_ENTRY_REV_DONT_CARE;
                orgDevCapTable->enumRevision = DEVICE_CAP_ENTRY_REV_DONT_CARE;

                orgDevCapTable->asicGoldenSettings->goldenSettings =
                    NRed::callback->chipType < ChipType::Raven2 ? goldenSettingsRaven :
                    NRed::callback->chipType < ChipType::Renoir ? goldenSettingsRaven2 :
                                                                  goldenSettingsRenoir;

                break;
            }
        }
        PANIC_COND(!orgDevCapTable->familyId, "HWLibs", "Failed to find device capability table entry");
        MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);
        DBGLOG("HWLibs", "Applied DDI Caps patches");

        if (!catalina) {
            const LookupPatchPlus patch {&kextRadeonX5000HWLibs, kGcSwInitOriginal, kGcSwInitOriginalMask,
                kGcSwInitPatched, kGcSwInitPatchedMask, 1};
            PANIC_COND(!patch.apply(patcher, slide, size), "HWLibs", "Failed to apply gc_sw_init spoof patch");
            if (sonoma144) {
                const LookupPatchPlus patches[] = {
                    {&kextRadeonX5000HWLibs, kPspSwInit1Original14_4, kPspSwInit1Patched14_4, 1},
                    {&kextRadeonX5000HWLibs, kPspSwInit2Original14_4, kPspSwInit2OriginalMask14_4,
                        kPspSwInit2Patched14_4, 1},
                    {&kextRadeonX5000HWLibs, kGcSetFwEntryInfoOriginal14_4, kGcSetFwEntryInfoOriginalMask14_4,
                        kGcSetFwEntryInfoPatched14_4, kGcSetFwEntryInfoPatchedMask14_4, 1},
                };
                PANIC_COND(!LookupPatchPlus::applyAll(patcher, patches, slide, size), "HWLibs",
                    "Failed to apply spoof patches (>=14.4)");
            } else {
                const LookupPatchPlus patches[] = {
                    {&kextRadeonX5000HWLibs, kPspSwInit1Original, kPspSwInit1Patched, 1},
                    {&kextRadeonX5000HWLibs, kPspSwInit2Original, kPspSwInit2OriginalMask, kPspSwInit2Patched, 1},
                    {&kextRadeonX5000HWLibs, kGcSetFwEntryInfoOriginal, kGcSetFwEntryInfoOriginalMask,
                        kGcSetFwEntryInfoPatched, kGcSetFwEntryInfoPatchedMask, 1},
                };
                PANIC_COND(!LookupPatchPlus::applyAll(patcher, patches, slide, size), "HWLibs",
                    "Failed to apply spoof patches (<14.4)");
            }
        } else if (renoir) {
            const LookupPatchPlus patches[] = {
                {&kextRadeonX5000HWLibs, kPspSwInitCatalina1Original, kPspSwInitCatalina1Patched, 1},
                {&kextRadeonX5000HWLibs, kPspSwInitCatalina2Original, kPspSwInitCatalina2OriginalMask,
                    kPspSwInitCatalina2Patched, 1},
            };
            PANIC_COND(!LookupPatchPlus::applyAll(patcher, patches, slide, size), "HWLibs",
                "Failed to apply spoof patches");
        }

        if (sonoma144) {
            const LookupPatchPlus patch {&kextRadeonX5000HWLibs, kCreatePowerTuneServices1Original14_4,
                kCreatePowerTuneServices1Patched14_4, 1};
            PANIC_COND(!patch.apply(patcher, slide, size), "HWLibs",
                "Failed to apply PowerTuneServices patch (>=14.4)");
        } else if (getKernelVersion() >= KernelVersion::Monterey) {
            const LookupPatchPlus patch {&kextRadeonX5000HWLibs, kCreatePowerTuneServices1Original12_0,
                kCreatePowerTuneServices1Patched12_0, 1};
            PANIC_COND(!patch.apply(patcher, slide, size), "HWLibs", "Failed to apply PowerTuneServices patch (<14.4)");
        } else {
            const LookupPatchPlus patch {&kextRadeonX5000HWLibs, kCreatePowerTuneServices1Original,
                kCreatePowerTuneServices1Patched, 1};
            PANIC_COND(!patch.apply(patcher, slide, size), "HWLibs", "Failed to apply PowerTuneServices patch (<12.0)");
        }

        if (sonoma144) {
            const LookupPatchPlus patches[] = {
                {&kextRadeonX5000HWLibs, kSmuInitFunctionPointerListOriginal14_4,
                    kSmuInitFunctionPointerListOriginalMask14_4, kSmuInitFunctionPointerListPatched14_4,
                    kSmuInitFunctionPointerListPatchedMask14_4, 1},
                {&kextRadeonX5000HWLibs, kCreatePowerTuneServices2Original14_4, kCreatePowerTuneServices2Mask14_4,
                    kCreatePowerTuneServices2Patched14_4, 1},
            };
            PANIC_COND(!LookupPatchPlus::applyAll(patcher, patches, slide, size), "HWLibs",
                "Failed to apply patches (>=14.4)");
        } else {
            const LookupPatchPlus patches[] = {
                {&kextRadeonX5000HWLibs, kSmuInitFunctionPointerListOriginal, kSmuInitFunctionPointerListOriginalMask,
                    kSmuInitFunctionPointerListPatched, kSmuInitFunctionPointerListPatchedMask, 1},
                {&kextRadeonX5000HWLibs, kCreatePowerTuneServices2Original, kCreatePowerTuneServices2Mask,
                    kCreatePowerTuneServices2Patched, 1},
            };
            PANIC_COND(!LookupPatchPlus::applyAll(patcher, patches, slide, size), "HWLibs",
                "Failed to apply patches (<14.4)");
        }
        const LookupPatchPlus patches[] = {
            {&kextRadeonX5000HWLibs, kGcGoldenSettingsExecutionOriginal, kGcGoldenSettingsExecutionOriginalMask,
                kGcGoldenSettingsExecutionPatched, kGcGoldenSettingsExecutionPatchedMask, 1},
            {&kextRadeonX5000HWLibs, kSdma40GdbExecutionCallOriginal, kSdma40GdbExecutionCallOriginalMask,
                kSdma40GdbExecutionCallPatched, kSdma40GdbExecutionCallPatchedMask, 1},
            {&kextRadeonX5000HWLibs, kGc90GdbExecutionCallOriginal, kGc90GdbExecutionCallOriginalMask,
                kGc90GdbExecutionCallPatched, kGc90GdbExecutionCallPatchedMask, 1},
        };
        PANIC_COND(!LookupPatchPlus::applyAll(patcher, patches, slide, size), "HWLibs", "Failed to apply patches");

        if (ventura) {
            const LookupPatchPlus patches[] = {
                {&kextRadeonX5000HWLibs, kCailQueryAdapterInfoOriginal, kCailQueryAdapterInfoPatched, 1},
                {&kextRadeonX5000HWLibs, kSDMAInitFunctionPointerListOriginal, kSDMAInitFunctionPointerListOriginalMask,
                    kSDMAInitFunctionPointerListPatched, kSDMAInitFunctionPointerListPatchedMask, 1},
            };
            PANIC_COND(!LookupPatchPlus::applyAll(patcher, patches, slide, size), "HWLibs",
                "Failed to apply macOS 13.0+ patches");
        }

        return true;
    }

    return false;
}

void X5000HWLibs::wrapPopulateFirmwareDirectory(void *that) {
    FunctionCast(wrapPopulateFirmwareDirectory, callback->orgGetIpFw)(that);

    bool isRenoirDerivative = NRed::callback->chipType >= ChipType::Renoir;

    auto *filename = isRenoirDerivative ? "ativvaxy_nv.dat" : "ativvaxy_rv.dat";
    const auto &vcnFW = getFWByName(filename);
    DBGLOG("HWLibs", "VCN firmware filename is %s", filename);

    //! VCN 2.2, VCN 1.0
    auto *fw = callback->orgCreateFirmware(vcnFW.data, vcnFW.length, isRenoirDerivative ? 0x0202 : 0x0100, filename);
    PANIC_COND(!fw, "HWLibs", "Failed to create '%s' firmware", filename);
    auto *fwDir = getMember<void *>(that, getKernelVersion() > KernelVersion::BigSur ? 0xB0 : 0xB8);
    PANIC_COND(!callback->orgPutFirmware(fwDir, 6, fw), "HWLibs", "Failed to insert '%s' firmware", filename);
}

bool X5000HWLibs::wrapGetIpFw(void *that, UInt32 ipVersion, char *name, void *out) {
    if (!strncmp(name, "ativvaxy_rv.dat", 16) || !strncmp(name, "ativvaxy_nv.dat", 16)) {
        const auto &fwDesc = getFWByName(name);
        getMember<const void *>(out, 0x0) = fwDesc.data;
        getMember<UInt32>(out, 0x8) = fwDesc.length;
        return true;
    }
    return FunctionCast(wrapGetIpFw, callback->orgGetIpFw)(that, ipVersion, name, out);
}

CAILResult X5000HWLibs::hwLibsGeneralFailure() { return kCAILResultGeneralFailure; }
CAILResult X5000HWLibs::hwLibsUnsupported() { return kCAILResultUnsupported; }
CAILResult X5000HWLibs::hwLibsNoop() { return kCAILResultSuccess; }

CAILResult X5000HWLibs::pspBootloaderLoadSos10(void *ctx) {
    size_t fieldBase;
    switch (getKernelVersion()) {
        case KernelVersion::Catalina:
            UNREACHABLE();
        case KernelVersion::BigSur... KernelVersion::Monterey:
            fieldBase = 0x3124;
            break;
        case KernelVersion::Ventura... KernelVersion::Sequoia:
            fieldBase = 0x391C;
            break;
        default:
            PANIC("HWLibs", "Unsupported kernel version %d", getKernelVersion());
    }
    getMember<UInt32>(ctx, fieldBase) = NRed::callback->readReg32(MP_BASE + mmMP0_SMN_C2PMSG_59);
    getMember<UInt32>(ctx, fieldBase + 0x4) = NRed::callback->readReg32(MP_BASE + mmMP0_SMN_C2PMSG_58);
    getMember<UInt32>(ctx, fieldBase + 0x8) = NRed::callback->readReg32(MP_BASE + mmMP0_SMN_C2PMSG_58);
    return kCAILResultSuccess;
}

CAILResult X5000HWLibs::pspSecurityFeatureCapsSet10(void *ctx) {
    size_t fieldBase;
    switch (getKernelVersion()) {
        case KernelVersion::Catalina:
            UNREACHABLE();
        case KernelVersion::BigSur... KernelVersion::Monterey:
            fieldBase = 0x3120;
            break;
        case KernelVersion::Ventura... KernelVersion::Sequoia:
            fieldBase = 0x3918;
            break;
        default:
            PANIC("HWLibs", "Unsupported kernel version %d", getKernelVersion());
    }
    auto &securityCaps = getMember<UInt8>(ctx, fieldBase);
    securityCaps &= ~static_cast<UInt8>(1);
    auto &tOSVer = getMember<UInt32>(ctx, fieldBase + 0x8);
    if ((tOSVer & 0xFFFF0000) == 0x80000 && (tOSVer & 0xFF) > 0x50) {
        auto policyVer = NRed::callback->readReg32(MP_BASE + mmMP0_SMN_C2PMSG_91);
        SYSLOG_COND((policyVer & 0xFF000000) != 0xA000000, "HWLibs", "Invalid security policy version: 0x%X",
            policyVer);
        if (policyVer == 0xA02031A || ((policyVer & 0xFFFFFF00) == 0xA020400 && (policyVer & 0xFC) > 0x23) ||
            ((policyVer & 0xFFFFFF00) == 0xA020300 && (policyVer & 0xFE) > 0x1D)) {
            securityCaps |= 1;
        }
    }

    return kCAILResultSuccess;
}

CAILResult X5000HWLibs::pspSecurityFeatureCapsSet12(void *ctx) {
    size_t fieldBase;
    switch (getKernelVersion()) {
        case KernelVersion::Catalina:
            UNREACHABLE();
        case KernelVersion::BigSur... KernelVersion::Monterey:
            fieldBase = 0x3120;
            break;
        case KernelVersion::Ventura... KernelVersion::Sequoia:
            fieldBase = 0x3918;
            break;
        default:
            PANIC("HWLibs", "Unsupported kernel version %d", getKernelVersion());
    }
    auto &securityCaps = getMember<UInt8>(ctx, fieldBase);
    securityCaps &= ~static_cast<UInt8>(1);
    auto &tOSVer = getMember<UInt32>(ctx, fieldBase + 0x8);
    if ((tOSVer & 0xFFFF0000) == 0x110000 && (tOSVer & 0xFF) > 0x2A) {
        auto policyVer = NRed::callback->readReg32(MP_BASE + mmMP0_SMN_C2PMSG_91);
        SYSLOG_COND((policyVer & 0xFF000000) != 0xB000000, "HWLibs", "Invalid security policy version: 0x%X",
            policyVer);
        if ((policyVer & 0xFFFF0000) == 0xB090000 && (policyVer & 0xFE) > 0x35) { securityCaps |= 1; }
    }

    return kCAILResultSuccess;
}

CAILResult X5000HWLibs::wrapPspCmdKmSubmit(void *ctx, void *cmd, void *param3, void *param4) {
    char filename[64] = {0};
    size_t off;
    switch (getKernelVersion()) {
        case KernelVersion::Catalina:
            off = 0xB00;
            break;
        case KernelVersion::BigSur... KernelVersion::Monterey:
            off = 0xAF8;
            break;
        case KernelVersion::Ventura... KernelVersion::Sequoia:
            off = 0xB48;
            break;
        default:
            PANIC("HWLibs", "Unsupported kernel version %d", getKernelVersion());
    }
    auto *data = getMember<UInt8 *>(ctx, off);

    switch (getMember<AMDPSPCommand>(cmd, 0x0)) {
        case kPSPCommandLoadTA: {
            const char *name = reinterpret_cast<char *>(data + 0x8DB);
            if (!strncmp(name, "AMD DTM Application", 20)) {
                strncpy(filename, "psp_dtm.bin", 12);
                break;
            }
            if (!strncmp(name, "AMD HDCP Application", 21)) {
                strncpy(filename, "psp_hdcp.bin", 13);
                break;
            }
            if (!strncmp(name, "AMD AUC Application", 20)) {
                strncpy(filename, "psp_auc.bin", 12);
                break;
            }
            if (!strncmp(name, "AMD FP Application", 19)) {
                strncpy(filename, "psp_fp.bin", 11);
                break;
            }
            return FunctionCast(wrapPspCmdKmSubmit, callback->orgPspCmdKmSubmit)(ctx, cmd, param3, param4);
        }
        case kPSPCommandLoadASD: {
            strncpy(filename, "psp_asd.bin", 12);
            break;
        }
        case kPSPCommandLoadIPFW: {
            auto *prefix = NRed::getGCPrefix();
            switch (getMember<AMDUCodeID>(cmd, 0x10)) {
                case kUCodeCE:
                    snprintf(filename, sizeof(filename), "%sce_ucode.bin", prefix);
                    break;
                case kUCodePFP:
                    snprintf(filename, sizeof(filename), "%spfp_ucode.bin", prefix);
                    break;
                case kUCodeME:
                    snprintf(filename, sizeof(filename), "%sme_ucode.bin", prefix);
                    break;
                case kUCodeMEC1JT:
                    snprintf(filename, sizeof(filename), "%smec_jt_ucode.bin", prefix);
                    break;
                case kUCodeMEC2JT:
                    if (NRed::callback->chipType >= ChipType::Renoir) { return kCAILResultSuccess; }
                    snprintf(filename, sizeof(filename), "%smec_jt_ucode.bin", prefix);
                    break;
                case kUCodeMEC1:
                    snprintf(filename, sizeof(filename), "%smec_ucode.bin", prefix);
                    break;
                case kUCodeMEC2:
                    if (NRed::callback->chipType >= ChipType::Renoir) { return kCAILResultSuccess; }
                    snprintf(filename, sizeof(filename), "%smec_ucode.bin", prefix);
                    break;
                case kUCodeRLC:
                    //! Fake CGPG
                    if (NRed::callback->chipType == ChipType::Raven ||
                        (NRed::callback->deviceId == 0x15D8 &&
                            ((NRed::callback->pciRevision >= 0xC8 && NRed::callback->pciRevision <= 0xCC) ||
                                (NRed::callback->pciRevision >= 0xD8 && NRed::callback->pciRevision <= 0xDD)))) {
                        snprintf(filename, sizeof(filename), "%srlc_fake_cgpg_ucode.bin", prefix);
                    } else {
                        snprintf(filename, sizeof(filename), "%srlc_ucode.bin", prefix);
                    }
                    break;
                case kUCodeSDMA0:
                    strncpy(filename, "sdma_4_1_ucode.bin", 19);
                    break;
                case kUCodeDMCUERAM:
                    if (NRed::callback->chipType < ChipType::Renoir) {
                        strncpy(filename, "dmcu_eram_dcn10.bin", 20);
                    } else {
                        strncpy(filename, "dmcu_eram_dcn21.bin", 20);
                    }
                    break;
                case kUCodeDMCUISR:
                    if (NRed::callback->chipType < ChipType::Renoir) {
                        strncpy(filename, "dmcu_intvectors_dcn10.bin", 26);
                    } else {
                        strncpy(filename, "dmcu_intvectors_dcn21.bin", 26);
                    }
                    break;
                case kUCodeRLCV:
                    //! No RLC V on Renoir
                    if (NRed::callback->chipType >= ChipType::Renoir) { return kCAILResultSuccess; }
                    snprintf(filename, sizeof(filename), "%srlcv_ucode.bin", prefix);
                    break;
                case kUCodeRLCSRListGPM:
                    snprintf(filename, sizeof(filename), "%srlc_srlist_gpm_mem.bin", prefix);
                    break;
                case kUCodeRLCSRListSRM:
                    snprintf(filename, sizeof(filename), "%srlc_srlist_srm_mem.bin", prefix);
                    break;
                case kUCodeRLCSRListCntl:
                    snprintf(filename, sizeof(filename), "%srlc_srlist_cntl.bin", prefix);
                    break;
                case kUCodeDMCUB:
                    if (NRed::callback->chipType < ChipType::Renoir) {    // Just in case
                        SYSLOG("HWLibs", "DMCU version B is not supposed to be loaded on this ASIC!");
                        return kCAILResultSuccess;
                    }
                    strncpy(filename, "atidmcub_rn.dat", 16);
                    break;
                default:
                    return FunctionCast(wrapPspCmdKmSubmit, callback->orgPspCmdKmSubmit)(ctx, cmd, param3, param4);
            }
            break;
        }
        default:
            return FunctionCast(wrapPspCmdKmSubmit, callback->orgPspCmdKmSubmit)(ctx, cmd, param3, param4);
    }

    const auto &fw = getFWByName(filename);
    memcpy(data, fw.data, fw.length);
    getMember<UInt32>(cmd, 0xC) = fw.length;

    return FunctionCast(wrapPspCmdKmSubmit, callback->orgPspCmdKmSubmit)(ctx, cmd, param3, param4);
}

void X5000HWLibs::smuSoftReset() {    //! Likely soft resets SDMA, but logic's not present in Linux
    NRed::callback->sendMsgToSmc(PPSMC_MSG_SoftReset, 0x40);
}

void X5000HWLibs::smu10PowerUp() {
    NRed::callback->sendMsgToSmc(PPSMC_MSG_ForceGfxContentSave);
    NRed::callback->sendMsgToSmc(PPSMC_MSG_PowerUpSdma);
    NRed::callback->sendMsgToSmc(PPSMC_MSG_PowerUpGfx);
}

CAILResult X5000HWLibs::smuInternalSwInit(void *ctx) {
    size_t fieldBase;
    switch (getKernelVersion()) {
        case KernelVersion::Catalina:
            UNREACHABLE();
        case KernelVersion::BigSur... KernelVersion::Monterey:
            fieldBase = 0x280;
            break;
        case KernelVersion::Ventura... KernelVersion::Sequoia:
            fieldBase = 0x2D0;
            break;
        default:
            PANIC("HWLibs", "Unsupported kernel version %d", getKernelVersion());
    }
    //! is_sw_init
    getMember<bool>(ctx, fieldBase) = true;
    return kCAILResultSuccess;
}

CAILResult X5000HWLibs::smu10InternalHwInit(void *) {
    smuSoftReset();
    smu10PowerUp();
    return kCAILResultSuccess;
}

CAILResult X5000HWLibs::smu12InternalHwInit(void *) {
    UInt32 i = 0;
    for (; i < AMDGPU_MAX_USEC_TIMEOUT; i++) {
        if (NRed::callback->readReg32(MP1_Public | smnMP1_FIRMWARE_FLAGS) & smnMP1_FIRMWARE_FLAGS_INTERRUPTS_ENABLED) {
            break;
        }
        IOSleep(1);
    }
    if (i >= AMDGPU_MAX_USEC_TIMEOUT - 1) { return kCAILResultGeneralFailure; }
    smuSoftReset();
    smu10PowerUp();
    NRed::callback->sendMsgToSmc(PPSMC_MSG_PowerGateAtHub);
    return kCAILResultSuccess;
}

CAILResult X5000HWLibs::smuInternalHwExit(void *) {
    smuSoftReset();
    return kCAILResultSuccess;
}

CAILResult X5000HWLibs::smuFullAsicReset(void *, void *data) {
    NRed::callback->sendMsgToSmc(PPSMC_MSG_DeviceDriverReset, getMember<UInt32>(data, 4));
    return kCAILResultSuccess;
}

CAILResult X5000HWLibs::smu10NotifyEvent(void *, void *data) {
    auto event = getMember<UInt32>(data, 4);
    if (event >= 11) {
        SYSLOG("HWLibs", "Invalid input event to SMU notify event");
        return kCAILResultGeneralFailure;
    }
    switch (event) {
        case 0:
            [[fallthrough]];
        case 4:
            [[fallthrough]];
        case 10:
            smu10PowerUp();
            return kCAILResultSuccess;
        default:
            return kCAILResultSuccess;
    }
}

CAILResult X5000HWLibs::smu12NotifyEvent(void *, void *data) {
    auto event = getMember<UInt32>(data, 4);
    if (event >= 11) {
        SYSLOG("HWLibs", "Invalid input event to SMU notify event");
        return kCAILResultGeneralFailure;
    }
    switch (event) {
        case 0:
            [[fallthrough]];
        case 4:
            [[fallthrough]];
        case 8:
            [[fallthrough]];
        case 10:
            NRed::callback->sendMsgToSmc(PPSMC_MSG_PowerUpSdma);
            return kCAILResultSuccess;
        default:
            return kCAILResultSuccess;
    }
}

CAILResult X5000HWLibs::smuFullScreenEvent(void *, UInt32 event) {
    switch (event) {
        case 1:
            NRed::callback->writeReg32(MP_BASE + mmMP1_SMN_FPS_CNT,
                NRed::callback->readReg32(MP_BASE + mmMP1_SMN_FPS_CNT) + 1);
            return kCAILResultSuccess;
        case 2:
            NRed::callback->writeReg32(MP_BASE + mmMP1_SMN_FPS_CNT, 0);
            return kCAILResultSuccess;
        default:
            SYSLOG("HWLibs", "Invalid input event to SMU full screen event");
            return kCAILResultGeneralFailure;
    }
}

CAILResult X5000HWLibs::wrapSmu901CreateFunctionPointerList(void *ctx) {
    size_t fieldBase;
    switch (getKernelVersion()) {
        case KernelVersion::Catalina:
            fieldBase = 0x378;
            break;
        case KernelVersion::BigSur:
            fieldBase = 0x638;
            break;
        case KernelVersion::Monterey:
            fieldBase = 0x648;
            break;
        case KernelVersion::Ventura... KernelVersion::Sequoia:
            fieldBase = 0x6C0;
            break;
        default:
            PANIC("HWLibs", "Unsupported kernel version %d", getKernelVersion());
    }
    if (getKernelVersion() == KernelVersion::Catalina) {
        getMember<mach_vm_address_t>(ctx, fieldBase + 0x0) = reinterpret_cast<mach_vm_address_t>(hwLibsNoop);
        getMember<mach_vm_address_t>(ctx, fieldBase + 0x40) = reinterpret_cast<mach_vm_address_t>(smuFullScreenEvent);
    } else {
        getMember<mach_vm_address_t>(ctx, fieldBase + 0x0) = reinterpret_cast<mach_vm_address_t>(smuInternalSwInit);
        getMember<mach_vm_address_t>(ctx, fieldBase + 0x48) = reinterpret_cast<mach_vm_address_t>(smuFullScreenEvent);
        //! get_ucode_consts
        getMember<mach_vm_address_t>(ctx, fieldBase + 0xE8) = reinterpret_cast<mach_vm_address_t>(hwLibsNoop);
    }
    if (NRed::callback->chipType >= ChipType::Renoir) {
        getMember<mach_vm_address_t>(ctx, fieldBase + 0x8) = reinterpret_cast<mach_vm_address_t>(smu12InternalHwInit);
        getMember<mach_vm_address_t>(ctx, fieldBase + 0x30) = reinterpret_cast<mach_vm_address_t>(smu12NotifyEvent);
    } else {
        getMember<mach_vm_address_t>(ctx, fieldBase + 0x8) = reinterpret_cast<mach_vm_address_t>(smu10InternalHwInit);
        getMember<mach_vm_address_t>(ctx, fieldBase + 0x30) = reinterpret_cast<mach_vm_address_t>(smu10NotifyEvent);
    }
    //! internal_sw_exit
    getMember<mach_vm_address_t>(ctx, fieldBase + 0x10) = reinterpret_cast<mach_vm_address_t>(hwLibsNoop);
    getMember<mach_vm_address_t>(ctx, fieldBase + 0x18) = reinterpret_cast<mach_vm_address_t>(smuInternalHwExit);
    getMember<mach_vm_address_t>(ctx, fieldBase + 0x28) = reinterpret_cast<mach_vm_address_t>(smuFullAsicReset);
    return kCAILResultSuccess;
}
