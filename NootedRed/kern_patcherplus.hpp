//  Copyright © 2022-2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.0. See LICENSE for
//  details.

#pragma once
#include <Headers/kern_patcher.hpp>

struct SolveRequestPlus : KernelPatcher::SolveRequest {
    const uint8_t *pattern {nullptr};
    const uint8_t *mask {nullptr};
    size_t patternSize {0};
    bool guard {true};

    template<typename T>
    SolveRequestPlus(const char *s, T &addr, bool guard = true) : KernelPatcher::SolveRequest(s, addr), guard {guard} {}

    template<typename T, typename P, size_t N>
    SolveRequestPlus(const char *s, T &addr, const P (&pattern)[N], bool guard = true)
        : KernelPatcher::SolveRequest(s, addr), pattern {pattern}, patternSize {N}, guard {guard} {}

    template<typename T, typename P, size_t N>
    SolveRequestPlus(const char *s, T &addr, const P (&pattern)[N], const uint8_t (&mask)[N], bool guard = true)
        : KernelPatcher::SolveRequest(s, addr), pattern {pattern}, mask {mask}, patternSize {N}, guard {guard} {}

    bool solve(KernelPatcher *patcher, size_t index, mach_vm_address_t address, size_t size);

    static bool solveAll(KernelPatcher *patcher, size_t index, SolveRequestPlus *requests, size_t count,
        mach_vm_address_t address, size_t size);

    template<size_t N>
    static bool solveAll(KernelPatcher *patcher, size_t index, SolveRequestPlus (&requests)[N],
        mach_vm_address_t address, size_t size) {
        return solveAll(patcher, index, requests, N, address, size);
    }
};

struct RouteRequestPlus : KernelPatcher::RouteRequest {
    const uint8_t *pattern {nullptr};
    const uint8_t *mask {nullptr};
    size_t patternSize {0};
    bool guard {true};

    template<typename T>
    RouteRequestPlus(const char *s, T t, mach_vm_address_t &o, bool guard = true)
        : KernelPatcher::RouteRequest(s, t, o), guard {guard} {}

    template<typename T, typename O>
    RouteRequestPlus(const char *s, T t, O &o, bool guard = true)
        : KernelPatcher::RouteRequest(s, t, o), guard {guard} {}

    template<typename T>
    RouteRequestPlus(const char *s, T t, bool guard = true) : KernelPatcher::RouteRequest(s, t), guard {guard} {}

    template<typename T, typename P, size_t N>
    RouteRequestPlus(const char *s, T t, mach_vm_address_t &o, const P (&pattern)[N], bool guard = true)
        : KernelPatcher::RouteRequest(s, t, o), pattern {pattern}, patternSize {N}, guard {guard} {}

    template<typename T, typename O, typename P, size_t N>
    RouteRequestPlus(const char *s, T t, O &o, const P (&pattern)[N], bool guard = true)
        : KernelPatcher::RouteRequest(s, t, o), pattern {pattern}, patternSize {N}, guard {guard} {}

    template<typename T, typename P, size_t N>
    RouteRequestPlus(const char *s, T t, const P (&pattern)[N], bool guard = true)
        : KernelPatcher::RouteRequest(s, t), pattern {pattern}, patternSize {N}, guard {guard} {}

    template<typename T, typename P, size_t N>
    RouteRequestPlus(const char *s, T t, mach_vm_address_t &o, const P (&pattern)[N], const uint8_t (&mask)[N],
        bool guard = true)
        : KernelPatcher::RouteRequest(s, t, o), pattern {pattern}, mask {mask}, patternSize {N}, guard {guard} {}

    template<typename T, typename O, typename P, size_t N>
    RouteRequestPlus(const char *s, T t, O &o, const P (&pattern)[N], const uint8_t (&mask)[N], bool guard = true)
        : KernelPatcher::RouteRequest(s, t, o), pattern {pattern}, mask {mask}, patternSize {N}, guard {guard} {}

    template<typename T, typename P, size_t N>
    RouteRequestPlus(const char *s, T t, const P (&pattern)[N], const uint8_t (&mask)[N], bool guard = true)
        : KernelPatcher::RouteRequest(s, t), pattern {pattern}, mask {mask}, patternSize {N}, guard {guard} {}

    bool route(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size);

    static bool routeAll(KernelPatcher &patcher, size_t index, RouteRequestPlus *requests, size_t count,
        mach_vm_address_t address, size_t size);

    template<size_t N>
    static bool routeAll(KernelPatcher &patcher, size_t index, RouteRequestPlus (&requests)[N],
        mach_vm_address_t address, size_t size) {
        return routeAll(patcher, index, requests, N, address, size);
    }
};

struct LookupPatchPlus : KernelPatcher::LookupPatch {
    const uint8_t *findMask {nullptr};
    const uint8_t *replaceMask {nullptr};
    const size_t replaceSize {0};
    const bool guard {true};
    const size_t skip {0};

    LookupPatchPlus(KernelPatcher::KextInfo *kext, const uint8_t *find, const uint8_t *replace, size_t size,
        size_t count, bool guard = true, size_t skip = 0)
        : KernelPatcher::LookupPatch {kext, find, replace, size, count}, replaceSize {size}, guard {guard},
          skip {skip} {}

    template<size_t N>
    LookupPatchPlus(KernelPatcher::KextInfo *kext, const uint8_t (&find)[N], const uint8_t (&replace)[N], size_t count,
        bool guard = true, size_t skip = 0)
        : LookupPatchPlus(kext, find, replace, N, count, guard, skip) {}

    template<size_t N>
    LookupPatchPlus(KernelPatcher::KextInfo *kext, const uint8_t (&find)[N], const uint8_t (&findMask)[N],
        const uint8_t (&replace)[N], size_t count, bool guard = true, size_t skip = 0)
        : KernelPatcher::LookupPatch {kext, find, replace, N, count}, findMask {findMask}, replaceSize {N},
          guard {guard}, skip {skip} {}

    template<size_t N, size_t M>
    LookupPatchPlus(KernelPatcher::KextInfo *kext, const uint8_t (&find)[N], const uint8_t (&findMask)[N],
        const uint8_t (&replace)[M], size_t count, bool guard = true, size_t skip = 0)
        : KernelPatcher::LookupPatch {kext, find, replace, N, count}, findMask {findMask}, replaceSize {M},
          guard {guard}, skip {skip} {}

    template<size_t N, size_t M>
    LookupPatchPlus(KernelPatcher::KextInfo *kext, const uint8_t (&find)[N], const uint8_t (&findMask)[N],
        const uint8_t (&replace)[M], const uint8_t (&replaceMask)[M], size_t count, bool guard = true, size_t skip = 0)
        : KernelPatcher::LookupPatch {kext, find, replace, N, count}, findMask {findMask}, replaceMask {replaceMask},
          replaceSize {M}, guard {guard}, skip {skip} {}

    bool apply(KernelPatcher *patcher, mach_vm_address_t address, size_t size) const;

    static bool applyAll(KernelPatcher *patcher, LookupPatchPlus const *patches, size_t count,
        mach_vm_address_t address, size_t size);

    template<size_t N>
    static bool applyAll(KernelPatcher *patcher, LookupPatchPlus const (&patches)[N], mach_vm_address_t address,
        size_t size) {
        return applyAll(patcher, patches, N, address, size);
    }
};
