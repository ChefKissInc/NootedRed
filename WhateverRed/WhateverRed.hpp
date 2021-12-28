#pragma once
#include <Headers/kern_api.hpp>

class WER {
public:
    void init();
    void deinit();

private:
    void processKernel(KernelPatcher &patcher);
    void processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size);
};
