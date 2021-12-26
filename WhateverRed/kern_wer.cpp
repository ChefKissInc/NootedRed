#include "kern_wer.hpp"
#include <Headers/kern_api.hpp>


static const char *pathRadeonX5000HWLibs[] = {
    "/System/Library/Extensions/AMDRadeonX5000HWServices.kext/Contents/PlugIns/AMDRadeonX5000HWLibs.kext/Contents/MacOS/AMDRadeonX5000HWLibs"
};

static KernelPatcher::KextInfo kextRadeonX5000HWLibs {
    "com.apple.kext.AMDRadeonX5000HWLibs", pathRadeonX5000HWLibs, arrsize(pathRadeonX5000HWLibs), {}, {}, KernelPatcher::KextInfo::Unloaded
};

void WhateverRed::init() {
    SYSLOG("wer", "WhateverRed plugin loaded");

    lilu.onKextLoad(&kextRadeonX5000HWLibs);
}
