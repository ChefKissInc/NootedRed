//  Copyright © 2022 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.0. See LICENSE for
//  details.

#include "kern_wred.hpp"
#include <Headers/kern_api.hpp>
#include <Headers/plugin_start.hpp>

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
    LiluAPI::AllowNormal | LiluAPI::AllowSafeMode,
    bootargOff,
    arrsize(bootargOff),
    bootargDebug,
    arrsize(bootargDebug),
    bootargBeta,
    arrsize(bootargBeta),
    KernelVersion::Catalina,
    KernelVersion::BigSur,
    []() { wred.init(); },
};
