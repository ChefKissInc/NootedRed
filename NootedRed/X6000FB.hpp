// AMDRadeonX6000Framebuffer patches for AMD Vega iGPUs
//
// Copyright © 2022-2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <GPUDriversAMD/FB/AmdAsicInfo.hpp>
#include <GPUDriversAMD/FB/AmdDeviceMemoryManager.hpp>
#include <Headers/kern_patcher.hpp>

class X6000FB
{
    using mapMemorySubRange_t = IOReturn (*)(void* self, AmdReservedMemorySelector selector, size_t atOffset,
                                             size_t withSize, IOOptionBits andAttributes);

    static constexpr UInt32 IOFBRequestControllerEnabled = 0x1B;

    mach_vm_address_t   orgIH40IVRingInitHardware{0};
    mach_vm_address_t   orgIRQMGRWriteRegister{0};
    mach_vm_address_t   orgCreateRegisterAccess{0};
    mapMemorySubRange_t mapMemorySubRange{nullptr};
    mach_vm_address_t   orgDpReceiverPowerCtrl{0};
    IOReturn (*orgMessageAccelerator)(void* self, UInt32 requestType, void* arg2, void* arg3, void* arg4){nullptr};
    mach_vm_address_t orgControllerPowerUp{0};
    bool              fixedVBIOS{false};
    mach_vm_address_t orgGetNumberOfConnectors{0};

public:
    static X6000FB& singleton();

    void processKext(KernelPatcher& patcher, size_t id, mach_vm_address_t slide, size_t size);

private:
    static UInt16                           getEnumeratedRevision();
    static IOReturn                         populateVramInfo(void* self, void* fwInfo);
    static bool                             wrapIH40IVRingInitHardware(void* ctx, void* param2);
    static void                             wrapIRQMGRWriteRegister(void* ctx, UInt64 index, UInt32 value);
    static void*                            wrapCreateRegisterAccess(void* initData);
    static IOReturn                         initialiseReservedVRAM(void* self);
    static const AmdAsicBrandingTableEntry* getGpuBrandingNameListRaven(const void* self);
    static const AmdAsicBrandingTableEntry* getGpuBrandingNameListPicasso(const void* self);
    static const AmdAsicBrandingTableEntry* getGpuBrandingNameListRenoir(const void* self);
    static IOReturn                         dummyIOReturnSuccess();
    static IOReturn                         getTriageHardwareDataRV(void* self, UInt32 fbIndex, void* triageData);
    static IOReturn                         getTriageHardwareDataRN(void* self, UInt32 fbIndex, void* triageData);
    static void                             wrapDpReceiverPowerCtrl(void* link, bool power_on);
    static UInt32                           wrapControllerPowerUp(void* self);
    static UInt32                           wrapGetNumberOfConnectors(void* self);
};
