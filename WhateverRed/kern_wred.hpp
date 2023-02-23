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
    void processPatcher(KernelPatcher &patcher);
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
    uint64_t fbOffset {};
    uint16_t enumeratedRevision {};
    void *hwAlignManager = nullptr;
    void *hwAlignManagerVtableX5000 = nullptr;
    void *hwAlignManagerVtableX6000 = nullptr;

    OSMetaClass *metaClassMap[4][2] = {{nullptr}};

    mach_vm_address_t orgSafeMetaCast {};
    static OSMetaClassBase *wrapSafeMetaCast(const OSMetaClassBase *anObject, const OSMetaClass *toMeta);

    /** X6000Framebuffer */
    mach_vm_address_t orgPopulateDeviceInfo {};
    CailAsicCapEntry *orgAsicCapsTable = nullptr;
    mach_vm_address_t orgHwReadReg32 {};

    /** X5000HWLibs */
    uint32_t *orgDeviceTypeTable = nullptr;
    mach_vm_address_t orgAmdTtlServicesConstructor {};
    mach_vm_address_t orgSmuGetHwVersion {};
    mach_vm_address_t orgPspSwInit {};
    mach_vm_address_t orgPopulateFirmwareDirectory {};
    t_createFirmware orgCreateFirmware = nullptr;
    t_putFirmware orgPutFirmware = nullptr;
    t_Vega10PowerTuneConstructor orgVega10PowerTuneConstructor = nullptr;
    CailAsicCapEntry *orgCapsTableHWLibs = nullptr;
    CailInitAsicCapEntry *orgAsicInitCapsTable = nullptr;
    t_sendMsgToSmc orgRavenSendMsgToSmc = nullptr;
    t_sendMsgToSmc orgRenoirSendMsgToSmc = nullptr;
    mach_vm_address_t orgSmuRavenInitialize {};
    mach_vm_address_t orgSmuRenoirInitialize {};
    mach_vm_address_t orgPspCmdKmSubmit {};

    /** X6000 */
    t_HWEngineNew orgVCN2EngineNewX6000 = nullptr;
    t_HWEngineConstructor orgVCN2EngineConstructorX6000 = nullptr;
    mach_vm_address_t orgAllocateAMDHWDisplayX6000 {};
    mach_vm_address_t orgInitWithPciInfo {};
    mach_vm_address_t orgNewVideoContextX6000 {};
    mach_vm_address_t orgCreateSMLInterfaceX6000 {};
    mach_vm_address_t orgNewSharedX6000 {};
    mach_vm_address_t orgNewSharedUserClientX6000 {};
    mach_vm_address_t orgInitDCNRegistersOffsets {};
    mach_vm_address_t orgGetPreferredSwizzleMode2 {};
    mach_vm_address_t orgAccelSharedSurfaceCopy {};
    mach_vm_address_t orgAllocateScanoutFB {};
    mach_vm_address_t orgFillUBMSurface {};
    mach_vm_address_t orgConfigureDisplay {};
    mach_vm_address_t orgGetDisplayInfo {};

    /** X5000 */
    t_HWEngineNew orgGFX9PM4EngineNew = nullptr;
    t_HWEngineConstructor orgGFX9PM4EngineConstructor = nullptr;
    t_HWEngineNew orgGFX9SDMAEngineNew = nullptr;
    t_HWEngineConstructor orgGFX9SDMAEngineConstructor = nullptr;
    mach_vm_address_t orgSetupAndInitializeHWCapabilities {};
    mach_vm_address_t orgRTGetHWChannel {};
    mach_vm_address_t orgAdjustVRAMAddress {};
    mach_vm_address_t orgAccelSharedUserClientStart {};
    mach_vm_address_t orgAccelSharedUserClientStop {};
    mach_vm_address_t orgAllocateAMDHWAlignManager {};

    /** X6000Framebuffer */
    static IOReturn wrapPopulateDeviceInfo(void *that);
    static uint16_t wrapGetEnumeratedRevision(void *that);
    static IOReturn wrapPopulateVramInfo(void *that, void *fwInfo);
    static uint32_t wrapHwReadReg32(void *that, uint32_t param1);

    /** X5000HWLibs */
    static void wrapAmdTtlServicesConstructor(void *that, IOPCIDevice *provider);
    static uint32_t wrapSmuGetHwVersion(uint64_t param1, uint32_t param2);
    static uint32_t wrapPspSwInit(uint32_t *param1, uint32_t *param2);
    static uint32_t wrapGcGetHwVersion(uint32_t *param1);
    static void wrapPopulateFirmwareDirectory(void *that);
    static void *wrapCreatePowerTuneServices(void *that, void *param2);
    static uint32_t wrapSmuGetFwConstants(void *param1);
    static uint32_t wrapSmuInternalHwInit(void *param1);
    static uint32_t wrapSmuRavenInitialize(void *smum, uint32_t param2);
    static uint32_t wrapSmuRenoirInitialize(void *smum, uint32_t param2);
    static uint32_t wrapPspCmdKmSubmit(void *psp, void *ctx, void *param3, void *param4);

    /** X6000 */
    static bool wrapAccelStartX6000();
    static bool wrapInitWithPciInfo(void *that, void *param1);
    static void *wrapNewVideoContext(void *that);
    static void *wrapCreateSMLInterface(uint32_t configBit);
    static bool wrapAccelSharedUserClientStartX6000(void *that, void *provider);
    static bool wrapAccelSharedUserClientStopX6000(void *that, void *provider);
    static void wrapInitDCNRegistersOffsets(void *that);
    static uint64_t wrapAccelSharedSurfaceCopy(void *that, void *param1, uint64_t param2, void *param3);
    static uint64_t wrapAllocateScanoutFB(void *that, uint32_t param1, void *param2, void *param3, void *param4);
    static uint64_t wrapFillUBMSurface(void *that, uint32_t param1, void *param2, void *param3);
    static bool wrapConfigureDisplay(void *that, uint32_t param1, uint32_t param2, void *param3, void *param4);
    static uint64_t wrapGetDisplayInfo(void *that, uint32_t param1, bool param2, bool param3, void *param4,
        void *param5);

    /** X5000 */
    static bool wrapAllocateHWEngines(void *that);
    static void wrapSetupAndInitializeHWCapabilities(void *that);
    static void *wrapRTGetHWChannel(void *that, uint32_t param1, uint32_t param2, uint32_t param3);
    static void wrapInitializeFamilyType(void *that);
    static void *wrapAllocateAMDHWDisplay(void *that);
    static uint64_t wrapAdjustVRAMAddress(void *that, uint64_t addr);
    static void wrapDmLoggerWrite(void *dalLogger, uint32_t logType, char *fmt, ...);
    static void *wrapNewShared();
    static void *wrapNewSharedUserClient();
    static void *wrapAllocateAMDHWAlignManager();

    mach_vm_address_t orgInitFramebufferResource {};
    static void *wrapInitFramebufferResource(void *that, uint32_t param1, void *param2);
    mach_vm_address_t orgReserveFrameBuffer {};
    static uint64_t wrapReserveFrameBuffer(void *that, uint64_t param1, uint32_t param2, void *param3);

    mach_vm_address_t orgWriteUpdateFrameBufferOffsetCommands {};
    static uint32_t wrapWriteUpdateFrameBufferOffsetCommands(void *that, uint32_t param1, void *param2, uint32_t param3,
        uint32_t param4, void *param5, void *param6, void *param7);
    mach_vm_address_t orgGetDisplayPipeTransactionFlip {};
    static bool wrapGetDisplayPipeTransactionFlip(void *that, uint32_t param1, void *param2, void *param3,
        uint32_t param4, void *param5, bool param6, uint32_t param7, void *param8);
    mach_vm_address_t orgGetGPUSystemAddress {};
    static uint64_t wrapGetGPUSystemAddress(void *that);
    mach_vm_address_t orgGetApertureRange {};
    static void *wrapGetApertureRange(void *that, uint32_t param1);
    mach_vm_address_t orgGetVRAMRange {};
    static void *wrapGetVRAMRange(void *that);
};
