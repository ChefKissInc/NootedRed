// Patches for HDMI audio on AMD Vega iGPUs
//
// Copyright © 2022-2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#include <Headers/kern_iokit.hpp>
#include <Headers/kern_patcher.hpp>
#include <Headers/kern_util.hpp>
#include <Kexts.hpp>
#include <NRed.hpp>
#include <iVega/AppleGFXHDA.hpp>
#include <libkern/OSTypes.h>

static constexpr UInt32 AMDVendorID        = 0x1002;
static constexpr UInt32 RavenHDMIDeviceID  = 0x15DE;
static constexpr UInt32 RenoirHDMIDeviceID = 0x1637;

static iVega::AppleGFXHDA moduleInstance;

iVega::AppleGFXHDA& iVega::AppleGFXHDA::singleton() { return moduleInstance; }

void iVega::AppleGFXHDA::processKext(KernelPatcher& patcher, size_t id, mach_vm_address_t slide, size_t size)
{
    if (kextAppleGFXHDA.loadIndex != id) { return; }

    NRed::singleton().hwLateInit();

    KernelPatcher::SolveRequest solveRequests[] = {
        {"__ZN34AppleGFXHDAFunctionGroupATI_Tahiti10gMetaClassE", this->orgFunctionGroupTahiti},
        {        "__ZN26AppleGFXHDAWidget_1002AAA010gMetaClassE",      this->orgWidget1002AAA0},
    };
    PANIC_COND(!patcher.solveMultiple(id, solveRequests, slide, size, true), "AGFXHDA", "Failed to solve symbols");

    KernelPatcher::RouteRequest requests[] = {
        {                                  "__ZN21AppleGFXHDAController5probeEP9IOServicePi",wrapProbe,this->orgProbe                                         },
        {"__ZN31AppleGFXHDAFunctionGroupFactory27createAppleHDAFunctionGroupEP11DevIdStruct",
         wrapCreateAppleHDAFunctionGroup, this->orgCreateAppleHDAFunctionGroup},
        {              "__ZN24AppleGFXHDAWidgetFactory20createAppleHDAWidgetEP11DevIdStruct", wrapCreateAppleHDAWidget,
         this->orgCreateAppleHDAWidget                                        },
    };
    PANIC_COND(!patcher.routeMultipleLong(id, requests, slide, size), "AGFXHDA", "Failed to route symbols");
}

IOService* iVega::AppleGFXHDA::wrapProbe(IOService* that, IOService* provider, SInt32* score)
{
    const auto dev = OSDynamicCast(IOPCIDevice, provider);
    if (dev == nullptr) { return FunctionCast(wrapProbe, singleton().orgProbe)(that, provider, score); }

    const auto vendorID = WIOKit::readPCIConfigValue(dev, WIOKit::kIOPCIConfigVendorID);
    const auto deviceID = WIOKit::readPCIConfigValue(dev, WIOKit::kIOPCIConfigDeviceID);
    if (vendorID != AMDVendorID || (deviceID != RavenHDMIDeviceID && deviceID != RenoirHDMIDeviceID)) {
        DBGLOG("GFXHDA", "Device is not Raven or Renoir, calling org.");
        return FunctionCast(wrapProbe, singleton().orgProbe)(that, provider, score);
    }

    UInt8 bytes[] = {0x00};
    dev->setProperty("built-in", bytes, sizeof(bytes));

    that->setProperty("IsNRedHDA", bytes, sizeof(bytes));
    dev->setProperty("IsNRedHDA", bytes, sizeof(bytes));

    singleton().controller.provider(that)                   = provider;
    singleton().controller.vendorID(that)                   = vendorID;
    singleton().controller.deviceID(that)                   = deviceID;
    singleton().controller.integratedCodecAddressMask(that) = 0x1;
    singleton().controller.codecAddressMask(that)           = 0x1;
    singleton().controller.useRirb(that)                    = true;
    singleton().controller.intIndex(that)                   = 1;
    singleton().controller.inputSampleLatency(that)         = 1;
    singleton().controller.outputSampleLatency(that)        = 1;
    singleton().controller.inputSafetyOffset(that)          = 1;
    singleton().controller.outputSafetyOffset(that)         = 1;
    singleton().controller.inputSafetyOffsetLowPower(that)  = 1;
    singleton().controller.outputSafetyOffsetLowPower(that) = 1;
    singleton().controller.inputEntrySize(that)             = 0x3000;
    singleton().controller.outputEntrySize(that)            = 0x3000;
    singleton().controller.regAccessReady(that)             = true;

    DBGLOG("GFXHDA", "Initialised Raven or Renoir device.");

    return that;
}

void* iVega::AppleGFXHDA::wrapCreateAppleHDAFunctionGroup(void* devId)
{
    const auto vendorID = getMember<UInt16>(devId, 0x2);
    const auto deviceID = getMember<UInt32>(devId, 0x8);
    if (vendorID == AMDVendorID && (deviceID == RavenHDMIDeviceID || deviceID == RenoirHDMIDeviceID)) {
        return singleton().orgFunctionGroupTahiti->alloc();
    }
    return FunctionCast(wrapCreateAppleHDAFunctionGroup, singleton().orgCreateAppleHDAFunctionGroup)(devId);
}

void* iVega::AppleGFXHDA::wrapCreateAppleHDAWidget(void* devId)
{
    auto vendorID = getMember<UInt16>(devId, 0x2);
    auto deviceID = getMember<UInt32>(devId, 0x8);
    if (vendorID == AMDVendorID && (deviceID == RavenHDMIDeviceID || deviceID == RenoirHDMIDeviceID)) {
        return singleton().orgWidget1002AAA0->alloc();
    }
    return FunctionCast(wrapCreateAppleHDAFunctionGroup, singleton().orgCreateAppleHDAFunctionGroup)(devId);
}
