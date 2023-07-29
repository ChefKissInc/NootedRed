//  Copyright © 2022-2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.0. See LICENSE for
//  details.

#pragma once
#include "kern_amd.hpp"
#include "kern_fw.hpp"
#include "kern_vbios.hpp"
#include <Headers/kern_patcher.hpp>
#include <IOKit/acpi/IOACPIPlatformExpert.h>
#include <IOKit/graphics/IOFramebuffer.h>
#include <IOKit/pci/IOPCIDevice.h>

class EXPORT PRODUCT_NAME : public IOService {
    OSDeclareDefaultStructors(PRODUCT_NAME);

    public:
    IOService *probe(IOService *provider, SInt32 *score) override;
    bool start(IOService *provider) override;
};

enum struct ChipType : uint32_t {
    Raven = 0,
    Picasso,
    Raven2,
    Renoir,
    GreenSardine,
    Unknown,
};

// Hack
class AppleACPIPlatformExpert : IOACPIPlatformExpert {
    friend class NRed;
};

// https://elixir.bootlin.com/linux/latest/source/drivers/gpu/drm/amd/amdgpu/amdgpu_bios.c#L49
static bool checkAtomBios(const uint8_t *bios, size_t size) {
    uint16_t tmp, bios_header_start;

    if (size < 0x49) {
        DBGLOG("nred", "VBIOS size is invalid");
        return false;
    }

    if (bios[0] != 0x55 || bios[1] != 0xAA) {
        DBGLOG("nred", "VBIOS signature <%x %x> is invalid", bios[0], bios[1]);
        return false;
    }

    bios_header_start = bios[0x48] | (bios[0x49] << 8);
    if (!bios_header_start) {
        DBGLOG("nred", "Unable to locate VBIOS header");
        return false;
    }

    tmp = bios_header_start + 4;
    if (size < tmp) {
        DBGLOG("nred", "BIOS header is broken");
        return false;
    }

    if (!memcmp(bios + tmp, "ATOM", 4) || !memcmp(bios + tmp, "MOTA", 4)) {
        DBGLOG("nred", "ATOMBIOS detected");
        return true;
    }

    return false;
}

class NRed {
    friend class HDMI;
    friend class DYLDPatches;
    friend class X6000FB;
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
        PANIC_COND(callback->chipType == ChipType::Unknown, "nred", "Unknown chip type");
        static const char *chipNames[] = {"raven", "raven2", "picasso", "renoir", "green_sardine"};
        return chipNames[static_cast<int>(callback->chipType)];
    }

    bool getVBIOSFromVFCT(IOPCIDevice *obj) {
        DBGLOG("nred", "Fetching VBIOS from VFCT table");
        auto *expert = reinterpret_cast<AppleACPIPlatformExpert *>(obj->getPlatform());
        PANIC_COND(!expert, "nred", "Failed to get AppleACPIPlatformExpert");

        auto *vfctData = expert->getACPITableData("VFCT", 0);
        if (!vfctData) {
            DBGLOG("nred", "No VFCT from AppleACPIPlatformExpert");
            return false;
        }

        auto *vfct = static_cast<const VFCT *>(vfctData->getBytesNoCopy());
        PANIC_COND(!vfct, "nred", "VFCT OSData::getBytesNoCopy returned null");

        auto offset = vfct->vbiosImageOffset;

        while (offset < vfctData->getLength()) {
            auto *vHdr =
                static_cast<const GOPVideoBIOSHeader *>(vfctData->getBytesNoCopy(offset, sizeof(GOPVideoBIOSHeader)));
            if (!vHdr) {
                DBGLOG("nred", "VFCT header out of bounds");
                return false;
            }

            auto *vContent = static_cast<const uint8_t *>(
                vfctData->getBytesNoCopy(offset + sizeof(GOPVideoBIOSHeader), vHdr->imageLength));
            if (!vContent) {
                DBGLOG("nred", "VFCT VBIOS image out of bounds");
                return false;
            }

            offset += sizeof(GOPVideoBIOSHeader) + vHdr->imageLength;

            if (vHdr->imageLength && vHdr->pciBus == obj->getBusNumber() && vHdr->pciDevice == obj->getDeviceNumber() &&
                vHdr->pciFunction == obj->getFunctionNumber() &&
                vHdr->vendorID == obj->configRead16(kIOPCIConfigVendorID) &&
                vHdr->deviceID == obj->configRead16(kIOPCIConfigDeviceID)) {
                if (!checkAtomBios(vContent, vHdr->imageLength)) {
                    DBGLOG("nred", "VFCT VBIOS is not an ATOMBIOS");
                    return false;
                }
                this->vbiosData = OSData::withBytes(vContent, vHdr->imageLength);
                PANIC_COND(!this->vbiosData, "nred", "VFCT OSData::withBytes failed");
                obj->setProperty("ATY,bin_image", this->vbiosData);
                return true;
            }
        }

        return false;
    }

    bool getVBIOSFromVRAM(IOPCIDevice *provider) {
        auto *bar0 = provider->mapDeviceMemoryWithRegister(kIOPCIConfigBaseAddress0);
        if (!bar0 || !bar0->getLength()) {
            DBGLOG("nred", "FB BAR not enabled");
            OSSafeReleaseNULL(bar0);
            return false;
        }
        auto *fb = reinterpret_cast<const uint8_t *>(bar0->getVirtualAddress());
        uint32_t size = 256 * 1024;    // ???
        if (!checkAtomBios(fb, size)) {
            DBGLOG("nred", "VRAM VBIOS is not an ATOMBIOS");
            bar0->release();
            return false;
        }
        this->vbiosData = OSData::withBytes(fb, size);
        PANIC_COND(!this->vbiosData, "nred", "VRAM OSData::withBytes failed");
        provider->setProperty("ATY,bin_image", this->vbiosData);
        bar0->release();
        return true;
    }

    uint32_t readReg32(uint32_t reg) {
        if (reg * 4 < this->rmmio->getLength()) {
            return this->rmmioPtr[reg];
        } else {
            this->rmmioPtr[mmPCIE_INDEX2] = reg;
            return this->rmmioPtr[mmPCIE_DATA2];
        }
    }

    void writeReg32(uint32_t reg, uint32_t val) {
        if ((reg * 4) < this->rmmio->getLength()) {
            this->rmmioPtr[reg] = val;
        } else {
            this->rmmioPtr[mmPCIE_INDEX2] = reg;
            this->rmmioPtr[mmPCIE_DATA2] = val;
        }
    }

    uint32_t sendMsgToSmc(uint32_t msg, uint32_t param = 0) {
        auto smuWaitForResp = [=]() {
            uint32_t ret = 0;
            for (uint32_t i = 0; i < AMDGPU_MAX_USEC_TIMEOUT; i++) {
                ret = this->readReg32(MP_BASE + mmMP1_SMN_C2PMSG_90);
                if (ret) return ret;

                IOSleep(1);
            }

            return ret;
        };

        PANIC_COND(smuWaitForResp() != 1, "nred", "Msg issuing pre-check failed; SMU may be in an improper state");

        this->writeReg32(MP_BASE + mmMP1_SMN_C2PMSG_90, 0);
        this->writeReg32(MP_BASE + mmMP1_SMN_C2PMSG_82, param);
        this->writeReg32(MP_BASE + mmMP1_SMN_C2PMSG_66, msg);

        PANIC_COND(smuWaitForResp() != 1, "nred", "No response from SMU");

        return this->readReg32(MP_BASE + mmMP1_SMN_C2PMSG_82);
    }

    template<typename T>
    T *getVBIOSDataTable(uint32_t index) {
        auto *vbios = static_cast<const uint8_t *>(this->vbiosData->getBytesNoCopy());
        auto base = *reinterpret_cast<const uint16_t *>(vbios + ATOM_ROM_TABLE_PTR);
        auto dataTable = *reinterpret_cast<const uint16_t *>(vbios + base + ATOM_ROM_DATA_PTR);
        auto *mdt = reinterpret_cast<const uint16_t *>(vbios + dataTable + 4);
        auto offset = mdt[index];
        return offset ? reinterpret_cast<T *>(const_cast<uint8_t *>(vbios) + offset) : nullptr;
    }

    OSData *vbiosData {nullptr};
    ChipType chipType {ChipType::Unknown};
    uint64_t fbOffset {0};
    IOMemoryMap *rmmio {nullptr};
    volatile uint32_t *rmmioPtr {nullptr};
    uint32_t deviceId {0};
    uint16_t enumRevision {0};
    uint16_t revision {0};
    uint32_t pciRevision {0};
    IOPCIDevice *iGPU {nullptr};
    OSMetaClass *metaClassMap[4][2] = {{nullptr}};
    mach_vm_address_t orgSafeMetaCast {0};
    mach_vm_address_t orgApplePanelSetDisplay {0};

    static OSMetaClassBase *wrapSafeMetaCast(const OSMetaClassBase *anObject, const OSMetaClass *toMeta);
    static size_t wrapFunctionReturnZero();
    static bool wrapApplePanelSetDisplay(IOService *that, IODisplay *display);
};
