//
//  kern_rad.hpp
//  WhateverRed
//
//  Copyright © 2017 vit9696. All rights reserved.
//  Copyright © 2022 ChefKiss Inc. All rights reserved.
//

#ifndef kern_rad_hpp
#define kern_rad_hpp

#include "kern_agdc.hpp"
#include "kern_atom.hpp"
#include "stdint.h"
#include <Headers/kern_devinfo.hpp>
#include <Headers/kern_patcher.hpp>
#include <IOKit/IOService.h>
#include <IOKit/graphics/IOFramebuffer.h>
#include <IOKit/pci/IOPCIDevice.h>

enum struct ASICType {
    Unknown,
    Raven,
    Raven2,
    Picasso,
    Renoir,
};

class RAD {
    public:
    void init();
    void deinit();

    void processKernel(KernelPatcher &patcher);
    bool processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size);

    private:
    using t_createFirmware = void *(*)(const void *data, uint32_t size, uint32_t param3, const char *filename);
    using t_putFirmware = bool (*)(void *that, uint32_t deviceType, void *fw);
    using t_Vega10PowerTuneConstructor = void (*)(void *that, void *param1, void *param2);
    using t_HWEngineConstructor = void (*)(void *that);
    using t_HWEngineNew = void *(*)(size_t size);
    using t_sendMsgToSmcWithParam = uint32_t (*)(void *smumData, uint32_t msgId, uint32_t parameter);

    static RAD *callbackRAD;

    OSData *vbiosData = nullptr;
    ASICType asicType = ASICType::Unknown;
    void *callbackFirmwareDirectory = nullptr;

    /**
     * Kernel
     */
    mach_vm_address_t orgSetProperty {}, orgGetProperty {};

    /**
     * AMDSupport
     */
    mach_vm_address_t orgNotifyLinkChange {};

    /**
     * X6000Framebuffer
     */
    mach_vm_address_t orgPopulateDeviceInfo {};
    CailAsicCapEntry *orgAsicCapsTable = nullptr;
    mach_vm_address_t orgHwReadReg32 {};

    /**
     * X5000HWLibs
     */
    uint32_t *orgDeviceTypeTable = nullptr;
    mach_vm_address_t orgAmdTtlServicesConstructor {};
    mach_vm_address_t orgSmuGetHwVersion {};
    mach_vm_address_t orgPspSwInit {}, orgGcGetHwVersion {};
    mach_vm_address_t orgPopulateFirmwareDirectory {};
    t_createFirmware orgCreateFirmware = nullptr;
    t_putFirmware orgPutFirmware = nullptr;
    t_Vega10PowerTuneConstructor orgVega10PowerTuneConstructor = nullptr;
    mach_vm_address_t orgCosDebugPrint {}, orgMCILDebugPrint {};
    mach_vm_address_t orgCosDebugPrintVaList {};
    mach_vm_address_t orgCosReleasePrintVaList {};
    CailAsicCapEntry *orgAsicCapsTableHWLibs = nullptr;
    CailInitAsicCapEntry *orgAsicInitCapsTable = nullptr;
    mach_vm_address_t orgPspAsdLoad {};
    mach_vm_address_t orgPspDtmLoad {};
    mach_vm_address_t orgPspHdcpLoad {};
    mach_vm_address_t orgCosDebugAssert {};
    GcFwConstant *orgGcRlcUcode = nullptr;
    GcFwConstant *orgGcMeUcode = nullptr;
    GcFwConstant *orgGcCeUcode = nullptr;
    GcFwConstant *orgGcPfpUcode = nullptr;
    GcFwConstant *orgGcMecUcode = nullptr;
    GcFwConstant *orgGcMecJtUcode = nullptr;
    SdmaFwConstant *orgSdmaUcode = nullptr;
    t_sendMsgToSmcWithParam orgRavenSendMsgToSmcWithParam = nullptr;
    t_sendMsgToSmcWithParam orgRenoirSendMsgToSmcWithParam = nullptr;
    mach_vm_address_t orgSmuRavenInitialize {};
    mach_vm_address_t orgSmuRenoirInitialize {};
    mach_vm_address_t orgVm9XWriteRegister {};
    mach_vm_address_t orgVm9XWriteRegisterExt {};
    mach_vm_address_t orgGmmCbSetMemoryAttributes {};
    mach_vm_address_t orgIpiGvmHwInit {};

    /**
     * X6000
     */
    t_HWEngineNew orgGFX10VCN2EngineNew = nullptr;
    t_HWEngineConstructor orgGFX10VCN2EngineConstructor = nullptr;
    mach_vm_address_t orgGFX10SetupAndInitializeHWCapabilities {};

    /**
     * X5000
     */
    t_HWEngineNew orgGFX9PM4EngineNew = nullptr;
    t_HWEngineConstructor orgGFX9PM4EngineConstructor = nullptr;
    t_HWEngineNew orgGFX9SDMAEngineNew = nullptr;
    t_HWEngineConstructor orgGFX9SDMAEngineConstructor = nullptr;
    mach_vm_address_t orgGetHWEngine {};
    mach_vm_address_t orgGetHWCapabilities {};
    mach_vm_address_t orgSetupAndInitializeHWCapabilities {};
    mach_vm_address_t orgDumpASICHangStateCold {};
    mach_vm_address_t orgAccelDisplayPipeWriteDiagnosisReport {};
    mach_vm_address_t orgSetMemoryAllocationsEnabled {};
    mach_vm_address_t orgPowerUpHW {};
    mach_vm_address_t orgHWsetMemoryAllocationsEnabled {};
    mach_vm_address_t orgRTGetHWChannel {};
    void *sdma0HWChannel = nullptr;
    void *sdma0HWEngine = nullptr;
    mach_vm_address_t orgMapVA {};
    mach_vm_address_t orgMapVMPT {};
    bool isThreeLevelVMPT = false;
    mach_vm_address_t orgVMMInit {};
    mach_vm_address_t orgWriteWritePTEPDECommand {};
    mach_vm_address_t orgGetPDEValue {};
    mach_vm_address_t orgGetPTEValue {};
    mach_vm_address_t orgUpdateContiguousPTEsWithDMAUsingAddr {};

    /**
     * Kernel
     */
    void mergeProperty(OSDictionary *props, const char *name, OSObject *value);
    void mergeProperties(OSDictionary *props, const char *prefix, IOService *provider);
    static bool wrapSetProperty(IORegistryEntry *that, const char *aKey, void *bytes, unsigned length);
    static OSObject *wrapGetProperty(IORegistryEntry *that, const char *aKey);

    /**
     * AMDSupport
     */
    static bool wrapNotifyLinkChange(void *atiDeviceControl, kAGDCRegisterLinkControlEvent_t event, void *eventData,
        uint32_t eventFlags);

    /**
     * X6000Framebuffer
     */
    static uint16_t wrapGetFamilyId();
    static IOReturn wrapPopulateDeviceInfo(void *that);
    static uint16_t wrapGetEnumeratedRevision(void *that);
    static uint32_t wrapGetVideoMemoryType();
    static uint32_t wrapGetVideoMemoryBitWidth();
    static IOReturn wrapPopulateVramInfo();
    static uint32_t wrapHwReadReg32(void *that, uint32_t param1);

    /**
     * X5000HWLibs
     */
    static void wrapAmdTtlServicesConstructor(IOService *that, IOPCIDevice *provider);
    static uint64_t wrapSmuGetHwVersion(uint64_t param1, uint32_t param2);
    static uint64_t wrapPspSwInit(uint32_t *param1, uint32_t *param2);
    static uint32_t wrapGcGetHwVersion(uint32_t *param1);
    static void wrapPopulateFirmwareDirectory(void *that);
    static void *wrapCreatePowerTuneServices(void *param1, void *param2);
    static uint64_t wrapSmuGetFwConstants();
    static uint64_t wrapSmuInternalHwInit();
    static void wrapCosDebugPrint(char *fmt, ...);
    static void wrapMCILDebugPrint(uint32_t level_max, char *fmt, uint64_t param3, uint64_t param4, uint64_t param5,
        uint32_t level);
    static void wrapCosDebugPrintVaList(void *ttl, char *header, char *fmt, va_list args);
    static void wrapCosReleasePrintVaList(void *ttl, char *header, char *fmt, va_list args);
    static const char *getASICName();
    static uint32_t wrapPspAsdLoad(void *pspData);
    static uint32_t wrapPspDtmLoad(void *pspData);
    static uint32_t wrapPspHdcpLoad(void *pspData);
    static void wrapCosDebugAssert(void *param1, uint8_t *param2, uint8_t *param3, uint32_t param4, uint8_t *param5);
    static void powerUpSDMA(void *smumData);
    static uint32_t wrapSmuRavenInitialize(void *smumData, uint32_t param2);
    static uint32_t wrapSmuRenoirInitialize(void *smumData, uint32_t param2);
    static uint32_t pspFeatureUnsupported();
    static void wrapVm9XWriteRegister(uint64_t *param1, uint32_t param2, uint32_t param3, uint32_t param4,
        uint32_t param5);
    static void wrapVm9XWriteRegisterExt(uint64_t *param1, uint32_t param2, uint64_t param3, uint32_t param4,
        uint32_t param5);
    static uint64_t wrapGmmCbSetMemoryAttributes(void *param1, uint32_t param2, void *param3);
    static bool wrapIpiGvmHwInit(void *ctx);
    static void wrapCgsWriteRegister(void **tlsInstance, uint32_t regIndex, uint32_t val);

    /**
     * X6000
     */
    static bool wrapGFX10AcceleratorStart();

    /**
     * X5000
     */
    static bool wrapAllocateHWEngines(void *that);
    static void wrapSetupAndInitializeHWCapabilities(void *that);
    static void wrapDumpASICHangStateCold(uint64_t param1);
    static void wrapAccelDisplayPipeWriteDiagnosisReport(void *that);
    static uint64_t wrapSetMemoryAllocationsEnabled(void *that, uint64_t param1);
    static bool wrapPowerUpHW(void *that);
    static void wrapHWsetMemoryAllocationsEnabled(void *that, bool param1);
    static void *wrapRTGetHWChannel(void *that, uint32_t param1, uint32_t param2, uint32_t param3);
    static void wrapInitializeFamilyType(void *that);
    static bool sdma1IsIdleHack(void *that);
    static bool sdma1AllocateAndInitHWRingsHack(void *that);
    static uint64_t wrapMapVA(void *that, uint64_t param1, void *memory, uint64_t param3, uint64_t sizeToMap,
        uint64_t flags);
    static uint64_t wrapMapVMPT(void *that, void *vmptCtl, uint64_t vmptLevel, uint32_t param3, uint64_t param4,
        uint64_t param5, uint64_t sizeToMap);
    static bool wrapVMMInit(void *that, void *param1);
    static uint32_t wrapWriteWritePTEPDECommand(void *that, uint32_t *buf, uint64_t pe, uint32_t count, uint64_t flags,
        uint64_t addr, uint64_t incr);
    static uint64_t wrapGetPDEValue(void *that, uint64_t param1, uint64_t param2);
    static uint64_t wrapGetPTEValue(void *that, uint64_t param1, uint64_t param2, uint64_t param3, uint32_t param4);
    static void wrapUpdateContiguousPTEsWithDMAUsingAddr(void *that, uint64_t param1, uint64_t param2, uint64_t param3,
        uint64_t param4, uint64_t param5);

    mach_vm_address_t orgFbEnableController{};
    static uint32_t wrapFbEnableController(void* that);
};

#endif /* kern_rad_hpp */
