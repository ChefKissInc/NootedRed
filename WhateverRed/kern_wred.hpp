//  Copyright © 2022 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.0. See LICENSE for
//  details.

#pragma once
#include "kern_amd.hpp"
#include <Headers/kern_iokit.hpp>
#include <IOKit/pci/IOPCIDevice.h>

enum struct ASICType {
    Raven,
    Raven2,
    Picasso,
    Renoir,
    Unknown,
};

class WRed {
    public:
    static WRed *callbackWRed;

    void init();
    void deinit();
    void processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size);

    private:
    static const char *getASICName() {
        switch (callbackWRed->asicType) {
            case ASICType::Picasso:
                return "picasso";
            case ASICType::Raven:
                return "raven";
            case ASICType::Raven2:
                return "raven2";
            case ASICType::Renoir:
                return "renoir";
            default:
                PANIC("wred", "Unknown ASIC type");
        }
    }

    OSData *vbiosData = nullptr;
    ASICType asicType = ASICType::Unknown;
    void *callbackFirmwareDirectory = nullptr;

    /** X6000Framebuffer */
    mach_vm_address_t orgPopulateDeviceInfo {};
    CailAsicCapEntry *orgAsicCapsTable = nullptr;
    mach_vm_address_t orgHwReadReg32 {};

    /** X5000HWLibs */
    uint32_t *orgDeviceTypeTable = nullptr;
    mach_vm_address_t orgAmdTtlServicesConstructor {};
    mach_vm_address_t orgSmuGetHwVersion {};
    mach_vm_address_t orgPspSwInit {}, orgGcGetHwVersion {};
    mach_vm_address_t orgPopulateFirmwareDirectory {};
    t_createFirmware orgCreateFirmware = nullptr;
    t_putFirmware orgPutFirmware = nullptr;
    t_Vega10PowerTuneConstructor orgVega10PowerTuneConstructor = nullptr;
    CailAsicCapEntry *orgAsicCapsTableHWLibs = nullptr;
    CailInitAsicCapEntry *orgAsicInitCapsTable = nullptr;
    mach_vm_address_t orgPspAsdLoad {};
    mach_vm_address_t orgPspDtmLoad {};
    mach_vm_address_t orgPspHdcpLoad {};
    GcFwConstant *orgGcRlcUcode = nullptr;
    GcFwConstant *orgGcMeUcode = nullptr;
    GcFwConstant *orgGcCeUcode = nullptr;
    GcFwConstant *orgGcPfpUcode = nullptr;
    GcFwConstant *orgGcMecUcode = nullptr;
    GcFwConstant *orgGcMecJtUcode = nullptr;
    SdmaFwConstant *orgSdma41Ucode = nullptr;
    SdmaFwConstant *orgSdma42Ucode = nullptr;
    t_sendMsgToSmcWithParam orgRavenSendMsgToSmcWithParam = nullptr;
    t_sendMsgToSmcWithParam orgRenoirSendMsgToSmcWithParam = nullptr;
    mach_vm_address_t orgSmuRavenInitialize {};
    mach_vm_address_t orgSmuRenoirInitialize {};

    /** X6000 */
    t_HWEngineNew orgVCN2EngineNewX6000 = nullptr;
    t_HWEngineConstructor orgVCN2EngineConstructorX6000 = nullptr;
    mach_vm_address_t orgSetupAndInitializeHWCapabilitiesX6000 {};
    mach_vm_address_t orgAllocateAMDHWDisplayX6000 {};

    /** X5000 */
    t_HWEngineNew orgGFX9PM4EngineNew = nullptr;
    t_HWEngineConstructor orgGFX9PM4EngineConstructor = nullptr;
    t_HWEngineNew orgGFX9SDMAEngineNew = nullptr;
    t_HWEngineConstructor orgGFX9SDMAEngineConstructor = nullptr;
    mach_vm_address_t orgSetupAndInitializeHWCapabilities {};
    mach_vm_address_t orgAccelDisplayPipeWriteDiagnosisReport {};
    mach_vm_address_t orgRTGetHWChannel {};
    mach_vm_address_t orgMapVA {};
    mach_vm_address_t orgMapVMPT {};
    mach_vm_address_t orgWriteWritePTEPDECommand {};
    mach_vm_address_t orgGetPDEValue {};
    mach_vm_address_t orgGetPTEValue {};
    mach_vm_address_t orgUpdateContiguousPTEsWithDMAUsingAddr {};

    /** X6000Framebuffer */
    static uint16_t wrapGetFamilyId(void *that);
    static IOReturn wrapPopulateDeviceInfo(void *that);
    static uint16_t wrapGetEnumeratedRevision(void *that);
    static uint32_t wrapGetVideoMemoryType(void *that);
    static uint32_t wrapGetVideoMemoryBitWidth(void *that);
    static IOReturn wrapPopulateVramInfo(void *that);
    static uint32_t wrapHwReadReg32(void *that, uint32_t param1);

    /** X5000HWLibs */
    static void wrapAmdTtlServicesConstructor(void *that, IOPCIDevice *provider);
    static uint64_t wrapSmuGetHwVersion(uint64_t param1, uint32_t param2);
    static uint64_t wrapPspSwInit(uint32_t *param1, uint32_t *param2);
    static uint32_t wrapGcGetHwVersion(uint32_t *param1);
    static void wrapPopulateFirmwareDirectory(void *that);
    static void *wrapCreatePowerTuneServices(void *that, void *param2);
    static uint32_t wrapSmuGetFwConstants(void *param1);
    static uint32_t wrapSmuInternalHwInit(void *param1);
    static uint32_t wrapPspAsdLoad(void *pspData);
    static uint32_t wrapPspDtmLoad(void *pspData);
    static uint32_t wrapPspHdcpLoad(void *pspData);
    static void powerUpSDMA(void *smumData);
    static uint32_t wrapSmuRavenInitialize(void *smumData, uint32_t param2);
    static uint32_t wrapSmuRenoirInitialize(void *smumData, uint32_t param2);
    static uint32_t pspFeatureUnsupported();

    /** X6000 */
    static bool wrapAccelStartX6000();

    /** X5000 */
    static bool wrapAllocateHWEngines(void *that);
    static void wrapSetupAndInitializeHWCapabilities(void *that);
    static void wrapAccelDisplayPipeWriteDiagnosisReport();
    static void *wrapRTGetHWChannel(void *that, uint32_t param1, uint32_t param2, uint32_t param3);
    static void wrapInitializeFamilyType(void *that);
    static uint64_t wrapMapVA(void *that, uint64_t param1, void *memory, uint64_t param3, uint64_t sizeToMap,
        uint64_t flags);
    static uint64_t wrapMapVMPT(void *that, void *vmptCtl, uint64_t vmptLevel, uint32_t param3, uint64_t param4,
        uint64_t param5, uint64_t sizeToMap);
    static uint32_t wrapWriteWritePTEPDECommand(void *that, uint32_t *buf, uint64_t pe, uint32_t count, uint64_t flags,
        uint64_t addr, uint64_t incr);
    static uint64_t wrapGetPDEValue(void *that, uint64_t level, uint64_t param2);
    static uint64_t wrapGetPTEValue(void *that, uint64_t param1, uint64_t param2, uint64_t param3, uint32_t param4);
    static void wrapUpdateContiguousPTEsWithDMAUsingAddr(void *that, uint64_t param1, uint64_t param2, uint64_t param3,
        uint64_t param4, uint64_t param5);
    static void *wrapAllocateAMDHWDisplay(void *that);
    mach_vm_address_t orgPspCosLog {};
    static void wrapPspCosLog(void *pspData, uint32_t param2, uint64_t param3, uint32_t param4, char *param5);
    mach_vm_address_t orgPspCmdKmSubmit {};
    static uint32_t wrapPspCmdKmSubmit(void *pspData, void *context, void *param3, void *param4);
    mach_vm_address_t orgAtiPowerPlayServicesConstructor {};
    static void wrapAtiPowerPlayServicesConstructor(void *that, void *ppCallbacks);
    mach_vm_address_t orgInitWithPciInfo {};
    static bool wrapInitWithPciInfo(void *that, void *param1);
    mach_vm_address_t orgEnableController {};
    static IOReturn wrapEnableController(void *that);
    mach_vm_address_t orgControllerPowerUp {};
    static uint64_t wrapControllerPowerUp(void *that);
    mach_vm_address_t orgPpPowerUp {};
    static uint64_t wrapPpPowerUp(void *that);
};
