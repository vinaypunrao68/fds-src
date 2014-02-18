#ifndef _FDS_MIGRATIONS_H
#define _FDS_MIGRATIONS_H

#include <string>
#include <memory>
#include <unordered_map>
#include <set>
#include <fds_module.h>
#include <fdsp/FDSP_MigrationPathReq.h>
#include <fdsp/FDSP_MigrationPathResp.h>
#include <concurrency/Mutex.h>
#include <util/Log.h>
#include <fds_config.hpp>
#include <fds_counters.h>
#include <fds_process.h>
#include <concurrency/fds_actor.h>
#include <concurrency/fds_actor_request.h>
#include <NetSession.h>
#include <ClusterCommMgr.h>
#include <TokenCopySender.h>
#include <TokenCopyReceiver.h>

using namespace  ::FDS_ProtocolInterface;

namespace fds
{

/* Forward declarations */
class FDSP_MigrationPathRpc;

typedef std::function<void (const Error&)> MigSvcCbType;

/**
 * Send this message to FdsMigrationSvc for copying tokens
 */
class MigSvcCopyTokensReq
{
public:
    /* In: Tokens to copy */
    std::set<fds_token_id> tokens;
    /* In: Callback to invoke after completion */
    MigSvcCbType migsvc_resp_cb;
};
typedef boost::shared_ptr<MigSvcCopyTokensReq> MigSvcCopyTokensReqPtr;

/* Migration service counters */
class MigrationCounters : public FdsCounters
{
 public:
  MigrationCounters(const std::string &id, FdsCountersMgr *mgr)
      : FdsCounters(id, mgr),
        tokens_sent("tokens_sent", this),
        tokens_rcvd("tokens_rcvd", this)
  {
  }

  NumericCounter tokens_sent; 
  NumericCounter tokens_rcvd;
};

/* Service for migrating objects */
class FdsMigrationSvc : public Module, public FdsRequestQueueActor
{
public:
    /* For tracking migrators */
    class MigratorInfo {
    public:
        FdsActorUPtr migrator;
        /* Callback to invoke after migrator completion */
        MigSvcCbType migsvc_resp_cb;
    };

public:
    FdsMigrationSvc(SmIoReqHandler *data_store,
            const FdsConfigAccessor &conf_helper,
            fds_log *log,
            netSessionTblPtr nst,
            ClusterCommMgrPtr clust_comm_mgr);

    fds_log* GetLog() {return log_;}

    virtual std::string log_string() {
        return "FdsMigrationSvc";
    }

    /* Overrides from Module */
    virtual void mod_startup() override ;
    virtual void mod_shutdown() override ;


    /* Overrides from FdsRequestQueueActor */
    virtual Error handle_actor_request(FdsActorRequestPtr req) override;

    netMigrationPathClientSession*
    get_migration_client(const std::string &ip, const int &port);

    boost::shared_ptr<FDSP_MigrationPathRespClient>
    get_resp_client(const std::string &session_uuid);

    std::string get_ip();
    int get_port();

public:
    /* Migration counters */
    MigrationCounters mig_cntrs;

private:
    void route_to_mig_actor(const std::string &mig_id, FdsActorRequestPtr req);
    void handle_migsvc_copy_token(FdsActorRequestPtr req);
    void handle_migsvc_copy_token_rpc(FdsActorRequestPtr req);
    void handle_migsvc_migration_complete(FdsActorRequestPtr req);
    void setup_migpath_server();
    Error ack_copy_token_req(FdsActorRequestPtr req);

    inline boost::shared_ptr<FDSP_MigrationPathRespClient>
    migpath_resp_client(const std::string session_uuid) {
        return migpath_session_->getRespClient(session_uuid);
    }

    /* Token data store */
    SmIoReqHandler *data_store_;

    /* Config access helper */
    FdsConfigAccessor conf_helper_;

    /* logger */
    fds_log *log_;

    /* Net session table */
    netSessionTblPtr nst_;
    boost::shared_ptr<FDSP_MigrationPathRpc> migpath_handler_;
    netMigrationPathServerSession *migpath_session_;

    /* Communication manager */
    ClusterCommMgrPtr clust_comm_mgr_;

    /* Migrations that are in progress.  Keyed by migration id */
    std::unordered_map<std::string, MigratorInfo> mig_actors_;
}; 
typedef boost::shared_ptr<FdsMigrationSvc> FdsMigrationSvcPtr;

class FDSP_MigrationPathRpc : virtual public FDSP_MigrationPathReqIf ,
                              virtual public FDSP_MigrationPathRespIf
{
public:
    FDSP_MigrationPathRpc(FdsMigrationSvc &mig_svc, fds_log *log);

    fds_log* GetLog() {
        return log_;
    }
    std::string log_string() {
        return "FDSP_MigrationPathRpc";
    }

    void CopyToken(const FDSP_CopyTokenReq& migrate_req) {
        fds_assert(!"Invalid");
    }
    void PushTokenObjects(const FDSP_PushTokenObjectsReq& mig_put_req) {
        fds_assert(!"Invalid");
    }
    void CopyToken(boost::shared_ptr<FDSP_CopyTokenReq>& migrate_req);
    void PushTokenObjects(boost::shared_ptr<FDSP_PushTokenObjectsReq>& mig_put_req);


    void CopyTokenResp(const FDSP_CopyTokenResp& copytok_resp) {
        fds_assert(!"Invalid");
    }
    void PushTokenObjectsResp(const FDSP_PushTokenObjectsResp& pushtok_resp) {
        fds_assert(!"Invalid");
    }
    void CopyTokenResp(boost::shared_ptr<FDSP_CopyTokenResp>& copytok_resp);
    void PushTokenObjectsResp(boost::shared_ptr<FDSP_PushTokenObjectsResp>& pushtok_resp);

protected:
    FdsMigrationSvc &mig_svc_;
    fds_log *log_;
};
}  // namespace fds

#endif
