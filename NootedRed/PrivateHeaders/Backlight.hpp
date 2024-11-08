// Copyright © 2024 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <Headers/kern_patcher.hpp>
#include <IOKit/IOService.h>
#include <IOKit/graphics/IOFramebuffer.h>
#include <IOKit/graphics/IOGraphicsTypes.h>
#include <PrivateHeaders/ObjectField.hpp>

class Backlight {
    using t_DceDriverSetBacklight = void (*)(void *panelCntl, UInt32 backlightPwm);
    using t_DcLinkSetBacklightLevel = bool (*)(void *link, UInt32 backlightPwm, UInt32 frameRamp);
    using t_DcLinkSetBacklightLevelNits = bool (*)(void *link, bool isHDR, UInt32 backlightMillinits,
        UInt32 transitionTimeMs);

    bool initialised {false};
    ObjectField<UInt8> dcLinkCapsField {};
    UInt32 curPwmBacklightLvl {0}, maxPwmBacklightLvl {0xFFFF};
    UInt32 maxOLED {1000 * 512};
    IONotifier *dispNotif {nullptr};
    void *embeddedPanelLink {nullptr};
    bool supportsAUX {false};
    t_DceDriverSetBacklight orgDceDriverSetBacklight {nullptr};
    mach_vm_address_t orgDcePanelCntlHwInit {0};
    void *panelCntlPtr {nullptr};
    mach_vm_address_t orgLinkCreate {0};
    t_DcLinkSetBacklightLevel orgDcLinkSetBacklightLevel {0};
    t_DcLinkSetBacklightLevelNits orgDcLinkSetBacklightLevelNits {0};
    mach_vm_address_t orgSetAttributeForConnection {0};
    mach_vm_address_t orgGetAttributeForConnection {0};
    mach_vm_address_t orgApplePanelSetDisplay {0};

    public:
    static Backlight &singleton();

    void init();

    private:
    void processKext(KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size);

    static bool OnAppleBacklightDisplayLoad(void *target, void *refCon, IOService *newService, IONotifier *notifier);
    void registerDispMaxBrightnessNotif();

    static UInt32 wrapDcePanelCntlHwInit(void *panelCntl);
    static void *wrapLinkCreate(void *data);
    static IOReturn wrapSetAttributeForConnection(IOService *framebuffer, IOIndex connectIndex, IOSelect attribute,
        uintptr_t value);
    static IOReturn wrapGetAttributeForConnection(IOService *framebuffer, IOIndex connectIndex, IOSelect attribute,
        uintptr_t *value);
    static size_t wrapFunctionReturnZero();
    static bool wrapApplePanelSetDisplay(IOService *that, IODisplay *display);
};
