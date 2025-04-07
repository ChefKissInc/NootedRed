// Copyright © 2022-2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#include <PrivateHeaders/PatcherPlus.hpp>

bool PatcherPlus::PatternSolveRequest::solve(KernelPatcher &patcher, const size_t id, const mach_vm_address_t start,
    const size_t size) {
    PANIC_COND(this->address == nullptr, "Patcher+", "this->address is null for sym `%s`", safeString(this->symbol));

    if (this->symbol != nullptr) {
        patcher.clearError();
        if (start == 0 || size == 0) {
            *this->address = patcher.solveSymbol(id, this->symbol);
        } else {
            *this->address = patcher.solveSymbol(id, this->symbol, start, size, true);
        }
        if (*this->address != 0) { return true; }
        SYSLOG("Patcher+", "Failed to solve `%s` using symbol: %d", this->symbol, patcher.getError());
    }

    if (this->pattern == nullptr || this->patternSize == 0) {
        PANIC_COND(this->symbol == nullptr, "Patcher+",
            "Improperly made pattern solve request; no symbol or pattern present");
        SYSLOG("Patcher+", "Cannot solve `%s` using pattern", this->symbol);
        return false;
    }

    DBGLOG("Patcher+", "Failed to solve `%s` using symbol: %d. Attempting to use pattern.", safeString(this->symbol),
        patcher.getError());
    PANIC_COND(start == 0 || size == 0, "Patcher+", "Improper jump pattern route request; no start or size");

    size_t offset = 0;
    if (!KernelPatcher::findPattern(this->pattern, this->mask, this->patternSize, reinterpret_cast<const void *>(start),
            size, &offset)) {
        SYSLOG("Patcher+", "Failed to solve `%s` using pattern", safeString(this->symbol));
        return false;
    }

    *this->address = start + offset;
    DBGLOG("Patcher+", "Resolved `%s` at 0x%llX", safeString(this->symbol), *this->address);
    return true;
}

bool PatcherPlus::PatternSolveRequest::solveAll(KernelPatcher &patcher, const size_t id,
    PatternSolveRequest *const requests, const size_t count, const mach_vm_address_t start, const size_t size) {
    for (size_t i = 0; i < count; i++) {
        if (requests[i].solve(patcher, id, start, size)) {
            DBGLOG("Patcher+", "Solved pattern request (i: %zu)", i);
        } else {
            DBGLOG("Patcher+", "Failed to solve pattern request (i: %zu)", i);
            return false;
        }
    }
    return true;
}

bool PatcherPlus::PatternRouteRequest::route(KernelPatcher &patcher, const size_t id, const mach_vm_address_t start,
    const size_t size) {
    if (this->symbol != nullptr) {
        patcher.clearError();
        if (start == 0 || size == 0) {
            this->from = patcher.solveSymbol(id, this->symbol);
        } else {
            this->from = patcher.solveSymbol(id, this->symbol, start, size, true);
        }
    }

    if (this->from == 0) {
        if (this->pattern == nullptr || this->patternSize == 0) {
            PANIC_COND(this->symbol == nullptr, "Patcher+",
                "Improperly made pattern route request; no symbol or pattern present");
            SYSLOG("Patcher+", "Failed to route `%s` using symbol: %d", this->symbol, patcher.getError());
            return false;
        }
        DBGLOG("Patcher+", "Failed to solve `%s` using symbol: %d. Attempting to use pattern.",
            safeString(this->symbol), patcher.getError());
        PANIC_COND(start == 0 || size == 0, "Patcher+", "Improper pattern route request (`%s`); no start or size",
            safeString(this->symbol));
        size_t offset = 0;
        if (!KernelPatcher::findPattern(this->pattern, this->mask, this->patternSize,
                reinterpret_cast<const void *>(start), size, &offset)) {
            SYSLOG("Patcher+", "Failed to route `%s` using pattern", safeString(this->symbol));
            return false;
        }
        this->from = start + offset;
        DBGLOG("Patcher+", "Resolved `%s` at 0x%llX", safeString(this->symbol), this->from);
    }

    // Workaround as patcher internals will attempt to resolve
    // the symbol without checking if the `from` field is 0.
    this->symbol = nullptr;
    return patcher.routeMultiple(id, this, 1, start, size);
}

bool PatcherPlus::PatternRouteRequest::routeAll(KernelPatcher &patcher, const size_t id,
    PatternRouteRequest *const requests, const size_t count, const mach_vm_address_t start, const size_t size) {
    for (size_t i = 0; i < count; i++) {
        if (requests[i].route(patcher, id, start, size)) {
            DBGLOG("Patcher+", "Applied pattern route (i: %zu)", i);
        } else {
            DBGLOG("Patcher+", "Failed to apply pattern route (i: %zu)", i);
            return false;
        }
    }
    return true;
}

bool PatcherPlus::MaskedLookupPatch::apply(KernelPatcher &patcher, const mach_vm_address_t start,
    const size_t size) const {
    if (this->findMask == nullptr && this->replaceMask == nullptr && this->skip == 0) {
        patcher.clearError();
        patcher.applyLookupPatch(this, reinterpret_cast<UInt8 *>(start), size);
        return patcher.getError() == KernelPatcher::Error::NoError;
    }
    PANIC_COND(start == 0 || size == 0, "Patcher+", "Improper mask lookup patch; no start or size");
    return KernelPatcher::findAndReplaceWithMask(reinterpret_cast<UInt8 *>(start), size, this->find, this->size,
        this->findMask, this->findMask ? this->size : 0, this->replace, this->size, this->replaceMask,
        this->replaceMask ? this->size : 0, this->count, this->skip);
}

bool PatcherPlus::MaskedLookupPatch::applyAll(KernelPatcher &patcher, const MaskedLookupPatch *const patches,
    const size_t count, const mach_vm_address_t start, const size_t size, const bool force) {
    for (size_t i = 0; i < count; i++) {
        if (patches[i].apply(patcher, start, size)) {
            DBGLOG("Patcher+", "Applied patches[%zu]", i);
        } else {
            DBGLOG("Patcher+", "Failed to apply patches[%zu]", i);
            if (!force) { return false; }
        }
    }
    return true;
}

mach_vm_address_t PatcherPlus::jumpInstDestination(const mach_vm_address_t start, const mach_vm_address_t end) {
    SInt64 off;

    if (start == 0 || end == 0) {
        SYSLOG("Patcher+", "jumpInstDestination start AND/OR end IS 0!!!");
        return 0;
    }

    const auto inst = *reinterpret_cast<const UInt8 *>(start);
    size_t instSize;
    if (inst == 0x0F) {
        instSize = sizeof(UInt8) + sizeof(UInt8) + sizeof(SInt32);
        if (start + instSize > end) { return 0; }

        const auto inst1 = *reinterpret_cast<const UInt8 *>(start + sizeof(UInt8));
        if (inst1 >= 0x80 && inst1 <= 0x8F) {
            off = *reinterpret_cast<const SInt32 *>(start + sizeof(UInt8) + sizeof(UInt8));
        } else {
            return 0;
        }
    } else if (inst == 0xE8 || inst == 0xE9) {
        instSize = sizeof(UInt8) + sizeof(SInt32);
        if (start + instSize > end) { return 0; }

        off = *reinterpret_cast<const SInt32 *>(start + sizeof(UInt8));
    } else if (inst >= 0x70 && inst <= 0x7F) {
        instSize = sizeof(UInt8) + sizeof(UInt8);
        if (start + instSize > end) { return 0; }

        off = *reinterpret_cast<const SInt8 *>(start + sizeof(UInt8));
    } else {
        return 0;
    }

    mach_vm_address_t result;
    if (off < 0) {
        const auto abs = static_cast<UInt64>(-off);
        if (abs > start) {    // This should never EVER happen!!!
            return 0;
        }
        result = start - abs + instSize;
    } else {
        result = start + static_cast<UInt64>(off) + instSize;
    }

    return result >= end ? 0 : result;
}

bool PatcherPlus::JumpPatternRouteRequest::route(KernelPatcher &patcher, const size_t id, const mach_vm_address_t start,
    const size_t size) {
    if (this->symbol != nullptr) {
        patcher.clearError();
        if (start == 0 || size == 0) {
            this->from = patcher.solveSymbol(id, this->symbol);
        } else {
            this->from = patcher.solveSymbol(id, this->symbol, start, size, true);
        }
    }

    if (this->from == 0) {
        if (this->pattern == nullptr || this->patternSize == 0) {
            PANIC_COND(this->symbol == nullptr, "Patcher+",
                "Improperly made jump pattern route request; no symbol or pattern present");
            DBGLOG("Patcher+", "Failed to solve `%s` using symbol: %d", safeString(this->symbol), patcher.getError());
            return false;
        }
        DBGLOG("Patcher+", "Failed to solve `%s` using symbol: %d. Attempting to use jump pattern.",
            safeString(this->symbol), patcher.getError());
        PANIC_COND(start == 0 || size == 0, "Patcher+", "Improper jump pattern route request (`%s`); no start or size",
            safeString(this->symbol));
        size_t offset = 0;
        if (!KernelPatcher::findPattern(this->pattern, this->mask, this->patternSize,
                reinterpret_cast<const void *>(start), size, &offset)) {
            SYSLOG("Patcher+", "Failed to solve `%s` using jump pattern", safeString(this->symbol));
            return false;
        }
        this->from = jumpInstDestination(start + offset + this->jumpInstOff, start + size);
        if (this->from == 0) {
            SYSLOG("Patcher+", "Failed to solve `%s` using jump pattern", safeString(this->symbol));
            return false;
        }
        DBGLOG("Patcher+", "Resolved `%s` at 0x%llX", safeString(this->symbol), this->from);
    }

    auto hasOrg = this->org != nullptr;
    auto wrapper = patcher.routeFunction(this->from, this->to, hasOrg);
    if (hasOrg) {
        if (wrapper == 0) { return false; }
        *this->org = wrapper;
        return true;
    } else {
        return wrapper == 0;
    }
}

bool PatcherPlus::JumpPatternRouteRequest::routeAll(KernelPatcher &patcher, const size_t id,
    JumpPatternRouteRequest *const requests, const size_t count, const mach_vm_address_t start, const size_t size) {
    for (size_t i = 0; i < count; i++) {
        if (requests[i].route(patcher, id, start, size)) {
            DBGLOG("Patcher+", "Applied jump pattern route (i: %zu)", i);
        } else {
            DBGLOG("Patcher+", "Failed to apply jump pattern route (i: %zu)", i);
            return false;
        }
    }
    return true;
}
