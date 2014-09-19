#ifndef _FDS_MIGRATIONS_H
#define _FDS_MIGRATIONS_H

#include <string>
#include <memory>
#include <unordered_map>
#include <set>
#include <list>
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
#include <ClusterCommMgr.h>
#include <TokenSender.h>
#include <TokenReceiver.h>
#include <kvstore/tokenstatedb.h>

using namespace  ::FDS_ProtocolInterface;

namespace FDS_ProtocolInterface {
    class FDSP_MigrationPathReqProcessor;
    class FDSP_MigrationPathReqIf;
    class FDSP_MigrationPathRespClient;
}
typedef netServerSessionEx<FDSP_MigrationPathReqProcessor,
                FDSP_MigrationPathReqIf,
                FDSP_MigrationPathRespClient> netMigrationPathServerSession;

namespace fds
{

/* Forward declarations */
class FDSP_MigrationPathRpc;
class FdsMigrationSvc;

/**
 * Migration status
 */
enum MigrationStatus {
    /* Copy is complete */
    TOKEN_COPY_COMPLETE,
    /* Entire migration operation is complete */
    MIGRATION_OP_COMPLETE
};

typedef std::function<void (const Error&,
        const MigrationStatus& mig_status)> MigSvcCbType;

/**
 * Send this message to FdsMigrationSvc for bulk copying tokens
 * NOTE: This is temporary.  It should go away
 */
class MigSvcBulkCopyTokensReq
{
public:
    /* In: Tokens to copy */
    std::set<fds_token_id> tokens;
    /* In: Callback to invoke after completion */
    MigSvcCbType migsvc_resp_cb;
};
typedef boost::shared_ptr<MigSvcBulkCopyTokensReq> MigSvcBulkCopyTokensReqPtr;

/**
 * Send this message to FdsMigrationSvc for copying tokens
 */
class MigSvcCopyTokensReq
{
public:
    /* In: Token to copy */
    fds_token_id token;
    /* In: Callback to invoke after completion */
    MigSvcCbType migsvc_resp_cb;
};
typedef boost::shared_ptr<MigSvcCopyTokensReq> MigSvcCopyTokensReqPtr;

/* Notification to close sync.  Once OM starts to do per token sync
 * this may change
 */
class MigSvcSyncCloseReq
{
public:
    uint64_t sync_close_ts;
};
typedef boost::shared_ptr<MigSvcSyncCloseReq> MigSvcSyncCloseReqPtr;

/* Migration service counters */
class MigrationCounters : public FdsCounters
{
 public:
    MigrationCounters(const std::string &id, FdsCountersMgr *mgr)
     : FdsCounters(id, mgr),
       tok_sent("tok_sent", this),
       tok_rcvd("tok_rcvd", this),
       tok_copy_sent("tok_copy_sent", this),
       tok_copy_rcvd("tok_copy_rcvd", this),
       tok_sync_md_sent("tok_sync_md_sent", this),
       tok_sync_md_rcvd("tok_sync_md_rcvd", this),
       tok_pull_obj_sent("tok_pull_obj_sent", this),
       tok_pull_obj_rcvd("tok_pull_obj_rcvd", this)
    {
    }

    /* Number of tokens sent */
    NumericCounter tok_sent;
    /* Number of tokens received */
    NumericCounter tok_rcvd;
    /* Number of token objects sent */
    NumericCounter tok_copy_sent;
    /* Number of token objects received */
    NumericCounter tok_copy_rcvd;
    /* Number of metadata entries sent */
    NumericCounter tok_sync_md_sent;
    /* Number of metadata entries received */
    NumericCounter tok_sync_md_rcvd;
    /* Number of pull objects sent */
    NumericCounter tok_pull_obj_sent;
    /* Number of pull objects received */
    NumericCounter tok_pull_obj_rcvd;
};

/**
 * Token copy tracker for copying multiple tokens.  This is temporary and should
 * go away once OM starts doing granular token migrations
 */
class TokenCopyTracker {
 public:
    TokenCopyTracker(FdsMigrationSvc *migrationSvc,
            std::set<fds_token_id> tokens, MigSvcCbType copy_cb);
    void start();
    void token_complete_cb(const Error& e,
            const MigrationStatus& mig_status);

 private:
    void issue_copy_req();
    void issue_sync_req();

    FdsMigrationSvc *migrationSvc_;
    /* Tokens to copy and sync */
    std::set<fds_token_id> tokens_;
    /* Migration stream ids */
    std::list<std::string> mig_ids_;
    /* next token to issue a copy request for */
    std::set<fds_token_id>::iterator cur_copy_itr_;
    /* Completed copy count */
    uint32_t copy_completed_cnt_;
    /* next token to issue a copy request for */
    std::list<std::string>::iterator cur_sync_itr_;
    /* Completed sync count */
    uint32_t sync_completed_cnt_;
    /* Callback to issue after token copy is done (not token sync) */
    MigSvcCbType copy_cb_;

    friend class FdsMigrationSvc;
};

/* Service for migrating objects */
class FdsMigrationSvc : public Module, public FdsRequestQueueActor, public HasLogger
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
            const std::string &meta_dir,
            fds_log *log,
            netSessionTblPtr nst,
            ClusterCommMgrPtr clust_comm_mgr,
            kvstore::TokenStateDBPtr tokenStateDb);

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

    ClusterCommMgrPtr get_cluster_comm_mgr();
    kvstore::TokenStateDBPtr getTokenStateDb();

    std::string get_ip();
    TVIRTUAL int get_port();

    std::string get_metadir_root();

public:
    /* Migration counters */
    MigrationCounters mig_cntrs;

protected:
    virtual void setup_migpath_server();
    void route_to_mig_actor(const std::string &mig_id, FdsActorRequestPtr req);
    void handle_migsvc_copy_token(FdsActorRequestPtr req);
    void handle_migsvc_copy_token_rpc(FdsActorRequestPtr req);
    void handle_migsvc_migration_complete(FdsActorRequestPtr req);
    Error ack_copy_token_req(FdsActorRequestPtr req);
    void handle_migsvc_sync_close(FdsActorRequestPtr req);

    boost::shared_ptr<FDSP_MigrationPathRespClient>
    migpath_resp_client(const std::string session_uuid);

    /* Token data store */
    SmIoReqHandler *data_store_;

    /* Config access helper */
    FdsConfigAccessor conf_helper_;

    /* Meta directory */
    std::string meta_dir_;

    /* Net session table */
    netSessionTblPtr nst_;
    boost::shared_ptr<FDSP_MigrationPathRpc> migpath_handler_;
    netMigrationPathServerSession *migpath_session_;

    /* Communication manager */
    ClusterCommMgrPtr clust_comm_mgr_;

    /* Token state db */
    kvstore::TokenStateDBPtr tokenStateDb_;

    /* Migrations that are in progress.  Keyed by migration id */
    std::unordered_map<std::string, MigratorInfo> mig_actors_;

    /* For tracking bulk token copy */
    std::unique_ptr<TokenCopyTracker> copy_tracker_;

    friend class TokenCopyTracker;
}; 
typedef boost::shared_ptr<FdsMigrationSvc> FdsMigrationSvcPtr;

class FDSP_MigrationPathRpc : virtual public FDSP_MigrationPathReqIf ,
        virtual public FDSP_MigrationPathRespIf, public HasLogger
{
public:
    FDSP_MigrationPathRpc(FdsMigrationSvc &mig_svc, fds_log *log);

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

    void SyncToken(const FDSP_SyncTokenReq& sync_req) {}
    void SyncToken(boost::shared_ptr<FDSP_SyncTokenReq>& sync_req);

    void SyncTokenResp(const FDSP_SyncTokenResp& synctok_resp) {}
    void SyncTokenResp(boost::shared_ptr<FDSP_SyncTokenResp>& synctok_resp);

    void PushTokenMetadata(boost::shared_ptr<FDSP_PushTokenMetadataReq>& push_md_req);
    void PushTokenMetadata(const FDSP_PushTokenMetadataReq& push_md_req) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }

    void NotifyTokenSyncComplete(boost::shared_ptr<FDSP_NotifyTokenSyncComplete>& sync_complete);
    void NotifyTokenSyncComplete(const FDSP_NotifyTokenSyncComplete& sync_complete) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }

    void PushTokenMetadataResp(boost::shared_ptr<FDSP_PushTokenMetadataResp>& push_md_resp);
    void PushTokenMetadataResp(const FDSP_PushTokenMetadataResp& push_md_resp) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }

    void PullObjects(boost::shared_ptr<FDSP_PullObjectsReq>& pull_req);
    void PullObjects(const FDSP_PullObjectsReq& pull_req) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }

    void PushObjects(boost::shared_ptr<FDSP_PushObjectsReq>& push_req);
    void PushObjects(const FDSP_PushObjectsReq& push_req) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }

    void NotifyTokenPullComplete(boost::shared_ptr<FDSP_NotifyTokenPullComplete>& pull_complete);
    void NotifyTokenPullComplete(const FDSP_NotifyTokenPullComplete& pull_complete) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }

 protected:
    FdsMigrationSvc &mig_svc_;
};
}  // namespace fds

#endif
