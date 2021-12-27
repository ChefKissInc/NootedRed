#ifndef kern_wer_hpp
#define kern_wer_hpp

#include <Headers/kern_api.hpp>

class WhateverRed {
public:
    void init();

private:
    void processKernel(KernelPatcher &patcher);
    void processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size);
};

#endif /* kern_wer_hpp */
