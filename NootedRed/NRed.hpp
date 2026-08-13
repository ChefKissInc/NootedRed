// Master Logic
//
// Copyright © 2022-2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <GPUDriversAMD/ATOMBIOS.hpp>
#include <GPUDriversAMD/CAIL/Result.hpp>
#include <GPUDriversAMD/PowerPlay.hpp>
#include <Headers/kern_patcher.hpp>
#include <IOKit/pci/IOPCIDevice.h>

class NRed
{
    class Attributes
    {    // TODO: Remove!
        static constexpr UInt8 IsPicasso      = getBit(0);
        static constexpr UInt8 IsRaven2       = getBit(1);
        static constexpr UInt8 IsRenoir       = getBit(2);
        static constexpr UInt8 IsRenoirE      = getBit(3);
        static constexpr UInt8 IsGreenSardine = getBit(4);

        UInt8 value{0};

    public:
        constexpr bool isPicasso() const { return (this->value & IsPicasso) != 0; }
        constexpr bool isRaven2() const { return (this->value & IsRaven2) != 0; }
        constexpr bool isRenoir() const { return (this->value & IsRenoir) != 0; }
        constexpr bool isRenoirE() const { return (this->value & IsRenoirE) != 0; }
        constexpr bool isGreenSardine() const { return (this->value & IsGreenSardine) != 0; }

        constexpr void setPicasso() { this->value |= IsPicasso; }
        constexpr void setRaven2() { this->value |= IsRaven2; }
        constexpr void setRenoir() { this->value |= IsRenoir; }
        constexpr void setRenoirE() { this->value |= IsRenoirE; }
        constexpr void setGreenSardine() { this->value |= IsGreenSardine; }
    };

    Attributes       attributes;           // TODO: Remove!
    IOPCIDevice*     iGPU{nullptr};        // TODO: Remove!
    IOMemoryMap*     rmmio{nullptr};       // TODO: Remove!
    volatile UInt32* rmmioPtr{nullptr};    // TODO: Remove!
    UInt16           deviceID{0};          // TODO: Remove!
    UInt8            pciRevision{0};       // TODO: Remove!
    UInt16           devRevision{0};       // TODO: Remove!
    UInt16           enumRevision{0};      // TODO: Remove!
    UInt64           fbOffset{0};          // TODO: Remove!

public:
    static NRed& singleton();

    auto& getAttributes() const { return this->attributes; }    // TODO: Remove!
    auto  getDeviceID() const { return deviceID; }              // TODO: Remove!
    auto  getPciRevision() const { return pciRevision; }        // TODO: Remove!
    auto  getDevRevision() const { return devRevision; }        // TODO: Remove!
    auto  getEnumRevision() const { return enumRevision; }      // TODO: Remove!
    auto  getFbOffset() const { return fbOffset; }              // TODO: Remove!

    void init();
    void hwLateInit();        // TODO: Remove!
    void processPatcher();    // TODO: Remove!

    void   setProp32(const char* key, UInt32 value) const;    // TODO: Remove!
    UInt32 readReg32(UInt32 reg) const;                       // TODO: Remove!
    void   writeReg32(UInt32 reg, const UInt32 val) const;    // TODO: Remove!

private:
    OSData* copyVBIOSFromVFCT(bool strict);    // TODO: Remove!
    OSData* copyVBIOSFromVRAM();               // TODO: Remove!
    OSData* copyVBIOSFromExpansionROM();       // TODO: Remove!
    OSData* copyVBIOS();                       // TODO: Remove!
};
