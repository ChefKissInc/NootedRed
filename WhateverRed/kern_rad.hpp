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
#include "kern_con.hpp"
#include "stdint.h"
#include <Headers/kern_devinfo.hpp>
#include <Headers/kern_patcher.hpp>
#include <IOKit/IOService.h>
#include <IOKit/graphics/IOFramebuffer.h>
#include <IOKit/pci/IOPCIDevice.h>

class RAD {
    public:
    void init();
    void deinit();

    void processKernel(KernelPatcher &patcher);
    bool processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size);

    private:
    static constexpr size_t MaxGetFrameBufferProcs = 3;

    using t_getAtomObjectTableForType = void *(*)(void *that, AtomObjectTableType type, uint8_t *sz);
    using t_getHWInfo = IOReturn (*)(IOService *accelVideoCtx, void *hwInfo);
    using t_createFirmware = void *(*)(const void *data, uint32_t size, uint32_t param3, const char *filename);
    using t_putFirmware = bool (*)(void *that, uint32_t deviceType, void *fw);
    using t_Vega10PowerTuneConstructor = void (*)(void *that, void *param1, void *param2);
    using t_HWEngineConstructor = void (*)(void *that);
    using t_HWEngineNew = void *(*)(size_t size);

    static RAD *callbackRAD;
    ThreadLocal<IOService *, 8> currentPropProvider;
    OSData *vbiosData = nullptr;

    mach_vm_address_t orgSetProperty {}, orgGetProperty {}, orgGetConnectorsInfoV2 {};
    mach_vm_address_t orgGetConnectorsInfoV1 {}, orgTranslateAtomConnectorInfoV1 {};
    mach_vm_address_t orgTranslateAtomConnectorInfoV2 {}, orgATIControllerStart {};
    mach_vm_address_t orgNotifyLinkChange {}, orgGetHWInfo[1] {};
    uint32_t *orgDeviceTypeTable = nullptr;
    mach_vm_address_t orgAmdTtlServicesConstructor {};
    mach_vm_address_t orgGetState {}, orgInitializeTtl {};
    mach_vm_address_t orgCreateAtomBiosProxy {};
    mach_vm_address_t orgPopulateDeviceMemory {}, orgQueryComputeQueueIsIdle {};
    mach_vm_address_t orgAMDHWChannelWaitForIdle {};

    /**
     * X6000Framebuffer
     */
    mach_vm_address_t orgPopulateDeviceInfo {};
    mach_vm_address_t orgGetVideoMemoryType {}, orgGetVideoMemoryBitWidth {};
    CailAsicCapEntry *orgAsicCapsTable = nullptr;

    /**
     * X5000HWLibs
     */
    mach_vm_address_t orgIpiSmuSwInit {}, orgSmuSwInit {};
    mach_vm_address_t orgSmuInternalSwInit {}, orgSmuGetHwVersion {};
    mach_vm_address_t orgPspSwInit {}, orgGcGetHwVersion {};
    mach_vm_address_t orgInternalCosReadFw {}, orgPopulateFirmwareDirectory {};
    t_createFirmware orgCreateFirmware = nullptr;
    t_putFirmware orgPutFirmware = nullptr;
    mach_vm_address_t orgMCILUpdateGfxCGPG {}, orgQueryEngineRunningState {};
    mach_vm_address_t orgCAILQueryEngineRunningState {};
    mach_vm_address_t orgCailMonitorEngineInternalState {};
    mach_vm_address_t orgCailMonitorPerformanceCounter {};
    mach_vm_address_t orgSMUMInitialize {};
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

    bool force24BppMode = false;
    bool dviSingleLink = false;

    void mergeProperty(OSDictionary *props, const char *name, OSObject *value);
    void mergeProperties(OSDictionary *props, const char *prefix, IOService *provider);
    void applyPropertyFixes(IOService *service, uint32_t connectorNum = 0);
    void updateConnectorsInfo(void *atomutils, t_getAtomObjectTableForType gettable, IOService *ctrl,
        RADConnectors::Connector *connectors, uint8_t *sz);
    void autocorrectConnectors(uint8_t *baseAddr, AtomDisplayObjectPath *displayPaths, uint8_t displayPathNum,
        AtomConnectorObject *connectorObjects, uint8_t connectorObjectNum, RADConnectors::Connector *connectors,
        uint8_t sz);
    void autocorrectConnector(uint8_t connector, uint8_t sense, uint8_t txmit, uint8_t enc,
        RADConnectors::Connector *connectors, uint8_t sz);
    void reprioritiseConnectors(const uint8_t *senseList, uint8_t senseNum, RADConnectors::Connector *connectors,
        uint8_t sz);

    void process24BitOutput(KernelPatcher &patcher, KernelPatcher::KextInfo &info, mach_vm_address_t address,
        size_t size);
    void processConnectorOverrides(KernelPatcher &patcher, mach_vm_address_t address, size_t size);
    static uint64_t wrapGetState(void *that);
    static bool wrapInitializeTtl(void *that, void *param1);
    static void *wrapCreateAtomBiosProxy(void *param1);
    static IOReturn wrapPopulateDeviceMemory(void *that, uint32_t reg);
    static IOReturn wrapQueryComputeQueueIsIdle(void *that, uint64_t param1);
    static bool wrapAMDHWChannelWaitForIdle(void *that, uint64_t param1);

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
    static uint64_t wrapIpiSmuSwInit(void *tlsInstance);
    static uint64_t wrapSmuSwInit(void *input, uint64_t *output);
    static uint32_t wrapSmuInternalSwInit(uint64_t param1, uint64_t param2, void *param3);
    static uint64_t wrapSmuGetHwVersion(uint64_t param1, uint32_t param2);
    static uint64_t wrapPspSwInit(uint32_t *param1, uint32_t *param2);
    static uint32_t wrapGcGetHwVersion(uint32_t *param1);
    static uint32_t wrapInternalCosReadFw(uint64_t param1, uint64_t *param2);
    static void wrapPopulateFirmwareDirectory(void *that);
    static uint64_t wrapMCILUpdateGfxCGPG(void *param1);
    static IOReturn wrapQueryEngineRunningState(void *that, void *param1, void *param2);
    static uint64_t wrapCAILQueryEngineRunningState(void *param1, uint32_t *param2, uint64_t param3);
    static uint64_t wrapCailMonitorEngineInternalState(void *that, uint32_t param1, uint32_t *param2);
    static uint64_t wrapCailMonitorPerformanceCounter(void *that, uint32_t *param1);
    static uint64_t wrapSMUMInitialize(uint64_t param1, uint32_t *param2, uint64_t param3);
    static void *wrapCreatePowerTuneServices(void *param1, void *param2);
    static uint64_t wrapSmuGetFwConstants();
    static uint64_t wrapSmuInternalHwInit();
    static void wrapCosDebugPrint(char *fmt, ...);
    static void wrapMCILDebugPrint(uint32_t level_max, char *fmt, uint64_t param3, uint64_t param4, uint64_t param5,
        uint level);
    static void wrapCosDebugPrintVaList(void *ttl, char *header, char *fmt, va_list args);
    static void wrapCosReleasePrintVaList(void *ttl, char *header, char *fmt, va_list args);
    static const char *getChipName();
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
    static void *wrapGetHWEngine(void *that, uint32_t engineType);
    static void wrapSetupAndInitializeHWCapabilities(void *that);

    void processHardwareKext(KernelPatcher &patcher, size_t hwIndex, mach_vm_address_t address, size_t size);

    static bool wrapSetProperty(IORegistryEntry *that, const char *aKey, void *bytes, unsigned length);
    static OSObject *wrapGetProperty(IORegistryEntry *that, const char *aKey);
    static uint32_t wrapGetConnectorsInfoV1(void *that, RADConnectors::Connector *connectors, uint8_t *sz);
    static uint32_t wrapGetConnectorsInfoV2(void *that, RADConnectors::Connector *connectors, uint8_t *sz);
    static uint32_t wrapTranslateAtomConnectorInfoV1(void *that, RADConnectors::AtomConnectorInfo *info,
        RADConnectors::Connector *connector);
    static uint32_t wrapTranslateAtomConnectorInfoV2(void *that, RADConnectors::AtomConnectorInfo *info,
        RADConnectors::Connector *connector);
    static bool wrapATIControllerStart(IOService *ctrl, IOService *provider);
    static bool wrapNotifyLinkChange(void *atiDeviceControl, kAGDCRegisterLinkControlEvent_t event, void *eventData,
        uint32_t eventFlags);
    static bool doNotTestVram(IOService *ctrl, uint32_t reg, bool retryOnFail);
    static void updateGetHWInfo(IOService *accelVideoCtx, void *hwInfo);

    mach_vm_address_t orgPM4EnginePowerUp {};
    static bool wrapPM4EnginePowerUp(void *that);

    mach_vm_address_t orgDumpASICHangStateCold {};
    static void wrapDumpASICHangStateCold(uint64_t param1);

    mach_vm_address_t orgAccelStart {};
    void *callbackAccelerator = nullptr;
    static bool wrapAccelStart(void *that, IOService *provider);
    using t_writeDiagnosisReport = void (*)(void *that, char **buf, size_t *size);
    t_writeDiagnosisReport orgWriteDiagnosisReport = nullptr;

    mach_vm_address_t orgGFX9RTRingGetHead {};
    static uint64_t wrapGFX9RTRingGetHead(void *that);

    mach_vm_address_t orgGFX10RTRingGetHead {};
    static uint64_t wrapGFX10RTRingGetHead(void *that);

    mach_vm_address_t orgHwRegRead {};
    static uint64_t wrapHwRegRead(void *that, uint64_t addr);

    mach_vm_address_t orgHwRegWrite {};
    static uint64_t wrapHwRegWrite(void *that, uint64_t addr, uint64_t val);

    mach_vm_address_t orgAccelDisplayPipeWriteDiagnosisReport {};
    static void wrapAccelDisplayPipeWriteDiagnosisReport(void *that);

    mach_vm_address_t orgSetMemoryAllocationsEnabled {};
    static uint64_t wrapSetMemoryAllocationsEnabled(void *that, uint64_t param1);

    mach_vm_address_t orgGetEventMachine {};
    static uint64_t wrapGetEventMachine(void *that);

    mach_vm_address_t orgGetVMUpdateChannel {};
    static uint64_t wrapGetVMUpdateChannel(void *that);

    mach_vm_address_t orgCreateVMCommandBufferPool {};
    static uint64_t wrapCreateVMCommandBufferPool(void *that, void *param1, uint64_t param2, uint64_t param3);

    mach_vm_address_t orgPoolGetChannel {};
    static uint64_t wrapPoolGetChannel(void *that);

    mach_vm_address_t orgAccelGetHWChannel {};
    static uint64_t wrapAccelGetHWChannel(void *that);

    mach_vm_address_t orgCreateAccelChannels {};
    static uint64_t wrapCreateAccelChannels(void *that, uint64_t param1);

    mach_vm_address_t orgPopulateAccelConfig {};
    static uint64_t wrapPopulateAccelConfig(void *that, void *param1);

    mach_vm_address_t orgPowerUpHW {};
    static bool wrapPowerUpHW(void *that);

    mach_vm_address_t orgHWsetMemoryAllocationsEnabled {};
    static void wrapHWsetMemoryAllocationsEnabled(void *that, bool param1);

    mach_vm_address_t orgAccelCallPlatformFunction {};
    static uint64_t wrapAccelCallPlatformFunction(void *param1, uint64_t param2, void *param3, void *param4,
        void *param5, void *param6, void *param7);

    mach_vm_address_t orgVega10PowerUp {};
    static bool wrapVega10PowerUp(void *that);

    mach_vm_address_t orgCreateBltMgr {};
    static uint64_t wrapCreateBltMgr(void *that);

    mach_vm_address_t orgPowerUpHWEngines {};
    static bool wrapPowerUpHWEngines(void *that);

    mach_vm_address_t orgStartHWEngines {};
    static bool wrapStartHWEngines(void *that);

    mach_vm_address_t orgMicroEngineControlLoadMicrocode {};
    static uint64_t wrapMicroEngineControlLoadMicrocode(void *that, void *param1);

    mach_vm_address_t orgMicroEngineControlInitializeEngine {};
    static uint64_t wrapMicroEngineControlInitializeEngine(void *that, void *param1, void *param2);

    mach_vm_address_t orgMicroEngineControlStartEngine {};
    static uint64_t wrapMicroEngineControlStartEngine(void *that, void *param1);

    mach_vm_address_t orgSdmaEngineStart {};
    static bool wrapSdmaEngineStart(void *that);

    mach_vm_address_t orgRtRingEnable {};
    static uint64_t wrapRtRingEnable(void *that);

    mach_vm_address_t orgCailMCILTrace0 {};
    static void wrapCailMCILTrace0(void *that);

    mach_vm_address_t orgCailMCILTrace1 {};
    static void wrapCailMCILTrace1(void *that);

    mach_vm_address_t orgCailMCILTrace2 {};
    static void wrapCailMCILTrace2(void *that);

    mach_vm_address_t orgGreenlandLoadRlcUcode {};
    static uint64_t wrapGreenlandLoadRlcUcode(void *param1);

    mach_vm_address_t orgGreenlandMicroEngineControl {};
    static uint64_t wrapGreenlandMicroEngineControl(void *param1, uint64_t param2, void *param3);

    mach_vm_address_t orgSdmaMicroEngineControl {};
    static uint64_t wrapSdmaMicroEngineControl(void *param1, void *param2, void *param3);

    mach_vm_address_t orgWaitForHwStamp {};
    static bool wrapWaitForHwStamp(void *that, uint64_t param1);

    mach_vm_address_t orgRTGetHWChannel {};
    static uint64_t wrapRTGetHWChannel(void *that, uint32_t param1, uint32_t param2, uint32_t param3);

    uint32_t *orgChannelTypes = nullptr;

    mach_vm_address_t orgSdmaSwInit {};
    static uint32_t wrapSdmaSwInit(uint32_t *param1, uint32_t *param2);

    mach_vm_address_t orgSdmaAssertion {};
    static void wrapSdmaAssertion(uint64_t param1, bool param2, uint8_t *param3, uint8_t *param4, uint64_t param5,
        uint8_t *param6);

    mach_vm_address_t orgSdmaGetHwVersion {};
    static uint64_t wrapSdmaGetHwVersion(uint32_t param1, uint32_t param2);

    mach_vm_address_t orgTtlDevIsVega10Device {};
    static bool wrapTtlDevIsVega10Device(void *param1);

    mach_vm_address_t orgCosDebugAssert {};
    static void wrapCosDebugAssert(void *param1, uint8_t *param2, uint8_t *param3, uint32_t param4, uint8_t *param5);

    mach_vm_address_t orgIpiSdmaHwInit{};
    static bool wrapIpiSdmaHwInit(void* ctx);

    mach_vm_address_t orgIpiSdmaHwInitInstance{};
    static uint64_t wrapIpiSdmaHwInitInstance(void* param1, uint32_t* param2);
};

#endif /* kern_rad_hpp */
