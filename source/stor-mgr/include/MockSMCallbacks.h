/* Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_STOR_MGR_INCLUDE_MOCKSMCALLBACKS_H_
#define SOURCE_STOR_MGR_INCLUDE_MOCKSMCALLBACKS_H_

#include <StorMgr.h>
#include <fdsp_utils.h>
#include <SMSvcHandler.h>

namespace fds {

struct MockSMCallbacks : SMSvcHandler {
    void mockGetCb(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr);
    void mockPutCb(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr);
};
}  // namespace fds

#endif  // SOURCE_STOR_MGR_INCLUDE_MOCKSMCALLBACKS_H_
