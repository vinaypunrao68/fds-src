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

namespace FDS_ProtocolInterface {

/**
 * @details
 *
 */
class FakeOMSvcClient : public OMSvcClient {

public:
    FakeOMSvcClient() : OMSvcClient(nullptr) {}
    void getSvcEndpoints(std::vector< ::FDS_ProtocolInterface::FDSP_Node_Info_Type>& _return,
        boost::shared_ptr< ::FDS_ProtocolInterface::FDSP_MgrIdType>& svctype,
        boost::shared_ptr<int32_t>& localDomainId);
};

void FakeOMSvcClient::getSvcEndpoints(std::vector< ::FDS_ProtocolInterface::FDSP_Node_Info_Type>& _return,
        boost::shared_ptr< ::FDS_ProtocolInterface::FDSP_MgrIdType>& svctype,
        boost::shared_ptr<int32_t>& localDomainId) {

    fpi::FDSP_Node_Info_Type record1;
    record1.node_name = "fakeServiceNameA";
    record1.ip_lo_addr = 168035329;
    record1.control_port = 7400;
    record1.node_type = *svctype;
    _return.push_back(record1);

    fpi::FDSP_Node_Info_Type record2;
    record2.node_name = "fakeServiceNameA";
    record2.ip_lo_addr = 168034817;
    record2.control_port = 7400;
    record2.node_type = *svctype;
    _return.push_back(record2);
}

} // namespace FDS_ProtocolInterface

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
    bool setCapacityUsedNode(const int64_t svcUuid, const unsigned long usedCapacityInBytes) {
        return true;
    }
};

kvstore::ConfigDB::ReturnType FakeDataStore::getLocalDomain(int32_t id, LocalDomain& ld) {

    switch (id) {
        case 1:
        {
            ld.setID(id);
            ld.setName("fakeDomain1");
            ld.setSite("fakeSite1");
            ld.setCurrent(true);
        }
        break;
        case 2:
        {
            ld.setID(id);
            ld.setName("fakeDomain2");
            ld.setSite("fakeSite2");
            ld.setCurrent(false);
            fpi::FDSP_RegisterNodeType n;
            n.node_type = fpi::FDSP_ORCH_MGR;
            n.domain_id = id;
            n.ip_lo_addr = 2130706433;
            n.control_port = 7402;
            ld.setOMNode(n);
        }
        break;
        default: return kvstore::ConfigDB::ReturnType::NOT_FOUND;
    }
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

/**
 * @details
 * Same as OmSvcHandler, except createOmSvcClient creates a fake.
 * Enables use of fake RPC when testing the base OmSvcHandler.
 * Satisfies the requirement that the unit test should not have
 * cross-process dependencies.
 */
template<class DataStoreT>
class FakeOmSvcHandler : public OmSvcHandler<DataStoreT> {

public:
    explicit FakeOmSvcHandler(CommonModuleProviderIf *provider);
    virtual ~FakeOmSvcHandler() {}

protected:
    fpi::OMSvcClientPtr createOMSvcClient(const std::string& strIPAddress,
        const int32_t& port) override;
};

template <class DataStoreT>
FakeOmSvcHandler<DataStoreT>::FakeOmSvcHandler(CommonModuleProviderIf *provider)
: OmSvcHandler<DataStoreT>(provider) {
}

template <class DataStoreT>
fpi::OMSvcClientPtr FakeOmSvcHandler<DataStoreT>::createOMSvcClient(
    const std::string& strIPAddress,
    const int32_t& port) {

    return boost::make_shared< ::FDS_ProtocolInterface::FakeOMSvcClient>();
}

} // namespace fds

// Get a list of AM service endpoints for current service domain
TEST(GetSvcEndpoints, NominalLocal) {

    fds::FakeDataStore dataStore;
    fds::OmSvcHandler<fds::FakeDataStore> dut(nullptr);
    dut.setConfigDB(&dataStore);

    std::vector<fpi::FDSP_Node_Info_Type> records;
    fpi::FDSP_MgrIdType svctype = fpi::FDSP_ACCESS_MGR;
    int32_t localDomainId = 1;

    auto pArg1 = boost::make_shared<fpi::FDSP_MgrIdType>(svctype);
    auto pArg2 = boost::make_shared<int32_t>(localDomainId);
    dut.getSvcEndpoints(records, pArg1, pArg2);

    EXPECT_EQ(1, records.size()) << "Incorrect size for service record list";
}

// Get a list of AM service endpoints for a remote service domain
TEST(GetSvcEndpoints, NominalRemote) {

    // FDS process depends on global logger. This dependency prevents true
    // isolation testing.
    // Initialize logging (logptr, g_fdslog).
    // Host volume must have permissions in /fds to log test.
    fds::fds_log *temp_log = new fds::fds_log("fds", "logs");
    temp_log->setSeverityFilter(
        fds::fds_log::getLevelFromName("DEBUG"));
    fds::g_fdslog = temp_log;
    GLOGNORMAL << "omsvchandler_gtest";

    fds::FakeDataStore dataStore;
    fds::FakeOmSvcHandler<fds::FakeDataStore> dut(nullptr);
    dut.setConfigDB(&dataStore);

    std::vector<fpi::FDSP_Node_Info_Type> records;
    fpi::FDSP_MgrIdType svctype = fpi::FDSP_ACCESS_MGR;
    int32_t localDomainId = 2;

    auto pArg1 = boost::make_shared<fpi::FDSP_MgrIdType>(svctype);
    auto pArg2 = boost::make_shared<int32_t>(localDomainId);
    dut.getSvcEndpoints(records, pArg1, pArg2);

    EXPECT_EQ(2, records.size()) << "Incorrect size for service record list";
}

int main(int argc, char * argv[]) {

    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}
