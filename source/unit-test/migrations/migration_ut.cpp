#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <boost/shared_ptr.hpp>
#include <unistd.h>
#include <thread>
#include <functional>
#include <iostream>

#include <fds_assert.h>
#include <net/net_utils.h>
#include <concurrency/fds_actor.h>
#include <fdsp/FDSP_types.h>
#include <fds_base_migrators.h>
#include <fds_migration.h>
#include <util/Log.h>
#include <NetSession.h>
#include <StorMgrVolumes.h>
#include<ObjectStoreMock.h>

/**
 * In this mock version ClusterCommMgr node table just contains
 * sender information.  We just require the sender config here.
 */
class MockClusterCommMgr : public ClusterCommMgr {
  public:
    MockClusterCommMgr(FdsConfigAccessor sender_config, DLT* dlt)
    : ClusterCommMgr(nullptr),
      sender_config_(sender_config)
    {
        dlt_ = dlt;
    }
    virtual ~MockClusterCommMgr() {}
    virtual NodeTokenTbl
    partition_tokens_by_node(const std::set<fds_token_id> &tokens) override
    {
        /* Some dummy id */
        NodeUuid id(1);
        NodeTokenTbl tbl;
        tbl[id] = tokens;
        return tbl;
    }

    virtual bool
    get_node_mig_ip_port(const fds_token_id &token_id, uint32_t &ip, uint32_t &port) override
    {
        std::string str_ip = sender_config_.get<std::string>("ip");
        ip = netSessionTbl::ipString2Addr(str_ip);
        port = sender_config_.get<int>("port");
        return true;
    }

    TVIRTUAL const DLT* get_dlt()
    {
        return dlt_;
    }
private:
    FdsConfigAccessor sender_config_;
    DLT* dlt_;
};

/**
 * Mock migration service
 */
class MockMigrationSvc : public FdsMigrationSvc {
public:
    MockMigrationSvc(SmIoReqHandler *data_store,
            const FdsConfigAccessor &conf_helper,
            fds_log *log,
            netSessionTblPtr nst,
            ClusterCommMgrPtr clust_comm_mgr,
            kvstore::TokenStateDBPtr tokenStateDb)
    : FdsMigrationSvc(data_store,
            conf_helper,
            std::string(""),
            log,
            nst,
            clust_comm_mgr,
            tokenStateDb)
    {
    }
    TVIRTUAL int get_port()
    {
        return conf_helper_.get<int>("port");
    }
protected:
    virtual void setup_migpath_server()
    {
        migpath_handler_.reset(new FDSP_MigrationPathRpc(*this, GetLog()));

        std::string myIp = net::get_local_ip("eth0");
        int port = conf_helper_.get<int>("port");
        int myIpInt = netSession::ipString2Addr(myIp);
        // TODO(rao): Do not hard code.  Get from config
        std::string node_name = "localhost-mig";
        migpath_session_ = nst_->createServerSession<netMigrationPathServerSession>(
                myIpInt,
                port,
                node_name,
                FDSP_MIGRATION_MGR,
                migpath_handler_);

        LOGNORMAL << "Migration path server setup ip: "
                << myIp << " port: " << port;
    }
};

class MigrationTester : public FdsProcess
{
public:
    MigrationTester(int argc, char *argv[],
                    const std::string &def_cfg_file,
                    const std::string &base_path,
                    const std::string &def_log_file,  Module **mod_vec) :
        FdsProcess(argc, argv, def_cfg_file, base_path, def_log_file, mod_vec),
        oid_((uint32_t)0)
    {
    }
    virtual ~MigrationTester() {
        delete log_;
    }
    // From FdsProcess
    int run() override { return 0; }

    void init()
    {
        log_ = g_fdslog;
        log_->setSeverityFilter(fds::fds_log::debug);

        nst_ = boost::shared_ptr<netSessionTbl>(new netSessionTbl(FDSP_STOR_MGR));

        sender_store_.reset(create_mock_obj_store("SenderMockObjstor"));
        rcvr_store_.reset(create_mock_obj_store("RcvrMockObjstor"));

        FdsConfigAccessor sender_config(
                FdsConfigPtr(new FdsConfig("sender.conf", 0, NULL)),
                "fds.migration.");
        FdsConfigAccessor rcvr_config(
                FdsConfigPtr(new FdsConfig("receiver.conf", 0, NULL)),
                "fds.migration.");

        clust_comm_mgr_.reset(new MockClusterCommMgr(sender_config,
                const_cast<DLT*>(sender_store_->getDLT())));

        sender_mig_svc_.reset(create_mig_svc(sender_config, clust_comm_mgr_, sender_store_.get()));
        rcvr_mig_svc_.reset(create_mig_svc(rcvr_config, clust_comm_mgr_, rcvr_store_.get()));

        migration_complete_ = false;
    }

    /*
     * For incrmenting object id
     * For now only work 256 times
     */
    void incr_obj_id() {
        char* data = (char*) oid_.GetId();
        data[19]++;
    }

    ObjectID get_obj_id(const fds_token_id &tok_id) {

        ObjectID start;
        ObjectID end;
        ObjectID::getTokenRange(tok_id, 8, start, end);
        char *data = (char*) start.GetId();
        data[start.GetLen()-1] = rand() % 255;
        return start;
    }

    /**
     * Mock object store for reading and writing token data (ex: simulates ObjectStore)
     * @return
     */
    MObjStore* create_mock_obj_store(const std::string &prefix)
    {
        return new MObjStore(prefix);
    }

    FdsMigrationSvc* create_mig_svc(FdsConfigAccessor config_helper,
            ClusterCommMgrPtr clust_comm_mgr_, MObjStore *obj_store)
    {
        return new MockMigrationSvc(obj_store,
                config_helper,
                log_,
                nst_,
                clust_comm_mgr_,
                obj_store->getTokenStateDb());
    }

    void mig_svc_cb(const Error& e, const MigrationStatus& mig_status)
    {
        if (mig_status == MigrationStatus::MIGRATION_OP_COMPLETE) {
            /* Give 3s for sender to cleanup */
            sleep(3);
            migration_complete_ = true;
        } else if (mig_status == MigrationStatus::TOKEN_COPY_COMPLETE) {
            /* Put some sync objects */
            for (int tok_id = 0; tok_id < 20; tok_id++) {
                for (int i = 0; i < 30; i++) {
                    std::ostringstream oss;
                    oss << "sync obj " << i;
                    sender_store_->putObject(get_obj_id(tok_id), oss.str());
                }
            }

            /* Send io close event */
            MigSvcSyncCloseReqPtr close_req(new MigSvcSyncCloseReq());
            close_req->sync_close_ts = get_fds_timestamp_ms();

            FdsActorRequestPtr close_far(new FdsActorRequest(
                    FAR_ID(MigSvcSyncCloseReq), close_req));

            sender_mig_svc_->send_actor_request(close_far);
        }
    }

    void test1()
    {
        /* Start sender and receiver migration services */
        sender_mig_svc_->mod_startup();
        rcvr_mig_svc_->mod_startup();

        sleep(5);

        /* Put some data in sender object store */
        for (int tok_id = 0; tok_id < 20; tok_id++) {
            for (int i = 0; i < 30; i++) {
                std::ostringstream oss;
                oss << "copy obj " << i;
                sender_store_->putObject(get_obj_id(tok_id), oss.str());
            }
        }

        /* Issue a request to receiver migrations service to copy the tokens that we
         * just put into sender store.
         */
        MigSvcBulkCopyTokensReqPtr copy_req(new MigSvcBulkCopyTokensReq());
        copy_req->tokens = sender_store_->getTokens();
        copy_req->migsvc_resp_cb = std::bind(
            &MigrationTester::mig_svc_cb, this,
            std::placeholders::_1, std::placeholders::_2);

        LOGDEBUG << "Tokens count: " << copy_req->tokens.size();

        FdsActorRequestPtr copy_far(new FdsActorRequest(FAR_ID(MigSvcBulkCopyTokensReq), copy_req));
        rcvr_mig_svc_->send_actor_request(copy_far);

        std::cout << "Tokens count: " << copy_req->tokens.size() << std::endl;

        /* Wait for migration to complete */
        int slept_time = 0;
        while (!migration_complete_ && slept_time < 20) {
            sleep(1);
            slept_time++;
        }

        fds_verify(migration_complete_ == true);
        // fds_verify(*sender_store_ == *rcvr_store_);

        std::cout << "Test completed" << std::endl;

        sender_mig_svc_->mod_shutdown();
        rcvr_mig_svc_->mod_shutdown();
        /* Wait a while for shutdown */
        sleep(2);

        /* For some reason thrift isn't shutting down the server */
        exit(0);
    }

    fds_log *log_;
    netSessionTblPtr nst_;
    MObjStorePtr sender_store_;
    MObjStorePtr rcvr_store_;
    FdsMigrationSvcPtr sender_mig_svc_;
    FdsMigrationSvcPtr rcvr_mig_svc_;
    ClusterCommMgrPtr clust_comm_mgr_;
    bool migration_complete_;
    ObjectID oid_;
};

int main(int argc, char *argv[]) {

    srand (time(NULL));

    fds::Module *smVec[] = {
        NULL
    };
    MigrationTester t(argc, argv, "migration_ut.conf", "", "temp.log", smVec);
    t.init();
    t.test1();
    return 0;
}
