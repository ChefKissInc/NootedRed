// AMDRadeonX5000HWLibs Patches
//
// Copyright © 2022-2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <GPUDriversAMD/CAIL/DeviceType.hpp>
#include <GPUDriversAMD/CAIL/HWBlock.hpp>
#include <GPUDriversAMD/CAIL/Result.hpp>
#include <GPUDriversAMD/TTL/COS.hpp>
#include <GPUDriversAMD/TTL/Event.hpp>
#include <GPUDriversAMD/TTL/SWIP/DMCU.hpp>
#include <GPUDriversAMD/TTL/SWIP/GC.hpp>
#include <GPUDriversAMD/TTL/SWIP/IPVersion.hpp>
#include <GPUDriversAMD/TTL/SWIP/SDMA.hpp>
#include <GPUDriversAMD/TTL/SWIP/SMU.hpp>
#include <Headers/kern_patcher.hpp>
#include <Headers/kern_util.hpp>
#include <PenguinWizardry/ObjectField.hpp>

class X5000HWLibs
{
    using t_createFirmware = void* (*)(const void* data, UInt32 size, UInt32 ipVersion, const char* filename);
    using t_putFirmware    = bool     (*)(void* self, AMDDeviceType deviceType, void* fw);

    ObjectField<void*>                                                fwDirField;
    ObjectField<UInt32>                                               pspBootloaderVersionField;
    ObjectField<UInt8>                                                pspSecurityCapsField;
    ObjectField<UInt32>                                               pspTOSVersionField;
    ObjectField<UInt8*>                                               pspCommandDataField;
    ObjectField<bool>                                                 smuSwInitialisedFieldBase;
    ObjectField<void*>                                                smuInternalSWInitField;
    ObjectField<void*>                                                smuFullscreenEventField;
    ObjectField<void*>                                                smuGetUCodeConstsField;
    ObjectField<void*>                                                smuInternalHWInitField;
    ObjectField<void*>                                                smuNotifyEventField;
    ObjectField<void*>                                                smuInternalSWExitField;
    ObjectField<void*>                                                smuInternalHWExitField;
    ObjectField<void*>                                                smuFullAsicResetField;
    ObjectField<GCFirmwareInfo>                                       gcSwFirmwareField;
    ObjectField<UInt32>                                               dmcuEnablePSPFWLoadField;
    ObjectField<UInt32>                                               dmcuABMLevelField;
    ObjectField<bool (*)(void* instance, const SDMAFWConstant** out)> sdmaGetFwConstantsField;
    ObjectField<bool (*)(void* instance)>                             sdmaStartEngineField;
    mach_vm_address_t                                                 orgGetIpFw{0};
    t_createFirmware                                                  orgCreateFirmware{nullptr};
    t_putFirmware                                                     orgPutFirmware{nullptr};
    mach_vm_address_t                                                 orgPspCmdKmSubmit{0};
    mach_vm_address_t                                                 orgSmuInitFunctionPointerList{0};
    mach_vm_address_t                                                 orgGcSetFwEntryInfo{0};
    mach_vm_address_t                                                 orgSdmaInitFunctionPointerList{0};
    CAILResult (*smu90SendMessageWithParameter)(void* ctx, UInt32 message, UInt32 param){nullptr};
    CAILResult (*smuCosWaitFor)(void* ctx, CosWaitForFunc* func, void* handle, UInt32 duration){nullptr};
    UInt32     (*smuCgsReadRegister)(void* ctx, UInt32 regOff, UInt32 blockInstance, CAILHWBlock block,
                                     UInt32 regOffBase){nullptr};
    void (*smuCgsWriteRegister)(void* ctx, UInt32 regOff, UInt32 blockInstance, UInt32 regValue, CAILHWBlock block,
                                UInt32 regOffBase){nullptr};

public:
    static X5000HWLibs& singleton();

    X5000HWLibs();

    void processKext(KernelPatcher& patcher, size_t id, mach_vm_address_t slide, size_t size);

private:
    static void       wrapPopulateFirmwareDirectory(void* self);
    static bool       wrapGetIpFw(void* self, UInt32 ipVersion, const char* name, void* out);
    static CAILResult pspIsSosRunning();
    static CAILResult retUnsupported();
    static CAILResult retOK();
    static CAILResult pspBootloaderLoadSos10(void* instance);
    static CAILResult pspSecurityFeatureCapsSet10(void* instance);
    static CAILResult pspSecurityFeatureCapsSet12(void* instance);
    static CAILResult wrapPspCmdKmSubmit(void* instance, void* cmd, void* outData, void* outResponse);
    CAILResult        smuSendMessage(void* ctx, UInt32 message, UInt32 param = 0, UInt32* outParam = nullptr) const;
    static CAILResult smuPowerUpConfigCommon(void* ctx);
    static CAILResult smuInternalSwInit(void* ctx, void* input, AMDSMUSWInitOutput* output);
    static CAILResult smuInternalSwInitOld(void* ctx, void* input, AMDSMUSWInitOutput* output);
    static CAILResult smuGetUCodeConsts(void* ctx, AMDSMUUCodeConstants* consts);
    static CAILResult smu10PowerUpConfig(void* ctx);
    static CAILResult smu10InternalHwInit(void* ctx);
    static bool       smu12IsFwLoaded(void* ctx);
    static CAILResult smu12WaitForFwLoaded(void* ctx);
    static CAILResult smu12PowerUpConfig(void* ctx);
    static CAILResult smu12InternalHwInit(void* ctx);
    static CAILResult smuInternalHwExit(void* ctx);
    static CAILResult smuFullAsicReset(void* ctx, void* data);
    static CAILResult smu10NotifyEvent(void* ctx, TTLEventInput* input);
    static CAILResult smu12NotifyEvent(void* ctx, TTLEventInput* input);
    static CAILResult smuFullScreenEvent(void* ctx, TTLFullScreenEvent event);
    static CAILResult wrapSmuInitFunctionPointerList(void* ctx, SWIPIPVersion ipVersion);
    static void       gc91GetFwConstants(void* instance, GCFirmwareInfo* fwData);
    static void       gc92GetFwConstants(void* instance, GCFirmwareInfo* fwData);
    static void       gc93GetFwConstants(void* instance, GCFirmwareInfo* fwData);
    static void       processGCFWEntries(void* instance, void* initData);
    static CAILResult wrapGcSetFwEntryInfo(void* instance, SWIPIPVersion ipVersion, void* initData);
    static bool       getDcn1FwConstants(void* instance, DMCUFirmwareInfo* fwData);
    static bool       getDcn21FwConstants(void* instance, DMCUFirmwareInfo* fwData);
    static CAILResult wrapSdmaInitFunctionPointerList(void* instance, UInt32 verMajor, UInt32 verMinor,
                                                      UInt32 verPatch);
};
