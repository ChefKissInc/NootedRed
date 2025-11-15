// Copyright © 2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
#include <Headers/kern_patcher.hpp>
#include <Headers/kern_util.hpp>
#include <NRed.hpp>
#include <PenguinWizardry/EnableIf.hpp>
#include <PenguinWizardry/RuntimeVFT.hpp>
#include <PenguinWizardry/TypeName.hpp>

#define PWDeclareRuntimeMC(_cls, _ctor, ...)                                    \
    static PenguinWizardry::RuntimeMC<_cls, _ctor, ##__VA_ARGS__> gRTMetaClass; \
                                                                                \
    static OSMetaClass *getMetaClass(const _cls *self);
#define PWDefineRuntimeMC(_cls, _ctor, ...)                                             \
    DEFINE_TYPE_NAME(_cls);                                                             \
                                                                                        \
    PenguinWizardry::RuntimeMC<_cls, _cls::_ctor, ##__VA_ARGS__> _cls::gRTMetaClass {}; \
                                                                                        \
    OSMetaClass *_cls::getMetaClass(const _cls *const) { return gRTMetaClass.getMetaClass(); }

#define PWDeclareRuntimeMCWithExpansion(_cls, _ctor, _exp) \
    PWDeclareRuntimeMC(_cls, _ctor, _exp);                 \
                                                           \
    _exp &getExpansion() { return gRTMetaClass.getExpansion(this); }
#define PWDefineRuntimeMCWithExpansion(_cls, _ctor, _exp) PWDefineRuntimeMC(_cls, _ctor, _cls::_exp)

#define PWDeclareAbstractRuntimeMC(_cls, ...) \
    static PenguinWizardry::RuntimeMC<_cls, nullptr, ##__VA_ARGS__> gRTMetaClass
#define PWDeclareAbstractRuntimeMCWithExpansion(_cls, _exp) \
    PWDeclareAbstractRuntimeMC(_cls, _exp);                 \
                                                            \
    _exp &getExpansion() { return gRTMetaClass.getExpansion(this); }
#define PWDefineAbstractRuntimeMC(_cls, ...) \
    DEFINE_TYPE_NAME(_cls);                  \
                                             \
    PenguinWizardry::RuntimeMC<_cls, nullptr, ##__VA_ARGS__> _cls::gRTMetaClass {}
#define PWDefineAbstractRuntimeMCWithExpansion(_cls, _exp) PWDefineAbstractRuntimeMC(_cls, _cls::_exp)

#define PWPopulateRuntimeMCGetMetaClassVFTEntry() vft.get<decltype(getMetaClass)>(7) = getMetaClass

namespace PenguinWizardry {
    class RuntimeMCBase {
        friend class RuntimeMCManager;

        static UInt32 vftSize() { return 0x1A; }

        protected:
        RuntimeVFT<vftSize> vft {};
        OSMetaClass *mc {nullptr};    // intentional leak, meta classes don't get deallocated

        virtual ~RuntimeMCBase() {}

        virtual UInt32 getExpansionSize() const = 0;
        virtual const char *getClassName() const = 0;
        virtual void populateVFT() = 0;

        public:
        RuntimeMCBase() : mc {IONew(OSMetaClass, 1)} { bzero(static_cast<void *>(this->mc), sizeof(*this->mc)); }

        OSMetaClass *getMetaClass() const { return this->mc; }
    };

    class RuntimeMCManager {
        void (*orgMetaClassConstructor)(OSMetaClass *self, const char *className, const OSMetaClass *superclass,
            UInt32 classSize) {nullptr};
        void **orgMetaClassVFuncs {nullptr};
        mach_vm_address_t orgPostModLoad {0};
        OSDictionary *pendingMCsForKext {OSDictionary::withCapacity(0)};

        struct Pending {
            RuntimeMCBase *mc;
            const OSMetaClass *super;
        };

        void registerPending(Pending *pending);
        void registerMC(RuntimeMCBase &mc, const char *kext, const OSMetaClass *super);

        static OSReturn wrapPostModLoad(void *loadHandle);

        public:
        static RuntimeMCManager &singleton();

        void processPatcher(KernelPatcher &patcher);

        void registerMC(RuntimeMCBase &rtMC, const char *kext, KernelPatcher &patcher, const size_t id,
            const char *const symbol, const mach_vm_address_t start, const size_t size);
        void registerMC(RuntimeMCBase &rtMC, const char *kext, RuntimeMCBase &rtSuper);
    };

    template<typename Class, void (*const Constructor)(Class *self, const OSMetaClass *metaClass),
        typename Expansion = void>
    class RuntimeMC : public RuntimeMCBase {
        static constexpr UInt32 ALLOC_VFUNC_INDEX = 0x11;

        public:
        virtual UInt32 getExpansionSize() const APPLE_KEXT_OVERRIDE {
            if constexpr (is_void_v<Expansion>) {
                return 0;
            } else {
                return sizeof(Expansion);
            }
        }
        virtual const char *getClassName() const APPLE_KEXT_OVERRIDE { return type_name<Class>(); }

        static void *AllocFunc(OSMetaClass *const self) {
            if constexpr (Constructor == nullptr) {
                return nullptr;
            } else {
                auto *obj = static_cast<Class *>(OSObject::operator new(self->getClassSize()));
                Constructor(obj, self);
                return obj;
            }
        }

        virtual void populateVFT() APPLE_KEXT_OVERRIDE {
            this->vft.template get<decltype(AllocFunc)>(ALLOC_VFUNC_INDEX) = AllocFunc;
        }

        template<typename E = Expansion>
        enable_if_t<!is_void_v<E>, E &> getExpansion(Class *const obj) const {
            assert(this->mc != nullptr);
            return getMember<Expansion>(obj, this->mc->getSuperClass()->getClassSize());
        }

        Class *alloc() const {
            static_assert(Constructor != nullptr, "This function may only be called on non-abstract classes");
            assert(this->mc != nullptr);
            return reinterpret_cast<Class *>(this->mc->alloc());
        }

        void instanceConstructed() const {
            static_assert(Constructor != nullptr, "This function may only be called on non-abstract classes");
            assert(this->mc != nullptr);
            return this->mc->instanceConstructed();
        }
    };

};    // namespace PenguinWizardry
