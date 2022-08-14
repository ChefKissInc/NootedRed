//
//  kern_rad.hpp
//  WhateverRed
//
//  Copyright © 2017 vit9696. All rights reserved.
//  Copyright © 2022 VisualDevelopment. All rights reserved.
//

#ifndef kern_rad_hpp
#define kern_rad_hpp

#include <IOKit/IOService.h>
#include <IOKit/graphics/IOFramebuffer.h>
#include <IOKit/pci/IOPCIDevice.h>

#include <Headers/kern_devinfo.hpp>
#include <Headers/kern_patcher.hpp>

#include "kern_agdc.hpp"
#include "kern_atom.hpp"
#include "kern_con.hpp"

class RAD {
   public:
    void init();
    void deinit();

    void processKernel(KernelPatcher &patcher, DeviceInfo *info);
    bool processKext(KernelPatcher &patcher, size_t index,
                     mach_vm_address_t address, size_t size);

   private:
    static constexpr size_t MaxGetFrameBufferProcs = 3;

    using t_getAtomObjectTableForType = void *(*)(void *that,
                                                  AtomObjectTableType type,
                                                  uint8_t *sz);
    using t_getHWInfo = IOReturn (*)(IOService *accelVideoCtx, void *hwInfo);
    using t_createFirmware = void *(*)(const void *data, uint32_t size,
                                       uint32_t param3, const char *filename);
    using t_putFirmware = bool (*)(void *that, uint32_t deviceType, void *fw);
    using t_Vega10PowerTuneServicesConstructor = void (*)(void *that,
                                                          void *param1,
                                                          void *param2);

    static RAD *callbackRAD;
    ThreadLocal<IOService *, 8> currentPropProvider;

    mach_vm_address_t orgPanic{};
    mach_vm_address_t orgSetProperty{}, orgGetProperty{},
        orgGetConnectorsInfoV2{};
    mach_vm_address_t orgGetConnectorsInfoV1{},
        orgTranslateAtomConnectorInfoV1{};
    mach_vm_address_t orgTranslateAtomConnectorInfoV2{},
        orgATIControllerStart{};
    mach_vm_address_t orgNotifyLinkChange{}, orgPopulateAccelConfig[1]{},
        orgGetHWInfo[1]{};
    mach_vm_address_t orgConfigureDevice{}, orgInitLinkToPeer{},
        orgCreateHWHandler{};
    mach_vm_address_t orgCreateHWInterface{}, orgGetHWMemory{},
        orgGetATIChipConfigBit{};
    mach_vm_address_t orgAllocateAMDHWRegisters{}, orgSetupCAIL{},
        orgInitializeHWWorkarounds{};
    mach_vm_address_t orgAllocateAMDHWAlignManager{}, orgMapDoorbellMemory{};
    mach_vm_address_t orgHwInitializeFbMemSize{}, orgHwInitializeFbBase{},
        orgInitWithController{};
    mach_vm_address_t orgDeviceTypeTable{}, orgAmdTtlServicesConstructor{};
    mach_vm_address_t orgGetState{}, orgConfRegBase{}, orgReadChipRev{};
    mach_vm_address_t orgInitializeTtl{},
        orgInitializeProjectDependentResources{};
    mach_vm_address_t orgCreateAtomBiosProxy{}, orgInitializeResources{};
    mach_vm_address_t orgPopulateDeviceMemory{}, orgQueryComputeQueueIsIdle{};
    mach_vm_address_t orgAMDHWChannelWaitForIdle{}, orgAcceleratorPowerUpHw{};
    mach_vm_address_t orgInitializePP{}, orgCreatePowerPlayInterface{};
    mach_vm_address_t orgSendRequestToAccelerator{}, orgPPInitialize{};
    mach_vm_address_t orgUpdatePowerPlay{}, orgIsReady{};
    mach_vm_address_t orgPopulateDeviceInfo{};

    /* X5000HWLibs */
    mach_vm_address_t orgIpiSmuSwInit{}, orgSmuSwInit{};
    mach_vm_address_t orgSmuInternalSwInit{}, orgSmuGetHwVersion{},
        orgPspSwInit{};
    mach_vm_address_t orgGcGetHwVersion{}, orgInternalCosReadFw{},
        orgPopulateFirmwareDirectory{};
    t_createFirmware orgCreateFirmware = nullptr;
    t_putFirmware orgPutFirmware = nullptr;
    mach_vm_address_t orgGetGpuHwConstants{}, orgMCILUpdateGfxCGPG{},
        orgQueryEngineRunningState{};
    mach_vm_address_t orgCAILQueryEngineRunningState{},
        orgCailMonitorEngineInternalState{};
    mach_vm_address_t orgCailMonitorPerformanceCounter{}, orgPpEnable{};
    mach_vm_address_t orgPpDisplayConfigChange{}, orgPECISetupInitInfo{};
    mach_vm_address_t orgPECIReadRegistry{}, orgSMUMInitialize{},
        orgPECIRetrieveBiosDataTable{};
    t_Vega10PowerTuneServicesConstructor orgVega10PowerTuneServicesConstructor =
        nullptr;
    mach_vm_address_t orgCosDebugPrint{}, orgMCILDebugPrint{};
    mach_vm_address_t orgPspAsdLoad{}, orgPspDtmLoad{},
        orgCosDebugPrintVaList{};
    mach_vm_address_t orgCosReleasePrintVaList{};
    /* ----------- */

    template <size_t Index>
    static IOReturn populateGetHWInfo(IOService *accelVideoCtx, void *hwInfo) {
        if (callbackRAD->orgGetHWInfo[Index]) {
            int ret = FunctionCast(populateGetHWInfo<Index>,
                                   callbackRAD->orgGetHWInfo[Index])(
                accelVideoCtx, hwInfo);
            callbackRAD->updateGetHWInfo(accelVideoCtx, hwInfo);
            return ret;
        } else
            SYSLOG("rad", "populateGetHWInfo invalid use for %lu", Index);

        return kIOReturnInvalid;
    }

    t_getHWInfo wrapGetHWInfo[1] = {populateGetHWInfo<0>};

    const char *getHWInfoProcNames[1] = {
        "__ZN35AMDRadeonX5000_AMDAccelVideoContext9getHWInfoEP13sHardwareInfo",
    };

    bool force24BppMode = false;
    bool dviSingleLink = false;

    bool fixConfigName = false;
    bool enableGvaSupport = false;
    bool forceVesaMode = false;
    bool forceCodecInfo = false;
    size_t maxHardwareKexts = 1;

    void initHardwareKextMods();
    void mergeProperty(OSDictionary *props, const char *name, OSObject *value);
    void mergeProperties(OSDictionary *props, const char *prefix,
                         IOService *provider);
    void applyPropertyFixes(IOService *service, uint32_t connectorNum = 0);
    void updateConnectorsInfo(void *atomutils,
                              t_getAtomObjectTableForType gettable,
                              IOService *ctrl,
                              RADConnectors::Connector *connectors,
                              uint8_t *sz);
    void autocorrectConnectors(uint8_t *baseAddr,
                               AtomDisplayObjectPath *displayPaths,
                               uint8_t displayPathNum,
                               AtomConnectorObject *connectorObjects,
                               uint8_t connectorObjectNum,
                               RADConnectors::Connector *connectors,
                               uint8_t sz);
    void autocorrectConnector(uint8_t connector, uint8_t sense, uint8_t txmit,
                              uint8_t enc, RADConnectors::Connector *connectors,
                              uint8_t sz);
    void reprioritiseConnectors(const uint8_t *senseList, uint8_t senseNum,
                                RADConnectors::Connector *connectors,
                                uint8_t sz);

    template <size_t Index>
    static void populateAccelConfig(IOService *accelService,
                                    const char **accelConfig) {
        if (callbackRAD->orgPopulateAccelConfig[Index]) {
            FunctionCast(populateAccelConfig<Index>,
                         callbackRAD->orgPopulateAccelConfig[Index])(
                accelService, accelConfig);
            callbackRAD->updateAccelConfig(Index, accelService, accelConfig);
        } else {
            SYSLOG("rad", "populateAccelConfig invalid use for %lu", Index);
        }
    }

    [[noreturn]] [[gnu::cold]] static void wrapPanic(
        const char *panic_format_str, ...);
    [[noreturn]] [[gnu::cold]] static void wrapEnterDebugger(const char *cause);

    void process24BitOutput(KernelPatcher &patcher,
                            KernelPatcher::KextInfo &info,
                            mach_vm_address_t address, size_t size);
    void processConnectorOverrides(KernelPatcher &patcher,
                                   mach_vm_address_t address, size_t size);
    static IOReturn wrapProjectByPartNumber();
    static IOReturn wrapInitializeProjectDependentResources(void *that);
    static IOReturn wrapHwInitializeFbMemSize(void *that);
    static IOReturn wrapHwInitializeFbBase(void *that);
    static uint64_t wrapInitWithController(void *that, void *controller);
    static uint64_t wrapConfigureDevice(void *that, IOPCIDevice *dev);
    static uint64_t wrapCreateHWHandler(void *that);
    static uint64_t wrapCreateHWInterface(void *that, IOPCIDevice *dev);
    static uint64_t wrapGetHWMemory(void *that);
    static uint64_t wrapGetATIChipConfigBit(void *that);
    static uint64_t wrapAllocateAMDHWRegisters(void *that);
    static bool wrapSetupCAIL(void *that);
    static uint64_t wrapInitializeHWWorkarounds(void *that);
    static uint64_t wrapAllocateAMDHWAlignManager(void *that);
    static bool wrapMapDoorbellMemory(void *that);
    static IOService *wrapInitLinkToPeer(void *that,
                                         const char *matchCategoryName);
    static uint64_t wrapGetState(void *that);
    static bool wrapInitializeTtl(void *that, void *param1);
    static uint64_t wrapConfRegBase(void *that);
    static uint8_t wrapReadChipRev(void *that);
    static void *wrapCreateAtomBiosProxy(void *param1);
    static IOReturn wrapInitializeResources(void *that);
    static IOReturn wrapPopulateDeviceMemory(void *that, uint32_t reg);
    static IntegratedVRAMInfoInterface *createVramInfo(void *helper,
                                                       uint32_t offset);
    static IOReturn wrapQueryComputeQueueIsIdle(void *that, uint64_t param1);
    static bool wrapAMDHWChannelWaitForIdle(void *that, uint64_t param1);
    static uint64_t wrapAcceleratorPowerUpHw(void *that);
    static IOReturn wrapInitializePP(void *that);
    static IOReturn wrapCreatePowerPlayInterface(void *that);
    static IOReturn wrapSendRequestToAccelerator(void *that, uint32_t param1,
                                                 void *param2, void *param3,
                                                 void *param4);
    static IOReturn wrapPPInitialize(void *that);
    static uint16_t wrapGetFamilyId();
    static IOReturn wrapPopulateDeviceInfo(void *that);

    /* X5000HWLibs */
    static void wrapAmdTtlServicesConstructor(IOService *that,
                                              IOPCIDevice *provider);
    static uint64_t wrapIpiSmuSwInit(void *tlsInstance);
    static uint64_t wrapSmuSwInit(void *input, uint64_t *output);
    static uint32_t wrapSmuInternalSwInit(uint64_t param1, uint64_t param2,
                                          void *param3);
    static uint64_t wrapSmuGetHwVersion(uint64_t param1, uint32_t param2);
    static uint64_t wrapPspSwInit(uint32_t *param1, uint32_t *param2);
    static uint32_t wrapGcGetHwVersion(uint32_t *param1);
    static uint32_t wrapInternalCosReadFw(uint64_t param1, uint64_t *param2);
    static void wrapPopulateFirmwareDirectory(void *that);
    static void *wrapGetGpuHwConstants(uint8_t *param1);
    static uint64_t wrapMCILUpdateGfxCGPG(void *param1);
    static IOReturn wrapQueryEngineRunningState(void *that, void *param1,
                                                void *param2);
    static uint64_t wrapCAILQueryEngineRunningState(void *param1,
                                                    uint32_t *param2,
                                                    uint64_t param3);
    static uint64_t wrapCailMonitorEngineInternalState(void *that,
                                                       uint32_t param1,
                                                       uint32_t *param2);
    static uint64_t wrapCailMonitorPerformanceCounter(void *that,
                                                      uint32_t *param1);
    static IOReturn wrapPpEnable(void *that, bool param1);
    static IOReturn wrapUpdatePowerPlay(void *that);
    static bool wrapIsReady(void *that);
    static IOReturn wrapPpDisplayConfigChange(void *that, void *param1,
                                              void *param2);
    static uint64_t wrapPECISetupInitInfo(uint32_t *param1, uint32_t *param2);
    static uint64_t wrapPECIReadRegistry(void *param1, char *key,
                                         uint64_t param3, uint64_t param4);
    static uint64_t wrapSMUMInitialize(uint64_t param1, uint32_t *param2,
                                       uint64_t param3);
    static uint64_t wrapPECIRetrieveBiosDataTable(void *param1, uint64_t param2,
                                                  uint64_t **param3);
    static void *wrapCreatePowerTuneServices(void *param1, void *param2);
    static uint32_t wrapGetHwRevision(uint32_t major, uint32_t minor,
                                      uint32_t patch);
    static uint64_t wrapSmuGetFwConstants();
    static bool wrapTtlDevIsVega10Device();
    static uint64_t wrapSmu901InternalHwInit();
    static void wrapCosDebugPrint(char *fmt, ...);
    static void wrapMCILDebugPrint(uint32_t level_max, char *fmt,
                                   uint64_t param3, uint64_t param4,
                                   uint64_t param5, uint level);
    static uint64_t wrapPspAsdLoad(void *pspData);
    static uint64_t wrapPspDtmLoad(void *pspData);
    static uint64_t wrapPspPowerPlaySupported();
    static void wrapCosDebugPrintVaList(void *ttl, char *header, char *fmt,
                                        va_list args);
    static void wrapCosReleasePrintVaList(void *ttl, char *header, char *fmt,
                                          va_list args);
    /* ----------- */

    void processHardwareKext(KernelPatcher &patcher, size_t hwIndex,
                             mach_vm_address_t address, size_t size);
    void updateAccelConfig(size_t hwIndex, IOService *accelService,
                           const char **accelConfig);

    static bool wrapSetProperty(IORegistryEntry *that, const char *aKey,
                                void *bytes, unsigned length);
    static OSObject *wrapGetProperty(IORegistryEntry *that, const char *aKey);
    static uint32_t wrapGetConnectorsInfoV1(
        void *that, RADConnectors::Connector *connectors, uint8_t *sz);
    static uint32_t wrapGetConnectorsInfoV2(
        void *that, RADConnectors::Connector *connectors, uint8_t *sz);
    static uint32_t wrapTranslateAtomConnectorInfoV1(
        void *that, RADConnectors::AtomConnectorInfo *info,
        RADConnectors::Connector *connector);
    static uint32_t wrapTranslateAtomConnectorInfoV2(
        void *that, RADConnectors::AtomConnectorInfo *info,
        RADConnectors::Connector *connector);
    static bool wrapATIControllerStart(IOService *ctrl, IOService *provider);
    static bool wrapNotifyLinkChange(void *atiDeviceControl,
                                     kAGDCRegisterLinkControlEvent_t event,
                                     void *eventData, uint32_t eventFlags);
    static bool doNotTestVram(IOService *ctrl, uint32_t reg, bool retryOnFail);
    static void updateGetHWInfo(IOService *accelVideoCtx, void *hwInfo);
};

#endif /* kern_rad_hpp */
