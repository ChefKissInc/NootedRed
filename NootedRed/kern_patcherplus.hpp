//  Copyright © 2022-2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.0. See LICENSE for
//  details.

#pragma once
#include <Headers/kern_patcher.hpp>

struct SolveWithFallbackRequest : public KernelPatcher::SolveRequest {
    const uint8_t *pattern {nullptr};
    const uint8_t *mask {nullptr};
    size_t patternSize {0};

    template<typename T>
    SolveWithFallbackRequest(const char *s, T &addr) : KernelPatcher::SolveRequest(s, addr) {}

    template<typename T, typename P, size_t N>
    SolveWithFallbackRequest(const char *s, T &addr, const P (&pattern)[N])
        : KernelPatcher::SolveRequest(s, addr), pattern {pattern}, patternSize {N} {}

    template<typename T, typename P, size_t N>
    SolveWithFallbackRequest(const char *s, T &addr, const P (&pattern)[N], const uint8_t (&mask)[N])
        : KernelPatcher::SolveRequest(s, addr), pattern {pattern}, mask {mask}, patternSize {N} {}

    bool solve(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size);

    static bool solveAll(KernelPatcher &patcher, size_t index, SolveWithFallbackRequest *requests, size_t count,
        mach_vm_address_t address, size_t size);

    template<size_t N>
    static bool solveAll(KernelPatcher &patcher, size_t index, SolveWithFallbackRequest (&requests)[N],
        mach_vm_address_t address, size_t size) {
        return solveAll(patcher, index, requests, N, address, size);
    }
};

struct RouteWithFallbackRequest : public KernelPatcher::RouteRequest {
    const uint8_t *pattern {nullptr};
    const uint8_t *mask {nullptr};
    size_t patternSize {0};

    template<typename T, typename P, size_t N>
    RouteWithFallbackRequest(const char *s, T t, mach_vm_address_t &o, const P (&pattern)[N])
        : KernelPatcher::RouteRequest(s, t, o), pattern {pattern}, patternSize {N} {}

    template<typename T, typename O, typename P, size_t N>
    RouteWithFallbackRequest(const char *s, T t, O &o, const P (&pattern)[N])
        : KernelPatcher::RouteRequest(s, t, o), pattern {pattern}, patternSize {N} {}

    template<typename T, typename P, size_t N>
    RouteWithFallbackRequest(const char *s, T t, const P (&pattern)[N])
        : KernelPatcher::RouteRequest(s, t), pattern {pattern}, patternSize {N} {}

    template<typename T, typename P, size_t N>
    RouteWithFallbackRequest(const char *s, T t, mach_vm_address_t &o, const P (&pattern)[N], const uint8_t (&mask)[N])
        : KernelPatcher::RouteRequest(s, t, o), pattern {pattern}, mask {mask}, patternSize {N} {}

    template<typename T, typename O, typename P, size_t N>
    RouteWithFallbackRequest(const char *s, T t, O &o, const P (&pattern)[N], const uint8_t (&mask)[N])
        : KernelPatcher::RouteRequest(s, t, o), pattern {pattern}, mask {mask}, patternSize {N} {}

    template<typename T, typename P, size_t N>
    RouteWithFallbackRequest(const char *s, T t, const P (&pattern)[N], const uint8_t (&mask)[N])
        : KernelPatcher::RouteRequest(s, t), pattern {pattern}, mask {mask}, patternSize {N} {}

    template<typename T>
    RouteWithFallbackRequest(const char *s, T t, mach_vm_address_t &o) : KernelPatcher::RouteRequest(s, t, o) {}

    template<typename T, typename O>
    RouteWithFallbackRequest(const char *s, T t, O &o) : KernelPatcher::RouteRequest(s, t, o) {}

    template<typename T>
    RouteWithFallbackRequest(const char *s, T t) : KernelPatcher::RouteRequest(s, t) {}

    bool route(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size);

    static bool routeAll(KernelPatcher &patcher, size_t index, RouteWithFallbackRequest *requests, size_t count,
        mach_vm_address_t address, size_t size);

    template<size_t N>
    static bool routeAll(KernelPatcher &patcher, size_t index, RouteWithFallbackRequest (&requests)[N],
        mach_vm_address_t address, size_t size) {
        return routeAll(patcher, index, requests, N, address, size);
    }
};
