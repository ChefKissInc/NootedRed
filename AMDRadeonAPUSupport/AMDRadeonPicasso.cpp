#include "AMDRadeonPicasso.hpp"

#define super IOService
OSDefineMetaClassAndStructors(AMDRadeonPicasso, IOService);

void AMDRadeonPicasso::free() {
    super::free();
}

bool AMDRadeonPicasso::init(OSDictionary *properties) {
    if (!super::init(properties)) return false;

    return true;
}

bool AMDRadeonPicasso::start(IOService *provider) {
    if (!super::start(provider)) return false;

    return true;
}
