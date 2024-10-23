// Copyright © 2022-2024 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include "AMDCommon.hpp"
#include "ObjectField.hpp"
#include <Headers/kern_patcher.hpp>

enum AMDHWEngineType : UInt32 {
    kAMDHWEngineTypePM4 = 0,
    kAMDHWEngineTypeSDMA0,
    kAMDHWEngineTypeSDMA1,
    kAMDHWEngineTypeSDMA2,
    kAMDHWEngineTypeSDMA3,
    kAMDHWEngineTypeUVD0,
    kAMDHWEngineTypeUVD1,
    kAMDHWEngineTypeVCE,
    kAMDHWEngineTypeVCN0,
    kAMDHWEngineTypeVCN1,
    kAMDHWEngineTypeSAMU,
    kAMDHWEngineTypeMax,
};

class X5000 {
    friend class X6000;

    static X5000 *callback;

    public:
    void init();
    bool processKext(KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size);

    private:
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

    t_GenericConstructor orgGFX9PM4EngineConstructor {nullptr};
    t_GenericConstructor orgGFX9SDMAEngineConstructor {nullptr};
    mach_vm_address_t orgSetupAndInitializeHWCapabilities {0};
    mach_vm_address_t orgGFX9SetupAndInitializeHWCapabilities {0};
    mach_vm_address_t orgGetHWChannel {0};
    mach_vm_address_t orgAdjustVRAMAddress {0};
    mach_vm_address_t orgAllocateAMDHWAlignManager {0};
    mach_vm_address_t orgObtainAccelChannelGroup {0};
    mach_vm_address_t orgHwlConvertChipFamily {0};
    mach_vm_address_t orgGetNumericProperty {0};

    static bool wrapAllocateHWEngines(void *that);
    static void wrapSetupAndInitializeHWCapabilities(void *that);
    static void wrapGFX9SetupAndInitializeHWCapabilities(void *that);
    static void *wrapGetHWChannel(void *that, AMDHWEngineType engineType, UInt32 ringId);
    static void wrapInitializeFamilyType(void *that);
    static void *wrapAllocateAMDHWDisplay(void *that);
    static UInt64 wrapAdjustVRAMAddress(void *that, UInt64 addr);
    static void *wrapAllocateAMDHWAlignManager(void *that);
    static UInt32 wrapGetDeviceType();
    static UInt32 wrapReturnZero();
    static void *wrapObtainAccelChannelGroup(void *that, UInt32 priority);
    static void *wrapObtainAccelChannelGroup1304(void *that, UInt32 priority, void *task);
    static UInt32 wrapHwlConvertChipFamily(void *that, UInt32 family, UInt32 revision);
    static bool wrapGetNumericProperty(void *that, const char *name, uint32_t *value);
};

//------ Patterns ------//

static const UInt8 kChannelTypesPattern[] = {0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00};

static const UInt8 kHwlConvertChipFamilyPattern[] = {0x81, 0xFE, 0x8D, 0x00, 0x00, 0x00, 0x0F};

//------ Patches ------//

// Make for loop stop after one SDMA engine.
static const UInt8 kStartHWEnginesOriginal[] = {0x40, 0x83, 0xF0, 0x02};
static const UInt8 kStartHWEnginesMask[] = {0xF0, 0xFF, 0xF0, 0xFF};
static const UInt8 kStartHWEnginesPatched[] = {0x40, 0x83, 0xF0, 0x01};

// The check in Addr::Lib::Create on 10.15 & 13.4+ is `familyId == 0x8D` instead of `familyId - 0x8D < 2`.
// So, change the 0x8D (AI) to 0x8E (RV).
static const UInt8 kAddrLibCreateOriginal[] = {0x41, 0x81, 0x7D, 0x08, 0x8D, 0x00, 0x00, 0x00};
static const UInt8 kAddrLibCreatePatched[] = {0x41, 0x81, 0x7D, 0x08, 0x8E, 0x00, 0x00, 0x00};

// For some reason Navi (`0x8F`) was added in 14.4 here. Lazy copy pasting?
static const UInt8 kAddrLibCreateOriginal14_4[] = {0x41, 0x8B, 0x46, 0x08, 0x3D, 0x8F, 0x00, 0x00, 0x00, 0x74, 0x00,
    0x3D, 0x8D, 0x00, 0x00, 0x00, 0x0F, 0x85, 0x00, 0x00, 0x00, 0x00};
static const UInt8 kAddrLibCreateOriginalMask14_4[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00};
static const UInt8 kAddrLibCreatePatched14_4[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x8E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const UInt8 kAddrLibCreatePatchedMask14_4[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// Catalina only. Change loop condition to skip SDMA1_HP.
static const UInt8 kCreateAccelChannelsOriginal[] = {0x8D, 0x44, 0x09, 0x02};
static const UInt8 kCreateAccelChannelsPatched[] = {0x8D, 0x44, 0x09, 0x01};
