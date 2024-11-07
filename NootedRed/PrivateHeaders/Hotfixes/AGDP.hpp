#include <Headers/kern_patcher.hpp>

namespace Hotfixes {
    class AGDP {
        public:
        static AGDP &singleton();

        void init();

        private:
        void processKext(KernelPatcher &patcher, size_t id, mach_vm_address_t slide, size_t size);
    };
};    // namespace Hotfixes
