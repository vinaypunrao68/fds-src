/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef SOURCE_UNIT_TEST_DATA_MGR_DM_MOCKS_H_
#define SOURCE_UNIT_TEST_DATA_MGR_DM_MOCKS_H_
#include <unistd.h>
#include <DataMgr.h>
#include <net/net-service.h>
#include <util/fds_stat.h>

#include <fds_module.h>
#include <fds_process.h>
#include <concurrency/Mutex.h>
#include <string>
#include <vector>

#include "platform/platform_process.h"

namespace fds {
// TODO(Rao): Get rid of this singleton
DataMgr *dataMgr = 0;


class MockDataMgr : public PlatformProcess {
  public:
    MockDataMgr(int argc, char *argv[]) {
        static Module *modVec[] = {};
        init(argc, argv, "fds.dm.", "dm.log", &gl_DmPlatform, modVec);
    }

    virtual int run() override {
        return 0;
    }
};


struct TestDmContainer :  DmContainer
{
  public:
    // typedef boost::intrusive_ptr<DmContainer> pointer;
    TestDmContainer() : DmContainer(fpi::FDSP_DATA_MGR) {}

    virtual void agent_activate(NodeAgent::pointer agent) {
        rs_register(agent);
    }

    virtual ~TestDmContainer() {}
    virtual Resource *rs_new(const ResourceUUID &uuid) {
        return new DmAgent(uuid);
    }
};


struct TestPlatform : Platform {
    TestDmContainer dmContainer;
    TestPlatform() : Platform("DM-Platform",
                              fpi::FDSP_DATA_MGR,
                              new DomainContainer("DM-Platform-NodeInv",
                                                  NULL,
                                                  new SmContainer(fpi::FDSP_STOR_MGR),
                                                  new TestDmContainer(),
                                                  new AmContainer(fpi::FDSP_STOR_HVISOR),
                                                  new PmContainer(fpi::FDSP_PLATFORM),
                                                  new OmContainer(fpi::FDSP_ORCH_MGR)),
                              new DomainClusterMap("DM-Platform-ClusMap",
                                                   NULL,
                                                   new SmContainer(fpi::FDSP_STOR_MGR),
                                                   new TestDmContainer(),
                                                   new AmContainer(fpi::FDSP_STOR_HVISOR),
                                                   new PmContainer(fpi::FDSP_PLATFORM),
                                                   new OmContainer(fpi::FDSP_ORCH_MGR)),
                              new DomainResources("DM-Resources"),
                              NULL) {
        Platform::platf_assign_singleton(this);
        plf_node_type = fpi::FDSP_DATA_MGR;
        plf_my_ip = "127.0.0.1";
        plf_my_node_name = "dm-test";
        fpi::FDSP_Uuid uuid;
        uuid.uuid = 12345678;

        plf_my_uuid = 12345678;
        plf_my_svc_uuid = 12345678;
        plf_my_plf_svc_uuid = 12345678;
        fpi::FDSP_RegisterNodeTypePtr nodeTypeMsg(new fpi::FDSP_RegisterNodeType());
        nodeTypeMsg->node_type = fpi::FDSP_DATA_MGR;
        nodeTypeMsg->node_uuid = uuid;
        nodeTypeMsg->service_uuid = uuid;
        nodeTypeMsg->node_name = plf_my_node_name;

        NodeAgent::pointer dmAgent;
        plf_dm_cluster()->agent_register(plf_my_uuid, nodeTypeMsg, &dmAgent, true);
    }
};

struct TestOMgrClient : OMgrClient {
    TestOMgrClient(fpi::FDSP_MgrIdType node_type,
                   const std::string& _omIpStr,
                   fds_uint32_t _omPort,
                   const std::string& node_name,
                   fds_log *parent_log,
                   boost::shared_ptr<netSessionTbl> nst,
                   Platform *plf_mgr) : OMgrClient(node_type,
                                                   _omIpStr,
                                                   _omPort,
                                                   node_name,
                                                   parent_log,
                                                   nst,
                                                   plf_mgr) {
        setupTestMode();
    }

    void setupTestMode() {
        setNoNetwork(true);
        DLT *dlt = new DLT(1, 1, 1, true);

        for (uint i = 0; i < dlt->getNumTokens(); i++) {
            for (uint j= 0; j < dlt->getDepth(); j++) {
                dlt->setNode(i, j, 12345678);
            }
        }
        dltMgr->add(*dlt);

        DMT *dmt = new DMT(1, 1, 1, true);

        for (uint i = 0; i < dmt->getNumColumns(); i++) {
            for (uint j= 0; j < dmt->getDepth(); j++) {
                dmt->setNode(i, j, 12345678);
            }
        }
        dmtMgr->add(dmt, DMT_COMMITTED);
    }
};

static fds::Module *dmVec[] = {
    &fds::gl_fds_stat,
    NULL
};

struct DMTester :  FdsProcess {
    std::vector<boost::shared_ptr<VolumeDesc> > volumes;
    std::string TESTBLOB;
    fds_volid_t TESTVOLID;
    uint64_t txnId = 0;

    DMTester(int argc, char *argv[])
            : FdsProcess(argc,
                         argv,
                         "platform.conf",
                         "fds.dm.",
                         "dm.log",
                         dmVec) {
    }

    virtual ~DMTester() {}

    void initDM() {
        gl_PlatformSvc = new TestPlatform();
        std::cout << "initing dm" << std::endl;
        PerfTracer::setEnabled(false);
        dataMgr = new DataMgr(this);
        dataMgr->runMode = DataMgr::TEST_MODE;
        dataMgr->use_om = false;
        dataMgr->omConfigPort = 8904;
        dataMgr->omIpStr = "localhost";
        dataMgr->vol_map_mtx = new fds_mutex("Volume map mutex");
        dataMgr->feature.fQosEnabled= false;
        dataMgr->feature.fTestMode= true;
        dataMgr->feature.fCatSyncEnabled = false;
        auto nstable = boost::shared_ptr<netSessionTbl>(new netSessionTbl(FDSP_DATA_MGR));
        dataMgr->omClient = new TestOMgrClient(FDSP_DATA_MGR,
                                               dataMgr->omIpStr,
                                               dataMgr->omConfigPort,
                                               "dm",
                                               GetLog(),
                                               nstable,
                                               get_plf_manager());

        dataMgr->omClient->initialize();
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
                                        std::to_string(volumes[num]->volUUID),
                                        volumes[num]->volUUID, volumes[num].get(),
                                        false);
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
