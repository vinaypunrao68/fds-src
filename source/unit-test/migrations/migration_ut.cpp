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

class MigrationTester
{
public:
    MigrationTester() {

    }
    void init()
    {
        log_.reset(new fds_log("migration_ut.log"));
        log_->setSeverityFilter(fds::fds_log::debug);
        g_fdslog = log_.get();

        nst_ = boost::shared_ptr<netSessionTbl>(new netSessionTbl(FDSP_STOR_MGR));
        threadpool_.reset(new fds_threadpool());

        sender_store_.reset(create_mock_obj_store());
        rcvr_store_.reset(create_mock_obj_store());
        sender_mig_svc_.reset(create_mig_svc("sender.conf", sender_store_.get()));
        rcvr_mig_svc_.reset(create_mig_svc("receiver.conf", rcvr_store_.get()));

        migration_complete_ = false;
    }

    MObjStore* create_mock_obj_store()
    {
        return new MObjStore();
    }

    FdsMigrationSvc* create_mig_svc(const std::string &conf_file, MObjStore *obj_store)
    {
        FdsConfigPtr config(new FdsConfig(conf_file, 0, NULL));
        return new FdsMigrationSvc(obj_store,
                threadpool_,
                FdsConfigAccessor(config, "fds.migration."),
                log_,
                nst_);
    }

    void mig_svc_cb(const Error& e)
    {
        migration_complete_ = true;
    }

    void test1()
    {
        /* Start sender and receiver migration services */
        std::thread t1(&FdsMigrationSvc::mod_startup, sender_mig_svc_.get());
        std::thread t2(&FdsMigrationSvc::mod_startup, rcvr_mig_svc_.get());

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

    fds_threadpoolPtr threadpool_;
    fds_logPtr log_;
    netSessionTblPtr nst_;
    MObjStorePtr sender_store_;
    MObjStorePtr rcvr_store_;
    FdsMigrationSvcPtr sender_mig_svc_;
    FdsMigrationSvcPtr rcvr_mig_svc_;
    bool migration_complete_;
};

int main() {
    MigrationTester t;
    t.init();
    t.test1();
    return 0;
}
