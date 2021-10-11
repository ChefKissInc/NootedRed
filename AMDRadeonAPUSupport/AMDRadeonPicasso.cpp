#include "AMDRadeonPicasso.hpp"
#include <Availability.h>

#if !__ACIDANTHERA_MAC_SDK
#error Using wrong SDK, this project must be compiled using the Acidanthera MacKernelSDK
#endif

OSDefineMetaClassAndStructors(AMDRadeonPicasso, IOService);

void AMDRadeonPicasso::free() {
    super::free();

    IOLog("AMDRadeonPicasso: free");
}

bool AMDRadeonPicasso::init(OSDictionary *properties) {
    if (!super::init(properties)) return false;

    IOLog("AMDRadeonPicasso: init");

    while (1);

    return true;
}

bool AMDRadeonPicasso::start(IOService *provider) {
    if (!super::start(provider)) return false;

    IOLog("AMDRadeonPicasso: start");

    while (1);

    return true;
}
