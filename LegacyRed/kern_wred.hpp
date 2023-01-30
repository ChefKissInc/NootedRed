//  Copyright © 2023 ChefKiss Inc and Zorm Industries. Licensed under the Thou Shalt Not Profit License version 1.0. See LICENSE for
//  details.

#pragma once
#include "kern_amd.hpp"
#include <Headers/kern_devinfo.hpp>
#include <Headers/kern_iokit.hpp>

enum struct ASICType {
    Raven,
    Raven2,
    Picasso,
    Renoir,
    Carrizo,
    Czl,
    BmMl,
    Unknown,
};

class LRed {
    public:
    static LRed *callbackLRed
    void init();
    void deinit();

    void processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size);

    private:
    static const char *getASICName() {
        switch (callbackWRed->asicType) {
            case ASICType::Picasso:
                PANIC("lred", "ASIC Higher than Carrizo/Stoney!");
            case ASICType::Raven:
                PANIC("lred", "ASIC Higher than Carrizo/Stoney!");
            case ASICType::Raven2:
                PANIC("lred", "ASIC Higher than Carrizo/Stoney!");
            case ASICType::Renoir:
                PANIC("lred", "ASIC Higher than Carrizo/Stoney!");
            case ASICType::Carizzo:
                return "carrizo";
            case ASICType::CzL:
                return "mullins";
            case ASICType::BmMl:
                return "mullins";
            default:
                PANIC("lred", "Unknown ASIC type");
        }
    }

    OSData *vbiosData = nullptr;
    ASICType asicType = ASICType::Unknown;
    void *callbackFirmwareDirectory = nullptr;

    mach_vm_address_t orgSetProperty {}, orgGetProperty {}, orgGetConnectorsInfoV2 {};
    mach_vm_address_t orgGetConnectorsInfoV1 {}, orgTranslateAtomConnectorInfoV1 {};
    mach_vm_address_t orgTranslateAtomConnectorInfoV2 {}, orgATIControllerStart {};
    mach_vm_address_t orgNotifyLinkChange {}, orgGetHWInfo[1] {};
    uint32_t *orgDeviceTypeTable = nullptr;
    mach_vm_address_t orgAmdCailServicesConstructor {};
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
     * X4000
     */
    t_HWEngineNew orgGFX7PM4EngineNew = nullptr;
    t_HWEngineConstructor orgGFX7PM4EngineConstructor = nullptr;
    t_HWEngineNew orgGFX7SDMAEngineNew = nullptr;
    t_HWEngineConstructor orgGFX7SDMAEngineConstructor = nullptr;
    mach_vm_address_t orgGetHWEngine {};
    mach_vm_address_t orgGetHWCapabilities {};
    mach_vm_address_t orgSetupAndInitializeHWCapabilities {};
	t_HWEngineNew orgGFX7UVDEngineNew = nullptr;
	t_HWEngineConstructor orgGFX7UVDEngineConstructor = nullptr;
	t_HWEngineNew orgGFX7VCEEngineNew = nullptr;
	t_HWEngineConstructor orgGFX7VCEEngineConstructor = nullptr;
	t_HWEngineNew orgGFX7SAMUEngineNew = nullptr;
	t_HWEngineConstructor orgGFX7SAMUEngineConstructor = nullptr;
	t_HWEngineNew orgGFX8UVDEngineNew = nullptr;
	t_HWEngineConstructor orgGFX8UVDEngineConstructor = nullptr;
	t_HWEngineNew orgGFX8VCEEngineNew = nullptr;
	t_HWEngineConstructor orgGFX8VCEEngineConstructor = nullptr;
	t_HWEngineNew orgGFX8PM4EngineNew = nullptr;
	t_HWEngineConstructor orgGFX8PM4EngineConstructor = nullptr;
	t_HWEngineNew orgGFX8SDMAEngineNew = nullptr;
	t_HWEngineConstructor orgGFX8SDMAEngineConstructor = nullptr;

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
     * AMD8000Controller
     */
    static uint16_t wrapGetFamilyId();
    static IOReturn wrapPopulateDeviceInfo(void *that);
//    static uint32_t wrapGetVideoMemoryType(void *that);
//    static uint32_t wrapGetVideoMemoryBitWidth(void *that);
//    static IOReturn wrapPopulateVramInfo(void *that, void *param1);
	/**
	 * AMDSupport
	 */
	

    /**
     * X5000HWLibs
     */
    static void wrapAmdCailServicesConstructor(IOService *that, IOPCIDevice *provider);
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
    static const char *getASICName();
    static uint32_t wrapPspAsdLoad(void *pspData);
    static uint32_t wrapPspDtmLoad(void *pspData);
    static uint32_t wrapPspHdcpLoad(void *pspData);

    /**
     * X4000
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

    mach_vm_address_t orgGFX9RTRingGetHead {};
    static uint64_t wrapGFX9RTRingGetHead(void *that);

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

    mach_vm_address_t orgCosDebugAssert {};
    static void wrapCosDebugAssert(void *param1, uint8_t *param2, uint8_t *param3, uint32_t param4, uint8_t *param5);

    mach_vm_address_t orgIpiSdmaHwInit {};
    static bool wrapIpiSdmaHwInit(void *ctx);

    mach_vm_address_t orgSdmaHwInit {};
    static uint32_t wrapSdmaHwInit(uint64_t param1, uint64_t param2, uint64_t param3);

    mach_vm_address_t orgIpiSdmaFindInstanceByEngineIndexAndType {};
    static uint64_t wrapIpiSdmaFindInstanceByEngineIndexAndType(uint64_t param1, uint32_t param2, uint32_t param3);

    GcFwConstant *orgGcRlcUcode = nullptr;
    GcFwConstant *orgGcMeUcode = nullptr;
    GcFwConstant *orgGcCeUcode = nullptr;
    GcFwConstant *orgGcPfpUcode = nullptr;
    GcFwConstant *orgGcMecUcode = nullptr;
    GcFwConstant *orgGcMecJtUcode = nullptr;
    SdmaFwConstant *orgSdmaUcode = nullptr;

    mach_vm_address_t orgHwReadReg32 {};
    static uint32_t wrapHwReadReg32(void *that, uint32_t param1);

    mach_vm_address_t orgAtiAsicInfoGetPropertiesForUserClient{};
    static uint64_t* wrapAtiAsicInfoGetPropertiesForUserClient(void* that);

    mach_vm_address_t orgSmuCzReadSmuVersion{};
    static void wrapSmuCzReadSmuVersion(void* param1);
};