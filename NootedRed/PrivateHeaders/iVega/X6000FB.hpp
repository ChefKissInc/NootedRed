// Copyright © 2022-2024 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <Headers/kern_patcher.hpp>
#include <PrivateHeaders/GPUDriversAMD/FB/AmdDeviceMemoryManager.hpp>

namespace iVega {
    class X6000FB {
        using mapMemorySubRange_t = IOReturn (*)(void *that, AmdReservedMemorySelector selector, size_t atOffset,
            size_t withSize, IOOptionBits andAttributes);

        bool initialised {false};
        bool fixedVBIOS {false};
        mach_vm_address_t orgGetNumberOfConnectors {0};
        mach_vm_address_t orgIH40IVRingInitHardware {0};
        mach_vm_address_t orgIRQMGRWriteRegister {0};
        mach_vm_address_t orgCreateRegisterAccess {0};
        mapMemorySubRange_t orgMapMemorySubRange {nullptr};

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
        static void *wrapCreateRegisterAccess(void *initData);
        static IOReturn wrapIntializeReservedVram(void *that);    // AMD made this typo, not me
    };
};    // namespace iVega
