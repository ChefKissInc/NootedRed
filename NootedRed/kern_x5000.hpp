//  Copyright © 2022-2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5. See LICENSE for
//  details.

#pragma once
#include "kern_amd.hpp"
#include <Headers/kern_patcher.hpp>

class X5000 {
    friend class X6000;

    public:
    static X5000 *callback;
    void init();
    bool processKext(KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size);

    private:
    t_GenericConstructor orgGFX9PM4EngineConstructor {nullptr};
    t_GenericConstructor orgGFX9SDMAEngineConstructor {nullptr};
    mach_vm_address_t orgSetupAndInitializeHWCapabilities {0};
    mach_vm_address_t orgGetHWChannel {0};
    mach_vm_address_t orgAdjustVRAMAddress {0};
    mach_vm_address_t orgAccelSharedUCStart {0};
    mach_vm_address_t orgAccelSharedUCStop {0};
    mach_vm_address_t orgAllocateAMDHWAlignManager {0};
    mach_vm_address_t orgObtainAccelChannelGroup {0};
    void *hwAlignMgr {nullptr};
    uint8_t *hwAlignMgrVtX5000 {nullptr};
    uint8_t *hwAlignMgrVtX6000 {nullptr};

    static bool wrapAllocateHWEngines(void *that);
    static void wrapSetupAndInitializeHWCapabilities(void *that);
    static void *wrapGetHWChannel(void *that, uint32_t engineType, uint32_t ringId);
    static void wrapInitializeFamilyType(void *that);
    static void *wrapAllocateAMDHWDisplay(void *that);
    static uint64_t wrapAdjustVRAMAddress(void *that, uint64_t addr);
    static void *wrapNewVideoContext(void *that);
    static void *wrapCreateSMLInterface(uint32_t configBit);
    static void *wrapNewShared();
    static void *wrapNewSharedUserClient();
    static void *wrapAllocateAMDHWAlignManager();
    static uint32_t wrapGetDeviceType();
    static uint32_t wrapReturnZero();
    static void *wrapObtainAccelChannelGroup(void *that, uint32_t priority);
    static void *wrapObtainAccelChannelGroup1304(void *that, uint32_t priority, void *task);
    static uint32_t wrapHwlConvertChipFamily(void *that, uint32_t family, uint32_t revision);
};

/* ---- Patterns ---- */

static const uint8_t kChannelTypesPattern[] = {0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00};

static const uint8_t kHwlConvertChipFamilyPattern[] = {0x81, 0xFE, 0x8D, 0x00, 0x00, 0x00, 0x0F};

/* ---- Patches ---- */

// Make for loop stop after one SDMA engine.
static const uint8_t kStartHWEnginesOriginal[] = {0x40, 0x83, 0xF0, 0x02};
static const uint8_t kStartHWEnginesMask[] = {0xF0, 0xFF, 0xF0, 0xFF};
static const uint8_t kStartHWEnginesPatched[] = {0x40, 0x83, 0xF0, 0x01};

// The check inside was changed from `familyId - 0x8D < 2` to `familyId == 0x8D` in Ventura 13.4.
// Change the 0x8D (AI) to 0x8E (RV).
static const uint8_t kAddrLibCreateOriginal[] = {0x41, 0x81, 0x7D, 0x08, 0x8D, 0x00, 0x00, 0x00};
static const uint8_t kAddrLibCreatePatched[] = {0x41, 0x81, 0x7D, 0x08, 0x8E, 0x00, 0x00, 0x00};

// Catalina only. Change loop condition to skip SDMA1_HP.
static const uint8_t kCreateAccelChannelsOriginal[] = {0x8D, 0x44, 0x09, 0x02};
static const uint8_t kCreateAccelChannelsPatched[] = {0x8D, 0x44, 0x09, 0x01};
