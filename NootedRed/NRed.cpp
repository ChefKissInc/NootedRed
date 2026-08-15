// Master Logic
//
// Copyright © 2022-2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#include <AGDP.hpp>
#include <AppleGFXHDA.hpp>
#include <Backlight.hpp>
#include <DebugEnabler.hpp>
#include <DriverInjector.hpp>
#include <GPUDriversAMD/ATOMBIOS.hpp>
#include <GPUDriversAMD/CAIL/Result.hpp>
#include <GPUDriversAMD/RavenIPOffset.hpp>
#include <GPUDriversAMD/SMU.hpp>
#include <GPUDriversAMD/TTL/SWIP/SMU.hpp>
#include <HWLibs.hpp>
#include <Headers/kern_api.hpp>
#include <Headers/kern_devinfo.hpp>
#include <Headers/kern_iokit.hpp>
#include <Headers/kern_patcher.hpp>
#include <Headers/kern_util.hpp>
#include <IOKit/IOLib.h>
#include <IOKit/IOTypes.h>
#include <IOKit/pci/IOPCIDevice.h>
#include <Kexts.hpp>
#include <NRed.hpp>
#include <PenguinWizardry/RuntimeMC.hpp>
#include <Regs/GC.hpp>
#include <Regs/NBIO.hpp>
#include <Regs/SMU.hpp>
#include <X5000.hpp>
#include <X6000FB.hpp>
#include <kern/clock.h>
#include <libkern/OSTypes.h>
#include <libkern/c++/OSMetaClass.h>
#include <mach/i386/vm_types.h>

static NRed moduleInstance;

NRed& NRed::singleton() { return moduleInstance; }

void NRed::init()
{
    SYSLOG("NRed", "|-----------------------------------------------------------------|");
    SYSLOG("NRed", "| Copyright 2022-2025 ChefKiss.                                   |");
    SYSLOG("NRed", "| If you've paid for this, you've been scammed. Ask for a refund! |");
    SYSLOG("NRed", "| Do not support tonymacx86. Support us, we truly care.           |");
    SYSLOG("NRed", "| Change the world for the better.                                |");
    SYSLOG("NRed", "|-----------------------------------------------------------------|");

    Backlight::singleton().init();

    lilu.onKextLoadForce(&kextRadeonX6000Framebuffer);
    lilu.onKextLoadForce(&kextRadeonX5000HWLibs);
    lilu.onKextLoadForce(&kextRadeonX5000);
    lilu.onKextLoadForce(&kextAGDP);
    lilu.onKextLoadForce(&kextAppleGFXHDA);

    lilu.onPatcherLoadForce(
        [](void* const, KernelPatcher& patcher)
        {
            singleton().processPatcher();
            DriverInjector::singleton().processPatcher(patcher);
            PenguinWizardry::RuntimeMCManager::singleton().processPatcher(patcher);
        },
        nullptr);

    lilu.onKextLoadForce(
        nullptr, 0,
        [](void* const, KernelPatcher& patcher, const size_t id, const mach_vm_address_t slide, const size_t size)
        {
            AGDP::singleton().processKext(patcher, id, slide, size);
            Backlight::singleton().processKext(patcher, id, slide, size);
            DebugEnabler::singleton().processKext(patcher, id, slide, size);
            X6000FB::singleton().processKext(patcher, id, slide, size);
            AppleGFXHDA::singleton().processKext(patcher, id, slide, size);
            X5000HWLibs::singleton().processKext(patcher, id, slide, size);
            X5000::singleton().processKext(patcher, id, slide, size);
        },
        nullptr);
}

void NRed::hwLateInit()
{
    if (this->rmmio != nullptr) { return; }

    this->iGPU->setMemoryEnable(true);
    this->iGPU->setBusMasterEnable(true);

    this->rmmio =
        this->iGPU->mapDeviceMemoryWithRegister(kIOPCIConfigBaseAddress5, kIOMapInhibitCache | kIOMapAnywhere);
    PANIC_COND(this->rmmio == nullptr || this->rmmio->getLength() == 0, "NRed", "Failed to map RMMIO");
    this->rmmioPtr = reinterpret_cast<volatile UInt32*>(this->rmmio->getVirtualAddress());

    this->fbOffset    = static_cast<UInt64>(this->readReg32(GC_BASE_0 + MC_VM_FB_OFFSET) & 0xFFFFFF) << 24;
    this->devRevision = (this->readReg32(NBIO_BASE_2 + RCC_DEV0_EPF0_STRAP0) & RCC_DEV0_EPF0_STRAP0_ATI_REV_ID_MASK)
                        >> RCC_DEV0_EPF0_STRAP0_ATI_REV_ID_SHIFT;

    if (this->attributes.isRenoir()) {
        if (!this->attributes.isGreenSardine() && this->devRevision == 0 && this->pciRevision >= 0x80
            && this->pciRevision <= 0x84)
        {
            this->attributes.setRenoirE();
        }
    }
    else {
        if (this->devRevision >= 0x8) {
            this->attributes.setRaven2();
            this->enumRevision = 0x79;
        }
        else if (this->attributes.isPicasso()) {
            this->enumRevision = 0x41;
        }
        else if (this->devRevision == 1) {
            this->enumRevision = 0x20;
        }
        else {
            this->enumRevision = 0x1;
        }
    }

    DBGLOG("NRed", "deviceID = 0x%X", this->deviceID);
    DBGLOG("NRed", "pciRevision = 0x%X", this->pciRevision);
    DBGLOG("NRed", "fbOffset = 0x%llX", this->fbOffset);
    DBGLOG("NRed", "devRevision = 0x%X", this->devRevision);
    DBGLOG("NRed", "isPicasso = %s", this->attributes.isPicasso() ? "true" : "false");
    DBGLOG("NRed", "isRaven2 = %s", this->attributes.isRaven2() ? "true" : "false");
    DBGLOG("NRed", "isRenoir = %s", this->attributes.isRenoir() ? "true" : "false");
    DBGLOG("NRed", "isGreenSardine = %s", this->attributes.isGreenSardine() ? "true" : "false");
    DBGLOG("NRed", "enumRevision = 0x%X", this->enumRevision);
}

void NRed::processPatcher()
{
    const auto devInfo = DeviceInfo::create();
    assert(devInfo != nullptr);

    devInfo->processSwitchOff();

    PANIC_COND(devInfo->videoBuiltin == nullptr, "NRed", "No iGPU detected by Lilu");
    this->iGPU = OSDynamicCast(IOPCIDevice, devInfo->videoBuiltin);
    PANIC_COND(WIOKit::readPCIConfigValue(this->iGPU, WIOKit::kIOPCIConfigVendorID) != WIOKit::VendorID::ATIAMD, "NRed",
               "iGPU is not an AMD one");

    WIOKit::renameDevice(this->iGPU, "IGPU");
    WIOKit::awaitPublishing(this->iGPU);
    UInt8 builtInBytes[] = {0x00};
    this->iGPU->setProperty("built-in", builtInBytes, sizeof(builtInBytes));
    char slotNameBytes[] = "built-in";
    this->iGPU->setProperty("AAPL,slot-name", slotNameBytes, sizeof(slotNameBytes));
    char hdaGfxBytes[] = "onboard-1";
    this->iGPU->setProperty("hda-gfx", hdaGfxBytes, sizeof(hdaGfxBytes));

    this->deviceID = static_cast<UInt16>(WIOKit::readPCIConfigValue(this->iGPU, WIOKit::kIOPCIConfigDeviceID));
    switch (this->deviceID) {
        case 0x15D8: {
            this->attributes.setPicasso();
        } break;
        case 0x15DD: {
        } break;
        case 0x164C:
        case 0x1636: {
            this->attributes.setRenoir();
            this->enumRevision = 0x91;
        } break;
        case 0x15E7:
        case 0x1638: {
            this->attributes.setRenoir();
            this->attributes.setGreenSardine();
            this->enumRevision = 0xA1;
        } break;
        default: PANIC("NRed", "Unknown device ID: 0x%X", this->deviceID);
    }
    this->pciRevision = static_cast<UInt8>(WIOKit::readPCIConfigValue(this->iGPU, WIOKit::kIOPCIConfigRevisionID));

    char name[128];
    for (size_t i = 0, ii = 0; i < devInfo->videoExternal.size(); i++) {
        auto device = OSDynamicCast(IOPCIDevice, devInfo->videoExternal[i].video);
        if (device == nullptr) { continue; }

        snprintf(name, arrsize(name), "GFX%zu", ii++);
        WIOKit::renameDevice(device, name);
        WIOKit::awaitPublishing(device);
    }

    DeviceInfo::deleter(devInfo);
}

void NRed::setProp32(const char* const key, const UInt32 value) const { this->iGPU->setProperty(key, value, 32); }

UInt32 NRed::readReg32(const UInt32 reg) const
{
    if ((reg * sizeof(UInt32)) < this->rmmio->getLength()) { return this->rmmioPtr[reg]; }
    else {
        this->rmmioPtr[PCIE_INDEX2] = reg;
        return this->rmmioPtr[PCIE_DATA2];
    }
}
