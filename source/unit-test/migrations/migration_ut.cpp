#include <boost/shared_ptr.hpp>
#include <unistd.h>
#include <thread>
#include <functional>
#include <iostream>

#include <fds_assert.h>
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
    MockClusterCommMgr(FdsConfigAccessor sender_config)
    : ClusterCommMgr(nullptr),
      sender_config_(sender_config)
    {
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
    get_node_mig_ip_port(const NodeUuid &node_id, uint32_t &ip, uint32_t &port) override
    {
        std::string str_ip = sender_config_.get<std::string>("ip");
        ip = netSessionTbl::ipString2Addr(str_ip);
        port = sender_config_.get<int>("port");
        return true;
    }
private:
    FdsConfigAccessor sender_config_;
};

class MigrationTester
{
public:
    MigrationTester() {

    }
    virtual ~MigrationTester() {
        delete log_;
    }

    void init()
    {
        log_ = new fds_log("migration_ut.log");
        log_->setSeverityFilter(fds::fds_log::debug);
        g_fdslog = log_;

        nst_ = boost::shared_ptr<netSessionTbl>(new netSessionTbl(FDSP_STOR_MGR));

        sender_store_.reset(create_mock_obj_store());
        rcvr_store_.reset(create_mock_obj_store());

        FdsConfigAccessor sender_config(
                FdsConfigPtr(new FdsConfig("sender.conf", 0, NULL)),
                "fds.migration.");
        FdsConfigAccessor rcvr_config(
                FdsConfigPtr(new FdsConfig("receiver.conf", 0, NULL)),
                "fds.migration.");

        clust_comm_mgr_.reset(new MockClusterCommMgr(sender_config));

        sender_mig_svc_.reset(create_mig_svc(sender_config, clust_comm_mgr_, sender_store_.get()));
        rcvr_mig_svc_.reset(create_mig_svc(rcvr_config, clust_comm_mgr_, rcvr_store_.get()));

        migration_complete_ = false;
    }

    /**
     * Mock object store for reading and writing token data (ex: simulates ObjectStore)
     * @return
     */
    MObjStore* create_mock_obj_store()
    {
        return new MObjStore();
    }

    FdsMigrationSvc* create_mig_svc(FdsConfigAccessor config_helper,
            ClusterCommMgrPtr clust_comm_mgr_, MObjStore *obj_store)
    {
        return new FdsMigrationSvc(obj_store,
                config_helper,
                log_,
                nst_,
                clust_comm_mgr_);
    }

    void mig_svc_cb(const Error& e)
    {
        migration_complete_ = true;
    }

    void test1()
    {
        /* Start sender and receiver migration services */
        sender_mig_svc_->mod_startup();
        rcvr_mig_svc_->mod_startup();

        sleep(5);

        /* Put some data in sender object store */
        for (int i = 0; i < 20; i++) {
            sender_store_->putObject("Hello world" + i);
        }

        /* Issue a request to receiver migrations service copy the tokens that we
         * just put into sender store.
         */
        MigSvcCopyTokensReqPtr copy_req(new MigSvcCopyTokensReq());
        copy_req->tokens = sender_store_->getTokens();
        copy_req->migsvc_resp_cb = std::bind(
            &MigrationTester::mig_svc_cb, this,std::placeholders::_1);
        FdsActorRequestPtr copy_far(new FdsActorRequest(FAR_ID(MigSvcCopyTokensReq), copy_req));
        rcvr_mig_svc_->send_actor_request(copy_far);

        std::cout << "Tokens count: " << copy_req->tokens.size() << std::endl;

        /* Wait for migration to complete */
        int slept_time = 0;
        while (!migration_complete_ && slept_time < 20) {
            sleep(1);
            slept_time++;
        }

        fds_verify(migration_complete_ == true);
        fds_verify(*sender_store_ == *rcvr_store_);

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
};

int main() {
    MigrationTester t;
    t.init();
    t.test1();
    return 0;
}
