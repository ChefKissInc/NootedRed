//  Copyright © 2022-2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.0. See LICENSE for
//  details.

#ifndef kern_x6000fb_hpp
#define kern_x6000fb_hpp
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
    mach_vm_address_t orgFramebufferSetAttribute {0};
    mach_vm_address_t orgFramebufferGetAttribute {0};
    uint32_t curPwmBacklightLvl {0};
    uint32_t maxPwmBacklightLvl {0xFF7B};
    void *panelCntlPtr {nullptr};
    IONotifier *dispNotif {nullptr};

    static bool OnAppleBacklightDisplayLoad(void *target, void *refCon, IOService *newService, IONotifier *notifier);
    void registerDispMaxBrightnessNotif();

    static IOReturn wrapPopulateDeviceInfo(void *that);
    static uint16_t wrapGetEnumeratedRevision();
    static IOReturn wrapPopulateVramInfo(void *that, void *fwInfo);
    static uint32_t wrapHwReadReg32(void *that, uint32_t param1);
    static bool wrapInitWithPciInfo(void *that, void *param1);
    static void wrapDoGPUPanic();
    static uint32_t wrapDcePanelCntlHwInit(void *panelCntl);
    static IOReturn wrapFramebufferSetAttribute(IOService *framebuffer, IOIndex connectIndex, IOSelect attribute,
        uintptr_t value);
    static IOReturn wrapFramebufferGetAttribute(IOService *framebuffer, IOIndex connectIndex, IOSelect attribute,
        uintptr_t *value);
};

#endif /* kern_x6000fb_hpp */
