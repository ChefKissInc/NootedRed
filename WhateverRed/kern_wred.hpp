//  Copyright © 2022 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.0. See LICENSE for
//  details.

#pragma once
#include "kern_amd.hpp"
#include <Headers/kern_iokit.hpp>
#include <IOKit/acpi/IOACPIPlatformExpert.h>
#include <IOKit/pci/IOPCIDevice.h>

enum struct ASICType {
    Raven,
    Raven2,
    Picasso,
    Renoir,
    Unknown,
};

// Hack
class AppleACPIPlatformExpert : IOACPIPlatformExpert {
    friend class WRed;
};

// https://elixir.bootlin.com/linux/latest/source/drivers/gpu/drm/amd/amdgpu/amdgpu_bios.c#L49
static bool checkAtomBios(const uint8_t *bios, size_t size) {
    uint16_t tmp, bios_header_start;

    if (size < 0x49) {
        DBGLOG("wred", "VBIOS size is invalid");
        return false;
    }

    if (bios[0] != 0x55 || bios[1] != 0xAA) {
        DBGLOG("wred", "VBIOS signature <%x %x> is invalid", bios[0], bios[1]);
        return false;
    }

    bios_header_start = bios[0x48] | (bios[0x49] << 8);
    if (!bios_header_start) {
        DBGLOG("wred", "Unable to locate VBIOS header");
        return false;
    }

    tmp = bios_header_start + 4;
    if (size < tmp) {
        DBGLOG("wred", "BIOS header is broken");
        return false;
    }

    if (!memcmp(bios + tmp, "ATOM", 4) || !memcmp(bios + tmp, "MOTA", 4)) {
        DBGLOG("wred", "ATOMBIOS detected");
        return true;
    }

    return false;
}

class WRed {
    public:
    static WRed *callbackWRed;

    void init();
    void deinit();
    void processPatcher(KernelPatcher &patcher);
    void processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size);

    private:
    static const char *getASICName() {
        switch (callbackWRed->asicType) {
            case ASICType::Picasso:
                return "picasso";
            case ASICType::Raven:
                return "raven";
            case ASICType::Raven2:
                return "raven2";
            case ASICType::Renoir:
                return "renoir";
            default:
                PANIC("wred", "Unknown ASIC type");
        }
    }

    bool getVBIOSFromVFCT(IOPCIDevice *provider) {
        DBGLOG("wred", "Fetching VBIOS from VFCT table");
        auto *expert = reinterpret_cast<AppleACPIPlatformExpert *>(provider->getPlatform());
        PANIC_COND(!expert, "wred", "Failed to get AppleACPIPlatformExpert");

        auto *vfctData = expert->getACPITableData("VFCT", 0);
        if (!vfctData) {
            DBGLOG("wred", "No VFCT from AppleACPIPlatformExpert");
            return false;
        }

        auto *vfct = static_cast<const VFCT *>(vfctData->getBytesNoCopy());
        PANIC_COND(!vfct, "wred", "VFCT OSData::getBytesNoCopy returned null");

        auto offset = vfct->vbiosImageOffset;

        while (offset < vfctData->getLength()) {
            auto *vHdr =
                static_cast<const GOPVideoBIOSHeader *>(vfctData->getBytesNoCopy(offset, sizeof(GOPVideoBIOSHeader)));
            if (!vHdr) {
                DBGLOG("wred", "VFCT header out of bounds");
                return false;
            }

            auto *vContent = static_cast<const uint8_t *>(
                vfctData->getBytesNoCopy(offset + sizeof(GOPVideoBIOSHeader), vHdr->imageLength));
            if (!vContent) {
                DBGLOG("wred", "VFCT VBIOS image out of bounds");
                return false;
            }

            offset += sizeof(GOPVideoBIOSHeader) + vHdr->imageLength;

            if (vHdr->imageLength && vHdr->pciBus == provider->getBusNumber() &&
                vHdr->pciDevice == provider->getDeviceNumber() && vHdr->pciFunction == provider->getFunctionNumber() &&
                vHdr->vendorID == provider->configRead16(kIOPCIConfigVendorID) &&
                vHdr->deviceID == provider->configRead16(kIOPCIConfigDeviceID)) {
                if (!checkAtomBios(vContent, vHdr->imageLength)) {
                    DBGLOG("wred", "VFCT VBIOS is not an ATOMBIOS");
                    return false;
                }
                this->vbiosData = OSData::withBytes(vContent, vHdr->imageLength);
                PANIC_COND(!this->vbiosData, "wred", "VFCT OSData::withBytes failed");
                provider->setProperty("ATY,bin_image", this->vbiosData);
                return true;
            }
        }

        return false;
    }

    bool getVBIOSFromVRAM(IOPCIDevice *provider) {
        uint32_t size = 256 * 1024;    // ???
        auto *bar0 = provider->mapDeviceMemoryWithRegister(kIOPCIConfigBaseAddress0, kIOMemoryMapCacheModeCopyback);
        if (!bar0 || !bar0->getLength()) {
            DBGLOG("wred", "FB BAR not enabled");
            return false;
        }
        auto *fb = reinterpret_cast<const uint8_t *>(bar0->getVirtualAddress());
        if (!fb || !checkAtomBios(fb, size)) {
            DBGLOG("wred", "VRAM VBIOS is not an ATOMBIOS");
            bar0->release();
            return false;
        }
        this->vbiosData = OSData::withBytes(fb, size);
        PANIC_COND(!this->vbiosData, "wred", "VRAM OSData::withBytes failed");
        provider->setProperty("ATY,bin_image", this->vbiosData);
        bar0->release();
        return true;
    }

    OSData *vbiosData = nullptr;
    ASICType asicType = ASICType::Unknown;
    void *callbackFirmwareDirectory = nullptr;
    uint64_t fbOffset {};

    void *hwAlignMgr = nullptr;
    uint8_t *hwAlignMgrVtX5000 = nullptr;
    uint8_t *hwAlignMgrVtX6000 = nullptr;

    OSMetaClass *metaClassMap[4][2] = {{nullptr}};

    mach_vm_address_t orgSafeMetaCast {};
    static OSMetaClassBase *wrapSafeMetaCast(const OSMetaClassBase *anObject, const OSMetaClass *toMeta);

    /** X6000Framebuffer */
    mach_vm_address_t orgPopulateDeviceInfo {};
    CailAsicCapEntry *orgAsicCapsTable = nullptr;
    mach_vm_address_t orgHwReadReg32 {};

    /** X5000HWLibs */
    uint32_t *orgDeviceTypeTable = nullptr;
    mach_vm_address_t orgAmdTtlServicesConstructor {};
    mach_vm_address_t orgSmuGetHwVersion {};
    mach_vm_address_t orgPspSwInit {};
    mach_vm_address_t orgPopulateFirmwareDirectory {};
    t_createFirmware orgCreateFirmware = nullptr;
    t_putFirmware orgPutFirmware = nullptr;
    t_Vega10PowerTuneConstructor orgVega10PowerTuneConstructor = nullptr;
    CailAsicCapEntry *orgCapsTableHWLibs = nullptr;
    CailInitAsicCapEntry *orgAsicInitCapsTable = nullptr;
    t_sendMsgToSmc orgRavenSendMsgToSmc = nullptr;
    t_sendMsgToSmc orgRenoirSendMsgToSmc = nullptr;
    mach_vm_address_t orgSmuRavenInitialize {};
    mach_vm_address_t orgSmuRenoirInitialize {};
    mach_vm_address_t orgPspCmdKmSubmit {};

    /** X6000 */
    t_HWEngineConstructor orgVCN2EngineConstructorX6000 = nullptr;
    mach_vm_address_t orgAllocateAMDHWDisplayX6000 {};
    mach_vm_address_t orgInitWithPciInfo {};
    mach_vm_address_t orgNewVideoContextX6000 {};
    mach_vm_address_t orgCreateSMLInterfaceX6000 {};
    mach_vm_address_t orgNewSharedX6000 {};
    mach_vm_address_t orgNewSharedUserClientX6000 {};
    mach_vm_address_t orgInitDCNRegistersOffsets {};
    mach_vm_address_t orgGetPreferredSwizzleMode2 {};
    mach_vm_address_t orgAccelSharedSurfaceCopy {};
    mach_vm_address_t orgAllocateScanoutFB {};
    mach_vm_address_t orgFillUBMSurface {};
    mach_vm_address_t orgConfigureDisplay {};
    mach_vm_address_t orgGetDisplayInfo {};

    /** X5000 */
    t_HWEngineConstructor orgGFX9PM4EngineConstructor = nullptr;
    t_HWEngineConstructor orgGFX9SDMAEngineConstructor = nullptr;
    mach_vm_address_t orgSetupAndInitializeHWCapabilities {};
    mach_vm_address_t orgRTGetHWChannel {};
    mach_vm_address_t orgAdjustVRAMAddress {};
    mach_vm_address_t orgAccelSharedUCStart {};
    mach_vm_address_t orgAccelSharedUCStop {};
    mach_vm_address_t orgAllocateAMDHWAlignManager {};

    /** X6000Framebuffer */
    static IOReturn wrapPopulateDeviceInfo(void *that);
    static uint16_t wrapGetEnumeratedRevision(void *that);
    static IOReturn wrapPopulateVramInfo(void *that, void *fwInfo);
    static uint32_t wrapHwReadReg32(void *that, uint32_t param1);

    /** X5000HWLibs */
    static void wrapAmdTtlServicesConstructor(void *that, IOPCIDevice *provider);
    static uint32_t wrapSmuGetHwVersion(uint64_t param1, uint32_t param2);
    static uint32_t wrapPspSwInit(uint32_t *param1, uint32_t *param2);
    static uint32_t wrapGcGetHwVersion();
    static void wrapPopulateFirmwareDirectory(void *that);
    static void *wrapCreatePowerTuneServices(void *that, void *param2);
    static uint32_t wrapSmuGetFwConstants();
    static uint32_t wrapSmuInternalHwInit();
    static uint32_t wrapSmuRavenInitialize(void *smum, uint32_t param2);
    static uint32_t wrapSmuRenoirInitialize(void *smum, uint32_t param2);
    static uint32_t wrapPspCmdKmSubmit(void *psp, void *ctx, void *param3, void *param4);

    /** X6000 */
    static bool wrapAccelStartX6000();
    static bool wrapInitWithPciInfo(void *that, void *param1);
    static void *wrapNewVideoContext(void *that);
    static void *wrapCreateSMLInterface(uint32_t configBit);
    static bool wrapAccelSharedUCStartX6000(void *that, void *provider);
    static bool wrapAccelSharedUCStopX6000(void *that, void *provider);
    static void wrapInitDCNRegistersOffsets(void *that);
    static uint64_t wrapAccelSharedSurfaceCopy(void *that, void *param1, uint64_t param2, void *param3);
    static uint64_t wrapAllocateScanoutFB(void *that, uint32_t param1, void *param2, void *param3, void *param4);
    static uint64_t wrapFillUBMSurface(void *that, uint32_t param1, void *param2, void *param3);
    static bool wrapConfigureDisplay(void *that, uint32_t param1, uint32_t param2, void *param3, void *param4);
    static uint64_t wrapGetDisplayInfo(void *that, uint32_t param1, bool param2, bool param3, void *param4,
        void *param5);

    /** X5000 */
    static bool wrapAllocateHWEngines(void *that);
    static void wrapSetupAndInitializeHWCapabilities(void *that);
    static void *wrapRTGetHWChannel(void *that, uint32_t param1, uint32_t param2, uint32_t param3);
    static void wrapInitializeFamilyType(void *that);
    static void *wrapAllocateAMDHWDisplay(void *that);
    static uint64_t wrapAdjustVRAMAddress(void *that, uint64_t addr);
    static void *wrapNewShared();
    static void *wrapNewSharedUserClient();
    static void *wrapAllocateAMDHWAlignManager();
};
