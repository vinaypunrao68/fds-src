#ifndef _FDS_MIGRATIONS_H
#define _FDS_MIGRATIONS_H

#include <string>
#include <memory>
#include <unordered_map>
#include <fds_module.h>
#include <fdsp/FDSP_MigrationPathReq.h>
#include <fdsp/FDSP_MigrationPathResp.h>
#include <concurrency/Mutex.h>
#include <util/Log.h>
#include <fds_config.hpp>
#include <concurrency/fds_actor.h>
#include <concurrency/fds_actor_request.h>
#include <NetSession.h>
#include <TokenCopySender.h>
#include <TokenCopyReceiver.h>

using namespace  ::FDS_ProtocolInterface;

namespace fds
{

/* Forward declarations */
class FDSP_MigrationPathRpc;

#if 0
class MigrationHandler {
public:
    /* Token migration state */
    enum State {
        NO_DATA,
        IDLE,
        MIGRATE_ISSUED,
        MIGRATING,
        MIGRATE_FINALIZE,
        IN_ERROR
    };

public:
    MigrationHandler(const fds_token_id &id)
    : id_(id) {
    }

    virtual ~MigrationHandler() {
    }

protected:
    fds_mutex lock_;
    fds_token_id id_;
    State state_;
};

class PutTokenHandler : public MigrationHandler {
public:
    PutTokenHandler(const fds_token_id &id)
    : MigrationHandler(id)
    {
    }

    void put_data();
};
boost::shared_ptr<PutTokenHandler> PutTokenHandlerPtr;

class GetTokenHandler : public MigrationHandler {
public:
    GetTokenHandler(const fds_token_id &id)
    : MigrationHandler(id)
    {
    }
protected:
};
boost::shared_ptr<GetTokenHandler> GetTokenHandlerPtr;

/**
 * Book keeping for token migrations
 */
template <class MigrationHandlerT>
class MigrationTable {
public:
    virtual ~MigrationTable() {
    }

    boost::shared_ptr<MigrationHandlerT> add_handler(const fds_token_id &id) {
        fds_mutex::scoped_lock l(lock_);
        auto itr = table_.find(id);
        if (itr != table_.end()) {
            return false;
        }

        boost::shared_ptr<MigrationHandlerT> handler(new MigrationHandlerT(id));
        table_[id] = handler;
        return handler;
    }

    boost::shared_ptr<MigrationHandlerT> add_handler(const fds_token_id &id,
            boost::shared_ptr<MigrationHandlerT> handler) {
        fds_mutex::scoped_lock l(lock_);
        auto itr = table_.find(id);
        if (itr != table_.end()) {
            return false;
        }
        table_[id] = handler;
        return handler;
    }

    bool remove_handler(const fds_token_id &id) {
        fds_mutex::scoped_lock l(lock_);
        auto itr = table_.find(id);
        if (itr == table_.end()) {
            return false;
        }
        table_.erase(id);
        return true;
    }

    boost::shared_ptr<MigrationHandlerT> get_handler(const fds_token_id &id) {
        fds_mutex::scoped_lock l(lock_);
        auto itr = table_.find(id);
        if (itr == table_.end()) {
            return nullptr;
        }
        return itr.second;
    }

private:
  /* Lock to protected tok_mig_tbl */
  fds_mutex lock_;
  /* Token migration table */
  std::unordered_map<fds_token_id, boost::shared_ptr<MigrationHandlerT> > table_;
};
#endif

/* Service for migrating objects */
class FdsMigrationSvc : public Module,
public FdsRequestQueueActor
{
public:
    FdsMigrationSvc(fds_threadpoolPtr threadpool,
            const FdsConfigAccessor &conf_helper,
            fds_logPtr log,
            netSessionTblPtr nst);

    fds_log* get_log() {return log_.get();}

    virtual std::string log_string() {
        return "FdsMigrationSvc";
    }

    /* Overrides from Module */
    virtual void mod_startup() override ;
    virtual void mod_shutdown() override ;


    /* Overrides from FdsRequestQueueActor */
    virtual Error handle_actor_request(FdsActorRequestPtr req) override;

private:
    void setup_migpath_server();
    netMigrationPathClientSession* ack_copy_token_req(FdsActorRequestPtr req);

    inline boost::shared_ptr<FDSP_MigrationPathRespClient>
    migpath_resp_client(const std::string session_uuid) {
        return migpath_session_->getRespClient(session_uuid);
    }

    /* Config access helper */
    FdsConfigAccessor conf_helper_;

    /* logger */
    fds_logPtr log_;

    /* Net session table */
    netSessionTblPtr nst_;
    boost::shared_ptr<FDSP_MigrationPathRpc> migpath_handler_;
    netMigrationPathServerSession *migpath_session_;

    // TODO (rao):  We should maintain a table of in progress migrations
    std::unique_ptr<TokenCopySender> tok_sender_;
    std::unique_ptr<TokenCopySender> tok_receiver_;
}; 

class FDSP_MigrationPathRpc : virtual public FDSP_MigrationPathReqIf ,
                              virtual public FDSP_MigrationPathRespIf
{
public:
    FDSP_MigrationPathRpc(FdsMigrationSvc &mig_svc, fds_logPtr log);

    fds_log* get_log() {
        return log_.get();
    }
    std::string log_string() {
        return "FDSP_MigrationPathRpc";
    }

    void CopyToken(const FDSP_CopyTokenReq& migrate_req) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }

    void CopyToken(boost::shared_ptr<FDSP_CopyTokenReq>& migrate_req);

    void PushTokenObjects(const FDSP_PushTokenObjectsReq& mig_put_req) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }

    void PushTokenObjects(boost::shared_ptr<FDSP_PushTokenObjectsReq>& mig_put_req);

    void CopyTokenResp(const FDSP_MsgHdrType& fdsp_msg) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }

    void CopyTokenResp(boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg);

    void PushTokenObjectsResp(const FDSP_MsgHdrType& fdsp_msg) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }

    void PushTokenObjectsResp(boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg);

protected:
    FdsMigrationSvc &mig_svc_;
    fds_logPtr log_;
};
}  // namespace fds

#endif
