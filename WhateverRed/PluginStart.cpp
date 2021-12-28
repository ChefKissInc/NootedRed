#include <Headers/plugin_start.hpp>
#include <Headers/kern_api.hpp>
#include "WhateverRed.hpp"

static const char *bootargOff[] {
    "-weroff"
};

static const char *bootargDebug[] {
    "-werdbg"
};

static const char *bootargBeta[] {
    "-werbeta"
};

static WER wer;

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
    KernelVersion::Monterey,
    []() {
        wer.init();
    }
};
