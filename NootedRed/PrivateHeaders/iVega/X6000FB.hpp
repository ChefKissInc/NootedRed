// Copyright © 2022-2024 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <Headers/kern_patcher.hpp>

namespace iVega {
    class X6000FB {
        bool initialised {false};
        bool fixedVBIOS {false};
        mach_vm_address_t orgGetNumberOfConnectors {0};
        mach_vm_address_t orgIH40IVRingInitHardware {0};
        mach_vm_address_t orgIRQMGRWriteRegister {0};
        mach_vm_address_t orgInitWithPciInfo {0};
        mach_vm_address_t orgCreateRegisterAccess {0};

        public:
        static X6000FB &singleton();

        void init();

        private:
        void processKext(KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size);
        static UInt16 wrapGetEnumeratedRevision();
        static IOReturn wrapPopulateVramInfo(void *that, void *fwInfo);
        static UInt32 wrapGetNumberOfConnectors(void *that);
        static bool wrapIH40IVRingInitHardware(void *ctx, void *param2);
        static void wrapIRQMGRWriteRegister(void *ctx, UInt64 index, UInt32 value);
        static bool wrapInitWithPciInfo(void *that, void *pciDevice);
        static void wrapDoGPUPanic(void *that, char const *fmt, ...);
        static void wrapDmLoggerWrite(void *logger, const UInt32 logType, const char *fmt, ...);
        static void *wrapCreateRegisterAccess(void *initData);
        static void *wrapCreateDmcubService();
    };
};    // namespace iVega
