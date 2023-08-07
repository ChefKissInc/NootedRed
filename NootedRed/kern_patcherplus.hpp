//  Copyright © 2022-2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5. See LICENSE for
//  details.

#pragma once
#include <Headers/kern_patcher.hpp>

struct SolveRequestPlus : KernelPatcher::SolveRequest {
    const uint8_t *pattern {nullptr}, *mask {nullptr};
    size_t patternSize {0};
    bool guard {true};

    template<typename T>
    SolveRequestPlus(const char *s, T &addr, bool guard = true)
        : KernelPatcher::SolveRequest {s, addr}, guard {guard} {}

    template<typename T, typename P, size_t N>
    SolveRequestPlus(const char *s, T &addr, const P (&pattern)[N], bool guard = true)
        : KernelPatcher::SolveRequest {s, addr}, pattern {pattern}, patternSize {N}, guard {guard} {}

    template<typename T, typename P, size_t N>
    SolveRequestPlus(const char *s, T &addr, const P (&pattern)[N], const uint8_t (&mask)[N], bool guard = true)
        : KernelPatcher::SolveRequest {s, addr}, pattern {pattern}, mask {mask}, patternSize {N}, guard {guard} {}

    bool solve(KernelPatcher &patcher, size_t id, mach_vm_address_t address, size_t maxSize);

    static bool solveAll(KernelPatcher &patcher, size_t id, SolveRequestPlus *requests, size_t count,
        mach_vm_address_t address, size_t maxSize);

    template<size_t N>
    static bool solveAll(KernelPatcher &patcher, size_t id, SolveRequestPlus (&requests)[N], mach_vm_address_t address,
        size_t maxSize) {
        return solveAll(patcher, id, requests, N, address, maxSize);
    }
};

struct RouteRequestPlus : KernelPatcher::RouteRequest {
    const uint8_t *pattern {nullptr}, *mask {nullptr};
    size_t patternSize {0};
    bool guard {true};

    template<typename T>
    RouteRequestPlus(const char *s, T t, mach_vm_address_t &o, bool guard = true)
        : KernelPatcher::RouteRequest {s, t, o}, guard {guard} {}

    template<typename T, typename O>
    RouteRequestPlus(const char *s, T t, O &o, bool guard = true)
        : KernelPatcher::RouteRequest {s, t, o}, guard {guard} {}

    template<typename T>
    RouteRequestPlus(const char *s, T t, bool guard = true) : KernelPatcher::RouteRequest {s, t}, guard {guard} {}

    template<typename T, typename P, size_t N>
    RouteRequestPlus(const char *s, T t, mach_vm_address_t &o, const P (&pattern)[N], bool guard = true)
        : KernelPatcher::RouteRequest {s, t, o}, pattern {pattern}, patternSize {N}, guard {guard} {}

    template<typename T, typename O, typename P, size_t N>
    RouteRequestPlus(const char *s, T t, O &o, const P (&pattern)[N], bool guard = true)
        : KernelPatcher::RouteRequest {s, t, o}, pattern {pattern}, patternSize {N}, guard {guard} {}

    template<typename T, typename P, size_t N>
    RouteRequestPlus(const char *s, T t, const P (&pattern)[N], bool guard = true)
        : KernelPatcher::RouteRequest {s, t}, pattern {pattern}, patternSize {N}, guard {guard} {}

    template<typename T, typename P, size_t N>
    RouteRequestPlus(const char *s, T t, mach_vm_address_t &o, const P (&pattern)[N], const uint8_t (&mask)[N],
        bool guard = true)
        : KernelPatcher::RouteRequest {s, t, o}, pattern {pattern}, mask {mask}, patternSize {N}, guard {guard} {}

    template<typename T, typename O, typename P, size_t N>
    RouteRequestPlus(const char *s, T t, O &o, const P (&pattern)[N], const uint8_t (&mask)[N], bool guard = true)
        : KernelPatcher::RouteRequest {s, t, o}, pattern {pattern}, mask {mask}, patternSize {N}, guard {guard} {}

    template<typename T, typename P, size_t N>
    RouteRequestPlus(const char *s, T t, const P (&pattern)[N], const uint8_t (&mask)[N], bool guard = true)
        : KernelPatcher::RouteRequest {s, t}, pattern {pattern}, mask {mask}, patternSize {N}, guard {guard} {}

    bool route(KernelPatcher &patcher, size_t id, mach_vm_address_t address, size_t maxSize);

    static bool routeAll(KernelPatcher &patcher, size_t id, RouteRequestPlus *requests, size_t count,
        mach_vm_address_t address, size_t maxSize);

    template<size_t N>
    static bool routeAll(KernelPatcher &patcher, size_t id, RouteRequestPlus (&requests)[N], mach_vm_address_t address,
        size_t maxSize) {
        return routeAll(patcher, id, requests, N, address, maxSize);
    }
};

struct LookupPatchPlus : KernelPatcher::LookupPatch {
    const uint8_t *findMask {nullptr}, *replaceMask {nullptr};
    const bool guard {true};
    const size_t skip {0};

    LookupPatchPlus(KernelPatcher::KextInfo *kext, const uint8_t *find, const uint8_t *replace, size_t size,
        size_t count, bool guard = true, size_t skip = 0)
        : KernelPatcher::LookupPatch {kext, find, replace, size, count}, guard {guard}, skip {skip} {}

    LookupPatchPlus(KernelPatcher::KextInfo *kext, const uint8_t *find, const uint8_t *findMask, const uint8_t *replace,
        size_t size, size_t count, bool guard = true, size_t skip = 0)
        : KernelPatcher::LookupPatch {kext, find, replace, size, count}, findMask {findMask}, guard {guard},
          skip {skip} {}

    LookupPatchPlus(KernelPatcher::KextInfo *kext, const uint8_t *find, const uint8_t *findMask, const uint8_t *replace,
        const uint8_t *replaceMask, size_t size, size_t count, bool guard = true, size_t skip = 0)
        : KernelPatcher::LookupPatch {kext, find, replace, size, count}, findMask {findMask}, replaceMask {replaceMask},
          guard {guard}, skip {skip} {}

    template<size_t N>
    LookupPatchPlus(KernelPatcher::KextInfo *kext, const uint8_t (&find)[N], const uint8_t (&replace)[N], size_t count,
        bool guard = true, size_t skip = 0)
        : LookupPatchPlus {kext, find, replace, N, count, guard, skip} {}

    template<size_t N>
    LookupPatchPlus(KernelPatcher::KextInfo *kext, const uint8_t (&find)[N], const uint8_t (&findMask)[N],
        const uint8_t (&replace)[N], size_t count, bool guard = true, size_t skip = 0)
        : LookupPatchPlus {kext, find, findMask, replace, N, count, guard, skip} {}

    template<size_t N>
    LookupPatchPlus(KernelPatcher::KextInfo *kext, const uint8_t (&find)[N], const uint8_t (&findMask)[N],
        const uint8_t (&replace)[N], const uint8_t (&replaceMask)[N], size_t count, bool guard = true, size_t skip = 0)
        : LookupPatchPlus {kext, find, findMask, replace, replaceMask, N, count, guard, skip} {}

    bool apply(KernelPatcher &patcher, mach_vm_address_t address, size_t maxSize) const;

    static bool applyAll(KernelPatcher &patcher, LookupPatchPlus const *patches, size_t count,
        mach_vm_address_t address, size_t maxSize);

    template<size_t N>
    static bool applyAll(KernelPatcher &patcher, LookupPatchPlus const (&patches)[N], mach_vm_address_t address,
        size_t maxSize) {
        return applyAll(patcher, patches, N, address, maxSize);
    }
};
