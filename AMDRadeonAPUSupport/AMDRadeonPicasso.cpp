#include "AMDRadeonPicasso.hpp"

OSDefineMetaClassAndStructors(AMDRadeonPicasso, IOService);

void AMDRadeonPicasso::free() {
    super::free();

    IOLog("AMDRadeonPicasso: free");
}

bool AMDRadeonPicasso::init(OSDictionary *properties) {
    if (!super::init(properties)) return false;

    IOLog("AMDRadeonPicasso: init");

    return true;
}

bool AMDRadeonPicasso::start(IOService *provider) {
    if (!super::start(provider)) return false;

    IOLog("AMDRadeonPicasso: start");

    return true;
}
