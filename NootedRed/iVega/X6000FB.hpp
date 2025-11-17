// AMDRadeonX6000Framebuffer patches for AMD Vega iGPUs
//
// Copyright © 2022-2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <GPUDriversAMD/FB/AmdDeviceMemoryManager.hpp>
#include <Headers/kern_patcher.hpp>

namespace iVega {
    class X6000FB {
        using mapMemorySubRange_t = IOReturn (*)(void *self, AmdReservedMemorySelector selector, size_t atOffset,
            size_t withSize, IOOptionBits andAttributes);

        bool fixedVBIOS {false};
        mach_vm_address_t orgGetNumberOfConnectors {0};
        mach_vm_address_t orgIH40IVRingInitHardware {0};
        mach_vm_address_t orgIRQMGRWriteRegister {0};
        mach_vm_address_t orgCreateRegisterAccess {0};
        mapMemorySubRange_t mapMemorySubRange {nullptr};

        public:
        static X6000FB &singleton();

        void processKext(KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size);

        private:
        static UInt16 getEnumeratedRevision();
        static IOReturn populateVramInfo(void *self, void *fwInfo);
        static UInt32 wrapGetNumberOfConnectors(void *self);
        static bool wrapIH40IVRingInitHardware(void *ctx, void *param2);
        static void wrapIRQMGRWriteRegister(void *ctx, UInt64 index, UInt32 value);
        static void *wrapCreateRegisterAccess(void *initData);
        static IOReturn initialiseReservedVRAM(void *self);
    };
};    // namespace iVega
