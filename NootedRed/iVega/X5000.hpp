// Copyright © 2022-2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <GPUDriversAMD/Accel/HWEngine.hpp>
#include <GPUDriversAMD/AddrLib.hpp>
#include <Headers/kern_patcher.hpp>
#include <PenguinWizardry/ObjectField.hpp>

namespace iVega {
    class X5000 {
        using GenericConstructor_t = void (*)(void *self);

        ObjectField<void *> pm4EngineField {};
        ObjectField<void *> sdma0EngineField {};
        ObjectField<UInt32> displayPipeCountField {};
        ObjectField<UInt32> seCountField {};
        ObjectField<UInt32> shPerSEField {};
        ObjectField<UInt32> cuPerSHField {};
        ObjectField<bool> hasUVD0Field {};
        ObjectField<bool> hasVCEField {};
        ObjectField<bool> hasVCN0Field {};
        ObjectField<bool> hasSDMAPagingQueueField {};
        ObjectField<UInt32> familyTypeField {};
        ObjectField<Gfx9ChipSettings> chipSettingsField {};
        GenericConstructor_t pm4EngineConstructor {nullptr};
        GenericConstructor_t sdmaEngineConstructor {nullptr};
        mach_vm_address_t orgSetupAndInitializeHWCapabilities {0};
        mach_vm_address_t orgGFX9SetupAndInitializeHWCapabilities {0};
        mach_vm_address_t orgGetHWChannel {0};
        mach_vm_address_t orgAdjustVRAMAddress {0};
        mach_vm_address_t orgObtainAccelChannelGroup {0};
        mach_vm_address_t orgHwlConvertChipFamily {0};

        public:
        static X5000 &singleton();

        X5000();

        void processKext(KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size);

        private:
        static bool allocateHWEngines(void *self);
        static void wrapSetupAndInitializeHWCapabilities(void *self);
        static void wrapGFX9SetupAndInitializeHWCapabilities(void *self);
        static void *wrapGetHWChannel(void *self, AMDHWEngineType engineType, UInt32 ringId);
        static void initializeFamilyType(void *self);
        static void *allocateAMDHWDisplay(void *self);
        static UInt64 wrapAdjustVRAMAddress(void *self, UInt64 addr);
        static UInt32 returnZero();
        static void *wrapObtainAccelChannelGroup(void *self, UInt32 priority);
        static void *wrapObtainAccelChannelGroup1304(void *self, UInt32 priority, void *task);
        static UInt32 wrapHwlConvertChipFamily(void *self, UInt32 family, UInt32 revision);
    };
};    // namespace iVega
