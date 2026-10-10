// Debug Log Enablement Patches
//
// Copyright (C) 2024-2026  ChefKiss.
//
// Licensed under the ChefKiss Open Knowledge License version 1.
//
// This work is licensed to you under the terms of the ChefKiss Open Knowledge
// License, either version 1 of the License, or (at your option) any later
// version published by ChefKiss.
//
// This work may not be used to train or develop artificial intelligence,
// and may not be used with artificial intelligence in any other way. See
// the ChefKiss Open Knowledge License for details.
//
// This work is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// ChefKiss Open Knowledge License for more details.
//
// You should have received a copy of the ChefKiss Open Knowledge License
// along with this work.  If not, see https://git.chefkiss.dev/ChefKiss/OpenKnowledge/raw/branch/master/LICENSE-v1.md.

#pragma once
#include <Headers/kern_patcher.hpp>

class DebugEnabler
{
    mach_vm_address_t orgInitWithPciInfo{0};
    mach_vm_address_t orgGetNumericProperty{0};

public:
    static DebugEnabler& singleton();

    void processKext(KernelPatcher& patcher, size_t id, mach_vm_address_t slide, size_t size);

private:
    void processX6000FB(KernelPatcher& patcher, size_t id, mach_vm_address_t slide, size_t size);
    void processX5000HWLibs(KernelPatcher& patcher, size_t id, mach_vm_address_t slide, size_t size);
    void processX5000(KernelPatcher& patcher, size_t id, mach_vm_address_t slide, size_t size);

    static bool   wrapInitWithPciInfo(void* self, void* pciDevice);
    static void   doGPUPanic(void* self, const char* fmt, ...);
    static void   dmLoggerWrite(void* logger, const UInt32 logType, const char* fmt, ...);
    static void   ipAssertion(void* instance, UInt32 cond, const char* func, const char* file, UInt32 line,
                              const char* msg);
    static void   cosDebugAssert(void* cosHandle, const char* func, const char* file, UInt32 line, const char* msg);
    static void   gcDebugPrint(void* instance, const char* fmt, ...);
    static void   pspDebugPrint(void* instance, const char* fmt, ...);
    static bool   wrapGetNumericProperty(void* self, const char* name, UInt32* value);
    static UInt32 wrapGetNumericProperty1(void* self, const char* name);
};
