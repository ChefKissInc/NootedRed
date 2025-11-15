// Copyright © 2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#include <Headers/kern_api.hpp>
#include <PenguinWizardry/RuntimeMC.hpp>

static PenguinWizardry::RuntimeMCManager instance {};

PenguinWizardry::RuntimeMCManager &PenguinWizardry::RuntimeMCManager::singleton() { return instance; }

void PenguinWizardry::RuntimeMCManager::processPatcher(KernelPatcher &patcher) {
    this->orgMetaClassConstructor = reinterpret_cast<decltype(this->orgMetaClassConstructor)>(
        patcher.solveSymbol(KernelPatcher::KernelID, "__ZN11OSMetaClassC2EPKcPKS_j"));
    PANIC_COND(this->orgMetaClassConstructor == nullptr, "RuntimeMC", "Failed to solve `OSMetaClass` constructor");
    this->orgMetaClassVFuncs = reinterpret_cast<decltype(this->orgMetaClassVFuncs)>(
        patcher.solveSymbol(KernelPatcher::KernelID, "__ZTV11OSMetaClass"));
    PANIC_COND(this->orgMetaClassVFuncs == nullptr, "RuntimeMC", "Failed to solve `OSMetaClass` vtable");
    this->orgMetaClassVFuncs += 2;    // 2 ptrs into VFT is VFuncs
    KernelPatcher::RouteRequest request {"__ZN11OSMetaClass11postModLoadEPv", wrapPostModLoad, this->orgPostModLoad};
    PANIC_COND(!patcher.routeMultiple(KernelPatcher::KernelID, &request, 1), "RuntimeMC",
        "Failed to route `postModLoad`");
}

void PenguinWizardry::RuntimeMCManager::registerPending(Pending *pending) {
    assert(this->orgMetaClassConstructor != nullptr);
    assert(pending != nullptr);
    assert(pending->mc != nullptr);
    DBGLOG("RuntimeMC", "Registering `%s` meta class", pending->mc->getClassName());

    auto *mc = pending->mc->getMetaClass();
    assert(mc != nullptr);
    assert(pending->super != nullptr);
    this->orgMetaClassConstructor(mc, pending->mc->getClassName(), pending->super,
        pending->super->getClassSize() + pending->mc->getExpansionSize());
    pending->mc->vft.replaceVFT(mc);
}

void PenguinWizardry::RuntimeMCManager::registerMC(RuntimeMCBase &rtMC, const char *const kext,
    const OSMetaClass *const super) {
    rtMC.vft.init(this->orgMetaClassVFuncs);
    rtMC.populateVFT();
    auto *pending = new Pending {};
    assert(pending != nullptr);
    pending->mc = &rtMC;
    pending->super = super;
    auto *pendingWrapper = OSObjectWrapper::with(pending);
    assert(pendingWrapper != nullptr);
    auto *array = OSRequiredCast(OSArray, this->pendingMCsForKext->getObject(kext));
    if (array == nullptr) {
        array = OSArray::withCapacity(1);
        this->pendingMCsForKext->setObject(kext, array);
        array->release();
    }
    array->setObject(pendingWrapper);
    pendingWrapper->release();
}

void PenguinWizardry::RuntimeMCManager::registerMC(PenguinWizardry::RuntimeMCBase &rtMC, const char *const kext,
    KernelPatcher &patcher, const size_t id, const char *const symbol, const mach_vm_address_t start,
    const size_t size) {
    const auto *superclass = patcher.solveSymbol<const OSMetaClass *>(id, symbol, start, size, true);
    PANIC_COND(superclass == nullptr, "RuntimeMC", "Failed to resolve `%s`", symbol);
    DBGLOG("RuntimeMC", "`%s` resolved from `%s`", rtMC.getClassName(), symbol);
    this->registerMC(rtMC, kext, superclass);
}

void PenguinWizardry::RuntimeMCManager::registerMC(RuntimeMCBase &rtMC, const char *kext, RuntimeMCBase &rtSuper) {
    assert(rtSuper.getMetaClass() != nullptr);
    this->registerMC(rtMC, kext, rtSuper.getMetaClass());
}

OSReturn PenguinWizardry::RuntimeMCManager::wrapPostModLoad(void *loadHandle) {
    const auto *kextIdentifier = getMember<const char *>(loadHandle, 0);
    auto *array = OSRequiredCast(OSArray, singleton().pendingMCsForKext->getObject(kextIdentifier));
    if (array != nullptr) {
        auto *iter = OSCollectionIterator::withCollection(array);
        OSObjectWrapper *obj;
        while ((obj = OSRequiredCast(OSObjectWrapper, iter->getNextObject()))) {
            auto *pending = obj->get<Pending>();
            singleton().registerPending(pending);
            delete pending;
        }
        iter->release();
        singleton().pendingMCsForKext->removeObject(kextIdentifier);
    }
    return FunctionCast(wrapPostModLoad, singleton().orgPostModLoad)(loadHandle);
}
