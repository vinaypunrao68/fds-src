#include <boost/shared_ptr.hpp>


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
        g_fdslog = log_.get();

        nst_ = boost::shared_ptr<netSessionTbl>(new netSessionTbl(FDSP_STOR_MGR));
        threadpool_.reset(new fds_threadpool());

        sender_store_.reset(create_mock_obj_store());
        rcvr_store_.reset(create_mock_obj_store());
        sender_mig_svc_.reset(create_mig_svc("sender.conf", sender_store_.get()));
        rcvr_mig_svc_.reset(create_mig_svc("receiver.conf", rcvr_store_.get()));
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
                FdsConfigAccessor(config, "migration"),
                log_,
                nst_);
    }

    fds_threadpoolPtr threadpool_;
    fds_logPtr log_;
    netSessionTblPtr nst_;
    MObjStorePtr sender_store_;
    MObjStorePtr rcvr_store_;
    FdsMigrationSvcPtr sender_mig_svc_;
    FdsMigrationSvcPtr rcvr_mig_svc_;
};
int main() {
    MigrationTester t;
    t.init();
    return 0;
}
