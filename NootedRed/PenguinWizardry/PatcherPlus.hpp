// Copyright © 2022-2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <Headers/kern_patcher.hpp>

namespace PatcherPlus {
    struct PatternSolveRequest : KernelPatcher::SolveRequest {
        const UInt8 *const pattern {nullptr}, *const mask {nullptr};
        const size_t patternSize {0};

        template<typename T>
        PatternSolveRequest(const char *s, T &addr) : KernelPatcher::SolveRequest {s, addr} {}

        template<typename T, typename P, const size_t N>
        PatternSolveRequest(const char *s, T &addr, const P (&pattern)[N])
            : KernelPatcher::SolveRequest {s, addr}, pattern {pattern}, patternSize {N} {}

        template<typename T, typename P, const size_t N>
        PatternSolveRequest(const char *s, T &addr, const P (&pattern)[N], const UInt8 (&mask)[N])
            : KernelPatcher::SolveRequest {s, addr}, pattern {pattern}, mask {mask}, patternSize {N} {}

        bool solve(KernelPatcher &patcher, const size_t id, const mach_vm_address_t start, const size_t size);

        static bool solveAll(KernelPatcher &patcher, const size_t id, PatternSolveRequest *const requests,
            const size_t count, const mach_vm_address_t start, const size_t size);

        template<size_t N>
        static bool solveAll(KernelPatcher &patcher, const size_t id, PatternSolveRequest (&requests)[N],
            const mach_vm_address_t start, const size_t size) {
            return solveAll(patcher, id, requests, N, start, size);
        }
    };

    struct PatternRouteRequest : KernelPatcher::RouteRequest {
        const UInt8 *const pattern {nullptr}, *const mask {nullptr};
        const size_t patternSize {0};

        template<typename T>
        PatternRouteRequest(const char *s, T t, mach_vm_address_t &o) : KernelPatcher::RouteRequest {s, t, o} {}

        template<typename T, typename O>
        PatternRouteRequest(const char *s, T t, O &o) : KernelPatcher::RouteRequest {s, t, o} {}

        template<typename T>
        PatternRouteRequest(const char *s, T t) : KernelPatcher::RouteRequest {s, t} {}

        template<typename T, typename P, const size_t N>
        PatternRouteRequest(const char *s, T t, mach_vm_address_t &o, const P (&pattern)[N])
            : KernelPatcher::RouteRequest {s, t, o}, pattern {pattern}, patternSize {N} {}

        template<typename T, typename O, typename P, const size_t N>
        PatternRouteRequest(const char *s, T t, O &o, const P (&pattern)[N])
            : KernelPatcher::RouteRequest {s, t, o}, pattern {pattern}, patternSize {N} {}

        template<typename T, typename P, const size_t N>
        PatternRouteRequest(const char *s, T t, const P (&pattern)[N])
            : KernelPatcher::RouteRequest {s, t}, pattern {pattern}, patternSize {N} {}

        template<typename T, typename P, const size_t N>
        PatternRouteRequest(const char *s, T t, mach_vm_address_t &o, const P (&pattern)[N], const UInt8 (&mask)[N])
            : KernelPatcher::RouteRequest {s, t, o}, pattern {pattern}, mask {mask}, patternSize {N} {}

        template<typename T, typename O, typename P, const size_t N>
        PatternRouteRequest(const char *s, T t, O &o, const P (&pattern)[N], const UInt8 (&mask)[N])
            : KernelPatcher::RouteRequest {s, t, o}, pattern {pattern}, mask {mask}, patternSize {N} {}

        template<typename T, typename P, const size_t N>
        PatternRouteRequest(const char *s, T t, const P (&pattern)[N], const UInt8 (&mask)[N])
            : KernelPatcher::RouteRequest {s, t}, pattern {pattern}, mask {mask}, patternSize {N} {}

        bool route(KernelPatcher &patcher, const size_t id, const mach_vm_address_t start, const size_t size);

        static bool routeAll(KernelPatcher &patcher, const size_t id, PatternRouteRequest *const requests, size_t count,
            const mach_vm_address_t start, const size_t size);

        template<size_t N>
        static bool routeAll(KernelPatcher &patcher, const size_t id, PatternRouteRequest (&requests)[N],
            const mach_vm_address_t start, const size_t size) {
            return routeAll(patcher, id, requests, N, start, size);
        }
    };

    struct MaskedLookupPatch : KernelPatcher::LookupPatch {
        const UInt8 *const findMask {nullptr}, *const replaceMask {nullptr};
        const size_t skip {0};

        MaskedLookupPatch(KernelPatcher::KextInfo *kext, const UInt8 *find, const UInt8 *replace, size_t size,
            const size_t count, const size_t skip = 0)
            : KernelPatcher::LookupPatch {kext, find, replace, size, count}, skip {skip} {}

        MaskedLookupPatch(KernelPatcher::KextInfo *kext, const UInt8 *find, const UInt8 *findMask, const UInt8 *replace,
            const size_t size, const size_t count, const size_t skip = 0)
            : KernelPatcher::LookupPatch {kext, find, replace, size, count}, findMask {findMask}, skip {skip} {}

        MaskedLookupPatch(KernelPatcher::KextInfo *kext, const UInt8 *find, const UInt8 *findMask, const UInt8 *replace,
            const UInt8 *replaceMask, const size_t size, const size_t count, const size_t skip = 0)
            : KernelPatcher::LookupPatch {kext, find, replace, size, count}, findMask {findMask},
              replaceMask {replaceMask}, skip {skip} {}

        template<const size_t N>
        MaskedLookupPatch(KernelPatcher::KextInfo *kext, const UInt8 (&find)[N], const UInt8 (&replace)[N],
            const size_t count, const size_t skip = 0)
            : MaskedLookupPatch {kext, find, replace, N, count, skip} {}

        template<const size_t N>
        MaskedLookupPatch(KernelPatcher::KextInfo *kext, const UInt8 (&find)[N], const UInt8 (&findMask)[N],
            const UInt8 (&replace)[N], const size_t count, const size_t skip = 0)
            : MaskedLookupPatch {kext, find, findMask, replace, N, count, skip} {}

        template<const size_t N>
        MaskedLookupPatch(KernelPatcher::KextInfo *kext, const UInt8 (&find)[N], const UInt8 (&findMask)[N],
            const UInt8 (&replace)[N], const UInt8 (&replaceMask)[N], const size_t count, const size_t skip = 0)
            : MaskedLookupPatch {kext, find, findMask, replace, replaceMask, N, count, skip} {}

        bool apply(KernelPatcher &patcher, const mach_vm_address_t start, const size_t size) const;

        static bool applyAll(KernelPatcher &patcher, const MaskedLookupPatch *const patches, const size_t count,
            const mach_vm_address_t start, const size_t size, const bool force = false);

        template<const size_t N>
        static bool applyAll(KernelPatcher &patcher, const MaskedLookupPatch (&patches)[N],
            const mach_vm_address_t start, const size_t size, const bool force = false) {
            return applyAll(patcher, patches, N, start, size, force);
        }
    };

    mach_vm_address_t jumpInstDestination(const mach_vm_address_t start, const mach_vm_address_t end);

    struct JumpPatternRouteRequest : KernelPatcher::RouteRequest {
        const UInt8 *const pattern {nullptr}, *const mask {nullptr};
        const size_t patternSize {0};
        const size_t jumpInstOff {0};

        template<typename T>
        JumpPatternRouteRequest(const char *s, T t) : KernelPatcher::RouteRequest {s, t} {}

        template<typename T>
        JumpPatternRouteRequest(const char *s, T t, mach_vm_address_t &o) : KernelPatcher::RouteRequest {s, t, o} {}

        template<typename T, typename O>
        JumpPatternRouteRequest(const char *s, T t, O &o) : KernelPatcher::RouteRequest {s, t, o} {}

        template<typename T, typename P, const size_t N>
        JumpPatternRouteRequest(const char *s, T t, mach_vm_address_t &o, const P (&pattern)[N],
            const size_t jumpInstOff)
            : KernelPatcher::RouteRequest {s, t, o}, pattern {pattern}, patternSize {N}, jumpInstOff {jumpInstOff} {}

        template<typename T, typename O, typename P, const size_t N>
        JumpPatternRouteRequest(const char *s, T t, O &o, const P (&pattern)[N], const size_t jumpInstOff)
            : KernelPatcher::RouteRequest {s, t, o}, pattern {pattern}, patternSize {N}, jumpInstOff {jumpInstOff} {}

        template<typename T, typename P, const size_t N>
        JumpPatternRouteRequest(const char *s, T t, const P (&pattern)[N], const size_t jumpInstOff)
            : KernelPatcher::RouteRequest {s, t}, pattern {pattern}, patternSize {N}, jumpInstOff {jumpInstOff} {}

        template<typename T, typename P, const size_t N>
        JumpPatternRouteRequest(const char *s, T t, mach_vm_address_t &o, const P (&pattern)[N], const UInt8 (&mask)[N],
            const size_t jumpInstOff)
            : KernelPatcher::RouteRequest {s, t, o}, pattern {pattern}, mask {mask}, patternSize {N},
              jumpInstOff {jumpInstOff} {}

        template<typename T, typename O, typename P, const size_t N>
        JumpPatternRouteRequest(const char *s, T t, O &o, const P (&pattern)[N], const UInt8 (&mask)[N],
            const size_t jumpInstOff)
            : KernelPatcher::RouteRequest {s, t, o}, pattern {pattern}, mask {mask}, patternSize {N},
              jumpInstOff {jumpInstOff} {}

        template<typename T, typename P, const size_t N>
        JumpPatternRouteRequest(const char *s, T t, const P (&pattern)[N], const UInt8 (&mask)[N],
            const size_t jumpInstOff)
            : KernelPatcher::RouteRequest {s, t}, pattern {pattern}, mask {mask}, patternSize {N},
              jumpInstOff {jumpInstOff} {}

        bool route(KernelPatcher &patcher, const size_t id, const mach_vm_address_t start, const size_t size);

        static bool routeAll(KernelPatcher &patcher, const size_t id, JumpPatternRouteRequest *const requests,
            const size_t count, const mach_vm_address_t start, const size_t size);

        template<size_t N>
        static bool routeAll(KernelPatcher &patcher, const size_t id, JumpPatternRouteRequest (&requests)[N],
            const mach_vm_address_t start, const size_t size) {
            return routeAll(patcher, id, requests, N, start, size);
        }
    };
}    // namespace PatcherPlus
