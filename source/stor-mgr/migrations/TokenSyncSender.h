/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#ifndef TOKENSYNC_SENDER_H_
#define TOKENSYNC_SENDER_H_

#include <string>
#include <memory>
#include <boost/msm/back/state_machine.hpp>

#include <fds_assert.h>
#include <concurrency/fds_actor.h>
#include <fdsp/FDSP_types.h>
#include <fds_base_migrators.h>
#include <util/Log.h>
#include <NetSession.h>
#include <ClusterCommMgr.h>
#include <StorMgrVolumes.h>
#include <TokenSyncBaseEvents.h>


namespace fds {

using namespace  ::FDS_ProtocolInterface;

/* Forward declarations */
struct TokenSyncSenderFSM_;
typedef boost::msm::back::state_machine<TokenSyncSenderFSM_> TokenSyncSenderFSM;
class TokenCopySender;

/* State machine events */
/* Sync request event */
typedef FDSP_SyncTokenReq SyncReqEvt;
typedef FDSP_PushTokenMetadataResp SendDnEvt;

struct BldSyncLogDnEvt {};
struct IoClosedEvt {};
struct SyncDnAckEvt {};


struct TokenSyncSender {
public:
    TokenSyncSender();
    virtual ~TokenSyncSender();
    void init(const std::string &mig_stream_id,
            FdsMigrationSvc *migrationSvc,
            TokenCopySender *parent,
            SmIoReqHandler *data_store,
            const std::string &rcvr_ip,
            const int &rcvr_port,
            fds_token_id token_id,
            netMigrationPathClientSession *rcvr_session,
            boost::shared_ptr<FDSP_MigrationPathRespIf> client_resp_handler);
    void process_event(const SyncReqEvt& event);
    void process_event(const SendDnEvt& event);
private:
    TokenSyncSenderFSM *fsm_;

};
}  // namespace fds

#endif  /* TOKENSYNC_SENDER_H_*/
