// Patches for HDMI audio on AMD Vega iGPUs
//
// Copyright © 2022-2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <Headers/kern_patcher.hpp>
#include <Headers/kern_util.hpp>
#include <IOKit/IOService.h>
#include <PenguinWizardry/KernelVersion.hpp>
#include <PenguinWizardry/ObjectField.hpp>

namespace iVega
{

    struct GFXHDAController
    {
        ObjectField<IOService*> provider;
        ObjectField<uint32_t>   vendorID;
        ObjectField<uint32_t>   deviceID;
        ObjectField<uint16_t>   integratedCodecAddressMask;
        ObjectField<uint16_t>   codecAddressMask;
        ObjectField<bool>       useRirb;
        ObjectField<uint8_t>    intIndex;
        ObjectField<uint32_t>   inputSampleLatency;
        ObjectField<uint32_t>   outputSampleLatency;
        ObjectField<uint32_t>   inputSafetyOffset;
        ObjectField<uint32_t>   outputSafetyOffset;
        ObjectField<uint32_t>   inputSafetyOffsetLowPower;
        ObjectField<uint32_t>   outputSafetyOffsetLowPower;
        ObjectField<uint32_t>   inputEntrySize;
        ObjectField<uint32_t>   outputEntrySize;
        ObjectField<bool>       regAccessReady;

        GFXHDAController()
        {
            this->provider = 0x98;
            if (currentKernelVersion() >= MACOS_10_15) {
                this->vendorID                   = 0xEF0;
                this->deviceID                   = 0xEF4;
                this->integratedCodecAddressMask = 0x2CA;
                this->codecAddressMask           = 0x2CC;
                this->useRirb                    = 0x414;
                this->intIndex                   = 0xE69;
                this->inputSampleLatency         = 0xE6C;
                this->outputSampleLatency        = 0xE70;
                this->inputSafetyOffset          = 0xE74;
                this->outputSafetyOffset         = 0xE78;
                this->inputSafetyOffsetLowPower  = 0xE7C;
                this->outputSafetyOffsetLowPower = 0xE80;
                this->inputEntrySize             = 0x420;
                this->outputEntrySize            = 0x424;
                this->regAccessReady             = 0xEDC;
            }
            else {
                this->vendorID                   = 0xED8;
                this->deviceID                   = 0xEDC;
                this->integratedCodecAddressMask = 0x2BA;
                this->codecAddressMask           = 0x2BC;
                this->useRirb                    = 0x404;
                this->intIndex                   = 0xE59;
                this->inputSampleLatency         = 0xE5C;
                this->outputSampleLatency        = 0xE60;
                this->inputSafetyOffset          = 0xE64;
                this->outputSafetyOffset         = 0xE68;
                this->inputSafetyOffsetLowPower  = 0xE6C;
                this->outputSafetyOffsetLowPower = 0xE70;
                this->inputEntrySize             = 0x410;
                this->outputEntrySize            = 0x414;
                this->regAccessReady             = 0xEC4;
            }
        }
    };

    class AppleGFXHDA
    {
        GFXHDAController  controller;
        OSMetaClass*      orgFunctionGroupTahiti{nullptr};
        OSMetaClass*      orgWidget1002AAA0{nullptr};
        mach_vm_address_t orgProbe{0};
        mach_vm_address_t orgCreateAppleHDAFunctionGroup{0};
        mach_vm_address_t orgCreateAppleHDAWidget{0};

    public:
        static AppleGFXHDA& singleton();

        void processKext(KernelPatcher& patcher, size_t id, mach_vm_address_t slide, size_t size);

    private:
        static IOService* wrapProbe(IOService* that, IOService* provider, SInt32* score);
        static void*      wrapCreateAppleHDAFunctionGroup(void* devId);
        static void*      wrapCreateAppleHDAWidget(void* devId);
    };

};    // namespace iVega
