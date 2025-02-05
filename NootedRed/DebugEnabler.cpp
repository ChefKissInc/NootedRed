// Copyright © 2024 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#include <Headers/kern_api.hpp>
#include <PrivateHeaders/DebugEnabler.hpp>
#include <PrivateHeaders/NRed.hpp>
#include <PrivateHeaders/PatcherPlus.hpp>

//------ Target Kexts ------//

static const char *pathRadeonX6000Framebuffer =
    "/System/Library/Extensions/AMDRadeonX6000Framebuffer.kext/Contents/MacOS/AMDRadeonX6000Framebuffer";
static const char *pathRadeonX5000HWLibs = "/System/Library/Extensions/AMDRadeonX5000HWServices.kext/Contents/PlugIns/"
                                           "AMDRadeonX5000HWLibs.kext/Contents/MacOS/AMDRadeonX5000HWLibs";
static const char *pathRadeonX5000 = "/System/Library/Extensions/AMDRadeonX5000.kext/Contents/MacOS/AMDRadeonX5000";

static KernelPatcher::KextInfo kextX6000FB {
    "com.apple.kext.AMDRadeonX6000Framebuffer",
    &pathRadeonX6000Framebuffer,
    1,
    {true},
    {},
    KernelPatcher::KextInfo::Unloaded,
};
static KernelPatcher::KextInfo kextX5000HWLibs {
    "com.apple.kext.AMDRadeonX5000HWLibs",
    &pathRadeonX5000HWLibs,
    1,
    {true},
    {},
    KernelPatcher::KextInfo::Unloaded,
};
static KernelPatcher::KextInfo kextX5000 {
    "com.apple.kext.AMDRadeonX5000",
    &pathRadeonX5000,
    1,
    {true},
    {},
    KernelPatcher::KextInfo::Unloaded,
};

//------ Patterns ------//

// X6000FB
static const UInt8 kDmLoggerWritePattern[] = {0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x55, 0x41, 0x54,
    0x53, 0x48, 0x81, 0xEC, 0x88, 0x04, 0x00, 0x00};

// X6000FB
static const UInt8 kDalDmLoggerShouldLogPartialPattern[] = {0x48, 0x8D, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x8B, 0x04, 0x81,
    0x0F, 0xA3, 0xD0, 0x0F, 0x92, 0xC0};
static const UInt8 kDalDmLoggerShouldLogPartialPatternMask[] = {0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

//------ Patches ------//

// X6000FB: Enable all Display Core logs.
static const UInt8 kInitPopulateDcInitDataOriginal[] = {0x48, 0xB9, 0xDB, 0x1B, 0xFF, 0x7E, 0x10, 0x00, 0x00, 0x00};
static const UInt8 kInitPopulateDcInitDataPatched[] = {0x48, 0xB9, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// X6000FB: Enable more Display Core logs on Catalina (not sure how to enable all of them yet).
static const UInt8 kInitPopulateDcInitDataCatalinaOriginal[] = {0x48, 0xC7, 0x87, 0x20, 0x02, 0x00, 0x00, 0xDB, 0x1B,
    0xFF, 0x7E};
static const UInt8 kInitPopulateDcInitDataCatalinaPatched[] = {0x48, 0xC7, 0x87, 0x20, 0x02, 0x00, 0x00, 0xFF, 0xFF,
    0xFF, 0xFF};

// X6000FB: Enable all AmdBiosParserHelper logs.
static const UInt8 kBiosParserHelperInitWithDataOriginal[] = {0x08, 0xC7, 0x07, 0x01, 0x00, 0x00, 0x00};
static const UInt8 kBiosParserHelperInitWithDataPatched[] = {0x08, 0xC7, 0x07, 0xFF, 0x00, 0x00, 0x00};

// HWLibs: Enable all MCIL debug prints (debugLevel = 0xFFFFFFFF, mostly for PP_Log).
static const UInt8 kAtiPowerPlayServicesConstructorOriginal[] = {0x8B, 0x40, 0x60, 0x48, 0x8D};
static const UInt8 kAtiPowerPlayServicesConstructorPatched[] = {0x83, 0xC8, 0xFF, 0x48, 0x8D};

// HWLibs: Enable printing of all PSP event logs.
static const UInt8 kAmdLogPspOriginal[] = {0x83, 0x00, 0x02, 0x0F, 0x85, 0x00, 0x00, 0x00, 0x00, 0x41, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x83, 0x00, 0x02, 0x72, 0x00, 0x41, 0x00, 0x00, 0x09, 0x02, 0x18, 0x00, 0x74, 0x00, 0x41, 0x00,
    0x00, 0x01, 0x06, 0x10, 0x00, 0x0f, 0x85, 0x00, 0x00, 0x00, 0x00};
static const UInt8 kAmdLogPspOriginalMask[] = {0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xFF,
    0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00};
static const UInt8 kAmdLogPspPatched[] = {0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x66,
    0x90, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90,
    0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x90};

//------ Module Logic ------//

static DebugEnabler instance {};

DebugEnabler &DebugEnabler::singleton() { return instance; }

enum GpuChannelDebugPolicy {
    CHANNEL_WAIT_FOR_PM4_IDLE = 0x1,
    CHANNEL_WAIT_FOR_TS_AFTER_SUBMISSION = 0x2,
    // 0x8, 0x10 = ??, PM4-related
    CHANNEL_DISABLE_PREEMPTION = 0x20,
};

enum GpuDebugPolicy {
    WAIT_FOR_PM4_IDLE = 0x1,
    WAIT_FOR_TS_AFTER_SUBMISSION = 0x2,
    PANIC_AFTER_DUMPING_LOG = 0x4,
    PANIC_ON_POWEROFF_REGISTER_ACCESS = 0x8,
    PRINT_FUNC_ENTRY_EXIT = 0x40,
    DBX_SLEEP_BEFORE_GPU_RESTART = 0x200,
    DISABLE_PREEMPTION = 0x80000000,
};

void DebugEnabler::init() {
    PANIC_COND(this->initialised, "DebugEnabler", "Attempted to initialise module twice!");
    this->initialised = true;

    SYSLOG("DebugEnabler", "Module initialised.");

    if (!ADDPR(debugEnabled)) { return; }

    lilu.onKextLoadForce(&kextX6000FB);
    lilu.onKextLoadForce(&kextX5000HWLibs);
    lilu.onKextLoadForce(&kextX5000);

    lilu.onKextLoadForce(
        nullptr, 0,
        [](void *user, KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size) {
            static_cast<DebugEnabler *>(user)->processKext(patcher, id, slide, size);
        },
        this);
}

void DebugEnabler::processX6000FB(KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size) {
    NRed::singleton().setProp32("PP_LogLevel", 0xFFFFFFFF);
    NRed::singleton().setProp32("PP_LogSource", 0xFFFFFFFF);
    NRed::singleton().setProp32("PP_LogDestination", 0xFFFFFFFF);
    NRed::singleton().setProp32("PP_LogField", 0xFFFFFFFF);
    NRed::singleton().setProp32("PP_DumpRegister", TRUE);
    NRed::singleton().setProp32("PP_DumpSMCTable", TRUE);
    NRed::singleton().setProp32("PP_LogDumpTableBuffers", TRUE);

    RouteRequestPlus requests[] = {
        {"__ZN24AMDRadeonX6000_AmdLogger15initWithPciInfoEP11IOPCIDevice", wrapInitWithPciInfo,
            this->orgInitWithPciInfo},
        {"__ZN34AMDRadeonX6000_AmdRadeonController10doGPUPanicEPKcz", wrapDoGPUPanic},
        {"_dm_logger_write", wrapDmLoggerWrite, kDmLoggerWritePattern},
    };
    PANIC_COND(!RouteRequestPlus::routeAll(patcher, id, requests, slide, size), "DebugEnabler",
        "Failed to route X6000FB debug symbols");

    // Enable all DalDmLogger logs
    // TODO: Maybe replace this with some simpler patches?
    auto *logEnableMaskMinors =
        patcher.solveSymbol<void *>(id, "__ZN14AmdDalDmLogger19LogEnableMaskMinorsE", slide, size);
    patcher.clearError();
    if (logEnableMaskMinors == nullptr) {
        size_t offset = 0;
        PANIC_COND(!KernelPatcher::findPattern(kDalDmLoggerShouldLogPartialPattern,
                       kDalDmLoggerShouldLogPartialPatternMask, arrsize(kDalDmLoggerShouldLogPartialPattern),
                       reinterpret_cast<const void *>(slide), size, &offset),
            "DebugEnabler", "Failed to solve LogEnableMaskMinors");
        auto *instAddr = reinterpret_cast<UInt8 *>(slide + offset);
        // inst + instSize + imm32 = addr
        logEnableMaskMinors = instAddr + 7 + *reinterpret_cast<SInt32 *>(instAddr + 3);
    }
    PANIC_COND(MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) != KERN_SUCCESS, "DebugEnabler",
        "Failed to enable kernel writing");
    memset(logEnableMaskMinors, 0xFF, 0x80);
    MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);

    // Enable all Display Core logs
    if (NRed::singleton().getAttributes().isCatalina()) {
        const LookupPatchPlus patch = {&kextX6000FB, kInitPopulateDcInitDataCatalinaOriginal,
            kInitPopulateDcInitDataCatalinaPatched, 1};
        PANIC_COND(!patch.apply(patcher, slide, size), "DebugEnabler",
            "Failed to apply populateDcInitData patch (10.15)");
    } else {
        const LookupPatchPlus patch = {&kextX6000FB, kInitPopulateDcInitDataOriginal, kInitPopulateDcInitDataPatched,
            1};
        PANIC_COND(!patch.apply(patcher, slide, size), "DebugEnabler", "Failed to apply populateDcInitData patch");
    }

    // Enable all bios parser logs
    const LookupPatchPlus patch = {&kextX6000FB, kBiosParserHelperInitWithDataOriginal,
        kBiosParserHelperInitWithDataPatched, 1};
    PANIC_COND(!patch.apply(patcher, slide, size), "DebugEnabler",
        "Failed to apply AmdBiosParserHelper::initWithData patch");
}

void DebugEnabler::processX5000HWLibs(KernelPatcher &patcher, size_t, mach_vm_address_t slide, size_t size) {
    const LookupPatchPlus atiPpSvcCtrPatch = {&kextX5000HWLibs, kAtiPowerPlayServicesConstructorOriginal,
        kAtiPowerPlayServicesConstructorPatched, 1};
    PANIC_COND(!atiPpSvcCtrPatch.apply(patcher, slide, size), "DebugEnabler", "Failed to apply MCIL debugLevel patch");
    if (NRed::singleton().getAttributes().isBigSurAndLater()) {
        const LookupPatchPlus amdLogPspPatch = {&kextX5000HWLibs, kAmdLogPspOriginal, kAmdLogPspOriginalMask,
            kAmdLogPspPatched, 1};
        PANIC_COND(!amdLogPspPatch.apply(patcher, slide, size), "DebugEnabler", "Failed to apply amd_log_psp patch");
    }
}
void DebugEnabler::processX5000(KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size) {
    RouteRequestPlus request = {"__ZN37AMDRadeonX5000_AMDGraphicsAccelerator18getNumericPropertyEPKcPj",
        wrapGetNumericProperty, this->orgGetNumericProperty};
    PANIC_COND(!request.route(patcher, id, slide, size), "DebugEnabler", "Failed to route getNumericProperty");
}

void DebugEnabler::processKext(KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size) {
    if (kextX6000FB.loadIndex == id) {
        this->processX6000FB(patcher, id, slide, size);
    } else if (kextX5000HWLibs.loadIndex == id) {
        this->processX5000HWLibs(patcher, id, slide, size);
    } else if (kextX5000.loadIndex == id) {
        this->processX5000(patcher, id, slide, size);
    }
}

bool DebugEnabler::wrapInitWithPciInfo(void *that, void *pciDevice) {
    auto ret = FunctionCast(wrapInitWithPciInfo, singleton().orgInitWithPciInfo)(that, pciDevice);
    getMember<UInt64>(that, 0x28) = 0xFFFFFFFFFFFFFFFF;    // Enable all log types
    getMember<UInt32>(that, 0x30) = 0xFF;                  // Enable all log severities
    return ret;
}

void DebugEnabler::wrapDoGPUPanic(void *, char const *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    auto *buf = static_cast<char *>(IOMalloc(1000));
    bzero(buf, 1000);
    vsnprintf(buf, 1000, fmt, va);
    va_end(va);

    DBGLOG("DebugEnabler", "doGPUPanic: %s", buf);
    IOSleep(10000);
    panic("%s", buf);
}

static const char *LogTypes[] = {"Error", "Warning", "Debug", "DC_Interface", "DTN", "Surface", "HW_Hotplug", "HW_LKTN",
    "HW_Mode", "HW_Resume", "HW_Audio", "HW_HPDIRQ", "MST", "Scaler", "BIOS", "BWCalcs", "BWValidation", "I2C_AUX",
    "Sync", "Backlight", "Override", "Edid", "DP_Caps", "Resource", "DML", "Mode", "Detect", "LKTN", "LinkLoss",
    "Underflow", "InterfaceTrace", "PerfTrace", "DisplayStats"};

// Needed to prevent stack overflow
void DebugEnabler::wrapDmLoggerWrite(void *, const UInt32 logType, const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    auto *message = static_cast<char *>(IOMalloc(0x1000));
    vsnprintf(message, 0x1000, fmt, va);
    va_end(va);
    auto *epilogue = message[strnlen(message, 0x1000) - 1] == '\n' ? "" : "\n";
    if (logType < arrsize(LogTypes)) {
        kprintf("[%s]\t%s%s", LogTypes[logType], message, epilogue);
    } else {
        kprintf("%s%s", message, epilogue);
    }
    IOFree(message, 0x1000);
}

bool DebugEnabler::wrapGetNumericProperty(void *that, const char *name, uint32_t *value) {
    auto ret = FunctionCast(wrapGetNumericProperty, singleton().orgGetNumericProperty)(that, name, value);
    if (name == nullptr || strncmp(name, "GpuDebugPolicy", 15) != 0) { return ret; }
    if (value != nullptr) {
        // Enable entry traces
        if (ret) {
            *value |= PRINT_FUNC_ENTRY_EXIT;
        } else {
            *value = PRINT_FUNC_ENTRY_EXIT;
        }
    }
    return true;
}
