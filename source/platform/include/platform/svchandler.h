
#ifndef SOURCE_PLATFORM_INCLUDE_PLATFORM_SVCHANDLER_H_
#define SOURCE_PLATFORM_INCLUDE_PLATFORM_SVCHANDLER_H_
#include <net/PlatNetSvcHandler.h>
namespace fds {
namespace pm {

class PlatformManager;

class SvcHandler : public PlatNetSvcHandler {
  public:
    SvcHandler(CommonModuleProviderIf *provider, PlatformManager *platform);
    void activateServices(boost::shared_ptr <fpi::AsyncHdr> &hdr,
            boost::shared_ptr <fpi::ActivateServicesMsg> &activateMsg);
  protected:
    PlatformManager *platform;
};
}  // namespace pm
}  // namespace fds

#endif // SOURCE_PLATFORM_INCLUDE_PLATFORM_SVCHANDLER_H_
