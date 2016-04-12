/**
 * Copyright 2015 Formation Data Systems, Inc.
 */
#include <vector>
#include <string>
#include <util/timeutils.h>

#include <boost/program_options.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <orchMgr.h>
#include <testlib/FakeDataStore.h>
#include <testlib/TestOm.hpp>
#include <testlib/TestFixtures.h>
#include <testlib/TestUtils.h>
#include <orch-mgr/om-service.h>
#include <OmIntUtilApi.h>
#include <OmExtUtilApi.h>


using ::testing::AtLeast;
using ::testing::Return;
namespace po = boost::program_options;

namespace fds{
OM_Module *omModule;

OM_Module *OM_Module::om_singleton()
{
    return omModule;
}

void threadedOM(OrchMgr *ptr) {
    ptr->main();
}

void processStop(OrchMgr *ptr) {
    ptr->stop();
}

class CustomFakeDataStore : public FakeDataStore
{
public:

    CustomFakeDataStore() { }
    ~CustomFakeDataStore() {}

    /*
    * Custom implementation of functions for this specific set of tests
    */
       bool getSvcMap(std::vector<fpi::SvcInfo>& svcMap)
       {
           bool ret = false;

           if ( !fakeSvcMap.empty() )
           {
               svcMap = fakeSvcMap;
               ret = true;
           }

           return ret;
       }

       bool getSvcInfo(const fds_uint64_t svc_uuid, fpi::SvcInfo& svcInfo)
       {
           bool ret = false;

           int64_t svcUuid = static_cast<int64_t>(svc_uuid);
           for ( auto svc : fakeSvcMap )
           {
               if ( svc.svc_id.svc_uuid.svc_uuid == svcUuid)
               {
                   svcInfo = svc;
                   ret = true;
                   break;
               }
           }
           return ret;
       }

       bool changeStateSvcMap( fpi::SvcInfoPtr svcInfoPtr)
       {
           bool found = false;

           if ( !fakeSvcMap.empty())
           {
               std::vector<fpi::SvcInfo>::iterator iter;
               for ( iter = fakeSvcMap.begin(); iter != fakeSvcMap.end(); iter++)
               {
                   if ( (*iter).svc_id.svc_uuid.svc_uuid == svcInfoPtr->svc_id.svc_uuid.svc_uuid )
                   {
                       found = true;
                       break;
                   }
               }

               if ( found )
               {
                   // Simply erase and re add, easier than updating every field
                   fakeSvcMap.erase(iter);
               }

           }
           fakeSvcMap.push_back(*svcInfoPtr);

           return true;
       }

       bool deleteSvcMap(const fpi::SvcInfo& svcInfo)
       {
           bool found = false;

           std::vector<fpi::SvcInfo>::iterator iter;
           for ( iter = fakeSvcMap.begin(); iter != fakeSvcMap.end(); iter++)
           {
               if ( (*iter).svc_id.svc_uuid.svc_uuid == svcInfo.svc_id.svc_uuid.svc_uuid )
               {
                   found = true;
                   break;
               }
           }

           if ( found )
           {
               // Simply erase and re add, easier than updating every field
               fakeSvcMap.erase(iter);
               return true;
           }

           return false;
       }
};

struct SvcMapFixture : BaseTestFixture{

    OrchMgr*             testOm;
    OM_Module*           testOmModule;
    std::thread*         omThPtr;
    uint64_t             uuidCounter; // starts at 10 for fun
    CustomFakeDataStore* dataStore;

    std::vector<std::vector<fpi::ServiceStatus>> invalidTransitions =
    {
            // invalid current states for incoming: INVALID(0)
            { },
            // invalid incoming states for: ACTIVE(1)
            { fpi::SVC_STATUS_DISCOVERED, fpi::SVC_STATUS_ADDED,
              fpi::SVC_STATUS_INACTIVE_STOPPED, fpi::SVC_STATUS_REMOVED},
            // invalid incoming states for: INACTIVE_STOPPED(2)
            { fpi::SVC_STATUS_DISCOVERED, fpi::SVC_STATUS_STANDBY,fpi::SVC_STATUS_ADDED,
              fpi::SVC_STATUS_STOPPING, fpi::SVC_STATUS_INACTIVE_FAILED },
            // invalid incoming states for: DISCOVERED(3)
            { fpi::SVC_STATUS_INACTIVE_STOPPED, fpi::SVC_STATUS_STANDBY,
              fpi::SVC_STATUS_ADDED, fpi::SVC_STATUS_STARTED, fpi::SVC_STATUS_STOPPING,
              fpi::SVC_STATUS_REMOVED, fpi::SVC_STATUS_INACTIVE_FAILED },
            // invalid incoming s states for: STANDBY(4)
            { fpi::SVC_STATUS_INACTIVE_STOPPED, fpi::SVC_STATUS_DISCOVERED,
              fpi::SVC_STATUS_ADDED, fpi::SVC_STATUS_STARTED,
              fpi::SVC_STATUS_STOPPING, fpi::SVC_STATUS_INACTIVE_FAILED },
            // invalid incoming states for: ADDED(5)
            { fpi::SVC_STATUS_ACTIVE, fpi::SVC_STATUS_INACTIVE_STOPPED,
              fpi::SVC_STATUS_DISCOVERED, fpi::SVC_STATUS_STANDBY,fpi::SVC_STATUS_STOPPING,
              fpi::SVC_STATUS_INACTIVE_FAILED },
            // invalid incoming states for: STARTED(6)
            {  fpi::SVC_STATUS_INACTIVE_STOPPED, fpi::SVC_STATUS_DISCOVERED,
               fpi::SVC_STATUS_STANDBY, fpi::SVC_STATUS_ADDED, fpi::SVC_STATUS_REMOVED},
            // invalid incoming states for: STOPPING(7)
            { fpi::SVC_STATUS_ACTIVE, fpi::SVC_STATUS_DISCOVERED,
              fpi::SVC_STATUS_STANDBY, fpi::SVC_STATUS_ADDED,
              fpi::SVC_STATUS_INACTIVE_FAILED },
            // invalid incoming states for : REMOVED(8)
            { fpi::SVC_STATUS_ACTIVE, fpi::SVC_STATUS_INACTIVE_STOPPED,
              fpi::SVC_STATUS_STANDBY, fpi::SVC_STATUS_ADDED, fpi::SVC_STATUS_STARTED,
              fpi::SVC_STATUS_STOPPING, fpi::SVC_STATUS_INACTIVE_FAILED },
            // invalid current states for incoming: INACTIVE_FAILED(9)
            { fpi::SVC_STATUS_INACTIVE_STOPPED, fpi::SVC_STATUS_DISCOVERED,
              fpi::SVC_STATUS_STANDBY, fpi::SVC_STATUS_ADDED, fpi::SVC_STATUS_STARTED,
              fpi::SVC_STATUS_REMOVED}
    };


    SvcMapFixture() :  testOm(nullptr),
                       testOmModule(nullptr),
                       omThPtr(nullptr),
                       uuidCounter(10)
    {
        dataStore = new CustomFakeDataStore();
    }

    ~SvcMapFixture() {}


    void SetUp()
    {
        std::cout << "Running setup\n";

        testOmModule = omModule = new OM_Module("testOmModule");
        testOm       = new OrchMgr(argc_, argv_, testOmModule, true, true);
        g_fdsprocess = testOm;
        omThPtr      = new std::thread(threadedOM, testOm);

        testOm->getReadyWaiter().await();
        testOmModule->setOMTestMode(true);
    }

    void TearDown()
    {
        std::cout << "Running teardown\n";

        std::cout << "1. Signal stop thread spin/wait to OM\n";
        testOm->testTerminate();

        omThPtr->join();

        std::cout << "2. Stop SvcMgr \n";

        processStop(testOm);

        // Explicitly delete so that the setup (if run again) will create
        // all the modules correctly
        if (testOmModule != nullptr) {
            std::cout << "3. Delete OM modules \n";
            delete testOmModule;
        }

        // This still causes an exception as though some thread is still
        // accessing OM data, test runs without having to do this so
        // leaving it commented out
        //if (testOm != nullptr) {
        //     delete testOm;
        //}


    }

    /***********************************************
     * Functions to verify services in configDB
     **********************************************/
    bool dbServiceCheck( uint64_t uuid, fpi::ServiceStatus expectedStatus,
                         int32_t expIncarnationNo = -1 )
    {
        fpi::SvcInfo svcInfo;
        bool ret = dataStore->getSvcInfo(uuid, svcInfo);

        if ( !ret ) { return false; }

        if ( svcInfo.svc_status == expectedStatus )
        {
            if ( expIncarnationNo != -1)
            {
                if (expIncarnationNo == svcInfo.incarnationNo )
                {
                    return true;
                } else {
                    return false;
                }
            } else {
                return true;
            }
        }

        return false;
    }


    /***********************************************
     * Functions to verify services in svcLayer
     **********************************************/
    bool svcLayerSvcCheck( int64_t uuid, fpi::ServiceStatus expectedStatus,
                           int32_t expIncarnationNo = -1 )
    {
        fpi::SvcInfo svcInfo;
        fpi::SvcUuid svcUuid;
        svcUuid.svc_uuid = uuid;

        bool ret = MODULEPROVIDER()->getSvcMgr()->getSvcInfo(svcUuid, svcInfo);

        if ( !ret ) { return false; }

        if ( svcInfo.svc_status == expectedStatus )
        {
            if ( expIncarnationNo != -1)
            {
                if (expIncarnationNo == svcInfo.incarnationNo )
                {
                    return true;
                } else {
                    return false;
                }
            } else {
                return true;
            }
        }

        return false;
    }

    /***********************************************
     *              Generic Functions
     **********************************************/
    int64_t addFakeSvc(fpi::FDSP_MgrIdType type, std::string name, bool update)
    {
        uuidCounter += 1;
        NodeUuid uuid(uuidCounter);
        fpi::SvcInfo svcInfo;

        svcInfo.svc_id.svc_uuid.svc_uuid = uuid.uuid_get_val();
        svcInfo.incarnationNo            = 0;
        svcInfo.svc_type                 = type;
        svcInfo.svc_status               = fpi::SVC_STATUS_ADDED;
        svcInfo.name                     = name;

        if (update)
        {
            SvcMgr* svcMgr = MODULEPROVIDER()->getSvcMgr();

            fds::updateSvcMaps<CustomFakeDataStore>( dataStore,
                                               svcMgr,
                                               uuid.uuid_get_val(),
                                               svcInfo.svc_status,
                                               type,
                                               true,
                                               false,
                                               svcInfo
                                               );
        }

        return uuidCounter;
    }

    // Modify service state (for same incarnation number)
    void modifyServiceState1( fpi::ServiceStatus incomingStatus, uint64_t uuid, fpi::FDSP_MgrIdType type )
    {
        SvcMgr* svcMgr = MODULEPROVIDER()->getSvcMgr();

        fds::updateSvcMaps<CustomFakeDataStore>( dataStore,
                                           svcMgr,
                                           uuid,
                                           incomingStatus,
                                           type
                                           );
    }

    // Modify service state (potentially diff incarnation number)
    // For registration, other handler paths
    void modifyServiceState2(fpi::SvcInfo svcInfo)
    {
        SvcMgr* svcMgr = MODULEPROVIDER()->getSvcMgr();

        fds::updateSvcMaps<CustomFakeDataStore>( dataStore,
                                           svcMgr,
                                           svcInfo.svc_id.svc_uuid.svc_uuid,
                                           svcInfo.svc_status,
                                           svcInfo.svc_type,
                                           true,
                                           false,
                                           svcInfo
                                           );
    }
};

TEST_F(SvcMapFixture, SvcStatusTest)
{
    g_fdslog->setSeverityFilter(fds_log::severity_level::debug);

    uint64_t smUuid, dmUuid, amUuid;
    std::vector<fpi::SvcInfo> entries;

    std::cout << "STEP 1 : Add 1 SM, 1 DM, 1 AM service\n";
    smUuid = addFakeSvc(fpi::FDSP_STOR_MGR, "sm", true);
    dmUuid = addFakeSvc(fpi::FDSP_DATA_MGR, "dm", true);
    amUuid = addFakeSvc(fpi::FDSP_ACCESS_MGR, "am", true);

    // Check svc status in db
    ASSERT_TRUE(dbServiceCheck(smUuid, fpi::SVC_STATUS_ADDED));
    ASSERT_TRUE(dbServiceCheck(dmUuid, fpi::SVC_STATUS_ADDED));
    ASSERT_TRUE(dbServiceCheck(amUuid, fpi::SVC_STATUS_ADDED));

    // SvcLayer should not have a record of this svc until it goes ACTIVE
    MODULEPROVIDER()->getSvcMgr()->getSvcMap(entries);
    ASSERT_TRUE(entries.size() == 1 && entries.front().svc_type == fpi::FDSP_ORCH_MGR);

    std::cout << "STEP 2 : Modify services statuses to STARTED\n";
    modifyServiceState1(fpi::SVC_STATUS_STARTED, smUuid, fpi::FDSP_STOR_MGR);
    modifyServiceState1(fpi::SVC_STATUS_STARTED, dmUuid, fpi::FDSP_DATA_MGR);
    modifyServiceState1(fpi::SVC_STATUS_STARTED, amUuid, fpi::FDSP_ACCESS_MGR);

    ASSERT_TRUE(dbServiceCheck(smUuid, fpi::SVC_STATUS_STARTED));
    ASSERT_TRUE(dbServiceCheck(dmUuid, fpi::SVC_STATUS_STARTED));
    ASSERT_TRUE(dbServiceCheck(amUuid, fpi::SVC_STATUS_STARTED));

    // SvcLayer should not have a record of this svc until it goes ACTIVE
    MODULEPROVIDER()->getSvcMgr()->getSvcMap(entries);
    ASSERT_TRUE(entries.size() == 1 && entries.front().svc_type == fpi::FDSP_ORCH_MGR);

    entries.clear();

    // Set the services to active
    dataStore->getSvcMap(entries);

    std::cout << "STEP 3 : Modify services statuses to ACTIVE, with updated incarnationNo\n";
    std::cout << "Verify status, incarnationNo update in DB and svcLayer\n";
    for ( auto svc : entries )
    {
        svc.svc_status = fpi::SVC_STATUS_ACTIVE;
        svc.incarnationNo += 5; // just to be certain it updates

        modifyServiceState2(svc);

        int64_t uuid = svc.svc_id.svc_uuid.svc_uuid;

        ASSERT_TRUE(dbServiceCheck(uuid, fpi::SVC_STATUS_ACTIVE, svc.incarnationNo));

        ASSERT_TRUE(svcLayerSvcCheck(uuid, fpi::SVC_STATUS_ACTIVE, svc.incarnationNo));
    }

    // Map
    //+--------------------------+---------------------------------------+
    //|     Current State        |    Valid IncomingState                |
    //+--------------------------+---------------------------------------+
    //      ACTIVE               |  STANDBY, STOPPING, INACTIVE_FAILED
    //      INACTIVE_STOPPED     |  ACTIVE, STARTED, REMOVED
    //      DISCOVERED           |  ACTIVE
    //      STANDBY              |  REMOVED, ACTIVE
    //      ADDED                |  STARTED, REMOVED
    //      STARTED              |  ACTIVE, STOPPING, INACTIVE_FAILED
    //      STOPPING              |  INACTIVE_STOPPED, STARTED, REMOVED
    //      REMOVED              |  DISCOVERED
    //      INACTIVE_FAILED      |  ACTIVE, STOPPING
    //+--------------------------+---------------------------------------+
    //
    std::cout << "\nSTEP 4: Verify allowed,valid transitions, same incarnationNo \n";
    //cout << "4(a) Verify transition (ACTIVE  |  STANDBY, STOPPING, INACTIVE_FAILED)";

    fpi::SvcInfo svc;
    bool ret = dataStore->getSvcInfo(smUuid, svc);
    ASSERT_TRUE(ret);


    int64_t uuid = svc.svc_id.svc_uuid.svc_uuid;

    // In the real world, SM,DM,AM never transition to standby, but wer are just verifying
    // transitions, OK to do this
    std::cout << "4(a) Verify good transition ACTIVE -> STANDBY\n";
    modifyServiceState1(fpi::SVC_STATUS_STANDBY, uuid, svc.svc_type);

    ASSERT_TRUE(dbServiceCheck(uuid, fpi::SVC_STATUS_STANDBY));
    ASSERT_TRUE(svcLayerSvcCheck(uuid, fpi::SVC_STATUS_STANDBY));

    std::cout << "4(b) Verify good transition STANDBY -> REMOVED\n";
    modifyServiceState1(fpi::SVC_STATUS_REMOVED, uuid, svc.svc_type);

    ASSERT_TRUE(dbServiceCheck(uuid, fpi::SVC_STATUS_REMOVED));
    ASSERT_TRUE(svcLayerSvcCheck(uuid, fpi::SVC_STATUS_REMOVED));

    std::cout << "4(c) Verify good transition REMOVED -> DISCOVERED\n";
    modifyServiceState1(fpi::SVC_STATUS_DISCOVERED, uuid, svc.svc_type);

    ASSERT_TRUE(dbServiceCheck(uuid, fpi::SVC_STATUS_DISCOVERED));
    ASSERT_TRUE(svcLayerSvcCheck(uuid, fpi::SVC_STATUS_DISCOVERED));

    std::cout << "4(d) Verify good transition DISCOVERED -> ACTIVE\n";
    modifyServiceState1(fpi::SVC_STATUS_ACTIVE, uuid, svc.svc_type);

    ASSERT_TRUE(dbServiceCheck(uuid, fpi::SVC_STATUS_ACTIVE));
    ASSERT_TRUE(svcLayerSvcCheck(uuid, fpi::SVC_STATUS_ACTIVE));

    std::cout << "4(e) Verify good transition ACTIVE -> STOPPING\n";
    modifyServiceState1(fpi::SVC_STATUS_STOPPING, uuid, svc.svc_type);

    ASSERT_TRUE(dbServiceCheck(uuid, fpi::SVC_STATUS_STOPPING));
    ASSERT_TRUE(svcLayerSvcCheck(uuid, fpi::SVC_STATUS_STOPPING));

    std::cout << "4(f) Verify good transition STOPPING -> INACTIVE_STOPPED\n";
    svc.svc_status = fpi::SVC_STATUS_INACTIVE_STOPPED;
    modifyServiceState1(fpi::SVC_STATUS_INACTIVE_STOPPED, uuid, svc.svc_type);

    ASSERT_TRUE(dbServiceCheck(uuid, fpi::SVC_STATUS_INACTIVE_STOPPED));
    ASSERT_TRUE(svcLayerSvcCheck(uuid, fpi::SVC_STATUS_INACTIVE_STOPPED));

    std::cout << "4(g) Verify good transition INACTIVE_STOPPED -> STARTED (db only)\n";
    modifyServiceState1(fpi::SVC_STATUS_STARTED, uuid, svc.svc_type);

    ASSERT_TRUE(dbServiceCheck(uuid, fpi::SVC_STATUS_STARTED));
    //ASSERT_TRUE(svcLayerSvcCheck(uuid, fpi::SVC_STATUS_STARTED));

    std::cout << "4(h) Verify good transition STARTED -> ACTIVE (db only)\n";
    modifyServiceState1(fpi::SVC_STATUS_ACTIVE, uuid, svc.svc_type);

    ASSERT_TRUE(dbServiceCheck(uuid, fpi::SVC_STATUS_ACTIVE));

    // transition to active for svcLayer (currently in INACTIVE_STOPPED)
    modifyServiceState1(fpi::SVC_STATUS_ACTIVE, uuid, svc.svc_type);
    ASSERT_TRUE(svcLayerSvcCheck(uuid, fpi::SVC_STATUS_ACTIVE));

    std::cout << "4(i) Verify good transition ACTIVE -> INACTIVE_FAILED\n";
    modifyServiceState1(fpi::SVC_STATUS_INACTIVE_FAILED, uuid, svc.svc_type);

    ASSERT_TRUE(dbServiceCheck(uuid, fpi::SVC_STATUS_INACTIVE_FAILED));
    ASSERT_TRUE(svcLayerSvcCheck(uuid, fpi::SVC_STATUS_INACTIVE_FAILED));

    std::cout << "4(j) Verify good transition INACTIVE_FAILED -> STOPPING\n";
    modifyServiceState1(fpi::SVC_STATUS_STOPPING, uuid, svc.svc_type);

    ASSERT_TRUE(dbServiceCheck(uuid, fpi::SVC_STATUS_STOPPING));
    ASSERT_TRUE(svcLayerSvcCheck(uuid, fpi::SVC_STATUS_STOPPING));

    std::cout << "4(k) Verify good transition STOPPING -> STARTED (db only) \n";
    modifyServiceState1(fpi::SVC_STATUS_STARTED, uuid, svc.svc_type);

    ASSERT_TRUE(dbServiceCheck(uuid, fpi::SVC_STATUS_STARTED));
    //ASSERT_TRUE(svcLayerSvcCheck(uuid, fpi::SVC_STATUS_STARTED));

    std::cout << "4(l) Verify good transition STARTED -> STOPPING (db only) \n";
    modifyServiceState1(fpi::SVC_STATUS_STOPPING, uuid, svc.svc_type);

    ASSERT_TRUE(dbServiceCheck(uuid, fpi::SVC_STATUS_STOPPING));
    //ASSERT_TRUE(svcLayerSvcCheck(uuid, fpi::SVC_STATUS_STOPPING));

    std::cout << "4(m) Verify good transition STOPPING -> REMOVED\n";
    modifyServiceState1(fpi::SVC_STATUS_REMOVED, uuid, svc.svc_type);

    ASSERT_TRUE(dbServiceCheck(uuid, fpi::SVC_STATUS_REMOVED));
    ASSERT_TRUE(svcLayerSvcCheck(uuid, fpi::SVC_STATUS_REMOVED));


    // SWITCHING SERVICE.Pick another service since it's too much effort to go to ACTIVE for
    // above service, and we've already done these checks.

    ret = dataStore->getSvcInfo(dmUuid, svc);
    ASSERT_TRUE(ret);
    uuid = svc.svc_id.svc_uuid.svc_uuid;

    // Get to STANDBY, already verified this transition
    modifyServiceState1(fpi::SVC_STATUS_STANDBY, uuid, svc.svc_type);

    std::cout << "4(n) Verify good transition STANDBY -> ACTIVE\n";
    modifyServiceState1(fpi::SVC_STATUS_ACTIVE, uuid, svc.svc_type);

    ASSERT_TRUE(dbServiceCheck(uuid, fpi::SVC_STATUS_ACTIVE));
    ASSERT_TRUE(svcLayerSvcCheck(uuid, fpi::SVC_STATUS_ACTIVE));

    // Get to INACTIVE_STOPPED, already verified
    // ACTIVE - STOPPING, STOPPING - INACTIVE_STOPPED
    modifyServiceState1(fpi::SVC_STATUS_STOPPING, uuid, svc.svc_type);
    modifyServiceState1(fpi::SVC_STATUS_INACTIVE_STOPPED, uuid, svc.svc_type);

    std::cout << "4(o) Verify good transition INACTIVE_STOPPED -> ACTIVE\n";
    modifyServiceState1(fpi::SVC_STATUS_ACTIVE, uuid, svc.svc_type);

    ASSERT_TRUE(dbServiceCheck(uuid, fpi::SVC_STATUS_ACTIVE));
    ASSERT_TRUE(svcLayerSvcCheck(uuid, fpi::SVC_STATUS_ACTIVE));

    // Get to STARTED, ACTIVE - STOPPING, STOPPING - STARTED
    modifyServiceState1(fpi::SVC_STATUS_STOPPING, uuid, svc.svc_type);
    modifyServiceState1(fpi::SVC_STATUS_STARTED, uuid, svc.svc_type);


    std::cout << "4(p) Verify good transition STARTED -> INACTIVE_FAILED (db only) \n";
    modifyServiceState1(fpi::SVC_STATUS_INACTIVE_FAILED, uuid, svc.svc_type);

    ASSERT_TRUE(dbServiceCheck(uuid, fpi::SVC_STATUS_INACTIVE_FAILED));
    // SvcLayer will not updated "STARTED" state, which is expected, so this will not hold
    //ASSERT_TRUE(svcLayerSvcCheck(uuid, fpi::SVC_STATUS_INACTIVE_FAILED));

    // Get to INACTIVE_STOPPED, FAILED - ACTIVE (or for svcLayer STOPPING -ACTIVE)
    // , ACTIVE - STOPPING, STOPPING - INACTIVE_STOPPED
    modifyServiceState1(fpi::SVC_STATUS_ACTIVE, uuid, svc.svc_type);
    modifyServiceState1(fpi::SVC_STATUS_STOPPING, uuid, svc.svc_type);
    modifyServiceState1(fpi::SVC_STATUS_INACTIVE_STOPPED, uuid, svc.svc_type);


    std::cout << "4(q) Verify good transition INACTIVE_STOPPED -> REMOVED\n";
    modifyServiceState1(fpi::SVC_STATUS_REMOVED, uuid, svc.svc_type);

    ASSERT_TRUE(dbServiceCheck(uuid, fpi::SVC_STATUS_REMOVED));
    ASSERT_TRUE(svcLayerSvcCheck(uuid, fpi::SVC_STATUS_REMOVED));

    // SWITCHING SERVICE . Test added - removed
    int64_t secondSmUuid = addFakeSvc(fpi::FDSP_STOR_MGR, "sm", true);
    std::cout << "4(r) Verify good transition ADDED -> REMOVED\n";
    modifyServiceState1(fpi::SVC_STATUS_REMOVED, secondSmUuid, fpi::FDSP_STOR_MGR);

    ASSERT_TRUE(dbServiceCheck(secondSmUuid, fpi::SVC_STATUS_REMOVED));
    ASSERT_TRUE(svcLayerSvcCheck(secondSmUuid, fpi::SVC_STATUS_REMOVED));

    ret = dataStore->getSvcInfo(secondSmUuid, svc);
    ASSERT_TRUE(ret);

    // Get to INACTIVE_FAILED so we can test for updated incarnationNo
    modifyServiceState1(fpi::SVC_STATUS_DISCOVERED, secondSmUuid, svc.svc_type);
    modifyServiceState1(fpi::SVC_STATUS_ACTIVE, secondSmUuid, svc.svc_type);
    modifyServiceState1(fpi::SVC_STATUS_INACTIVE_FAILED, secondSmUuid, svc.svc_type);

    std::cout << "4(s) Verify good transition INACTIVE_FAILED -> ACTIVE (updated incarnationNo) \n";
    // Increase the incarnationNo and now test the INACTIVE_FAILED to active
    svc.incarnationNo += 5;
    svc.svc_status = fpi::SVC_STATUS_ACTIVE;
    modifyServiceState2(svc);

    ASSERT_TRUE(dbServiceCheck(secondSmUuid, fpi::SVC_STATUS_ACTIVE));
    ASSERT_TRUE(svcLayerSvcCheck(secondSmUuid, fpi::SVC_STATUS_ACTIVE));


    std::cout << "\nSTEP 5: Attempt bad transitions which should not be allowed\n";

    // SWITCHING SERVICE (back to dm)
    ret = dataStore->getSvcInfo(uuid, svc);
    ASSERT_TRUE(ret);

    ASSERT_TRUE(dbServiceCheck(uuid, fpi::SVC_STATUS_REMOVED));
    ASSERT_TRUE(svcLayerSvcCheck(uuid, fpi::SVC_STATUS_REMOVED));

    auto badTransitions = invalidTransitions[fpi::SVC_STATUS_REMOVED];

    for (auto status : badTransitions)
    {
        std::cout << "Verify bad transition: REMOVED to "
                  << OmExtUtilApi::getInstance()->printSvcStatus(status) << " \n";

        modifyServiceState1(status, uuid, svc.svc_type);

        ASSERT_TRUE(dbServiceCheck(uuid, fpi::SVC_STATUS_REMOVED));
        ASSERT_TRUE(svcLayerSvcCheck(uuid, fpi::SVC_STATUS_REMOVED));

    }

    badTransitions.clear();
    // Let's transition correctly to DISCOVERED then try other illegal transitions
    modifyServiceState1(fpi::SVC_STATUS_DISCOVERED, uuid, svc.svc_type);

    badTransitions = invalidTransitions[fpi::SVC_STATUS_DISCOVERED];

    for (auto status : badTransitions)
    {
        std::cout << "Verify bad transition: DISCOVERED to "
                  << OmExtUtilApi::getInstance()->printSvcStatus(status) << " \n";

        modifyServiceState1(status, uuid, svc.svc_type);

        ASSERT_TRUE(dbServiceCheck(uuid, fpi::SVC_STATUS_DISCOVERED));
        ASSERT_TRUE(svcLayerSvcCheck(uuid, fpi::SVC_STATUS_DISCOVERED));

    }

    badTransitions.clear();
    modifyServiceState1(fpi::SVC_STATUS_ACTIVE, uuid, svc.svc_type);

    badTransitions = invalidTransitions[fpi::SVC_STATUS_ACTIVE];

    for (auto status : badTransitions)
    {
        std::cout << "Verify bad transition: ACTIVE to "
                  << OmExtUtilApi::getInstance()->printSvcStatus(status) << " \n";

        modifyServiceState1(status, uuid, svc.svc_type);

        ASSERT_TRUE(dbServiceCheck(uuid, fpi::SVC_STATUS_ACTIVE));
        ASSERT_TRUE(svcLayerSvcCheck(uuid, fpi::SVC_STATUS_ACTIVE));

    }


    badTransitions.clear();
    // Transition to STOPPING now
    modifyServiceState1(fpi::SVC_STATUS_STOPPING, uuid, svc.svc_type);

    badTransitions = invalidTransitions[fpi::SVC_STATUS_STOPPING];

    for (auto status : badTransitions)
    {
        std::cout << "Verify bad transition: STOPPING to "
                  << OmExtUtilApi::getInstance()->printSvcStatus(status) << " \n";

        modifyServiceState1(status, uuid, svc.svc_type);

        ASSERT_TRUE(dbServiceCheck(uuid, fpi::SVC_STATUS_STOPPING));
        ASSERT_TRUE(svcLayerSvcCheck(uuid, fpi::SVC_STATUS_STOPPING));

    }

    badTransitions.clear();
    // Transition to INACTIVE STOPPING now
    modifyServiceState1(fpi::SVC_STATUS_INACTIVE_STOPPED, uuid, svc.svc_type);

    badTransitions = invalidTransitions[fpi::SVC_STATUS_INACTIVE_STOPPED];

    for (auto status : badTransitions)
    {
        std::cout << "Verify bad transition: INACTIVE_STOPPED to "
                  << OmExtUtilApi::getInstance()->printSvcStatus(status) << " \n";

        modifyServiceState1(status, uuid, svc.svc_type);

        ASSERT_TRUE(dbServiceCheck(uuid, fpi::SVC_STATUS_INACTIVE_STOPPED));
        ASSERT_TRUE(svcLayerSvcCheck(uuid, fpi::SVC_STATUS_INACTIVE_STOPPED));

    }

    badTransitions.clear();
    // Transition to STARTED now
    // It gets a little messy here, because this update will happen only for configDB
    modifyServiceState1(fpi::SVC_STATUS_STARTED, uuid, svc.svc_type);

    badTransitions = invalidTransitions[fpi::SVC_STATUS_STARTED];

    for (auto status : badTransitions)
    {
        std::cout << "Verify bad transition: STARTED to "
                  << OmExtUtilApi::getInstance()->printSvcStatus(status) << " \n";

        modifyServiceState1(status, uuid, svc.svc_type);

        ASSERT_TRUE(dbServiceCheck(uuid, fpi::SVC_STATUS_STARTED));
        //ASSERT_TRUE(svcLayerSvcCheck(uuid, fpi::SVC_STATUS_STARTED));

    }

    // At the end of this, svcLayer is in STOPPING, get back to active, then to failed
    modifyServiceState1(fpi::SVC_STATUS_ACTIVE, uuid, svc.svc_type);
    modifyServiceState1(fpi::SVC_STATUS_INACTIVE_FAILED, uuid, svc.svc_type);

    badTransitions.clear();
    // Transition to INACTIVE_FAILED now
    modifyServiceState1(fpi::SVC_STATUS_INACTIVE_FAILED, uuid, svc.svc_type);

    badTransitions = invalidTransitions[fpi::SVC_STATUS_INACTIVE_FAILED];

    for (auto status : badTransitions)
    {
        std::cout << "Verify bad transition: INACTIVE_FAILED to "
                  << OmExtUtilApi::getInstance()->printSvcStatus(status) << " \n";

        modifyServiceState1(status, uuid, svc.svc_type);

        ASSERT_TRUE(dbServiceCheck(uuid, fpi::SVC_STATUS_INACTIVE_FAILED));
        ASSERT_TRUE(svcLayerSvcCheck(uuid, fpi::SVC_STATUS_INACTIVE_FAILED));

    }

    badTransitions.clear();
    // Transition to STANDBY now (Failed - active, active - standby)
    svc.incarnationNo += 5;
    svc.svc_status = fpi::SVC_STATUS_ACTIVE;
    modifyServiceState2(svc);

    ASSERT_TRUE(dbServiceCheck(uuid, fpi::SVC_STATUS_ACTIVE));
    ASSERT_TRUE(svcLayerSvcCheck(uuid, fpi::SVC_STATUS_ACTIVE));

    modifyServiceState1(fpi::SVC_STATUS_STANDBY, uuid, svc.svc_type);

    ASSERT_TRUE(dbServiceCheck(uuid, fpi::SVC_STATUS_STANDBY));
    ASSERT_TRUE(svcLayerSvcCheck(uuid, fpi::SVC_STATUS_STANDBY));

    badTransitions = invalidTransitions[fpi::SVC_STATUS_STANDBY];

    for (auto status : badTransitions)
    {
        std::cout << "Verify bad transition: STANDBY to "
                  << OmExtUtilApi::getInstance()->printSvcStatus(status) << " \n";

        modifyServiceState1(status, uuid, svc.svc_type);

        ASSERT_TRUE(dbServiceCheck(uuid, fpi::SVC_STATUS_STANDBY));
        ASSERT_TRUE(svcLayerSvcCheck(uuid, fpi::SVC_STATUS_STANDBY));
    }

}


TEST_F(SvcMapFixture, IncarnationChecksTest)
{
    std::cout << "\nVerify valid updates for different incoming, db and svcLayer records\n\n";
    g_fdslog->setSeverityFilter(fds_log::severity_level::debug);

    //+----------------+---------------------+--------------------------+------------------+---------------------------+
    //| dbRecordFound  | svcLayer  | Incoming VS found Record |  Apply incoming? |              Action                 |
    //|                |RecordFound|                          |                  |                                     |
    //+----------------+-------------- -------+--------------------------+------------------+--------------------------+
    //     True        |   False   |   incoming >= current    |      Yes         | Update DB, svcLayer with incoming   |
    //                 |           |   incoming < current     |      No          | Only update svcLayer with DB record |
    //                 |           |   incoming = 0           |      Yes         | Update DB, svcLayer with incoming   |
    //                 |           |                          |                  | (after updating 0 incarnationNo)    |

    std::cout << "TEST CASE: DbRecord present, no svcLayer record\n";
    int64_t smUuid = addFakeSvc(fpi::FDSP_STOR_MGR, "sm", true);
    // the above will update only configDB, now update status to STARTED
    // do this using the non-incoming svc modify function because you would
    // never get a handler/reg path for STARTED state
    modifyServiceState1(fpi::SVC_STATUS_STARTED, smUuid, fpi::FDSP_STOR_MGR);

    fpi::SvcInfo smInfo;
    // Retrieve svcInfo
    dataStore->getSvcInfo(smUuid, smInfo);

    std::cout << " Incoming incarnationNo > current incarnationNo \n";
    smInfo.incarnationNo += 5;
    smInfo.svc_status = fpi::SVC_STATUS_ACTIVE;
    modifyServiceState2(smInfo);

    ASSERT_TRUE(dbServiceCheck(smUuid, fpi::SVC_STATUS_ACTIVE));
    ASSERT_TRUE(svcLayerSvcCheck(smUuid, fpi::SVC_STATUS_ACTIVE));

    // Refresh svcInfo
    dataStore->getSvcInfo(smUuid, smInfo);
    std::cout << " Incoming incarnationNo == current incarnationNo \n";
    smInfo.svc_status = fpi::SVC_STATUS_STOPPING;
    modifyServiceState2(smInfo);

    ASSERT_TRUE(dbServiceCheck(smUuid, fpi::SVC_STATUS_STOPPING));
    ASSERT_TRUE(svcLayerSvcCheck(smUuid, fpi::SVC_STATUS_STOPPING));

    // Refresh svcInfo
    dataStore->getSvcInfo(smUuid, smInfo);
    std::cout << " Incoming incarnationNo < current incarnationNo \n";
    smInfo.incarnationNo -= 5;
    smInfo.svc_status = fpi::SVC_STATUS_ACTIVE;
    modifyServiceState2(smInfo);

    ASSERT_TRUE(dbServiceCheck(smUuid, fpi::SVC_STATUS_STOPPING));
    ASSERT_TRUE(svcLayerSvcCheck(smUuid, fpi::SVC_STATUS_STOPPING));

    // Refresh svcInfo
    dataStore->getSvcInfo(smUuid, smInfo);
    std::cout << " Incoming incarnationNo = 0\n";
    smInfo.incarnationNo = 0;
    smInfo.svc_status = fpi::SVC_STATUS_ACTIVE;
    modifyServiceState2(smInfo);

    ASSERT_TRUE(dbServiceCheck(smUuid, fpi::SVC_STATUS_STOPPING));
    ASSERT_TRUE(svcLayerSvcCheck(smUuid, fpi::SVC_STATUS_STOPPING));

    dataStore->getSvcInfo(smUuid, smInfo);
    // make sure we updated the incarnationNo
    ASSERT_TRUE(smInfo.incarnationNo > 0);

    //+----------------+---------------------+--------------------------+------------------+---------------------------+
    //| dbRecordFound  | svcLayer  | Incoming VS found Record |  Apply incoming? |              Action                 |
    //|                |RecordFound|                          |                  |                                     |
    //+----------------+-------------- -------+--------------------------+------------------+--------------------------+
    //     True        |   True    |   incoming = 0           |      Yes         | Update svcLayer, DB with incoming   |
    //+----------------+-----------+--------------------------+------------------+-------------------------------------+

    std::cout << "\nTEST CASE: DbRecord present, svcLayer record present\n";
    std::cout << " Incoming incarnationNo = 0 \n";
    // Refresh svcInfo
    dataStore->getSvcInfo(smUuid, smInfo);
    smInfo.svc_status    = fpi::SVC_STATUS_STOPPING;
    smInfo.incarnationNo = 0;

    modifyServiceState2(smInfo);

    // Refresh svcInfo
    dataStore->getSvcInfo(smUuid, smInfo);

    ASSERT_TRUE(smInfo.incarnationNo != 0);
    ASSERT_TRUE(dbServiceCheck(smUuid, fpi::SVC_STATUS_STOPPING));
    ASSERT_TRUE(svcLayerSvcCheck(smUuid, fpi::SVC_STATUS_STOPPING));


    //+----------------+---------------------+--------------------------+------------------+---------------------------+
    //| dbRecordFound  | svcLayer  | Incoming VS found Record |  Apply incoming? |              Action                 |
    //|                |RecordFound|                          |                  |                                     |
    //+----------------+-------------- -------+--------------------------+------------------+--------------------------+
    //     True        |   True    |   incoming >= svcLayer   |      Yes         | Update svcLayer, DB with incoming   |
    //                 |           |   svcLayer > configDB    |                  |                                     |

    std::vector<fpi::SvcInfo> entries;
    // Retrieve svcInfo
    dataStore->getSvcInfo(smUuid, smInfo);

    std::cout << " (Incoming incarnationNo = svcLayer incarnationNo) > db incarnationNo \n";
    int32_t newIncarnation = smInfo.incarnationNo + 2;
    smInfo.incarnationNo = newIncarnation;

    entries.push_back(smInfo);

    // Update only svcLayer
    MODULEPROVIDER()->getSvcMgr()->updateSvcMap(entries);

    ASSERT_TRUE(svcLayerSvcCheck(smUuid, fpi::SVC_STATUS_STOPPING, newIncarnation));
    // Change the status now, send out update, incarnationNo is latest already
    smInfo.svc_status = fpi::SVC_STATUS_INACTIVE_STOPPED;

    modifyServiceState2(smInfo);

    ASSERT_TRUE(dbServiceCheck(smUuid, fpi::SVC_STATUS_INACTIVE_STOPPED, newIncarnation));
    ASSERT_TRUE(svcLayerSvcCheck(smUuid, fpi::SVC_STATUS_INACTIVE_STOPPED));

    std::cout << " Incoming incarnationNo > svcLayer incarnationNo > db incarnationNo \n";
    // Refresh svcInfo
    dataStore->getSvcInfo(smUuid, smInfo);
    // Increase incarnationNo again by 2
    smInfo.incarnationNo += 2;

    modifyServiceState2(smInfo);

    ASSERT_TRUE(dbServiceCheck(smUuid, fpi::SVC_STATUS_INACTIVE_STOPPED, smInfo.incarnationNo));
    ASSERT_TRUE(svcLayerSvcCheck(smUuid, fpi::SVC_STATUS_INACTIVE_STOPPED, smInfo.incarnationNo));

    //+----------------+---------------------+--------------------------+------------------+---------------------------+
    //| dbRecordFound  | svcLayer  | Incoming VS found Record |  Apply incoming? |              Action                 |
    //|                |RecordFound|                          |                  |                                     |
    //+----------------+-------------- -------+--------------------------+------------------+--------------------------+
    //   True          |   True    |   incoming < svcLayer    |      No          | DB has most current, update svcLayer|
    //                 |           |   svcLayer < configDB    |                  | with DB record, ignore incoming     |


    std::cout << " Incoming incarnationNo < svcLayer incarnationNo < db incarnationNo \n";
    // Currently we are at svcLayer = configDB, let's up the incarnationNo in the db
    // Refresh svcInfo
    dataStore->getSvcInfo(smUuid, smInfo);
    newIncarnation          = smInfo.incarnationNo + 2;
    smInfo.incarnationNo    = newIncarnation;
    int32_t prevIncarnation = smInfo.incarnationNo;

    dataStore->changeStateSvcMap(boost::make_shared<fpi::SvcInfo>(smInfo));
    ASSERT_TRUE(dbServiceCheck(smUuid, fpi::SVC_STATUS_INACTIVE_STOPPED, newIncarnation));

    // Refresh svcInfo
    dataStore->getSvcInfo(smUuid, smInfo);
    smInfo.incarnationNo -= 2;
    smInfo.svc_status = fpi::SVC_STATUS_ACTIVE;

    modifyServiceState2(smInfo);

    ASSERT_TRUE(dbServiceCheck(smUuid, fpi::SVC_STATUS_INACTIVE_STOPPED, prevIncarnation));
    ASSERT_TRUE(svcLayerSvcCheck(smUuid, fpi::SVC_STATUS_INACTIVE_STOPPED, prevIncarnation));

    //+----------------+---------------------+--------------------------+------------------+---------------------------+
    //| dbRecordFound  | svcLayer  | Incoming VS found Record |  Apply incoming? |              Action                 |
    //|                |RecordFound|                          |                  |                                     |
    //+----------------+-------------- -------+--------------------------+------------------+--------------------------+
    //      True       |    True   |   incoming < svcLayer    |      No          | svcLayer & DB have most current     |
    //                 |           |   svcLayer == configDB   |                  | No updates are necessary            |

    std::cout << " Incoming incarnationNo < (svcLayer incarnationNo = db incarnationNo) \n";
    // Refresh svcInfo
    dataStore->getSvcInfo(smUuid, smInfo);
    prevIncarnation       = smInfo.incarnationNo;
    smInfo.incarnationNo -= 2;
    smInfo.svc_status     = fpi::SVC_STATUS_ACTIVE;

    modifyServiceState2(smInfo);

    ASSERT_TRUE(dbServiceCheck(smUuid, fpi::SVC_STATUS_INACTIVE_STOPPED, prevIncarnation));
    ASSERT_TRUE(svcLayerSvcCheck(smUuid, fpi::SVC_STATUS_INACTIVE_STOPPED, prevIncarnation));

    //+----------------+---------------------+--------------------------+------------------+---------------------------+
    //| dbRecordFound  | svcLayer  | Incoming VS found Record |  Apply incoming? |              Action                 |
    //|                |RecordFound|                          |                  |                                     |
    //+----------------+-------------- -------+--------------------------+------------------+--------------------------+
    //     True        |    True   |   incoming >= svcLayer   |incoming >
    //                                                         configDB? Yes : No| Update with incoming : DB has most  |
    //                 |           |   svcLayer < configDB    |                  | current, update svcLayer with DB    |
    //                                                                             record ignore incoming              |


    std::cout << " (Incoming incarnationNo = svcLayer incarnationNo), Incoming incarnationNo < db incarnationNo) \n";
    // Up the configDB incarnationNo first
    // Refresh svcInfo
    dataStore->getSvcInfo(smUuid, smInfo);
    prevIncarnation      = smInfo.incarnationNo;
    newIncarnation       = smInfo.incarnationNo + 2;
    smInfo.incarnationNo = newIncarnation;

    dataStore->changeStateSvcMap(boost::make_shared<fpi::SvcInfo>(smInfo));
    ASSERT_TRUE(dbServiceCheck(smUuid, fpi::SVC_STATUS_INACTIVE_STOPPED, newIncarnation));

    // Refresh svcInfo
    dataStore->getSvcInfo(smUuid, smInfo);
    smInfo.incarnationNo = prevIncarnation; // == svcLayer incarnation

    modifyServiceState2(smInfo);

    ASSERT_TRUE(dbServiceCheck(smUuid, fpi::SVC_STATUS_INACTIVE_STOPPED, newIncarnation));
    ASSERT_TRUE(svcLayerSvcCheck(smUuid, fpi::SVC_STATUS_INACTIVE_STOPPED, newIncarnation));

    std::cout << " Incoming incarnationNo > svcLayer incarnationNo, Incoming incarnationNo < db incarnationNo) \n";
    // Current state : svcLayer = configDb

    // Up the configDB incarnationNo first
    // Refresh svcInfo
    dataStore->getSvcInfo(smUuid, smInfo);
    prevIncarnation      = smInfo.incarnationNo;
    newIncarnation       = smInfo.incarnationNo + 3;
    smInfo.incarnationNo = newIncarnation;

    dataStore->changeStateSvcMap(boost::make_shared<fpi::SvcInfo>(smInfo));
    ASSERT_TRUE(dbServiceCheck(smUuid, fpi::SVC_STATUS_INACTIVE_STOPPED, newIncarnation));

    // Refresh svcInfo
    dataStore->getSvcInfo(smUuid, smInfo);
    prevIncarnation      = smInfo.incarnationNo;
    newIncarnation       = prevIncarnation - 1;
    smInfo.incarnationNo = newIncarnation;

    modifyServiceState2(smInfo);

    ASSERT_TRUE(dbServiceCheck(smUuid, fpi::SVC_STATUS_INACTIVE_STOPPED, prevIncarnation));
    ASSERT_TRUE(svcLayerSvcCheck(smUuid, fpi::SVC_STATUS_INACTIVE_STOPPED, prevIncarnation));


    //+----------------+---------------------+--------------------------+------------------+---------------------------+
    //| dbRecordFound  | svcLayer  | Incoming VS found Record |  Apply incoming? |              Action                 |
    //|                |RecordFound|                          |                  |                                     |
    //+----------------+-------------- -------+--------------------------+------------------+--------------------------+
    //    True         |   True    |   incoming > svcLayer    |      Yes          | Incoming has the most current       |
    //                 |           |   svcLayer == configDB   |                  |                                     |

    std::cout << " Incoming incarnationNo > (svcLayer incarnationNo = db incarnationNo) \n";
    // Refresh svcInfo
    dataStore->getSvcInfo(smUuid, smInfo);
    prevIncarnation      = smInfo.incarnationNo;
    newIncarnation       = smInfo.incarnationNo + 2;
    smInfo.incarnationNo = newIncarnation;
    smInfo.svc_status    = fpi::SVC_STATUS_ACTIVE;

    modifyServiceState2(smInfo);

    ASSERT_TRUE(dbServiceCheck(smUuid, fpi::SVC_STATUS_ACTIVE, newIncarnation));
    ASSERT_TRUE(svcLayerSvcCheck(smUuid, fpi::SVC_STATUS_ACTIVE, newIncarnation));


    //+----------------+---------------------+--------------------------+------------------+---------------------------+
    //| dbRecordFound  | svcLayer  | Incoming VS found Record |  Apply incoming? |              Action                 |
    //|                |RecordFound|                          |                  |                                     |
    //+----------------+-------------- -------+--------------------------+------------------+--------------------------+
    //      True       |    True   |   incoming == svcLayer   |      Yes         | Incoming has the most current      |
    //                 |           |   svcLayer == configDB   |                  |                                     |

    std::cout << " Incoming incarnationNo = svcLayer incarnationNo = db incarnationNo \n";

    // Refresh svcInfo
    dataStore->getSvcInfo(smUuid, smInfo);
    smInfo.svc_status = fpi::SVC_STATUS_STOPPING;

    modifyServiceState2(smInfo);

    // No incarnation check required
    ASSERT_TRUE(dbServiceCheck(smUuid, fpi::SVC_STATUS_STOPPING));
    ASSERT_TRUE(svcLayerSvcCheck(smUuid, fpi::SVC_STATUS_STOPPING));

    //+----------------+---------------------+--------------------------+------------------+---------------------------+
    //| dbRecordFound  | svcLayer  | Incoming VS found Record |  Apply incoming? |              Action                 |
    //|                |RecordFound|                          |                  |                                     |
    //+----------------+-------------- -------+--------------------------+------------------+--------------------------+
    //       True      |    True   |   incoming < svcLayer    |      No          | SvcLayer has most current, update DB|
    //                 |           |   svcLayer > configDB    |                  | with svcLyr record, ignore incoming |

    std::cout << " Incoming incarnationNo < svcLayer incarnationNo, svcLayer incarnationNo > db incarnationNo \n";

    fpi::SvcUuid svcUuid;
    svcUuid.svc_uuid = smUuid;
    bool ret = MODULEPROVIDER()->getSvcMgr()->getSvcInfo(svcUuid, smInfo);

    prevIncarnation      = smInfo.incarnationNo;
    newIncarnation       = smInfo.incarnationNo + 2;
    smInfo.incarnationNo = newIncarnation;
    smInfo.svc_status    = fpi::SVC_STATUS_INACTIVE_STOPPED;

    entries.push_back(smInfo);

    // Update only svcLayer with a greater incarnationNumber
    MODULEPROVIDER()->getSvcMgr()->updateSvcMap(entries);

    ASSERT_TRUE(svcLayerSvcCheck(smUuid, fpi::SVC_STATUS_INACTIVE_STOPPED, newIncarnation));

    prevIncarnation = smInfo.incarnationNo;
    smInfo.incarnationNo -= 2;

    modifyServiceState2(smInfo);

    ASSERT_TRUE(dbServiceCheck(smUuid, fpi::SVC_STATUS_INACTIVE_STOPPED, prevIncarnation));
    ASSERT_TRUE(svcLayerSvcCheck(smUuid, fpi::SVC_STATUS_INACTIVE_STOPPED, prevIncarnation));



    std::cout << "\nTEST CASE: DbRecord not present, svcLayer record present\n";

    //+----------------+---------------------+--------------------------+------------------+---------------------------+
    //| dbRecordFound  | svcLayer  | Incoming VS found Record |  Apply incoming? |              Action                 |
    //|                |RecordFound|                          |                  |                                     |
    //+----------------+-----------+--------------------------+--------------------------------------------------------+
    //       False     |  True     |   incoming = 0           |      Yes         | Update svcLayer, DB with incoming   |
    //                 |           |                          |                  | (after updating 0 incarnationNo)    |


    std::cout << " Incoming incarnationNo = 0 \n";
    // Delete the record from data store so we go back to the test case
    //ret = dataStore->deleteSvcMap(dmInfo);
    //ASSERT_TRUE(ret);

    uuidCounter += 1;
    NodeUuid uuid(uuidCounter);
    fpi::SvcInfo dmInfo;

    dmInfo.svc_id.svc_uuid.svc_uuid = uuid.uuid_get_val();
    dmInfo.incarnationNo            = util::getTimeStampSeconds();
    dmInfo.svc_type                 = fpi::FDSP_DATA_MGR;
    dmInfo.svc_status               = fpi::SVC_STATUS_ACTIVE;
    dmInfo.name                     = "dm";

    int64_t dmUuid = dmInfo.svc_id.svc_uuid.svc_uuid;
    entries.clear();
    entries.push_back(dmInfo);

    MODULEPROVIDER()->getSvcMgr()->updateSvcMap(entries);

    svcUuid.svc_uuid = dmUuid;
    // Refresh svcInfo
    MODULEPROVIDER()->getSvcMgr()->getSvcInfo(svcUuid, dmInfo);

    ASSERT_TRUE(svcLayerSvcCheck(dmUuid, fpi::SVC_STATUS_ACTIVE));

    dmInfo.incarnationNo = 0;
    dmInfo.svc_status = fpi::SVC_STATUS_ACTIVE;

    modifyServiceState2(dmInfo);

    // Refresh svcInfo
    MODULEPROVIDER()->getSvcMgr()->getSvcInfo(svcUuid, dmInfo);

    ASSERT_TRUE(dmInfo.incarnationNo != 0);
    ASSERT_TRUE(dbServiceCheck(dmUuid, fpi::SVC_STATUS_ACTIVE));
    ASSERT_TRUE(svcLayerSvcCheck(dmUuid, fpi::SVC_STATUS_ACTIVE));

    //+----------------+---------------------+--------------------------+------------------+---------------------------+
    //| dbRecordFound  | svcLayer  | Incoming VS found Record |  Apply incoming? |              Action                 |
    //|                |RecordFound|                          |                  |                                     |
    //+----------------+-----------+--------------------------+--------------------------------------------------------+
    //     False       |   True    |   incoming > current     |      Yes         | Update svcLayer, DB with incoming   |
    std::cout << " Incoming incarnationNo > svcLayer incarnationNo \n";

    svcUuid.svc_uuid = dmUuid;
    // Refresh svcInfo
    MODULEPROVIDER()->getSvcMgr()->getSvcInfo(svcUuid, dmInfo);

    dmInfo.svc_status = fpi::SVC_STATUS_STOPPING;
    prevIncarnation   = dmInfo.incarnationNo;
    dmInfo.incarnationNo += 2;

    modifyServiceState2(dmInfo);

    ASSERT_TRUE(dbServiceCheck(dmUuid, fpi::SVC_STATUS_STOPPING, dmInfo.incarnationNo));
    ASSERT_TRUE(svcLayerSvcCheck(dmUuid, fpi::SVC_STATUS_STOPPING, dmInfo.incarnationNo));

    //+----------------+---------------------+--------------------------+------------------+---------------------------+
    //| dbRecordFound  | svcLayer  | Incoming VS found Record |  Apply incoming? |              Action                 |
    //|                |RecordFound|                          |                  |                                     |
    //+----------------+-----------+--------------------------+--------------------------------------------------------+
    //       False     |   True    |   incoming = current     |      No          | Update DB with svcLayer

    std::cout << " Incoming incarnationNo = svcLayer incarnationNo \n";
    // Delete the record from data store so we go back to the test case
    ret = dataStore->deleteSvcMap(dmInfo);
    ASSERT_TRUE(ret);

    svcUuid.svc_uuid = dmUuid;
    // Refresh svcInfo
    MODULEPROVIDER()->getSvcMgr()->getSvcInfo(svcUuid, dmInfo);

    dmInfo.svc_status = fpi::SVC_STATUS_STOPPING;

    modifyServiceState2(dmInfo);

    ASSERT_TRUE(dbServiceCheck(dmUuid, fpi::SVC_STATUS_STOPPING, dmInfo.incarnationNo));
    ASSERT_TRUE(svcLayerSvcCheck(dmUuid, fpi::SVC_STATUS_STOPPING));

    dmInfo.svc_status = fpi::SVC_STATUS_INACTIVE_STOPPED;

    modifyServiceState2(dmInfo);

    ASSERT_TRUE(dbServiceCheck(dmUuid, fpi::SVC_STATUS_INACTIVE_STOPPED));
    ASSERT_TRUE(svcLayerSvcCheck(dmUuid, fpi::SVC_STATUS_INACTIVE_STOPPED));

    //+----------------+---------------------+--------------------------+------------------+---------------------------+
    //| dbRecordFound  | svcLayer  | Incoming VS found Record |  Apply incoming? |              Action                 |
    //|                |RecordFound|                          |                  |                                     |
    //+----------------+-----------+--------------------------+--------------------------------------------------------+
    //       False     |   True    |    incoming < current    |      No          | Only update DB with svcLayer record |

    std::cout << " Incoming incarnationNo < svcLayer incarnationNo \n";
    // Delete the record from data store so we go back to the test case
    ret = dataStore->deleteSvcMap(dmInfo);
    ASSERT_TRUE(ret);

    svcUuid.svc_uuid = dmUuid;
    // Refresh svcInfo
    MODULEPROVIDER()->getSvcMgr()->getSvcInfo(svcUuid, dmInfo);

    prevIncarnation = dmInfo.incarnationNo;
    dmInfo.incarnationNo -= 2;

    modifyServiceState2(dmInfo);

    ASSERT_TRUE(dbServiceCheck(dmUuid, fpi::SVC_STATUS_INACTIVE_STOPPED, prevIncarnation));
    ASSERT_TRUE(svcLayerSvcCheck(dmUuid, fpi::SVC_STATUS_INACTIVE_STOPPED, prevIncarnation));

    //+----------------+---------------------+--------------------------+------------------+---------------------------+
    //| dbRecordFound  | svcLayer  | Incoming VS found Record |  Apply incoming? |              Action                 |
    //|                |RecordFound|                          |                  |                                     |
    //+----------------+-----------+--------------------------+--------------------------------------------------------+
    //     False       |   False   |   only incoming is valid |      Yes         | Check incoming incarnationNo, if    |
    //                 |           |                          |                  | 0, update, then update DB & svcLayer|
    //+----------------+-----------+--------------------------+--------------------------------------------------------+

    std::cout << "\nTEST CASE: No db record, or svcLayer record found \n";
    uuidCounter += 1;
    NodeUuid amuuid(uuidCounter);
    fpi::SvcInfo amInfo;

    amInfo.svc_id.svc_uuid.svc_uuid = amuuid.uuid_get_val();
    amInfo.incarnationNo            = 0;
    amInfo.svc_type                 = fpi::FDSP_ACCESS_MGR;
    amInfo.svc_status               = fpi::SVC_STATUS_ACTIVE;
    amInfo.name                     = "am";

    modifyServiceState2(amInfo);

    svcUuid.svc_uuid = amInfo.svc_id.svc_uuid.svc_uuid;
    // Refresh svcInfo
    MODULEPROVIDER()->getSvcMgr()->getSvcInfo(svcUuid, amInfo);

    ASSERT_TRUE(amInfo.incarnationNo != 0);

    // Refresh svcInfo
    dataStore->getSvcInfo(svcUuid.svc_uuid, amInfo);

    ASSERT_TRUE(amInfo.incarnationNo != 0);

    ASSERT_TRUE(dbServiceCheck(amInfo.svc_id.svc_uuid.svc_uuid, fpi::SVC_STATUS_ACTIVE));
    ASSERT_TRUE(svcLayerSvcCheck(amInfo.svc_id.svc_uuid.svc_uuid, fpi::SVC_STATUS_ACTIVE));

}


} // namespace fds

int main(int argc, char *argv[])
{
    ::testing::InitGoogleTest(&argc, argv);

    fds::g_fdslog = new fds::fds_log("svcmapsupdatetest");
    po::options_description opts("Allowed options");
    opts.add_options()
        ("help", "produce help message")
        ("puts-cnt", po::value<int>()->default_value(1), "puts count");

    fds::SvcMapFixture::init(argc, argv, opts);

    return RUN_ALL_TESTS();
}
