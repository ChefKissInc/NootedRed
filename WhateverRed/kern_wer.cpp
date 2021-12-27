#include "kern_wer.hpp"
#include <Headers/kern_api.hpp>
#include <Headers/kern_devinfo.hpp>

static const char *pathRadeonX5000[] = {
    "/System/Library/Extensions/AMDRadeonX5000.kext/Contents/MacOS/AMDRadeonX5000"
};

static KernelPatcher::KextInfo kextRadeonX5000 {
    "com.apple.kext.AMDRadeonX5000", pathRadeonX5000, arrsize(pathRadeonX5000), {}, {}, KernelPatcher::KextInfo::Unloaded
};

static const char *pathRadeonX5000HWLibs[] = {
    "/System/Library/Extensions/AMDRadeonX5000HWServices.kext/Contents/PlugIns/AMDRadeonX5000HWLibs.kext/Contents/MacOS/AMDRadeonX5000HWLibs"
};

static KernelPatcher::KextInfo kextRadeonX5000HWLibs {
    "com.apple.kext.AMDRadeonX5000HWLibs", pathRadeonX5000HWLibs, arrsize(pathRadeonX5000HWLibs), {}, {}, KernelPatcher::KextInfo::Unloaded
};

static mach_vm_address_t origGetHardwareInfo = 0;

static int patchedGetHardwareInfo(void *obj, uint16_t *hwInfo) {
    int ret = FunctionCast(patchedGetHardwareInfo, origGetHardwareInfo)(obj, hwInfo);
    SYSLOG("wer", "AMDRadeonX5000_AMDAccelDevice::getHardwareInfo(_sAMD_GET_HW_INFO_VALUES*): return 0x%08X", ret);
    if (ret == 0) {
        SYSLOG("wer", "getHardwareInfo: deviceId = 0x%x", *hwInfo);
        *hwInfo = 0x15D8;
    }

    return ret;
}

void WhateverRed::init() {
    SYSLOG("wer", "WhateverRed plugin loaded");

    lilu.onPatcherLoadForce([](void *user, KernelPatcher &patcher) {
        static_cast<WhateverRed *>(user)->processKernel(patcher);
    }, this);

    lilu.onKextLoadForce(nullptr, 0, [](void *user, KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
        static_cast<WhateverRed *>(user)->processKext(patcher, index, address, size);
    }, this);

    lilu.onKextLoadForce(&kextRadeonX5000HWLibs);
}

void WhateverRed::processKernel(KernelPatcher &patcher) {
    auto devInfo = DeviceInfo::create();
    if (!devInfo) {
        panic("WhateverRed: Failed to get device info");
    }

    devInfo->processSwitchOff();

    if (!devInfo->videoExternal.size() || devInfo->videoBuiltin) {
        DeviceInfo::deleter(devInfo);
        panic("WhateverRed: No AMD iGPU");
    }

    bool foundiGPU = false;
    for (size_t i = 0; i < devInfo->videoExternal.size(); i++) {
        auto *dev = &devInfo->videoExternal[i];
        if (dev->vendor == WIOKit::VendorID::ATIAMD && !foundiGPU) {
            foundiGPU = true;

            uint32_t devid = 0;

            WIOKit::getOSDataValue(dev->video, "device-id", devid);

            switch (devid) {
                case 0x15D8:
                    SYSLOG("wer", "Found Picasso device");
                    break;
                case 0x15DD:
                    SYSLOG("wer", "Found Raven device");
                    break;
                default:
                    panic("WhateverRed: Unsupported AMD iGPU");
            }

            WIOKit::renameDevice(dev->video, "IGPU");

            break;
        }
    }

    if (!foundiGPU) {
        DeviceInfo::deleter(devInfo);
        panic("WhateverRed: No AMD iGPU");
    }

    // Cleanup
    DeviceInfo::deleter(devInfo);
}

void WhateverRed::processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
    if (index == kextRadeonX5000.loadIndex) {
        KernelPatcher::RouteRequest requests[] = {
            KernelPatcher::RouteRequest {
                "__ZN29AMDRadeonX5000_AMDAccelDevice15getHardwareInfoEP24_sAMD_GET_HW_INFO_VALUES", patchedGetHardwareInfo, origGetHardwareInfo
            },
        };

        if (!patcher.routeMultiple(index, requests, address, size, true, true)) {
            panic("WhateverRed: Failed to reroute AMDRadeonX5000_AMDAccelDevice::getHardwareInfo");
        }
    }
}
