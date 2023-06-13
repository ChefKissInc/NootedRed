//  Copyright © 2022-2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.0. See LICENSE for
//  details.

#include "kern_patcherplus.hpp"

bool SolveRequestPlus::solve(KernelPatcher *patcher, size_t index, mach_vm_address_t address, size_t size) {
    PANIC_COND(!this->address, "patcher+", "this->address is null");
    if (!this->guard) { return true; }

    if (patcher) {
        *this->address = patcher->solveSymbol(index, this->symbol);
        if (*this->address) { return true; }
        patcher->clearError();
    }

    if (!this->pattern || !this->patternSize) {
        DBGLOG("patcher+", "Failed to solve %s using symbol", safeString(this->symbol));
        return false;
    }

    size_t offset = 0;
    if (!KernelPatcher::findPattern(this->pattern, this->mask, this->patternSize,
            reinterpret_cast<const void *>(address), size, &offset) ||
        !offset) {
        DBGLOG("patcher+", "Failed to solve %s using pattern", safeString(this->symbol));
        return false;
    }

    *this->address = address + offset;
    return true;
}

bool SolveRequestPlus::solveAll(KernelPatcher *patcher, size_t index, SolveRequestPlus *requests, size_t count,
    mach_vm_address_t address, size_t size) {
    for (size_t i = 0; i < count; i++) {
        if (!requests[i].solve(patcher, index, address, size)) { return false; }
    }
    return true;
}

bool RouteRequestPlus::route(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
    if (!this->guard) { return true; }

    if (patcher.routeMultiple(index, this, 1, address, size)) { return true; }
    patcher.clearError();

    if (!this->pattern || !this->patternSize) {
        DBGLOG("patcher+", "Failed to route %s using symbol", safeString(this->symbol));
        return false;
    }

    size_t offset = 0;
    if (!KernelPatcher::findPattern(this->pattern, this->mask, this->patternSize,
            reinterpret_cast<const void *>(address), size, &offset) ||
        !offset) {
        DBGLOG("patcher+", "Failed to route %s using pattern", safeString(this->symbol));
        return false;
    }

    auto org = patcher.routeFunction(address + offset, this->to, true);
    if (!org) {
        DBGLOG("patcher+", "Failed to route %s using pattern: %d", safeString(this->symbol), patcher.getError());
        return false;
    }
    if (this->org) { *this->org = org; }

    return true;
}

bool RouteRequestPlus::routeAll(KernelPatcher &patcher, size_t index, RouteRequestPlus *requests, size_t count,
    mach_vm_address_t address, size_t size) {
    for (size_t i = 0; i < count; i++) {
        if (!requests[i].route(patcher, index, address, size)) { return false; }
    }
    return true;
}

bool LookupPatchPlus::apply(KernelPatcher *patcher, mach_vm_address_t address, size_t size) const {
    if (!this->guard) { return true; }

    if (patcher && this->kext && !this->findMask && !this->replaceMask && this->size == this->replaceSize &&
        !this->skip) {
        patcher->applyLookupPatch(this, reinterpret_cast<uint8_t *>(address), size);
        return patcher->getError() == KernelPatcher::Error::NoError;
    }
    return KernelPatcher::findAndReplaceWithMask(reinterpret_cast<uint8_t *>(address), size, this->find, this->size,
        this->findMask, this->findMask ? this->size : 0, this->replace, this->replaceSize, this->replaceMask,
        this->replaceMask ? this->replaceSize : 0, this->count, this->skip);
}

bool LookupPatchPlus::applyAll(KernelPatcher *patcher, LookupPatchPlus const *patches, size_t count,
    mach_vm_address_t address, size_t size) {
    for (size_t i = 0; i < count; i++) {
        if (!patches[i].apply(patcher, address, size)) {
            DBGLOG("patcher+", "Failed to apply patches[%zu]", i);
            return false;
        }
    }
    return true;
}
