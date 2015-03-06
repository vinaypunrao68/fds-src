
#ifndef SOURCE_PLATFORM_INCLUDE_PLATFORM_SVCHANDLER_H_
#define SOURCE_PLATFORM_INCLUDE_PLATFORM_SVCHANDLER_H_
#include <net/PlatNetSvcHandler.h>
namespace fds {
namespace pm {
class SvcHandler : public PlatNetSvcHandler {
  public:
    explicit SvcHandler(CommonModuleProviderIf *provider);
};
}  // namespace pm
}  // namespace fds

#endif // SOURCE_PLATFORM_INCLUDE_PLATFORM_SVCHANDLER_H_
