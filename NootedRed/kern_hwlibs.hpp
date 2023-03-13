//  Copyright © 2022-2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.0. See LICENSE for
//  details.

#pragma once
#include "kern_amd.hpp"
#include <Headers/kern_patcher.hpp>
#include <Headers/kern_util.hpp>

using t_AMDFirmwareDirectoryConstructor = void (*)(void *that, uint32_t);
using t_createFirmware = void *(*)(const void *data, uint32_t size, uint32_t param3, const char *filename);
using t_putFirmware = bool (*)(void *that, uint32_t deviceType, void *fw);
using t_VegaXPowerTuneConstructor = void (*)(void *that, void *param1, void *param2);
using t_sendMsgToSmc = uint32_t (*)(void *smum, uint32_t msgId);

class X5000HWLibs {
    public:
    static X5000HWLibs *callback;
    void init();
    bool processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size);

    private:
    mach_vm_address_t orgPspSwInit {0};
    t_AMDFirmwareDirectoryConstructor orgAMDFirmwareDirectoryConstructor = nullptr;
    t_createFirmware orgCreateFirmware = nullptr;
    t_putFirmware orgPutFirmware = nullptr;
    t_VegaXPowerTuneConstructor orgVega10PowerTuneConstructor = nullptr;
    t_VegaXPowerTuneConstructor orgVega20PowerTuneConstructor = nullptr;
    t_sendMsgToSmc orgRavenSendMsgToSmc = nullptr;
    t_sendMsgToSmc orgRenoirSendMsgToSmc = nullptr;
    mach_vm_address_t orgSmuRavenInitialize {0};
    mach_vm_address_t orgSmuRenoirInitialize {0};
    mach_vm_address_t orgPspCmdKmSubmit {0};

    static uint32_t wrapSmuGetHwVersion();
    static uint32_t wrapPspSwInit(uint32_t *inputData, void *outputData);
    static uint32_t wrapGcGetHwVersion();
    static void wrapPopulateFirmwareDirectory(void *that);
    static void *wrapCreatePowerTuneServices(void *that, void *param2);
    static uint32_t wrapSmuRavenInitialize(void *smum, uint32_t param2);
    static uint32_t wrapSmuRenoirInitialize(void *smum, uint32_t param2);
    static uint32_t wrapPspCmdKmSubmit(void *psp, void *ctx, void *param3, void *param4);
    static uint32_t hwLibsNoop();
    static uint32_t hwLibsUnsupported();
};