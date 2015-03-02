/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <platform/svchandler.h>

namespace fds {
namespace pm {
SvcHandler::SvcHandler(CommonModuleProviderIf *provider) : PlatNetSvcHandler(provider) {
}
}  // namespace pm
}  // namespace fds
