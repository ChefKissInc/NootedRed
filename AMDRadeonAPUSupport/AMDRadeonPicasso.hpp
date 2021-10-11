#include <IOKit/IOService.h>
#include <IOKit/IOLib.h>

class AMDRadeonPicasso : public IOService {
    OSDeclareDefaultStructors(AMDRadeonPicasso);

    virtual void free() override;
public:
    virtual bool init(OSDictionary *properties) override;
    virtual bool start(IOService *provider) override;
    virtual void stop(IOService *provider) override;
};
