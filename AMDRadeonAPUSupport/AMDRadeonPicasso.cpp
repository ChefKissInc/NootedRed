#include "AMDRadeonPicasso.hpp"

OSDefineMetaClassAndStructors(AMDRadeonPicasso, super);

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
