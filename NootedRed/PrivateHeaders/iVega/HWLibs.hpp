// Copyright © 2022-2024 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <Headers/kern_patcher.hpp>
#include <Headers/kern_util.hpp>
#include <PrivateHeaders/GPUDriversAMD/CAIL/DeviceType.hpp>
#include <PrivateHeaders/GPUDriversAMD/CAIL/Result.hpp>
#include <PrivateHeaders/ObjectField.hpp>

namespace iVega {
    class X5000HWLibs {
        using t_createFirmware = void *(*)(const void *data, UInt32 size, UInt32 ipVersion, const char *filename);
        using t_putFirmware = bool (*)(void *that, AMDDeviceType deviceType, void *fw);

        bool initialised {false};
        ObjectField<void *> fwDirField {};
        ObjectField<UInt32> pspLoadSOSField {};
        ObjectField<UInt8> pspSecurityCapsField {};
        ObjectField<UInt32> pspTOSField {};
        ObjectField<UInt8 *> pspCommandDataField {};
        ObjectField<bool> smuSwInitialisedFieldBase {};
        ObjectField<mach_vm_address_t> smuInternalSWInitField {};
        ObjectField<mach_vm_address_t> smuFullscreenEventField {};
        ObjectField<mach_vm_address_t> smuGetUCodeConstsField {};
        ObjectField<mach_vm_address_t> smuInternalHWInitField {};
        ObjectField<mach_vm_address_t> smuNotifyEventField {};
        ObjectField<mach_vm_address_t> smuInternalSWExitField {};
        ObjectField<mach_vm_address_t> smuInternalHWExitField {};
        ObjectField<mach_vm_address_t> smuFullAsicResetField {};
        mach_vm_address_t orgGetIpFw {0};
        t_createFirmware orgCreateFirmware {nullptr};
        t_putFirmware orgPutFirmware {nullptr};
        mach_vm_address_t orgPspCmdKmSubmit {0};

        public:
        static X5000HWLibs &singleton();

        void init();

        private:
        void processKext(KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size);

        static void wrapPopulateFirmwareDirectory(void *that);
        static bool wrapGetIpFw(void *that, UInt32 ipVersion, char *name, void *out);
        static CAILResult hwLibsGeneralFailure();
        static CAILResult hwLibsUnsupported();
        static CAILResult hwLibsNoop();
        static CAILResult pspBootloaderLoadSos10(void *ctx);
        static CAILResult pspSecurityFeatureCapsSet10(void *ctx);
        static CAILResult pspSecurityFeatureCapsSet12(void *ctx);
        static CAILResult wrapPspCmdKmSubmit(void *ctx, void *cmd, void *outData, void *outResponse);
        static CAILResult smuReset();
        static CAILResult smuPowerUp();
        static CAILResult smuInternalSwInit(void *ctx);
        static CAILResult smu10InternalHwInit(void *ctx);
        static CAILResult smu12InternalHwInit(void *ctx);
        static CAILResult smuInternalHwExit(void *ctx);
        static CAILResult smuFullAsicReset(void *ctx, void *data);
        static CAILResult smu10NotifyEvent(void *ctx, void *data);
        static CAILResult smu12NotifyEvent(void *ctx, void *data);
        static CAILResult smuFullScreenEvent(void *ctx, UInt32 event);
        static CAILResult wrapSmu901CreateFunctionPointerList(void *ctx);
    };
};    // namespace iVega
