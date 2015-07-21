/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef SOURCE_UNIT_TEST_DATA_MGR_DM_MOCKS_H_
#define SOURCE_UNIT_TEST_DATA_MGR_DM_MOCKS_H_
#include <unistd.h>
#include <DataMgr.h>
#include <net/net-service.h>
#include <util/fds_stat.h>
#include <iostream>
#include <fds_module.h>
#include <fds_process.h>
#include <concurrency/Mutex.h>
#include <string>
#include <vector>

#include <net/SvcProcess.h>

static Module * modVec[] = {};

namespace fds {
// TODO(Rao): Get rid of this singleton
DataMgr *dataMgr = 0;

class MockDataMgr : public SvcProcess {
  public:
    MockDataMgr(int argc, char *argv[]) : SvcProcess(argc, argv, "platform.conf",
            "fds.dm.", "dm.log", modVec) {
    }

    virtual int run() override {
        return 0;
    }
};

static fds::Module *dmVec[] = {
    &fds::gl_fds_stat,
    NULL
};

struct DMTester :  SvcProcess {
    std::vector<boost::shared_ptr<VolumeDesc> > volumes;
    std::string TESTBLOB;
    fds_volid_t TESTVOLID;
    uint64_t txnId = 0;

    DMTester(int argc, char *argv[])
            : SvcProcess(argc,
                         argv,
                         "platform.conf",
                         "fds.dm.",
                         "dm.log",
                         nullptr) {
    }

    virtual ~DMTester() {
        delete dataMgr;
    }

    virtual void registerSvcProcess() override
    {
        /* Nothing to register.  Creaate fake dlt and dmt */
        DLT *dlt = new DLT(1, 1, 1, true);

        for (uint i = 0; i < dlt->getNumTokens(); i++) {
            for (uint j= 0; j < dlt->getDepth(); j++) {
                dlt->setNode(i, j, 12345678);
            }
        }
        getSvcMgr()->getDltManager()->add(*dlt);

        DMT *dmt = new DMT(1, 1, 1, true);

        for (uint i = 0; i < dmt->getNumColumns(); i++) {
            for (uint j= 0; j < dmt->getDepth(); j++) {
                dmt->setNode(i, j, 12345678);
            }
        }
        getSvcMgr()->getDmtManager()->add(dmt, DMT_COMMITTED);
    }

    void initDM() {
        std::cout << "initing dm" << std::endl;
        PerfTracer::setEnabled(false);
        dataMgr = new DataMgr(this);
        dataMgr->runMode = DataMgr::TEST_MODE;
        dataMgr->standalone = true;
        dataMgr->vol_map_mtx = new fds_mutex("Volume map mutex");
        dataMgr->features.setQosEnabled(false);
        dataMgr->features.setTestMode(true);
        dataMgr->features.setCatSyncEnabled(false);
        dataMgr->features.setTimelineEnabled(false);

        dataMgr->initHandlers();
        dataMgr->mod_enable_service();
    }

    std::string getBlobName(int num) {
        if (0 == num) {
            return TESTBLOB;
        }
        return TESTBLOB + std::to_string(num);
    }

    Error addVolume(uint num) {
        Error err(ERR_OK);
        std::cout << "adding volume: " << volumes[num]->volUUID
                  << ":" << volumes[num]->name
                  << ":" << dataMgr->getPrefix()
                  << std::endl;
        return dataMgr->_process_add_vol(dataMgr->getPrefix() +
                                        std::to_string(volumes[num]->volUUID.get()),
                                        volumes[num]->volUUID, volumes[num].get());
    }

    uint64_t getNextTxnId() {
        return ++txnId;
    }

    void start() {
        start_modules();
        initDM();
    }

    void stop() {
        shutdown_modules();
    }

    void clean() {
        const FdsRootDir* root = g_fdsprocess->proc_fdsroot();
        if (root) {
            std::ostringstream oss;
            oss << "rm -rf " << root->dir_user_repo_dm() << "*";
            int rc = system(oss.str().c_str());

            oss.str("");
            oss << "rm -rf " << root->dir_sys_repo_dm() << "*";
            rc = system(oss.str().c_str());
        } else {
            GLOGERROR << "unable to get fds root dir";
        }
    }

    int run() override {
        return 0;
    }
};
}  // namespace fds
/**
int main(int argc, char *argv[])
{
    auto tester = new fds::DMTester(argc, argv);
    tester->start();
    tester->stop();
    return 0;
}
*/
#endif  // SOURCE_UNIT_TEST_DATA_MGR_DM_MOCKS_H_
