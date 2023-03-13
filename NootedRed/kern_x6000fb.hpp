//  Copyright © 2022-2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.0. See LICENSE for
//  details.

#pragma once
#include <Headers/kern_patcher.hpp>
#include <Headers/kern_util.hpp>
#include <IOKit/graphics/IOFramebuffer.h>

using t_DceDriverSetBacklight = void (*)(void *panel_cntl, uint32_t backlight_pwm_u16_16);

class X6000FB {
    public:
    static X6000FB *callback;
    void init();
    bool processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size);

    private:
    mach_vm_address_t orgPopulateDeviceInfo {0};
    mach_vm_address_t orgHwReadReg32 {0};
    mach_vm_address_t orgInitWithPciInfo {0};
    t_DceDriverSetBacklight orgDceDriverSetBacklight {nullptr};
    mach_vm_address_t orgDcePanelCntlHwInit {0};
    mach_vm_address_t orgAMDRadeonX6000AmdRadeonFramebufferSetAttribute {0};
    mach_vm_address_t orgAMDRadeonX6000AmdRadeonFramebufferGetAttribute {0};
    uint32_t curPwmBacklightLvl = 0;
    uint32_t maxPwmBacklightLvl = 0xff7b;
    void *panelCntlPtr = NULL;

    void updatePwmMaxBrightnessFromInternalDisplay();

    static IOReturn wrapPopulateDeviceInfo(void *that);
    static uint16_t wrapGetEnumeratedRevision();
    static IOReturn wrapPopulateVramInfo(void *that, void *fwInfo);
    static uint32_t wrapHwReadReg32(void *that, uint32_t param1);
    static bool wrapInitWithPciInfo(void *that, void *param1);
    static void wrapDoGPUPanic();
    static uint32_t wrapDcePanelCntlHwInit(void *panel_cntl);
    static IOReturn wrapAMDRadeonX6000AmdRadeonFramebufferSetAttribute(IOService *framebuffer, IOIndex connectIndex,
        IOSelect attribute, uintptr_t value);
    static IOReturn wrapAMDRadeonX6000AmdRadeonFramebufferGetAttribute(IOService *framebuffer, IOIndex connectIndex,
        IOSelect attribute, uintptr_t *value);
};
