//  Copyright © 2022 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.0. See LICENSE for
//  details.

#include "kern_wred.hpp"
#include <Headers/kern_api.hpp>
#include <Headers/plugin_start.hpp>

static LRed lred;

static const char *bootargOff[] = {
    "-lredoff",
};

static const char *bootargDebug[] = {
    "-lreddbg",
};

static const char *bootargBeta[] = {
    "-lredbeta",
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
    []() { lred.init(); },
};
