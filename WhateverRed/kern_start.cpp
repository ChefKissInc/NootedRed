#include <Headers/plugin_start.hpp>
#include <Headers/kern_api.hpp>
#include "kern_wer.hpp"

static const char *bootargOff[] {
    "-weroff"
};

static const char *bootargDebug[] {
    "-werdbg"
};

static const char *bootargBeta[] {
    "-werbeta"
};

static WhateverRed wer;

PluginConfiguration ADDPR(config) {
    xStringify(PRODUCT_NAME),
    parseModuleVersion(xStringify(MODULE_VERSION)),
    LiluAPI::AllowNormal | LiluAPI::AllowInstallerRecovery,
    bootargOff,
    arrsize(bootargOff),
    bootargDebug,
    arrsize(bootargDebug),
    bootargBeta,
    arrsize(bootargBeta),
    KernelVersion::Catalina,
    KernelVersion::Monterey,
    []() {
        wer.init();
    }
};
