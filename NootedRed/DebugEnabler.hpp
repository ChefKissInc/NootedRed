// Patches to force-enable debug logs
//
// Copyright © 2024-2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <Headers/kern_patcher.hpp>

class DebugEnabler {
    mach_vm_address_t orgInitWithPciInfo {0};
    mach_vm_address_t orgGetNumericProperty {0};

    public:
    static DebugEnabler &singleton();

    void processKext(KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size);

    private:
    void processX6000FB(KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size);
    void processX5000HWLibs(KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size);
    void processX5000(KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size);

    static bool wrapInitWithPciInfo(void *self, void *pciDevice);
    static void doGPUPanic(void *self, char const *fmt, ...);
    static void dmLoggerWrite(void *logger, const UInt32 logType, const char *fmt, ...);
    static void ipAssertion(void *instance, UInt32 cond, const char *func, const char *file, UInt32 line,
        const char *msg);
    static void gcDebugPrint(void *instance, const char *fmt, ...);
    static void pspDebugPrint(void *instance, const char *fmt, ...);
    static bool wrapGetNumericProperty(void *self, const char *name, UInt32 *value);
};
