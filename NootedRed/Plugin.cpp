// Copyright © 2022-2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#include <Headers/kern_api.hpp>
#include <Headers/plugin_start.hpp>
#include <PrivateHeaders/NRed.hpp>

static const char *bootargDebug = "-NRedDebug";

PluginConfiguration ADDPR(config) {
    xStringify(PRODUCT_NAME),
    parseModuleVersion(xStringify(MODULE_VERSION)),
    LiluAPI::AllowNormal | LiluAPI::AllowInstallerRecovery | LiluAPI::AllowSafeMode,
    nullptr,
    0,
    &bootargDebug,
    1,
    nullptr,
    0,
    KernelVersion::Catalina,
    KernelVersion::Tahoe,
    []() { NRed::singleton().init(); },
};
