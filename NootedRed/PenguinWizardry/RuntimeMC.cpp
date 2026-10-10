// Kext MetaClass Injector
//
// Copyright (C) 2025-2026  ChefKiss.
//
// Licensed under the ChefKiss Open Knowledge License version 1.
//
// This work is licensed to you under the terms of the ChefKiss Open Knowledge
// License, either version 1 of the License, or (at your option) any later
// version published by ChefKiss.
//
// This work may not be used to train or develop artificial intelligence,
// and may not be used with artificial intelligence in any other way. See
// the ChefKiss Open Knowledge License for details.
//
// This work is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// ChefKiss Open Knowledge License for more details.
//
// You should have received a copy of the ChefKiss Open Knowledge License
// along with this work.  If not, see https://git.chefkiss.dev/ChefKiss/OpenKnowledge/raw/branch/master/LICENSE-v1.md.

#include <Headers/kern_patcher.hpp>
#include <Headers/kern_util.hpp>
#include <PenguinWizardry/RuntimeMC.hpp>
#include <libkern/OSReturn.h>
#include <libkern/c++/OSArray.h>
#include <libkern/c++/OSCollectionIterator.h>
#include <libkern/c++/OSMetaClass.h>
#include <mach/i386/vm_types.h>

PenguinWizardry::RuntimeMCBase::~RuntimeMCBase() { MetaClassDestructor(this->mc); }

static PenguinWizardry::RuntimeMCManager moduleInstance;

PenguinWizardry::RuntimeMCManager& PenguinWizardry::RuntimeMCManager::singleton() { return moduleInstance; }

static RuntimeVFTBase mcVft;

void PenguinWizardry::RuntimeMCManager::processPatcher(KernelPatcher& patcher)
{
    KernelPatcher::RouteRequest request{"__ZN11OSMetaClass11postModLoadEPv", wrapPostModLoad, this->orgPostModLoad};
    PANIC_COND(!patcher.routeMultiple(KernelPatcher::KernelID, &request, 1), "RuntimeMC",
               "Failed to route `postModLoad`");
    mcVft.init(MetaClassVT + 2);
}

void PenguinWizardry::RuntimeMCManager::registerPending(Pending* pending)
{
    assert(pending != nullptr);
    assert(pending->mc != nullptr);
    DBGLOG("RuntimeMC", "Registering `%s` meta class", pending->mc->getClassName());

    auto* mc = pending->mc->getMetaClass();
    assert(mc != nullptr);
    assert(pending->super != nullptr);
    MetaClassConstructor(mc, pending->mc->getClassName(), pending->super,
                         pending->super->getClassSize() + pending->mc->getExpansionSize());
    pending->mc->vft.replaceVFT(mc);
}

void PenguinWizardry::RuntimeMCManager::registerMC(RuntimeMCBase& rtMC, const char* const kext,
                                                   const OSMetaClass* const super)
{
    rtMC.vft.init(mcVft);
    rtMC.populateVFT();
    auto* pending = new Pending;
    assert(pending != nullptr);
    pending->mc          = &rtMC;
    pending->super       = super;
    auto* pendingWrapper = OSObjectWrapper::with(pending);
    assert(pendingWrapper != nullptr);
    auto* array = OSDynamicCast(OSArray, this->pendingMCsForKext->getObject(kext));
    if (array == nullptr) {
        array = OSArray::withCapacity(1);
        this->pendingMCsForKext->setObject(kext, array);
        array->release();
    }
    array->setObject(pendingWrapper);
    pendingWrapper->release();
}

void PenguinWizardry::RuntimeMCManager::registerMC(PenguinWizardry::RuntimeMCBase& rtMC, const char* const kext,
                                                   KernelPatcher& patcher, const size_t id, const char* const symbol,
                                                   const mach_vm_address_t start, const size_t size)
{
    const auto* superclass = patcher.solveSymbol<const OSMetaClass*>(id, symbol, start, size, true);
    PANIC_COND(superclass == nullptr, "RuntimeMC", "Failed to resolve `%s`", symbol);
    DBGLOG("RuntimeMC", "`%s` resolved from `%s`", rtMC.getClassName(), symbol);
    this->registerMC(rtMC, kext, superclass);
}

void PenguinWizardry::RuntimeMCManager::registerMC(RuntimeMCBase& rtMC, const char* kext, RuntimeMCBase& rtSuper)
{
    assert(rtSuper.getMetaClass() != nullptr);
    this->registerMC(rtMC, kext, rtSuper.getMetaClass());
}

OSReturn PenguinWizardry::RuntimeMCManager::wrapPostModLoad(void* loadHandle)
{
    const auto* kextIdentifier = getMember<const char*>(loadHandle, 0);
    auto*       array          = OSDynamicCast(OSArray, singleton().pendingMCsForKext->getObject(kextIdentifier));
    if (array != nullptr) {
        auto*            iter = OSCollectionIterator::withCollection(array);
        OSObjectWrapper* obj;
        while ((obj = OSDynamicCast(OSObjectWrapper, iter->getNextObject()))) {
            auto* pending = obj->get<Pending>();
            singleton().registerPending(pending);
            delete pending;
        }
        iter->release();
        singleton().pendingMCsForKext->removeObject(kextIdentifier);
    }
    return FunctionCast(wrapPostModLoad, singleton().orgPostModLoad)(loadHandle);
}
