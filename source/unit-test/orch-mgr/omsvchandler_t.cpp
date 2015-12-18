/**
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <unistd.h>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <om-svc-handler.h>
#include <orchMgr.h>
#include <orch-mgr/om-service.h>

// Get template using fake data store; don't do this outside of test fixtures
#include <orch-mgr/om-common/om-svc-handler.cpp>
namespace fds {

extern OrchMgr *orchMgr;
OM_Module *omModule;

OM_Module *OM_Module::om_singleton()
{
    return omModule;
}

/**
 * @brief A fake data store
 * @details
 * Inherits from no other, but must define interfaces used by OmSvcHandler<DataStoreT>.
 */
class FakeDataStore {

public:
    kvstore::ConfigDB::ReturnType getLocalDomain(fds_ldomid_t id, LocalDomain& localDomain);
    bool getSvcMap(std::vector<fpi::SvcInfo>& records);
};

kvstore::ConfigDB::ReturnType FakeDataStore::getLocalDomain(int32_t id, LocalDomain& ld) {

    ld.setID(id);
    ld.setName("fakeDomain1");
    ld.setSite("fakeSite1");
    ld.setCurrent(true);

    return kvstore::ConfigDB::ReturnType::SUCCESS;
}

bool FakeDataStore::getSvcMap(std::vector<fpi::SvcInfo>& records) {

    fpi::SvcInfo record1;
    record1.name = "fakeServiceName1";
    record1.ip = "127.0.0.1";
    record1.svc_port = 7400;
    record1.svc_type = fpi::FDSP_ACCESS_MGR;
    records.push_back(record1);

    fpi::SvcInfo record2;
    record2.name = "fakeServiceName2";
    record2.ip = "127.0.0.1";
    record2.svc_port = 7401;
    record2.svc_type = fpi::FDSP_DATA_MGR;
    records.push_back(record2);
    return true;
}

} // namespace fds

// Get a list of AM service endpoints for a remote local domain
TEST(GetSvcEndpoints, Nominal) {

    fds::FakeDataStore dataStore;
    fds::OmSvcHandler<fds::FakeDataStore> dut(nullptr);
    dut.setConfigDB(&dataStore);

    std::vector<fpi::FDSP_Node_Info_Type> records;
    fpi::FDSP_MgrIdType svctype = fpi::FDSP_ACCESS_MGR;
    int32_t localDomainId = 2;

    auto pArg1 = boost::make_shared<fpi::FDSP_MgrIdType>(svctype);
    auto pArg2 = boost::make_shared<int32_t>(localDomainId);
    dut.getSvcEndpoints(records, pArg1, pArg2);

    EXPECT_EQ(1, records.size()) << "Incorrect size for service record list";
}

int main(int argc, char * argv[]) {

    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}
