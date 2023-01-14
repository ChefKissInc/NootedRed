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
    static constexpr size_t MaxGetFrameBufferProcs = 3;

    using t_getHWInfo = IOReturn (*)(IOService *accelVideoCtx, void *hwInfo);
    using t_createFirmware = void *(*)(const void *data, uint32_t size, uint32_t param3, const char *filename);
    using t_putFirmware = bool (*)(void *that, uint32_t deviceType, void *fw);
    using t_Vega10PowerTuneConstructor = void (*)(void *that, void *param1, void *param2);
    using t_HWEngineConstructor = void (*)(void *that);
    using t_HWEngineNew = void *(*)(size_t size);

    static RAD *callbackRAD;
    ThreadLocal<IOService *, 8> currentPropProvider;
    OSData *vbiosData = nullptr;
    ASICType asicType = ASICType::Unknown;
    void *callbackFirmwareDirectory = nullptr;

    mach_vm_address_t orgSetProperty {}, orgGetProperty {};
    mach_vm_address_t orgNotifyLinkChange {};
    uint32_t *orgDeviceTypeTable = nullptr;
    mach_vm_address_t orgAmdTtlServicesConstructor {};

    /**
     * X6000Framebuffer
     */
    mach_vm_address_t orgPopulateDeviceInfo {};
    CailAsicCapEntry *orgAsicCapsTable = nullptr;

    /**
     * X5000HWLibs
     */
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

    void mergeProperty(OSDictionary *props, const char *name, OSObject *value);
    void mergeProperties(OSDictionary *props, const char *prefix, IOService *provider);

    /**
     * X6000Framebuffer
     */
    static uint16_t wrapGetFamilyId();
    static IOReturn wrapPopulateDeviceInfo(void *that);
    static uint16_t wrapGetEnumeratedRevision(void *that);
    static uint32_t wrapGetVideoMemoryType(void *that);
    static uint32_t wrapGetVideoMemoryBitWidth(void *that);
    static IOReturn wrapPopulateVramInfo(void *that, void *param1);

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

    /**
     * X6000
     */
    static bool wrapGFX10AcceleratorStart();

    /**
     * X5000
     */
    static bool wrapAllocateHWEngines(void *that);
    static void wrapSetupAndInitializeHWCapabilities(void *that);

    void processHardwareKext(KernelPatcher &patcher, size_t hwIndex, mach_vm_address_t address, size_t size);

    static bool wrapSetProperty(IORegistryEntry *that, const char *aKey, void *bytes, unsigned length);
    static OSObject *wrapGetProperty(IORegistryEntry *that, const char *aKey);
    static bool wrapNotifyLinkChange(void *atiDeviceControl, kAGDCRegisterLinkControlEvent_t event, void *eventData,
        uint32_t eventFlags);

    mach_vm_address_t orgDumpASICHangStateCold {};
    static void wrapDumpASICHangStateCold(uint64_t param1);

    mach_vm_address_t orgAccelDisplayPipeWriteDiagnosisReport {};
    static void wrapAccelDisplayPipeWriteDiagnosisReport(void *that);

    mach_vm_address_t orgSetMemoryAllocationsEnabled {};
    static uint64_t wrapSetMemoryAllocationsEnabled(void *that, uint64_t param1);

    mach_vm_address_t orgPowerUpHW {};
    static bool wrapPowerUpHW(void *that);

    mach_vm_address_t orgHWsetMemoryAllocationsEnabled {};
    static void wrapHWsetMemoryAllocationsEnabled(void *that, bool param1);

    mach_vm_address_t orgRTGetHWChannel {};
    static void *wrapRTGetHWChannel(void *that, uint32_t param1, uint32_t param2, uint32_t param3);

    mach_vm_address_t orgCosDebugAssert {};
    static void wrapCosDebugAssert(void *param1, uint8_t *param2, uint8_t *param3, uint32_t param4, uint8_t *param5);

    GcFwConstant *orgGcRlcUcode = nullptr;
    GcFwConstant *orgGcMeUcode = nullptr;
    GcFwConstant *orgGcCeUcode = nullptr;
    GcFwConstant *orgGcPfpUcode = nullptr;
    GcFwConstant *orgGcMecUcode = nullptr;
    GcFwConstant *orgGcMecJtUcode = nullptr;
    SdmaFwConstant *orgSdmaUcode = nullptr;

    mach_vm_address_t orgHwReadReg32 {};
    static uint32_t wrapHwReadReg32(void *that, uint32_t param1);

    using t_sendMsgToSmcWithParam = uint32_t (*)(void *smumData, uint32_t msgId, uint32_t parameter);
    t_sendMsgToSmcWithParam orgRavenSendMsgToSmcWithParam = nullptr;
    t_sendMsgToSmcWithParam orgRenoirSendMsgToSmcWithParam = nullptr;

    static void powerUpSDMA(void *smumData);

    mach_vm_address_t orgSmuRavenInitialize {};
    static uint32_t wrapSmuRavenInitialize(void *smumData, uint32_t param2);

    mach_vm_address_t orgSmuRenoirInitialize {};
    static uint32_t wrapSmuRenoirInitialize(void *smumData, uint32_t param2);

    void *sdma0HWChannel = nullptr;

    static bool sdma1IsIdleHack(void *that);

    void *sdma0HWEngine = nullptr;

    static bool sdma1AllocateAndInitHWRingsHack(void *that);

    mach_vm_address_t orgMapVA {};
    static uint64_t wrapMapVA(void *that, uint64_t param1, void *memory, uint64_t param3, uint64_t sizeToMap,
        uint64_t flags);

    mach_vm_address_t orgMapVMPT {};
    static uint64_t wrapMapVMPT(void *that, void *vmptCtl, uint64_t vmptLevel, uint32_t param3, uint64_t param4,
        uint64_t param5, uint64_t sizeToMap);

    bool isThreeLevelVMPT = false;

    mach_vm_address_t orgVMMInit {};
    static bool wrapVMMInit(void *that, void *param1);

    mach_vm_address_t orgWriteWritePTEPDECommand {};
    static uint32_t wrapWriteWritePTEPDECommand(void *that, uint32_t *buf, uint64_t pe, uint32_t count, uint64_t flags,
        uint64_t addr, uint64_t incr);

    mach_vm_address_t orgGetPDEValue {};
    static uint64_t wrapGetPDEValue(void *that, uint64_t param1, uint64_t param2);

    mach_vm_address_t orgGetPTEValue {};
    static uint64_t wrapGetPTEValue(void *that, uint64_t param1, uint64_t param2, uint64_t param3, uint32_t param4);

    mach_vm_address_t orgUpdateContiguousPTEsWithDMAUsingAddr {};
    static void wrapUpdateContiguousPTEsWithDMAUsingAddr(void *that, uint64_t param1, uint64_t param2, uint64_t param3,
        uint64_t param4, uint64_t param5);

    static void wrapInitializeFamilyType(void *that);

    static uint32_t wrapPspXgmiIsSupport();
    static uint32_t wrapPspRapIsSupported();

    mach_vm_address_t orgVm9XWriteRegister {};
    static void wrapVm9XWriteRegister(uint64_t *param1, uint32_t param2, uint32_t param3, uint32_t param4,
        uint32_t param5);

    mach_vm_address_t orgVm9XWriteRegisterExt {};
    static void wrapVm9XWriteRegisterExt(uint64_t *param1, uint32_t param2, uint64_t param3, uint32_t param4,
        uint32_t param5);

    mach_vm_address_t orgGmmCbSetMemoryAttributes {};
    static uint64_t wrapGmmCbSetMemoryAttributes(void *param1, uint32_t param2, void *param3);

    using t_TtlIsApuDevice = bool (*)(void *param_1);
    t_TtlIsApuDevice orgTtlIsApuDevice = nullptr;

    mach_vm_address_t orgIpiGvmHwInit {};
    static bool wrapIpiGvmHwInit(void *ctx);

    static void wrapCgsWriteRegister(void **tlsInstance, uint32_t regIndex, uint32_t val);
};

#endif /* kern_rad_hpp */
