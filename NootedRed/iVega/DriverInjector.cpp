// Copyright © 2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#include <Headers/kern_api.hpp>
#include <iVega/DriverInjector.hpp>
#include <libkern/c++/OSString.h>

static iVega::DriverInjector instance {};

static const char com_apple_kext_AMDRadeonX5000[] = {
#embed "../Personalities/com.apple.kext.AMDRadeonX5000.xml" suffix(, '\0')
};
static const char com_apple_kext_AMDRadeonX5000HWServices[] = {
#embed "../Personalities/com.apple.kext.AMDRadeonX5000HWServices.xml" suffix(, '\0')
};
static const char com_apple_kext_AMDRadeonX6000Framebuffer[] = {
#embed "../Personalities/com.apple.kext.AMDRadeonX6000Framebuffer.xml" suffix(, '\0')
};
static const char com_apple_driver_AppleGFXHDA[] = {
#embed "../Personalities/com.apple.driver.AppleGFXHDA.xml" suffix(, '\0')
};

iVega::DriverInjector::DriverInjector()
    : drivers {
          Driver("com.apple.kext.AMDRadeonX5000", com_apple_kext_AMDRadeonX5000),
          Driver("com.apple.kext.AMDRadeonX5000HWServices", com_apple_kext_AMDRadeonX5000HWServices),
          Driver("com.apple.kext.AMDRadeonX6000Framebuffer", com_apple_kext_AMDRadeonX6000Framebuffer),
          Driver("com.apple.driver.AppleGFXHDA", com_apple_driver_AppleGFXHDA),
      } {}

iVega::DriverInjector &iVega::DriverInjector::singleton() { return instance; }

void iVega::DriverInjector::processPatcher(KernelPatcher &patcher) {
    KernelPatcher::RouteRequest request {"__ZN11IOCatalogue10addDriversEP7OSArrayb", wrapAddDrivers,
        this->orgAddDrivers};
    PANIC_COND(!patcher.routeMultipleLong(KernelPatcher::KernelID, &request, 1), "DriverInjector",
        "Failed to route addDrivers");
}

bool iVega::DriverInjector::wrapAddDrivers(void *const self, OSArray *const array, const bool doNubMatching) {
    UInt32 driverCount = array->getCount();
    for (UInt32 driverIndex = 0; driverIndex < driverCount; driverIndex += 1) {
        OSObject *object = array->getObject(driverIndex);
        if (object == nullptr) { continue; }
        auto *dict = OSDynamicCast(OSDictionary, object);
        if (dict == nullptr) { continue; }
        auto *bundleIdentifier = OSDynamicCast(OSString, dict->getObject("CFBundleIdentifier"));
        if (bundleIdentifier == nullptr || bundleIdentifier->getLength() == 0) { continue; }

        for (size_t identifierIndex = 0; identifierIndex < arrsize(singleton().drivers); identifierIndex += 1) {
            auto &driver = singleton().drivers[identifierIndex];

            if ((singleton().matchedDrivers & getBit(identifierIndex)) != 0 ||
                !bundleIdentifier->isEqualTo(driver.identifier)) {
                continue;
            }

            singleton().matchedDrivers |= getBit(identifierIndex);

            DBGLOG("DriverInjector", "Matched %s, injecting.", driver.identifier);

            UInt32 injectedDriverCount = driver.personalities->getCount();

            array->ensureCapacity(driverCount + injectedDriverCount);

            for (UInt32 injectedDriverIndex = 0; injectedDriverIndex < injectedDriverCount; injectedDriverIndex += 1) {
                array->setObject(driverIndex, driver.personalities->getObject(injectedDriverIndex));
                driverIndex += 1;
                driverCount += 1;
            }

            break;
        }
    }

    return FunctionCast(wrapAddDrivers, singleton().orgAddDrivers)(self, array, doNubMatching);
}
