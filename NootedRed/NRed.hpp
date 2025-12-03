// Master plug-in logic
//
// Copyright © 2022-2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <GPUDriversAMD/ATOMBIOS.hpp>
#include <GPUDriversAMD/CAIL/Result.hpp>
#include <GPUDriversAMD/PowerPlay.hpp>
#include <Headers/kern_patcher.hpp>
#include <IOKit/pci/IOPCIDevice.h>

class NRed {
    class Attributes {    // TODO: Remove!
        static constexpr UInt8 IsPicasso = getBit(0);
        static constexpr UInt8 IsRaven2 = getBit(1);
        static constexpr UInt8 IsRenoir = getBit(2);
        static constexpr UInt8 IsRenoirE = getBit(3);
        static constexpr UInt8 IsGreenSardine = getBit(4);

        UInt8 value {0};

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

    Attributes attributes {};               // TODO: Remove!
    IOPCIDevice *iGPU {nullptr};            // TODO: Remove!
    IOMemoryMap *rmmio {nullptr};           // TODO: Remove!
    volatile UInt32 *rmmioPtr {nullptr};    // TODO: Remove!
    OSData *vbiosData {nullptr};            // TODO: Remove!
    UInt32 deviceID {0};                    // TODO: Remove!
    UInt32 pciRevision {0};                 // TODO: Remove!
    UInt64 fbOffset {0};                    // TODO: Remove!
    UInt16 devRevision {0};                 // TODO: Remove!
    UInt16 enumRevision {0};                // TODO: Remove!

    public:
    static NRed &singleton();

    const Attributes &getAttributes() const { return this->attributes; }    // TODO: Remove!
    UInt32 getDeviceID() const { return deviceID; }                         // TODO: Remove!
    UInt32 getPciRevision() const { return pciRevision; }                   // TODO: Remove!
    UInt64 getFbOffset() const { return fbOffset; }                         // TODO: Remove!
    UInt16 getDevRevision() const { return devRevision; }                   // TODO: Remove!
    UInt16 getEnumRevision() const { return enumRevision; }                 // TODO: Remove!

    void init();
    void hwLateInit();        // TODO: Remove!
    void processPatcher();    // TODO: Remove!

    void setProp32(const char *key, UInt32 value) const;    // TODO: Remove!
    UInt32 readReg32(UInt32 reg) const;                     // TODO: Remove!
    void writeReg32(UInt32 reg, const UInt32 val) const;    // TODO: Remove!
    static CAILResult waitForFunc(void *handle, bool (*func)(void *handle),
        const UInt32 timeoutMS = PP_WAIT_ON_REGISTER_TIMEOUT_DEFAULT);
    CAILResult smuWaitForResponse(UInt32 *outResp) const;    // TODO: Remove!
    CAILResult sendMsgToSmc(UInt32 msg, UInt32 param = 0,
        UInt32 *outParam = nullptr) const;    // TODO: Remove!

    template<typename T>    // TODO: Remove!
    T *getVBIOSDataTable(const UInt32 index) {
        auto *const vbios = static_cast<const UInt8 *>(this->vbiosData->getBytesNoCopy());
        auto const base = *reinterpret_cast<const UInt16 *>(vbios + ATOM_ROM_TABLE_PTR);
        auto const dataTable = *reinterpret_cast<const UInt16 *>(vbios + base + ATOM_ROM_DATA_PTR);
        auto *const mdt = reinterpret_cast<const UInt16 *>(vbios + dataTable + 4);
        auto const offset = mdt[index];
        return offset ? reinterpret_cast<T *>(const_cast<UInt8 *>(vbios) + offset) : nullptr;
    }

    private:
    bool getVBIOSFromVFCT(bool strict);    // TODO: Remove!
    bool getVBIOSFromVRAM();               // TODO: Remove!
    bool getVBIOSFromExpansionROM();       // TODO: Remove!
    bool getVBIOS();                       // TODO: Remove!
};
