//  Copyright © 2022-2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.0. See LICENSE for
//  details.

#include "kern_patcherplus.hpp"

bool SolveWithFallbackRequest::solve(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
    PANIC_COND(!this->address, "solver", "this->address is null");
    *this->address = patcher.solveSymbol(index, this->symbol);
    if (*this->address) { return true; }
    patcher.clearError();

    if (!this->pattern || !this->patternSize) {
        SYSLOG("solver", "Failed to solve %s using symbol", safeString(this->symbol));
        return false;
    }

    size_t offset = 0;
    if (!KernelPatcher::findPattern(this->pattern, this->mask, this->patternSize,
            reinterpret_cast<const void *>(address), size, &offset) ||
        !offset) {
        SYSLOG("solver", "Failed to solve %s using pattern", safeString(this->symbol));
        return false;
    }

    *this->address = address + offset;
    return true;
}

bool RouteWithFallbackRequest::route(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
    if (patcher.routeMultiple(index, this, 1, address, size)) { return true; }
    patcher.clearError();

    if (!this->pattern || !this->patternSize) {
        SYSLOG("solver", "Failed to route %s using symbol", safeString(this->symbol));
        return false;
    }

    size_t offset = 0;
    if (!KernelPatcher::findPattern(this->pattern, this->mask, this->patternSize,
            reinterpret_cast<const void *>(address), size, &offset) ||
        !offset) {
        SYSLOG("solver", "Failed to route %s using pattern", safeString(this->symbol));
        return false;
    }

    auto org = patcher.routeFunction(address + offset, this->to, true);
    if (!org) {
        SYSLOG("solver", "Failed to route %s using pattern: %d", safeString(this->symbol), patcher.getError());
        return false;
    }
    if (this->org) { *this->org = org; }

    return true;
}
