/**
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <unistd.h>
#include <set>
#include <string>

#include <boost/program_options.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <fds_process.h>
#include <chrono>
#include <util/stringutils.h>
#include <concurrency/RwLock.h>
#include <dlt.h>
#include <orch-mgr/om-service.h>
#include <counters.h>
#include <orchMgr.h>
#include <OmClusterMap.h>
#include <OmDataPlacement.h>
#include <OmVolumePlacement.h>
#include <OmDmtDeploy.h>

#include <qos_ctrl.h>
#include <testlib/TestFixtures.h>
#include <testlib/TestOm.hpp>
#include <testlib/ProcessHandle.hpp>
#include <testlib/SvcMsgFactory.h>
#include <testlib/TestUtils.h>
#include <net/VolumeGroupHandle.h>

using ::testing::AtLeast;
using ::testing::Return;
namespace po = boost::program_options;

namespace fds {
// The 7 lines below are to make this unit test compile,
// since everything in OM is global and depends on everything
OM_Module *omModule;

OM_Module *OM_Module::om_singleton()
{
    return omModule;
}

void threadedOM(OrchMgr *ptr) {
    ptr->main();
}

struct DmGroupFixture : BaseTestFixture {
    OrchMgr *testOm;
    OM_Module *testOmModule;
    std::thread* omThPtr;
    int nodeUuidCounter; // starts at 10 for fun

    DmGroupFixture() : testOm(nullptr),
                       testOmModule(nullptr),
                       omThPtr(nullptr),
                       nodeUuidCounter(10)
    {}

    ~DmGroupFixture() {}

    // Create a OM state machine - only once
    void SetUp()
    {
        std::string fdsSrcPath;
        auto findRet = TestUtils::findFdsSrcPath(fdsSrcPath);
        fds_verify(findRet);

        std::string homedir = boost::filesystem::path(getenv("HOME")).string();
        std::string baseDir =  homedir + "/temp";
        setupDmClusterEnv(fdsSrcPath, baseDir);

        testOmModule = omModule = new OM_Module("testOmModule");
        testOm = new OrchMgr(argc_, argv_, testOmModule, true, true);
        g_fdsprocess = testOm;
        omThPtr = new std::thread(threadedOM, testOm);
        omThPtr->detach();
        testOm->getReadyWaiter().await();
        omModule->setOMTestMode(true);
    }

    void TearDown()
    {
        if (testOm != nullptr) {
        //     delete testOm;
        }
        if (testOmModule != nullptr) {
        //     delete testOmModule;
        }
    }

    void setDmClusterSize(int size) {
        auto domainMod = testOmModule->om_nodedomain_mod();
        fds_verify(domainMod != nullptr);
        domainMod->setDmClusterSize(size);
    }

    void setVolumeGrpMode(fds_bool_t value) {
        auto dmtMod = testOmModule->om_dmt_mod();
        dmtMod->setVolumeGrpMode(value);
    }

    fds_bool_t hasCommittedDMT() {
        auto volmod = testOmModule->om_volplace_mod();
        return volmod->hasCommittedDMT();
    }

    fds_uint64_t getCommittedDMTVersion() {
        auto volmod = testOmModule->om_volplace_mod();
        return volmod->getCommittedDMTVersion();
    }

    void addNewFakeDMSvc(const NodeUuid &uuid) {
        std::vector<fpi::SvcInfo> entries;
        entries.emplace_back(); // just 1 entry
        auto entry = entries.begin();
        entry->svc_id.svc_uuid.svc_uuid = uuid.uuid_get_val();
        entry->svc_type = fpi::FDSP_MgrIdType::FDSP_DATA_MGR;
        // Everything else is zero
        MODULEPROVIDER()->getSvcMgr()->updateSvcMap(entries);
    }

    // Returns the ptr for book keeping, if the caller wants to
    fpi::FDSP_RegisterNodeTypePtr addNewFakeDm() {
        NodeUuid newUuid(nodeUuidCounter);
        ++nodeUuidCounter;
        NodeAgent::pointer      newNode;

        auto msgPtr = boost::make_shared<fpi::FDSP_RegisterNodeType>();
        std::string fakeDM = "FakeDM";
        std::string nodename = fakeDM + std::to_string(newUuid.uuid_get_val());
        msgPtr->node_name = nodename;
        msgPtr->node_type = fpi::FDSP_MgrIdType::FDSP_DATA_MGR;
        msgPtr->node_uuid.uuid = newUuid.uuid_get_val();
        msgPtr->service_uuid.uuid = newUuid.uuid_get_val();

        auto domainMod = testOmModule->om_nodedomain_mod();
        auto om_locDomain = domainMod->om_loc_domain_ctrl();

        // Fake a svc map entry
        addNewFakeDMSvc(newUuid);

        // Fake the setupNewNode info
        om_locDomain->dc_register_node(newUuid, msgPtr, &newNode);

        // Call setupNewNode directly instead of scheduling it
        domainMod->setupNewNode(newUuid, msgPtr, newNode, false);

        // Gotta sleep because we don't have control over boost state machine
        sleep(4);
        return msgPtr;
    }

    // Deletes a node
    void deleteFakeDm(fpi::FDSP_RegisterNodeTypePtr msg) {
        auto domainMod = testOmModule->om_nodedomain_mod();
        NodeUuid nodeUuid(msg->node_uuid.uuid);
        auto nodeName = msg->node_name;
        auto retVal = domainMod->om_del_services(nodeUuid, nodeName, false, true, false);
        ASSERT_TRUE(retVal.OK());
        sleep(2); // sleep for DMT state machine to churn
    }

    void setNewNodeUuid(NodeUuid value) {
        nodeUuidCounter = value.uuid_get_val();
    }

};

TEST_F(DmGroupFixture, DmClusterAddNodeTest) {
    // Create an OM module instance
    fpi::SvcUuid dummySvcInfo;
    fpi::SvcInfo dummyInfo;

    // We'll be testing with X DMs... so set it here.
    setDmClusterSize(4);
    setVolumeGrpMode(true);

    // Verify we have no committed DMT
    ASSERT_FALSE(hasCommittedDMT());
    ASSERT_TRUE(testOmModule->om_dmt_mod()->volumeGrpMode());
    ASSERT_TRUE(testOmModule->om_nodedomain_mod()->checkDmtModVGMode());

    // Add first node
    addNewFakeDm();
    // Ensure that only one dm is waiting
    ASSERT_TRUE(testOmModule->om_dmt_mod()->getWaitingDMs() == 1);
    ASSERT_FALSE(hasCommittedDMT());

    // Add second node
    addNewFakeDm();
    ASSERT_TRUE(testOmModule->om_dmt_mod()->getWaitingDMs() == 2);
    ASSERT_FALSE(hasCommittedDMT());

    // Add third node
    addNewFakeDm();
    ASSERT_TRUE(testOmModule->om_dmt_mod()->getWaitingDMs() == 3);
    ASSERT_FALSE(hasCommittedDMT());

    // Add fourth node
    auto lastNodePtr = addNewFakeDm();
    ASSERT_TRUE(hasCommittedDMT());
    // Should only have the first DMT published
    ASSERT_TRUE(getCommittedDMTVersion() == 1);
    ASSERT_TRUE(testOmModule->om_dmt_mod()->getWaitingDMs() == 0);

    // Make sure that the node is in svcmap. We will be removing it and re-adding it later.
    dummySvcInfo.svc_uuid = lastNodePtr->service_uuid.uuid;
    ASSERT_TRUE(MODULEPROVIDER()->getSvcMgr()->getSvcInfo(dummySvcInfo, dummyInfo));

    // Add fifth node - should be no-op
    lastNodePtr = addNewFakeDm();
    ASSERT_TRUE(hasCommittedDMT());
    ASSERT_TRUE(getCommittedDMTVersion() == 1);
    // Since we only support 1 set of 4 DMs, we shouldn't have waiting DMs
    ASSERT_TRUE(testOmModule->om_dmt_mod()->getWaitingDMs() == 0);
    // We shouldn't see it in the service map
    dummySvcInfo.svc_uuid = lastNodePtr->service_uuid.uuid;
    ASSERT_TRUE(MODULEPROVIDER()->getSvcMgr()->getSvcInfo(dummySvcInfo, dummyInfo));

    // Add all the way to 8 nodes... should be no-op. This will change once we support
    // multiple dm clusters
    for (int i = 0; i < 3; ++i) {
        addNewFakeDm();
    }
    ASSERT_TRUE(hasCommittedDMT());
    ASSERT_TRUE(getCommittedDMTVersion() == 1);
    ASSERT_TRUE(testOmModule->om_dmt_mod()->getWaitingDMs() == 0);

}

/**
 * Currently we can't delete OM_Module so we can't support multiple fixtures
 * in one cpp test file.
 */
TEST_F(DmGroupFixture, DISABLED_DmClusterRemoveNodeTest) {
    // Create an OM module instance

    // We'll be testing with X DMs... so set it here.
    setDmClusterSize(4);
    setVolumeGrpMode(true);

    // Verify we have no committed DMT
    ASSERT_FALSE(hasCommittedDMT());
    ASSERT_TRUE(testOmModule->om_dmt_mod()->volumeGrpMode());
    ASSERT_TRUE(testOmModule->om_nodedomain_mod()->checkDmtModVGMode());

    // Create a 4 DM cluster
    fpi::FDSP_RegisterNodeTypePtr msgPtr;
    for (int i=0; i < 4; i++) {
        msgPtr = addNewFakeDm();
    }

    // Remove the last node
    deleteFakeDm(msgPtr);
    ASSERT_TRUE(hasCommittedDMT());
    ASSERT_TRUE(getCommittedDMTVersion() == 1);
}

} // namespace fds

int main(int argc, char *argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    fds::g_fdslog = new fds::fds_log("volumeomgtest"); // haha... omg
    po::options_description opts("Allowed options");
    opts.add_options()
        ("help", "produce help message")
        ("puts-cnt", po::value<int>()->default_value(1), "puts count");
    fds::DmGroupFixture::init(argc, argv, opts);
    return RUN_ALL_TESTS();
}

