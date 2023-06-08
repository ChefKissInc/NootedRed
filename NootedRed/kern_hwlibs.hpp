//  Copyright © 2022-2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.0. See LICENSE for
//  details.

#pragma once
#include "kern_amd.hpp"
#include <Headers/kern_patcher.hpp>
#include <Headers/kern_util.hpp>

using t_createFirmware = void *(*)(const void *data, uint32_t size, uint32_t ipVersion, const char *filename);
using t_putFirmware = bool (*)(void *that, uint32_t deviceType, void *fw);

class X5000HWLibs {
    public:
    static X5000HWLibs *callback;
    void init();
    bool processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size);

    private:
    mach_vm_address_t orgPopulateFirmwareDirectory {0};
    t_createFirmware orgCreateFirmware {nullptr};
    t_putFirmware orgPutFirmware {nullptr};
    mach_vm_address_t orgUpdateSdmaPowerGating {0};
    mach_vm_address_t orgPspCmdKmSubmit {0};

    static uint32_t wrapSmuGetHwVersion();
    static void wrapPopulateFirmwareDirectory(void *that);
    static void wrapUpdateSdmaPowerGating(void *cail, uint32_t mode);
    static CAILResult wrapPspCmdKmSubmit(void *psp, void *ctx, void *param3, void *param4);
    static CAILResult hwLibsNoop();
};
