//! Copyright © 2022-2024 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5.
//! See LICENSE for details.

#pragma once
#include "AMDCommon.hpp"
#include "ATOMBIOS.hpp"
#include <Headers/kern_patcher.hpp>
#include <IOKit/acpi/IOACPIPlatformExpert.h>
#include <IOKit/graphics/IOFramebuffer.h>
#include <IOKit/pci/IOPCIDevice.h>

enum struct ChipType : UInt32 {
    Raven = 0,
    Picasso,
    Raven2,
    Renoir,
    GreenSardine,
    Unknown,
};

//! Hack
class AppleACPIPlatformExpert : IOACPIPlatformExpert {
    friend class NRed;
};

//! https://elixir.bootlin.com/linux/latest/source/drivers/gpu/drm/amd/amdgpu/amdgpu_bios.c#L49
static bool checkAtomBios(const UInt8 *bios, size_t size) {
    UInt16 tmp, bios_header_start;

    if (size < 0x49) {
        DBGLOG("NRed", "VBIOS size is invalid");
        return false;
    }

    if (bios[0] != 0x55 || bios[1] != 0xAA) {
        DBGLOG("NRed", "VBIOS signature <%x %x> is invalid", bios[0], bios[1]);
        return false;
    }

    bios_header_start = bios[0x48] | (bios[0x49] << 8);
    if (!bios_header_start) {
        DBGLOG("NRed", "Unable to locate VBIOS header");
        return false;
    }

    tmp = bios_header_start + 4;
    if (size < tmp) {
        DBGLOG("NRed", "BIOS header is broken");
        return false;
    }

    if (!memcmp(bios + tmp, "ATOM", 4) || !memcmp(bios + tmp, "MOTA", 4)) {
        DBGLOG("NRed", "ATOMBIOS detected");
        return true;
    }

    return false;
}

class NRed {
    friend class X6000FB;
    friend class AppleGFXHDA;
    friend class X5000HWLibs;
    friend class X6000;
    friend class X5000;

    public:
    static NRed *callback;

    void init();
    void processPatcher(KernelPatcher &patcher);
    void setRMMIOIfNecessary();
    void processKext(KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size);

    private:
    static const char *getChipName() {
        PANIC_COND(callback->chipType == ChipType::Unknown, "NRed", "Unknown chip type");
        static const char *chipNames[] = {"raven", "picasso", "raven2", "renoir", "green_sardine"};
        return chipNames[static_cast<int>(callback->chipType)];
    }

    static const char *getGCPrefix() {
        PANIC_COND(callback->chipType == ChipType::Unknown, "NRed", "Unknown chip type");
        static const char *gcPrefixes[] = {"gc_9_1_", "gc_9_1_", "gc_9_2_", "gc_9_3_", "gc_9_3_"};
        return gcPrefixes[static_cast<int>(callback->chipType)];
    }

    bool getVBIOSFromVFCT() {
        DBGLOG("NRed", "Fetching VBIOS from VFCT table");
        auto *expert = reinterpret_cast<AppleACPIPlatformExpert *>(this->iGPU->getPlatform());
        PANIC_COND(!expert, "NRed", "Failed to get AppleACPIPlatformExpert");

        auto *vfctData = expert->getACPITableData("VFCT", 0);
        if (!vfctData) {
            DBGLOG("NRed", "No VFCT from AppleACPIPlatformExpert");
            return false;
        }

        auto *vfct = static_cast<const VFCT *>(vfctData->getBytesNoCopy());
        PANIC_COND(!vfct, "NRed", "VFCT OSData::getBytesNoCopy returned null");

        for (auto offset = vfct->vbiosImageOffset; offset < vfctData->getLength();) {
            auto *vHdr =
                static_cast<const GOPVideoBIOSHeader *>(vfctData->getBytesNoCopy(offset, sizeof(GOPVideoBIOSHeader)));
            if (!vHdr) {
                DBGLOG("NRed", "VFCT header out of bounds");
                return false;
            }

            auto *vContent = static_cast<const UInt8 *>(
                vfctData->getBytesNoCopy(offset + sizeof(GOPVideoBIOSHeader), vHdr->imageLength));
            if (!vContent) {
                DBGLOG("NRed", "VFCT VBIOS image out of bounds");
                return false;
            }

            offset += sizeof(GOPVideoBIOSHeader) + vHdr->imageLength;

            if (vHdr->deviceID == this->deviceId) {
                if (!checkAtomBios(vContent, vHdr->imageLength)) {
                    DBGLOG("NRed", "VFCT VBIOS is not an ATOMBIOS");
                    return false;
                }
                this->vbiosData = OSData::withBytes(vContent, vHdr->imageLength);
                PANIC_COND(UNLIKELY(!this->vbiosData), "NRed", "VFCT OSData::withBytes failed");
                this->iGPU->setProperty("ATY,bin_image", this->vbiosData);
                return true;
            }
        }

        return false;
    }

    bool getVBIOSFromVRAM() {
        auto *bar0 =
            this->iGPU->mapDeviceMemoryWithRegister(kIOPCIConfigBaseAddress0, kIOInhibitCache | kIOMapAnywhere);
        if (!bar0 || !bar0->getLength()) {
            DBGLOG("NRed", "FB BAR not enabled");
            OSSafeReleaseNULL(bar0);
            return false;
        }
        auto *fb = reinterpret_cast<const UInt8 *>(bar0->getVirtualAddress());
        UInt32 size = 256 * 1024;    //! ???
        if (!checkAtomBios(fb, size)) {
            DBGLOG("NRed", "VRAM VBIOS is not an ATOMBIOS");
            OSSafeReleaseNULL(bar0);
            return false;
        }
        this->vbiosData = OSData::withBytes(fb, size);
        PANIC_COND(UNLIKELY(!this->vbiosData), "NRed", "VRAM OSData::withBytes failed");
        this->iGPU->setProperty("ATY,bin_image", this->vbiosData);
        OSSafeReleaseNULL(bar0);
        return true;
    }

    UInt32 readReg32(UInt32 reg) {
        if (reg * 4 < this->rmmio->getLength()) {
            return this->rmmioPtr[reg];
        } else {
            this->rmmioPtr[mmPCIE_INDEX2] = reg;
            return this->rmmioPtr[mmPCIE_DATA2];
        }
    }

    void writeReg32(UInt32 reg, UInt32 val) {
        if ((reg * 4) < this->rmmio->getLength()) {
            this->rmmioPtr[reg] = val;
        } else {
            this->rmmioPtr[mmPCIE_INDEX2] = reg;
            this->rmmioPtr[mmPCIE_DATA2] = val;
        }
    }

    UInt32 smuWaitForResponse() {
        UInt32 ret = 0;
        for (UInt32 i = 0; i < AMDGPU_MAX_USEC_TIMEOUT; i++) {
            ret = this->readReg32(MP_BASE + mmMP1_SMN_C2PMSG_90);
            if (ret) return ret;

            IOSleep(1);
        }

        return ret;
    }

    UInt32 sendMsgToSmc(UInt32 msg, UInt32 param = 0) {
        this->smuWaitForResponse();

        this->writeReg32(MP_BASE + mmMP1_SMN_C2PMSG_90, 0);
        this->writeReg32(MP_BASE + mmMP1_SMN_C2PMSG_82, param);
        this->writeReg32(MP_BASE + mmMP1_SMN_C2PMSG_66, msg);

        SYSLOG_COND(this->smuWaitForResponse() == 0, "NRed", "No response from SMU");

        return this->readReg32(MP_BASE + mmMP1_SMN_C2PMSG_82);
    }

    template<typename T>
    T *getVBIOSDataTable(UInt32 index) {
        auto *vbios = static_cast<const UInt8 *>(this->vbiosData->getBytesNoCopy());
        auto base = *reinterpret_cast<const UInt16 *>(vbios + ATOM_ROM_TABLE_PTR);
        auto dataTable = *reinterpret_cast<const UInt16 *>(vbios + base + ATOM_ROM_DATA_PTR);
        auto *mdt = reinterpret_cast<const UInt16 *>(vbios + dataTable + 4);
        auto offset = mdt[index];
        return offset ? reinterpret_cast<T *>(const_cast<UInt8 *>(vbios) + offset) : nullptr;
    }

    OSData *vbiosData {nullptr};
    ChipType chipType {ChipType::Unknown};
    UInt64 fbOffset {0};
    IOMemoryMap *rmmio {nullptr};
    volatile UInt32 *rmmioPtr {nullptr};
    UInt32 deviceId {0};
    UInt16 enumRevision {0};
    UInt16 revision {0};
    UInt32 pciRevision {0};
    IOPCIDevice *iGPU {nullptr};
    OSMetaClass *metaClassMap[5][2] = {{nullptr}};
    mach_vm_address_t orgSafeMetaCast {0};
    bool enableBacklight {false};
    mach_vm_address_t orgApplePanelSetDisplay {0};

    static OSMetaClassBase *wrapSafeMetaCast(const OSMetaClassBase *anObject, const OSMetaClass *toMeta);
    static size_t wrapFunctionReturnZero();
    static bool wrapApplePanelSetDisplay(IOService *that, IODisplay *display);
};

//------ Patches ------//

//! Change frame-buffer count >= 2 check to >= 1.
static const UInt8 kAGDPFBCountCheckOriginal[] = {0x02, 0x00, 0x00, 0x83, 0xF8, 0x02};
static const UInt8 kAGDPFBCountCheckPatched[] = {0x02, 0x00, 0x00, 0x83, 0xF8, 0x01};

//! Ditto
static const UInt8 kAGDPFBCountCheckVenturaOriginal[] = {0x41, 0x83, 0xBE, 0x14, 0x02, 0x00, 0x00, 0x02};
static const UInt8 kAGDPFBCountCheckVenturaPatched[] = {0x41, 0x83, 0xBE, 0x14, 0x02, 0x00, 0x00, 0x01};

//! Neutralise access to AGDP configuration by board identifier.
static const UInt8 kAGDPBoardIDKeyOriginal[] = "board-id";
static const UInt8 kAGDPBoardIDKeyPatched[] = "applehax";
