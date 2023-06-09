//  Copyright © 2022-2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.0. See LICENSE for
//  details.

#pragma once
#include <Headers/kern_patcher.hpp>
#include <IOKit/IOService.h>
#include <IOKit/graphics/IOGraphicsTypes.h>

using t_DceDriverSetBacklight = void (*)(void *panelCntl, uint32_t backlightPwm);
using t_MessageAccelerator = IOReturn (*)(void *that, uint32_t requestType, void *arg2, void *arg3, void *arg4);

class X6000FB {
    friend class PRODUCT_NAME;

    public:
    static X6000FB *callback;
    void init();
    bool processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size);

    private:
    t_DceDriverSetBacklight orgDceDriverSetBacklight {nullptr};
    mach_vm_address_t orgDcePanelCntlHwInit {0};
    mach_vm_address_t orgFramebufferSetAttribute {0}, orgFramebufferGetAttribute {0};
    uint32_t curPwmBacklightLvl {0}, maxPwmBacklightLvl {0xFF7B};
    void *panelCntlPtr {nullptr};
    IONotifier *dispNotif {nullptr};
    mach_vm_address_t orgGetNumberOfConnectors {0};
    mach_vm_address_t orgIH40IVRingInitHardware {0}, orgIRQMGRWriteRegister {0};
    t_MessageAccelerator orgMessageAccelerator {nullptr};
    mach_vm_address_t orgControllerPowerUp {0};

    static bool OnAppleBacklightDisplayLoad(void *target, void *refCon, IOService *newService, IONotifier *notifier);
    void registerDispMaxBrightnessNotif();

    static uint16_t wrapGetEnumeratedRevision();
    static IOReturn wrapPopulateVramInfo(void *that, void *fwInfo);
    static bool wrapInitWithPciInfo(void *that, void *param1);
    static void wrapDoGPUPanic();
    static uint32_t wrapDcePanelCntlHwInit(void *panelCntl);
    static IOReturn wrapFramebufferSetAttribute(IOService *framebuffer, IOIndex connectIndex, IOSelect attribute,
        uintptr_t value);
    static IOReturn wrapFramebufferGetAttribute(IOService *framebuffer, IOIndex connectIndex, IOSelect attribute,
        uintptr_t *value);
    static uint32_t wrapGetNumberOfConnectors(void *that);
    static bool wrapIH40IVRingInitHardware(void *ctx, void *param2);
    static void wrapIRQMGRWriteRegister(void *ctx, uint64_t index, uint32_t value);
    static uint32_t wrapControllerPowerUp(void *that);
};
