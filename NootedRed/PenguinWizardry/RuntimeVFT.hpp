// Runtime Virtual Function Table
//
// Copyright © 2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <Headers/kern_patcher.hpp>
#include <Headers/kern_util.hpp>
#include <PenguinWizardry/IsFunction.hpp>

extern "C" [[noreturn]]
void __cxa_pure_virtual();    // NOLINT(bugprone-reserved-identifier)

class RuntimeVFTBase
{
    UInt32 _count{0};
    void** _vft{nullptr};

protected:
    void init(const RuntimeVFTBase& super, const UInt32 expansion)
    {
        assert(this->_vft == nullptr);
        assert(super._count != 0);
        this->_count = super._count;
        this->_vft   = IONew(void*, this->_count + expansion);
        memcpy(static_cast<void*>(this->_vft), static_cast<void*>(super._vft), this->_count * sizeof(*this->_vft));
        for (UInt32 i = 0; i < expansion; ++i) {
            this->_vft[this->_count + i] = reinterpret_cast<void*>(&__cxa_pure_virtual);
        }
    }

public:
    void init(void** const src)
    {
        assert(src != nullptr);
        assert(this->_count == 0);
        while (src[this->_count] != nullptr) { ++this->_count; }
        assert(this->_count != 0);
        this->_vft = src;
    }

    void resolve(KernelPatcher& patcher, const size_t id, const char* const symbol, const mach_vm_address_t start,
                 const size_t size)
    {
        assert(symbol != nullptr);
        const auto vt = patcher.solveSymbol<void**>(id, symbol, start, size, true);
        PANIC_COND(vt == nullptr, "RuntimeVFT", "Failed to resolve %s", symbol);
        this->init(vt + 2);
    }

    void replaceVFT(void* const obj) const
    {
        assert(obj != nullptr);
        assert(this->_vft != nullptr);
        getMember<void**>(obj, 0) = this->_vft;
    }

    template<typename T>
    auto& get(const UInt32 i) const
    {
        static_assert(is_function_v<T>, "T must be a function");
        assert(this->_vft != nullptr);
        assertf(i < this->_count, "i=0x%X, _count=0x%X", i, this->_count);
        return reinterpret_cast<T*&>(this->_vft[i]);
    }

    template<typename T>
    auto getExpanded(const void* const obj, const UInt32 i) const
    {
        static_assert(is_function_v<T>, "T must be a function");
        assert(obj != nullptr);
        assert(this->_vft != nullptr);
        return reinterpret_cast<T*&>((*static_cast<void** const*>(obj))[this->_count + i]);
    }

    auto inner() const
    {
        assert(this->_vft != nullptr);
        return this->_vft;
    }

    auto count() const { return this->_count; }
};

template<const UInt32 ExpansionCount = 0>
class RuntimeVFT : public RuntimeVFTBase
{
public:
    void init(const RuntimeVFTBase& super) { RuntimeVFTBase::init(super, ExpansionCount); }

    template<typename T>
    auto& getExpanded(const UInt32 i) const
    {
        static_assert(is_function_v<T>, "T must be a function");
        assert(this->inner() != nullptr);
        assertf(i < ExpansionCount, "i=0x%X, ExpansionCount=0x%X", i, ExpansionCount);
        return reinterpret_cast<T*&>(this->inner()[this->count() + i]);
    }
};
