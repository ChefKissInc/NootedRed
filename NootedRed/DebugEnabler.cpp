// Patches to force-enable debug logs
//
// Copyright © 2024-2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#include <DebugEnabler.hpp>
#include <Headers/kern_api.hpp>
#include <Kexts.hpp>
#include <NRed.hpp>
#include <PenguinWizardry/KernelVersion.hpp>
#include <PenguinWizardry/PatcherPlus.hpp>

// X6000FB
static const UInt8 kDmLoggerWritePattern[] = {0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x55, 0x41, 0x54,
    0x53, 0x48, 0x81, 0xEC, 0x88, 0x04, 0x00, 0x00};

// X6000FB
static const UInt8 kDalDmLoggerShouldLogPartialPattern[] = {0x48, 0x8D, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x8B, 0x04, 0x81,
    0x0F, 0xA3, 0xD0, 0x0F, 0x92, 0xC0};
static const UInt8 kDalDmLoggerShouldLogPartialPatternMask[] = {0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

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
    GPU_TASK_SINGLE_CHANNEL = 0x80000,
    DISABLE_PREEMPTION = 0x80000000,
};

void DebugEnabler::processX6000FB(KernelPatcher &patcher, const size_t id, const mach_vm_address_t slide,
    const size_t size) {
    NRed::singleton().setProp32("PP_LogLevel", 0xFFFF);
    NRed::singleton().setProp32("PP_LogSource", 0xFFFFFFFF);
    NRed::singleton().setProp32("PP_LogDestination", 0xFFFFFFFF);
    NRed::singleton().setProp32("PP_LogField", 0xFFFFFFFF);
    NRed::singleton().setProp32("PP_DumpRegister", TRUE);
    NRed::singleton().setProp32("PP_DumpSMCTable", TRUE);
    NRed::singleton().setProp32("PP_LogDumpTableBuffers", TRUE);

    PenguinWizardry::PatternRouteRequest requests[] = {
        {"__ZN24AMDRadeonX6000_AmdLogger15initWithPciInfoEP11IOPCIDevice", wrapInitWithPciInfo,
            this->orgInitWithPciInfo},
        {"__ZN34AMDRadeonX6000_AmdRadeonController10doGPUPanicEPKcz", doGPUPanic},
        {"_dm_logger_write", dmLoggerWrite, kDmLoggerWritePattern},
    };
    PANIC_COND(!PenguinWizardry::PatternRouteRequest::routeAll(patcher, id, requests, slide, size), "DebugEnabler",
        "Failed to route X6000FB debug symbols");

    // Enable all DalDmLogger logs
    // TODO: Maybe replace this with some simpler patches?
    auto *logEnableMaskMinors =
        patcher.solveSymbol<void *>(id, "__ZN14AmdDalDmLogger19LogEnableMaskMinorsE", slide, size, true);
    patcher.clearError();
    if (logEnableMaskMinors == nullptr) {
        size_t offset;
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
    if (currentKernelVersion() == MACOS_10_15) {
        const PenguinWizardry::MaskedLookupPatch patch {&kextRadeonX6000Framebuffer,
            kInitPopulateDcInitDataCatalinaOriginal, kInitPopulateDcInitDataCatalinaPatched, 1};
        PANIC_COND(!patch.apply(patcher, slide, size), "DebugEnabler",
            "Failed to apply populateDcInitData patch (10.15)");
    } else {
        const PenguinWizardry::MaskedLookupPatch patch {&kextRadeonX6000Framebuffer, kInitPopulateDcInitDataOriginal,
            kInitPopulateDcInitDataPatched, 1};
        PANIC_COND(!patch.apply(patcher, slide, size), "DebugEnabler", "Failed to apply populateDcInitData patch");
    }

    // Enable all bios parser logs
    const PenguinWizardry::MaskedLookupPatch patch {&kextRadeonX6000Framebuffer, kBiosParserHelperInitWithDataOriginal,
        kBiosParserHelperInitWithDataPatched, 1};
    PANIC_COND(!patch.apply(patcher, slide, size), "DebugEnabler",
        "Failed to apply AmdBiosParserHelper::initWithData patch");
}

void DebugEnabler::processX5000HWLibs(KernelPatcher &patcher, const size_t id, const mach_vm_address_t slide,
    const size_t size) {
    // TODO: Find the empty functions using a pattern of a call to them for Monterey and newer.
    if (currentKernelVersion() <= MACOS_11) {
        KernelPatcher::RouteRequest requests[] = {
            {"_dmcu_assertion", ipAssertion},
            {"_gc_assertion", ipAssertion},
            {"_gvm_assertion", ipAssertion},
            {"_mes_assertion", ipAssertion},
            {"_psp_assertion", ipAssertion},
            {"_sdma_assertion", ipAssertion},
            {"_smu_assertion", ipAssertion},
            {"_vcn_assertion", ipAssertion},
            {"_gc_debug_print", gcDebugPrint},
            {"_psp_debug_print", pspDebugPrint},
        };
        SYSLOG_COND(!patcher.routeMultiple(id, requests, slide, size, true, true), "DebugEnabler",
            "Failed to route X5000HWLibs debug symbols");
    }

    const PenguinWizardry::MaskedLookupPatch atiPpSvcCtrPatch {&kextRadeonX5000HWLibs,
        kAtiPowerPlayServicesConstructorOriginal, kAtiPowerPlayServicesConstructorPatched, 1};
    PANIC_COND(!atiPpSvcCtrPatch.apply(patcher, slide, size), "DebugEnabler", "Failed to apply MCIL debugLevel patch");
    if (currentKernelVersion() >= MACOS_11) {
        const PenguinWizardry::MaskedLookupPatch amdLogPspPatch {&kextRadeonX5000HWLibs, kAmdLogPspOriginal,
            kAmdLogPspOriginalMask, kAmdLogPspPatched, 1};
        PANIC_COND(!amdLogPspPatch.apply(patcher, slide, size), "DebugEnabler", "Failed to apply amd_log_psp patch");
    }
}

void DebugEnabler::processX5000(KernelPatcher &patcher, const size_t id, const mach_vm_address_t slide,
    const size_t size) {
    PenguinWizardry::PatternRouteRequest request {
        "__ZN37AMDRadeonX5000_AMDGraphicsAccelerator18getNumericPropertyEPKcPj", wrapGetNumericProperty,
        this->orgGetNumericProperty};
    PANIC_COND(!request.route(patcher, id, slide, size), "DebugEnabler", "Failed to route getNumericProperty");
}

void DebugEnabler::processKext(KernelPatcher &patcher, const size_t id, const mach_vm_address_t slide,
    const size_t size) {
    if (!ADDPR(debugEnabled)) { return; }

    if (kextRadeonX6000Framebuffer.loadIndex == id) {
        DebugEnabler::singleton().processX6000FB(patcher, id, slide, size);
    } else if (kextRadeonX5000HWLibs.loadIndex == id) {
        DebugEnabler::singleton().processX5000HWLibs(patcher, id, slide, size);
    } else if (kextRadeonX5000.loadIndex == id) {
        DebugEnabler::singleton().processX5000(patcher, id, slide, size);
    }
}

bool DebugEnabler::wrapInitWithPciInfo(void *self, void *pciDevice) {
    auto ret = FunctionCast(wrapInitWithPciInfo, singleton().orgInitWithPciInfo)(self, pciDevice);
    getMember<UInt64>(self, 0x28) = 0xFFFFFFFFFFFFFFFF;    // Enable all log types
    getMember<UInt32>(self, 0x30) = 0xFF;                  // Enable all log severities
    return ret;
}

void DebugEnabler::doGPUPanic(void *, char const *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    auto *buf = static_cast<char *>(IOMalloc(1000));
    vsnprintf(buf, 1000, fmt, va);
    va_end(va);

    SYSLOG("DebugEnabler", "doGPUPanic: %s", buf);
    IOSleep(10000);
    panic("%s", buf);
}

static const char *LogTypes[] = {"Error", "Warning", "Debug", "DC_Interface", "DTN", "Surface", "HW_Hotplug", "HW_LKTN",
    "HW_Mode", "HW_Resume", "HW_Audio", "HW_HPDIRQ", "MST", "Scaler", "BIOS", "BWCalcs", "BWValidation", "I2C_AUX",
    "Sync", "Backlight", "Override", "Edid", "DP_Caps", "Resource", "DML", "Mode", "Detect", "LKTN", "LinkLoss",
    "Underflow", "InterfaceTrace", "PerfTrace", "DisplayStats"};

// Needed to prevent stack overflow
void DebugEnabler::dmLoggerWrite(void *, const UInt32 logType, const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    auto *message = IONew(char, 0x1000);
    bzero(message, 0x1000);
    vsnprintf(message, 0x1000, fmt, va);
    va_end(va);
    auto *epilogue = message[strnlen(message, 0x1000) - 1] == '\n' ? "" : "\n";
    if (logType < arrsize(LogTypes)) {
        kprintf("[%s]\t%s%s", LogTypes[logType], message, epilogue);
    } else {
        kprintf("%s%s", message, epilogue);
    }
    IODelete(message, char, 0x1000);
}

// Port of `AmdTtlServices::cosDebugAssert` for empty `_*_assertion` functions
void DebugEnabler::ipAssertion(void *, UInt32 cond, const char *func, const char *file, UInt32 line, const char *msg) {
    if (cond != 0) { return; }

    kprintf("AMD TTL COS: \n----------------------------------------------------------------\n");
    kprintf("AMD TTL COS: ASSERT FUNCTION: %s\n", safeString(func));
    kprintf("AMD TTL COS: ASSERT FILE: %s\n", safeString(file));
    kprintf("AMD TTL COS: ASSERT LINE: %d\n", line);
    kprintf("AMD TTL COS: ASSERT REASON: %s\n", safeString(msg));
    kprintf("AMD TTL COS: \n----------------------------------------------------------------\n");
}

void DebugEnabler::gcDebugPrint(void *, const char *fmt, ...) {
    kprintf("[GC DEBUG]: ");
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

void DebugEnabler::pspDebugPrint(void *, const char *fmt, ...) {
    kprintf("[PSP DEBUG]: ");
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

bool DebugEnabler::wrapGetNumericProperty(void *self, const char *name, UInt32 *value) {
    auto ret = FunctionCast(wrapGetNumericProperty, singleton().orgGetNumericProperty)(self, name, value);
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
