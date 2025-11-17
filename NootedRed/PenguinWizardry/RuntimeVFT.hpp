// Dynamic Virtual Function Table
//
// Copyright © 2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <Headers/kern_patcher.hpp>
#include <Headers/kern_util.hpp>
#include <PenguinWizardry/IsFunction.hpp>

template<UInt32 (*const SourceCount)(), const UInt32 ExpansionCount = 0>
class RuntimeVFT {
    UInt32 sourceCount {0};
    void **vft {nullptr};    // intentional leak, VTables don't get deallocated

    static void *operator new(size_t) {}       // disallow allocating this
    void operator delete(void *, size_t) {}    // class in the heap or copying
    void operator=(OSMetaClassBase &) {}       // it, that's the wrong usage.

    public:
    void init(void **const srcVft) {
        assert(srcVft != nullptr);
        assert(this->sourceCount == 0);
        assert(this->vft == nullptr);
        this->sourceCount = SourceCount();
        assert(this->sourceCount != 0);
        this->vft = IONew(void *, this->sourceCount + ExpansionCount);
        memcpy(this->vft, srcVft, this->sourceCount * sizeof(*this->vft));
        bzero(&this->vft[this->sourceCount], ExpansionCount * sizeof(*this->vft));
    }

    void resolve(KernelPatcher &patcher, const size_t id, const char *const symbol, const mach_vm_address_t start,
        const size_t size) {
        assert(symbol != nullptr);
        auto *srcVft = patcher.solveSymbol<void **>(id, symbol, start, size, true);
        PANIC_COND(srcVft == nullptr, "RuntimeVFT", "Failed to resolve `%s`", symbol);
        this->init(srcVft + 2);    // 2 * pointer-size offset is where vfuncs reside
    }

    void replaceVFT(void *const obj) const {
        assert(obj != nullptr);
        assert(this->vft != nullptr);
        assert(this->sourceCount != 0);
        getMember<decltype(this->vft)>(obj, 0) = this->vft;
    }

    template<typename T>
    T *&get(const UInt32 i) const {
        static_assert(is_function_v<T>, "T must be a function");
        assert(this->vft != nullptr);
        assert(this->sourceCount != 0);
        assertf(i < this->sourceCount, "i=0x%X, sourceCount=0x%X", i, this->sourceCount);
        return reinterpret_cast<T *&>(this->vft[i]);
    }

    template<typename T>
    T *&getExpanded(const UInt32 i) const {
        static_assert(is_function_v<T>, "T must be a function");
        assert(this->vft != nullptr);
        assert(this->sourceCount != 0);
        assertf(i < ExpansionCount, "i=0x%X, ExpansionCount=0x%X", i, ExpansionCount);
        return *reinterpret_cast<T **>(this->vft + this->sourceCount + i);
    }
};
