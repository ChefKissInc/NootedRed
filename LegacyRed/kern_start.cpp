//
//  kern_start.cpp
//  LegacyRed
//
//  Copyright © 2017 vit9696. All rights reserved.
//  Copyright © 2022 ChefKiss Inc. All rights reserved.
//

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
    LiluAPI::AllowNormal | LiluAPI::AllowInstallerRecovery | LiluAPI::AllowSafeMode,
    bootargOff,
    arrsize(bootargOff),
    bootargDebug,
    arrsize(bootargDebug),
    bootargBeta,
    arrsize(bootargBeta),
    KernelVersion::Catalina,
    KernelVersion::Ventura,
    []() { lred.init(); },
};
