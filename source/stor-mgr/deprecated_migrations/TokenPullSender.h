/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#ifndef TOKENPULLSENDER_H_
#define TOKENPULLSENDER_H_

#include <string>
#include <list>
#include <memory>
#include <boost/msm/back/state_machine.hpp>

#include <fds_assert.h>
#include <concurrency/fds_actor.h>
#include <fdsp/FDSP_types.h>
#include <fds_base_migrators.h>
#include <util/Log.h>
// #include <NetSession.h>
#include <ClusterCommMgr.h>
#include <StorMgrVolumes.h>
#include <TokenSyncBaseEvents.h>


namespace fds {

using namespace  ::FDS_ProtocolInterface;

/* Forward declarations */
class TokenSender;
struct TokenPullSenderFSM_;
typedef boost::msm::back::state_machine<TokenPullSenderFSM_> TokenPullSenderFSM;

/* Statemachine Events */
typedef FDSP_PullObjectsReq TSPullReqEvt;

typedef SmIoReadObjectdata TSDataReadEvt;
typedef boost::shared_ptr<TSDataReadEvt> TSDataReadEvtPtr;

typedef FDSP_NotifyTokenPullComplete TSPullDnEvt;

struct TokenPullSender {
public:
    TokenPullSender();
    virtual ~TokenPullSender();

    void init(const std::string &mig_stream_id,
            FdsMigrationSvc *migrationSvc,
            TokenSender *parent,
            SmIoReqHandler *data_store,
            const std::string &rcvr_ip,
            const int &rcvr_port,
            const fds_token_id &token_id,
            netMigrationPathClientSession *rcvr_session,
            boost::shared_ptr<FDSP_MigrationPathRespIf> client_resp_handler);
    void start();
    void process_event(const TSPullReqEvt& event);
    void process_event(const TSDataReadEvt& event);
    void process_event(const TSPullDnEvt& event);

 private:
    TokenPullSenderFSM *fsm_;
};
}  // namespace fds

#endif  /* TOKENPULLSENDER_H_*/
