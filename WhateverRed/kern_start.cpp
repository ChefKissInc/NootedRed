//  Copyright © 2022 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.0. See LICENSE for
//  details.

#include "kern_wred.hpp"
#include <Headers/kern_api.hpp>
#include <Headers/kern_version.hpp>
#include <Headers/plugin_start.hpp>
#include <IOKit/IOCatalogue.h>

static WRed wred;

static const char *bootargOff[] = {
    "-wredoff",
};

static const char *bootargDebug[] = {
    "-wreddbg",
};

static const char *bootargBeta[] = {
    "-wredbeta",
};

PluginConfiguration ADDPR(config) {
    xStringify(PRODUCT_NAME),
    parseModuleVersion(xStringify(MODULE_VERSION)),
    LiluAPI::AllowNormal | LiluAPI::AllowInstallerRecovery | LiluAPI::AllowSafeMode,
    bootargOff,
    arrsize(bootargOff),
    bootargDebug,
    arrsize(bootargDebug),
    bootargBeta,
    arrsize(bootargBeta),
    KernelVersion::BigSur,
    KernelVersion::BigSur,
    []() { wred.init(); },
};

OSDefineMetaClassAndStructors(PRODUCT_NAME, IOService);

IOService *PRODUCT_NAME::probe(IOService *provider, SInt32 *score) {
    ADDPR(selfInstance) = this;
    setProperty("VersionInfo", kextVersion);
    auto service = IOService::probe(provider, score);
    return ADDPR(startSuccess) ? service : nullptr;
}

bool PRODUCT_NAME::start(IOService *provider) {
    ADDPR(selfInstance) = this;
    if (!IOService::start(provider)) {
        SYSLOG("init", "Failed to start the parent");
        return false;
    }

    if (!(lilu.getRunMode() & LiluAPI::RunningInstallerRecovery)) {
        auto *prop = OSDynamicCast(OSArray, this->getProperty("Drivers"));
        if (!prop) {
            SYSLOG("init", "Failed to get Drivers property");
            return false;
        }
        auto *drivers = OSDynamicCast(OSArray, prop->copyCollection());
        if (!drivers) {
            SYSLOG("init", "Failed to copy Drivers property");
            return false;
        }
        if (!gIOCatalogue->addDrivers(drivers)) {
            SYSLOG("init", "Failed to add drivers");
            OSSafeReleaseNULL(drivers);
            return false;
        }
        OSSafeReleaseNULL(drivers);
    }

    return ADDPR(startSuccess);
}

void PRODUCT_NAME::stop(IOService *provider) {
    ADDPR(selfInstance) = nullptr;
    IOService::stop(provider);
}

PRODUCT_NAME *ADDPR(selfInstance) = nullptr;
