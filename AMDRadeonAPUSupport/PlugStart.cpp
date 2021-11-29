#include <Headers/plugin_start.hpp>
#include <Headers/kern_api.hpp>

static const char *bootargOff[] {
    "-apuoff"
};

static const char *bootargDebug[] {
    "-apudbg"
};

static const char *bootargBeta[] {
    "-apubeta"
};

static const char *pathRadeonX5000HWLibs[] = {
    "/System/Library/Extensions/AMDRadeonX5000HWServices.kext/Contents/PlugIns/AMDRadeonX5000HWLibs.kext/Contents/MacOS/AMDRadeonX5000HWLibs"
};

static KernelPatcher::KextInfo kextRadeonX5000HWLibs {
    "com.apple.kext.AMDRadeonX5000HWLibs", pathRadeonX5000HWLibs, arrsize(pathRadeonX5000HWLibs), {}, {}, KernelPatcher::KextInfo::Unloaded
};

void omgItsFuckingEverythingUp() {
    IOPanic("Non raven! It's fucking everything up");
}

void omgItsTryingToInit() {
    IOPanic("It's initializing raven!");
}

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
        DBGLOG("apu", "AMDRadeonAPUSupport plugin loaded");

        lilu.onKextLoadForce(&kextRadeonX5000HWLibs);

        lilu.onKextLoadForce(nullptr, 0, [](void *user, KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
            if (kextRadeonX5000HWLibs.loadIndex == index) {
                KernelPatcher::RouteRequest requests[] = {
                    {"_PP_Tables_BackEnd_Initialize", omgItsFuckingEverythingUp},
                    {"_PhwIceland_Initialize", omgItsFuckingEverythingUp},
                    {"_PPT_Tonga_BackEnd_Initialize", omgItsFuckingEverythingUp},
                    {"_PhwTonga_Initialize", omgItsFuckingEverythingUp},
                    {"_PhwFiji_Initialize", omgItsFuckingEverythingUp},
                    {"_PhwPolaris10_Initialize", omgItsFuckingEverythingUp},
                    {"_PhwPolaris22_Initialize", omgItsFuckingEverythingUp},
                    {"_PhwCz_Initialize", omgItsFuckingEverythingUp},
                    {"_PPT_Vega10_BackEnd_Initialize", omgItsFuckingEverythingUp},
                    {"_PhwVega10_Initialize", omgItsFuckingEverythingUp},
                    {"_PPT_Vega12_BackEnd_Initialize", omgItsFuckingEverythingUp},
                    {"_PhwVega12_Initialize", omgItsFuckingEverythingUp},
                    {"_PPT_Vega20_BackEnd_Initialize", omgItsFuckingEverythingUp},
                    {"_PhwVega20_Initialize", omgItsFuckingEverythingUp},
                    {"_PhwRaven_Initialize", omgItsTryingToInit},
                    {"_PPT_Tonga_BackEnd_Initialize", omgItsTryingToInit},
                    {"_PPT_Raven_BackEnd_Initialize", omgItsTryingToInit},
                    {"_PPT_Renoir_BackEnd_Initialize", omgItsTryingToInit},
                    {"_PhwSoc15_PPTables_Initialize", omgItsFuckingEverythingUp},
                    {"_Phw_SwSmu_BackEnd_Initialize", omgItsFuckingEverythingUp},
                    {"_PhwSoc15_Initialize", omgItsFuckingEverythingUp},
                };

                if (!patcher.routeMultiple(index, requests, address, size, true, true))
                    IOPanic("AMDRadeonAPUSupport: Failed to route Radeon X5000 HWLibs functions");
            }
        });
    }
};
