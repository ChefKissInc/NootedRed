// Copyright © 2022-2024 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <Headers/kern_patcher.hpp>
#include <IOKit/graphics/IOFramebuffer.h>
#include <IOKit/pci/IOPCIDevice.h>
#include <PrivateHeaders/AMDCommon.hpp>
#include <PrivateHeaders/ATOMBIOS.hpp>
#include <PrivateHeaders/NRedAttributes.hpp>

class NRed {
    NRedAttributes attributes {};
    IOPCIDevice *iGPU {nullptr};
    IOMemoryMap *rmmio {nullptr};
    volatile UInt32 *rmmioPtr {nullptr};
    OSData *vbiosData {nullptr};
    UInt32 deviceID {0};
    UInt32 pciRevision {0};
    UInt64 fbOffset {0};
    UInt16 devRevision {0};
    UInt16 enumRevision {0};

    mach_vm_address_t orgAddDrivers {0};    // TODO: Move all these to separate modules!
    mach_vm_address_t orgSafeMetaCast {0};
    mach_vm_address_t orgApplePanelSetDisplay {0};

    public:
    static NRed &singleton();

    const NRedAttributes &getAttributes() const { return this->attributes; }
    UInt32 getDeviceID() const { return deviceID; }
    UInt32 getPciRevision() const { return pciRevision; }
    UInt64 getFbOffset() const { return fbOffset; }
    UInt16 getDevRevision() const { return devRevision; }
    UInt16 getEnumRevision() const { return enumRevision; }

    void init();
    void hwLateInit();
    void processPatcher(KernelPatcher &patcher);
    void processKext(KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size);

    UInt32 readReg32(UInt32 reg) const;
    void writeReg32(UInt32 reg, UInt32 val) const;
    UInt32 smuWaitForResponse() const;
    CAILResult sendMsgToSmc(UInt32 msg, UInt32 param = 0, UInt32 *outParam = nullptr) const;

    template<typename T>
    T *getVBIOSDataTable(UInt32 index) {
        auto *vbios = static_cast<const UInt8 *>(this->vbiosData->getBytesNoCopy());
        auto base = *reinterpret_cast<const UInt16 *>(vbios + ATOM_ROM_TABLE_PTR);
        auto dataTable = *reinterpret_cast<const UInt16 *>(vbios + base + ATOM_ROM_DATA_PTR);
        auto *mdt = reinterpret_cast<const UInt16 *>(vbios + dataTable + 4);
        auto offset = mdt[index];
        return offset ? reinterpret_cast<T *>(const_cast<UInt8 *>(vbios) + offset) : nullptr;
    }

    // NOTE: Temporary hack, will removed when HWDisplay reimplementation lands!
    OSMetaClass *metaClassMap[5][2] = {{nullptr}};

    private:
    bool getVBIOSFromExpansionROM();
    bool getVBIOSFromVFCT();
    bool getVBIOSFromVRAM();

    static bool wrapAddDrivers(void *that, OSArray *array, bool doNubMatching);
    static OSMetaClassBase *wrapSafeMetaCast(const OSMetaClassBase *anObject, const OSMetaClass *toMeta);
    static size_t wrapFunctionReturnZero();
    static bool wrapApplePanelSetDisplay(IOService *that, IODisplay *display);
};
