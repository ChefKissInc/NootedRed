//  Copyright © 2022-2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.0. See LICENSE for
//  details.

#ifndef kern_hwlibs_hpp
#define kern_hwlibs_hpp
#include "kern_amd.hpp"
#include <Headers/kern_patcher.hpp>
#include <Headers/kern_util.hpp>

using t_AMDFirmwareDirectoryConstructor = void (*)(void *that, uint32_t maxEntryCount);
using t_createFirmware = void *(*)(const void *data, uint32_t size, uint32_t ipVersion, const char *filename);
using t_putFirmware = bool (*)(void *that, uint32_t deviceType, void *fw);

class X5000HWLibs {
    public:
    static X5000HWLibs *callback;
    void init();
    bool processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size);

    private:
    mach_vm_address_t orgPspSwInit {0};
    t_AMDFirmwareDirectoryConstructor orgAMDFirmwareDirectoryConstructor {nullptr};
    t_createFirmware orgCreateFirmware {nullptr};
    t_putFirmware orgPutFirmware {nullptr};
    mach_vm_address_t orgSmuInitialize {0};
    mach_vm_address_t orgPspCmdKmSubmit {0};

    static uint32_t wrapSmuGetHwVersion();
    static AMDReturn wrapPspSwInit(uint32_t *inputData, void *outputData);
    static uint32_t wrapGcGetHwVersion();
    static void wrapPopulateFirmwareDirectory(void *that);
    static AMDReturn wrapSmuInitialize(void *smum, uint32_t param2);
    static AMDReturn wrapPspCmdKmSubmit(void *psp, void *ctx, void *param3, void *param4);
    static AMDReturn hwLibsNoop();
};

#endif /* kern_hwlibs_hpp */
