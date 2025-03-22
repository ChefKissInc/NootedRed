// Copyright © 2022-2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#include <PrivateHeaders/PatcherPlus.hpp>

bool PatcherPlus::PatternSolveRequest::solve(KernelPatcher &patcher, size_t id, mach_vm_address_t address,
    size_t maxSize) {
    PANIC_COND(this->address == nullptr, "Patcher+", "this->address is null for sym `%s`", safeString(this->symbol));
    patcher.clearError();

    if (this->symbol != nullptr) {
        *this->address = patcher.solveSymbol(id, this->symbol);
        if (*this->address != 0) { return true; }
        patcher.clearError();
    }

    if (this->pattern == nullptr || this->patternSize == 0 || address == 0 || maxSize == 0) {
        DBGLOG("Patcher+", "Failed to solve `%s` using symbol", safeString(this->symbol));
        return false;
    }

    size_t offset = 0;
    if (!KernelPatcher::findPattern(this->pattern, this->mask, this->patternSize,
            reinterpret_cast<const void *>(address), maxSize, &offset)) {
        DBGLOG("Patcher+", "Failed to solve `%s` using pattern", safeString(this->symbol));
        return false;
    }

    *this->address = address + offset;
    return true;
}

bool PatcherPlus::PatternSolveRequest::solveAll(KernelPatcher &patcher, size_t id, PatternSolveRequest *requests,
    size_t count, mach_vm_address_t address, size_t maxSize) {
    for (size_t i = 0; i < count; i++) {
        if (requests[i].solve(patcher, id, address, maxSize)) {
            DBGLOG("Patcher+", "Solved pattern request (i: %zu)", i);
        } else {
            DBGLOG("Patcher+", "Failed to solve pattern request (i: %zu)", i);
            return false;
        }
    }
    return true;
}

bool PatcherPlus::PatternRouteRequest::route(KernelPatcher &patcher, size_t id, mach_vm_address_t address,
    size_t maxSize) {
    patcher.clearError();

    if (this->symbol != nullptr) {
        if (address == 0 || maxSize == 0) {
            this->from = patcher.solveSymbol(id, this->symbol);
        } else {
            this->from = patcher.solveSymbol(id, this->symbol, address, maxSize, true);
        }
    }

    if (this->from == 0) {
        if (this->pattern == nullptr || this->patternSize == 0 || address == 0 || maxSize == 0) {
            DBGLOG("Patcher+", "Failed to route `%s` using symbol: %d", safeString(this->symbol), patcher.getError());
            return false;
        }
        DBGLOG("Patcher+", "Failed to route `%s` using symbol: %d. Attempting to use pattern.",
            safeString(this->symbol), patcher.getError());
        patcher.clearError();
        size_t offset = 0;
        if (!KernelPatcher::findPattern(this->pattern, this->mask, this->patternSize,
                reinterpret_cast<const void *>(address), maxSize, &offset)) {
            DBGLOG("Patcher+", "Failed to route `%s` using pattern", safeString(this->symbol));
            return false;
        }
        this->from = address + offset;
    }

    // Workaround as patcher internals will attempt to resolve
    // the symbol without checking if the `from` field is 0.
    this->symbol = nullptr;
    return patcher.routeMultipleLong(id, this, 1, address, maxSize);
}

bool PatcherPlus::PatternRouteRequest::routeAll(KernelPatcher &patcher, size_t id, PatternRouteRequest *requests,
    size_t count, mach_vm_address_t address, size_t maxSize) {
    for (size_t i = 0; i < count; i++) {
        if (requests[i].route(patcher, id, address, maxSize)) {
            DBGLOG("Patcher+", "Applied pattern route (i: %zu)", i);
        } else {
            DBGLOG("Patcher+", "Failed to apply pattern route (i: %zu)", i);
            return false;
        }
    }
    return true;
}

bool PatcherPlus::MaskedLookupPatch::apply(KernelPatcher &patcher, mach_vm_address_t address, size_t maxSize) const {
    patcher.clearError();
    if (this->findMask == nullptr && this->replaceMask == nullptr && this->skip == 0) {
        patcher.applyLookupPatch(this, reinterpret_cast<UInt8 *>(address), maxSize);
        return patcher.getError() == KernelPatcher::Error::NoError;
    }
    if (address == 0 || maxSize == 0) { return false; }
    return KernelPatcher::findAndReplaceWithMask(reinterpret_cast<UInt8 *>(address), maxSize, this->find, this->size,
        this->findMask, this->findMask ? this->size : 0, this->replace, this->size, this->replaceMask,
        this->replaceMask ? this->size : 0, this->count, this->skip);
}

bool PatcherPlus::MaskedLookupPatch::applyAll(KernelPatcher &patcher, const MaskedLookupPatch *patches, size_t count,
    mach_vm_address_t address, size_t maxSize, bool force) {
    for (size_t i = 0; i < count; i++) {
        if (patches[i].apply(patcher, address, maxSize)) {
            DBGLOG("Patcher+", "Applied patches[%zu]", i);
        } else {
            DBGLOG("Patcher+", "Failed to apply patches[%zu]", i);
            if (!force) { return false; }
        }
    }
    return true;
}

mach_vm_address_t PatcherPlus::jumpInstDestination(mach_vm_address_t address) {
    SInt64 off;

    auto inst = *reinterpret_cast<const UInt8 *>(address);
    if (inst == 0x0F) {
        inst = *reinterpret_cast<const UInt8 *>(address + 1);
        if (inst >= 0x80 && inst <= 0x8F) {
            off = *reinterpret_cast<const SInt32 *>(address + 2);
            off += 5;
        } else {
            return 0;
        }
    } else if (inst == 0xE8 || inst == 0xE9) {
        off = *reinterpret_cast<const SInt32 *>(address + 1);
        off += 5;
    } else if (inst >= 0x70 && inst <= 0x7F) {
        off = *reinterpret_cast<const SInt8 *>(address + 1);
        off += 2;
    } else {
        return 0;
    }

    if (off < 0) {
        auto abs = static_cast<UInt64>(-off);
        if (abs > address) {    // This should never happen.
            return 0;
        }
        return address - abs;
    } else {
        return address + static_cast<UInt64>(off);
    }
}

bool PatcherPlus::JumpPatternRouteRequest::route(KernelPatcher &patcher, size_t id, mach_vm_address_t address,
    size_t maxSize) {
    patcher.clearError();

    if (this->symbol != nullptr) {
        if (address == 0 || maxSize == 0) {
            this->from = patcher.solveSymbol(id, this->symbol);
        } else {
            this->from = patcher.solveSymbol(id, this->symbol, address, maxSize, true);
        }
    }

    if (this->from == 0) {
        if (this->pattern == nullptr || this->patternSize == 0 || address == 0 || maxSize == 0) {
            DBGLOG("Patcher+", "Failed to route `%s` using symbol: %d", safeString(this->symbol), patcher.getError());
            return false;
        }
        DBGLOG("Patcher+", "Failed to route `%s` using symbol: %d. Attempting to use jump pattern.",
            safeString(this->symbol), patcher.getError());
        patcher.clearError();
        size_t offset = 0;
        if (!KernelPatcher::findPattern(this->pattern, this->mask, this->patternSize,
                reinterpret_cast<const void *>(address), maxSize, &offset)) {
            DBGLOG("Patcher+", "Failed to route `%s` using jump pattern", safeString(this->symbol));
            return false;
        }
        this->from = jumpInstDestination(address + offset + this->jumpInstOff);
    }

    // Workaround as patcher internals will attempt to resolve
    // the symbol without checking if the `from` field is 0.
    this->symbol = nullptr;
    return patcher.routeMultipleLong(id, this, 1, address, maxSize);
}

bool PatcherPlus::JumpPatternRouteRequest::routeAll(KernelPatcher &patcher, size_t id,
    JumpPatternRouteRequest *requests, size_t count, mach_vm_address_t address, size_t maxSize) {
    for (size_t i = 0; i < count; i++) {
        if (requests[i].route(patcher, id, address, maxSize)) {
            DBGLOG("Patcher+", "Applied jump pattern route (i: %zu)", i);
        } else {
            DBGLOG("Patcher+", "Failed to apply jump pattern route (i: %zu)", i);
            return false;
        }
    }
    return true;
}
