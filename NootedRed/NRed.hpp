// Copyright © 2022-2024 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include "AMDCommon.hpp"
#include "ATOMBIOS.hpp"
#include "AppleGFXHDA.hpp"
#include "HWLibs.hpp"
#include "X5000.hpp"
#include "X6000.hpp"
#include "X6000FB.hpp"
#include <Headers/kern_patcher.hpp>
#include <IOKit/acpi/IOACPIPlatformExpert.h>
#include <IOKit/graphics/IOFramebuffer.h>
#include <IOKit/pci/IOPCIDevice.h>

class NRedAttributes {
    static constexpr UInt16 EnableBacklight = (1U << 0);
    static constexpr UInt16 IsCatalina = (1U << 1);
    static constexpr UInt16 IsBigSurAndLater = (1U << 2);
    static constexpr UInt16 IsMonterey = (1U << 3);
    static constexpr UInt16 IsMontereyAndLater = (1U << 4);
    static constexpr UInt16 IsVentura = (1U << 5);
    static constexpr UInt16 IsVenturaAndLater = (1U << 6);
    static constexpr UInt16 IsVentura1304Based = (1U << 7);
    static constexpr UInt16 IsVentura1304AndLater = (1U << 8);
    static constexpr UInt16 IsSonoma1404AndLater = (1U << 9);
    static constexpr UInt16 IsRaven = (1U << 10);
    static constexpr UInt16 IsPicasso = (1U << 11);
    static constexpr UInt16 IsRaven2 = (1U << 12);
    static constexpr UInt16 IsRenoir = (1U << 13);
    static constexpr UInt16 IsGreenSardine = (1U << 14);

    UInt16 value {0};

    public:
    inline bool isBacklightEnabled() const { return (this->value & EnableBacklight) != 0; }
    inline bool isCatalina() const { return (this->value & IsCatalina) != 0; }
    inline bool isBigSurAndLater() const { return (this->value & IsBigSurAndLater) != 0; }
    inline bool isMonterey() const { return (this->value & IsMonterey) != 0; }
    inline bool isMontereyAndLater() const { return (this->value & IsMontereyAndLater) != 0; }
    inline bool isVentura() const { return (this->value & IsVentura) != 0; }
    inline bool isVenturaAndLater() const { return (this->value & IsVenturaAndLater) != 0; }
    inline bool isVentura1304Based() const { return (this->value & IsVentura1304Based) != 0; }
    inline bool isVentura1304AndLater() const { return (this->value & IsVentura1304AndLater) != 0; }
    inline bool isSonoma1404AndLater() const { return (this->value & IsSonoma1404AndLater) != 0; }
    inline bool isRaven() const { return (this->value & IsRaven) != 0; }
    inline bool isPicasso() const { return (this->value & IsPicasso) != 0; }
    inline bool isRaven2() const { return (this->value & IsRaven2) != 0; }
    inline bool isRenoir() const { return (this->value & IsRenoir) != 0; }
    inline bool isGreenSardine() const { return (this->value & IsGreenSardine) != 0; }

    inline void setBacklightEnabled() { this->value |= EnableBacklight; }
    inline void setCatalina() { this->value |= IsCatalina; }
    inline void setBigSurAndLater() { this->value |= IsBigSurAndLater; }
    inline void setMonterey() { this->value |= IsMonterey; }
    inline void setMontereyAndLater() { this->value |= IsMontereyAndLater; }
    inline void setVentura() { this->value |= IsVentura; }
    inline void setVenturaAndLater() { this->value |= IsVenturaAndLater; }
    inline void setVentura1304Based() { this->value |= IsVentura1304Based; }
    inline void setVentura1304AndLater() { this->value |= IsVentura1304AndLater; }
    inline void setSonoma1404AndLater() { this->value |= IsSonoma1404AndLater; }
    inline void setRaven() { this->value |= IsRaven; }
    inline void setPicasso() { this->value |= IsPicasso; }
    inline void setRaven2() { this->value |= IsRaven2; }
    inline void setRenoir() { this->value |= IsRenoir; }
    inline void setGreenSardine() { this->value |= IsGreenSardine; }
};

class NRed {
    friend class X5000;
    friend class X6000;

    public:
    static NRed *callback;

    NRedAttributes attributes {};
    UInt32 deviceID {0};
    UInt32 pciRevision {0};
    UInt64 fbOffset {0};
    UInt16 devRevision {0};
    UInt16 enumRevision {0};

    X6000FB x6000fb {};
    AppleGFXHDA agfxhda {};
    X5000HWLibs hwlibs {};
    X6000 x6000 {};
    X5000 x5000 {};

    void init();
    void processPatcher(KernelPatcher &patcher);
    void hwLateInit();
    void processKext(KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size);

    const char *getChipName();
    const char *getGCPrefix();
    UInt32 readReg32(UInt32 reg);
    void writeReg32(UInt32 reg, UInt32 val);
    CAILResult sendMsgToSmc(UInt32 msg, UInt32 param = 0, UInt32 *outParam = nullptr);

    template<typename T>
    T *getVBIOSDataTable(UInt32 index) {
        auto *vbios = static_cast<const UInt8 *>(this->vbiosData->getBytesNoCopy());
        auto base = *reinterpret_cast<const UInt16 *>(vbios + ATOM_ROM_TABLE_PTR);
        auto dataTable = *reinterpret_cast<const UInt16 *>(vbios + base + ATOM_ROM_DATA_PTR);
        auto *mdt = reinterpret_cast<const UInt16 *>(vbios + dataTable + 4);
        auto offset = mdt[index];
        return offset ? reinterpret_cast<T *>(const_cast<UInt8 *>(vbios) + offset) : nullptr;
    }

    private:
    bool getVBIOSFromExpansionROM();
    bool getVBIOSFromVFCT();
    bool getVBIOSFromVRAM();
    UInt32 smuWaitForResponse();

    OSData *vbiosData {nullptr};
    IOMemoryMap *rmmio {nullptr};
    volatile UInt32 *rmmioPtr {nullptr};
    IOPCIDevice *iGPU {nullptr};
    mach_vm_address_t orgAddDrivers {0};
    OSMetaClass *metaClassMap[5][2] = {{nullptr}};
    mach_vm_address_t orgSafeMetaCast {0};
    mach_vm_address_t orgApplePanelSetDisplay {0};

    static bool wrapAddDrivers(void *that, OSArray *array, bool doNubMatching);
    static OSMetaClassBase *wrapSafeMetaCast(const OSMetaClassBase *anObject, const OSMetaClass *toMeta);
    static size_t wrapFunctionReturnZero();
    static bool wrapApplePanelSetDisplay(IOService *that, IODisplay *display);
};

//------ Patches ------//

// Change frame-buffer count >= 2 check to >= 1.
static const UInt8 kAGDPFBCountCheckOriginal[] = {0x02, 0x00, 0x00, 0x83, 0xF8, 0x02};
static const UInt8 kAGDPFBCountCheckPatched[] = {0x02, 0x00, 0x00, 0x83, 0xF8, 0x01};

// Ditto
static const UInt8 kAGDPFBCountCheckOriginal13[] = {0x41, 0x83, 0xBE, 0x14, 0x02, 0x00, 0x00, 0x02};
static const UInt8 kAGDPFBCountCheckPatched13[] = {0x41, 0x83, 0xBE, 0x14, 0x02, 0x00, 0x00, 0x01};

// Neutralise access to AGDP configuration by board identifier.
static const UInt8 kAGDPBoardIDKeyOriginal[] = "board-id";
static const UInt8 kAGDPBoardIDKeyPatched[] = "applehax";
