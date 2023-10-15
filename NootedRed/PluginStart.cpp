//! Copyright © 2022-2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5.
//! See LICENSE for details.

#include "NRed.hpp"
#include "X6000FB.hpp"
#include <Headers/kern_api.hpp>
#include <Headers/plugin_start.hpp>

static NRed nred;

static const char *bootargDebug = "-NRedDebug";
static const char *bootargBeta = "-NRedBeta";

PluginConfiguration ADDPR(config) {
    xStringify(PRODUCT_NAME),
    parseModuleVersion(xStringify(MODULE_VERSION)),
    LiluAPI::AllowNormal | LiluAPI::AllowInstallerRecovery | LiluAPI::AllowSafeMode,
    nullptr,
    0,
    &bootargDebug,
    1,
    &bootargBeta,
    1,
    KernelVersion::Catalina,
    KernelVersion::Sonoma,
    []() { nred.init(); },
};
