// AMD Display Backlight
// TODO: Get rid of this absolute garbage.
//
// Copyright (C) 2024-2026  ChefKiss.
//
// Licensed under the ChefKiss Open Knowledge License version 1.
//
// This work is licensed to you under the terms of the ChefKiss Open Knowledge
// License, either version 1 of the License, or (at your option) any later
// version published by ChefKiss.
//
// This work may not be used to train or develop artificial intelligence,
// and may not be used with artificial intelligence in any other way. See
// the ChefKiss Open Knowledge License for details.
//
// This work is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// ChefKiss Open Knowledge License for more details.
//
// You should have received a copy of the ChefKiss Open Knowledge License
// along with this work.  If not, see https://git.chefkiss.dev/ChefKiss/OpenKnowledge/raw/branch/master/LICENSE-v1.md.

#pragma once
#include <GPUDriversAMD/DC/DPCD.hpp>
#include <Headers/kern_patcher.hpp>
#include <IOKit/IOService.h>
#include <IOKit/graphics/IOFramebuffer.h>
#include <IOKit/graphics/IOGraphicsTypes.h>
#include <PenguinWizardry/ObjectField.hpp>

class Backlight
{
    using t_DceDriverSetBacklight       = void       (*)(void* panelCntl, UInt32 backlightPwm);
    using t_DcLinkSetBacklightLevel     = bool     (*)(void* link, UInt32 backlightPwm, UInt32 frameRamp);
    using t_DcLinkSetBacklightLevelNits = bool (*)(void* link, bool isHDR, UInt32 backlightMillinits,
                                                   UInt32 transitionTimeMs);

    ObjectField<DPCDSinkExtCaps> dcLinkCapsField;
    UInt32                       curBacklightLvl{0}, maxBacklightLvl{0xFFFF};
    // TODO: Obtain the actual max brightness for the screen
    UInt32                        maxBacklightMillinits{1000 * 512};
    IONotifier*                   dispNotif{nullptr};
    void*                         embeddedPanelLink{nullptr};
    bool                          supportsAUX{false};
    mach_vm_address_t             orgLinkCreate{0};
    t_DcLinkSetBacklightLevel     orgDcLinkSetBacklightLevel{0};
    t_DcLinkSetBacklightLevelNits orgDcLinkSetBacklightLevelNits{0};
    mach_vm_address_t             orgSetAttributeForConnection{0};
    mach_vm_address_t             orgGetAttributeForConnection{0};
    mach_vm_address_t             orgApplePanelSetDisplay{0};

public:
    static Backlight& singleton();

    Backlight();

    void init();

    void processKext(KernelPatcher& patcher, size_t id, mach_vm_address_t slide, size_t size);

private:
    static bool OnAppleBacklightDisplayLoad(void* target, void* refCon, IOService* newService, IONotifier* notifier);

    static void*    wrapLinkCreate(void* data);
    static IOReturn wrapSetAttributeForConnection(IOService* framebuffer, IOIndex connectIndex, IOSelect attribute,
                                                  uintptr_t value);
    static IOReturn wrapGetAttributeForConnection(IOService* framebuffer, IOIndex connectIndex, IOSelect attribute,
                                                  uintptr_t* value);
    static size_t   returnZero();
    static bool     wrapApplePanelSetDisplay(IOService* self, IODisplay* display);
};
