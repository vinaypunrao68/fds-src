

/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ORCH_MGR_INCLUDE_OMSVCHANDLER_H_
#define SOURCE_ORCH_MGR_INCLUDE_OMSVCHANDLER_H_

#include <string>
#include <fdsp/fds_service_types.h>
#include <net/PlatNetSvcHandler.h>
#include <fds_typedefs.h>
#include <fds_error.h>
#include <net/SvcRequestPool.h>

namespace fds {

class omSnapshotSvcHandler {
  public:
  /*
    explicit snapshotSvcHandler(void) {
    }
    virtual ~snapshotSvcHandler(void) {
    }
   */
    /*
     * message handler 
     */ 
     Error omSnapshotCreate(const fpi::CreateSnapshotMsgPtr &stSnapCreateTxMsg);
     void omSnapshotCreateResp(QuorumSvcRequest* svcReq,
                            const Error& error,
                            boost::shared_ptr<std::string> payload);
     Error omSnapshotDelete(fds_uint64_t snapshotId, fds_uint64_t volId);
     void omSnapshotdeleteResp(QuorumSvcRequest* svcReq,
                            const Error& error,
                            boost::shared_ptr<std::string> payload);
     Error omVolumeCloneCreate(const fpi::CreateVolumeCloneMsgPtr &stVolumeCloneTxMsg);
     void  omVolumeCloneCreateResp(QuorumSvcRequest* svcReq,
                            const Error& error,
                            boost::shared_ptr<std::string> payload);
   private:
};
}  // namespace fds
#endif  // SOURCE_ORCH_MGR_INCLUDE_OMSVCHANDLER_H_
