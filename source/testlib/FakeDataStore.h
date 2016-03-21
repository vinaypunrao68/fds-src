/*
 * Copyright 2016 Formation Data Systems, Inc.
 */

#ifndef TESTLIB_FAKEDATASTORE_H_
#define TESTLIB_FAKEDATASTORE_H_


#include <orchMgr.h>
#include <OmExtUtilApi.h>

namespace fds
{
/**
 * @brief A fake data store
 * @details
 * Inherits from no other, but must define interfaces used by
 * ConfigurationServiceHandler<DataStoreT>.
 */
class FakeDataStore {

private:
    std::vector<fpi::SvcInfo> fakeSvcMap;
public:

    FakeDataStore() { fakeSvcMap = {};}
    ~FakeDataStore() {}

    fds_uint64_t getLastModTimeStamp() { return 0; }

    /******************************************************************************
     *                      Local Domain Section
     *****************************************************************************/
    fds_ldomid_t getNewLocalDomainId() {
        static fds_ldomid_t d = 0;
        return ++d;
    }
    fds_ldomid_t putLocalDomain(const LocalDomain& localDomain, const bool isNew = true) {
        return 0;
    }
    fds_ldomid_t addLocalDomain(const LocalDomain& localDomain) {
        return 0;
    }
    kvstore::ConfigDB::ReturnType updateLocalDomain(const LocalDomain& localDomain) {
        return kvstore::ConfigDB::ReturnType::NOT_UPDATED;
    }
    kvstore::ConfigDB::ReturnType deleteLocalDomain(fds_ldomid_t id) {
        return kvstore::ConfigDB::ReturnType::NOT_UPDATED;
    }
    kvstore::ConfigDB::ReturnType localDomainExists(fds_ldomid_t id) {
        return kvstore::ConfigDB::ReturnType::NOT_FOUND;
    }
    kvstore::ConfigDB::ReturnType localDomainExists(const std::string& name) {
        return kvstore::ConfigDB::ReturnType::NOT_FOUND;
    }
    kvstore::ConfigDB::ReturnType getLocalDomainIds(std::vector<fds_ldomid_t>&ldomIds) {
        return kvstore::ConfigDB::ReturnType::NOT_FOUND;
    }
    fds_ldomid_t getLocalDomainId(const std::string& name) {
        return 0;
    }
    kvstore::ConfigDB::ReturnType listLocalDomains(std::vector<LocalDomain>& localDomains) {
        return kvstore::ConfigDB::ReturnType::NOT_FOUND;
    }
    kvstore::ConfigDB::ReturnType getLocalDomains(std::vector<LocalDomain>& localDomains) {
        return kvstore::ConfigDB::ReturnType::NOT_FOUND;
    }
    kvstore::ConfigDB::ReturnType getLocalDomain(fds_ldomid_t id, LocalDomain& localDomain) {
        return kvstore::ConfigDB::ReturnType::NOT_FOUND;
    }
    kvstore::ConfigDB::ReturnType getLocalDomain(const std::string& name, LocalDomain& localDomain) {
        return kvstore::ConfigDB::ReturnType::NOT_FOUND;
    }

    /******************************************************************************
     *                      Volume Section
     *****************************************************************************/
    fds_volid_t getNewVolumeId() {
        static int v = 0;
        return fds::fds_value_type<uint64_t>(++v);
    }
    bool setVolumeState(fds_volid_t volumeId, fpi::ResourceState state) {
        return false;
    }
    bool addVolume(const VolumeDesc& volumeDesc) {
        return false;
    }
    bool updateVolume(const VolumeDesc& volumeDesc) {
        return false;
    }
    bool deleteVolume(fds_volid_t volumeId, int localDomain = 0) {
        return false;
    }
    bool volumeExists(fds_volid_t volumeId) {
        return false;
    }
    bool volumeExists(ConstString volumeName, int localDomain = 0) {
        return false;
    }
    bool getVolumeIds(std::vector<fds_volid_t>& volumeIds, int localDomain = 0) {
        return false;
    }
    bool getVolumes(std::vector<VolumeDesc>& volumes, int localDomain = 0) {
        return false;
    }
    bool getVolume(fds_volid_t volumeId, VolumeDesc& volumeDesc) {
        return false;
    }

    /******************************************************************************
     *                      Service Map Section
     *****************************************************************************/
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

        //std::cout << "ConfigDB updated Service Map:"
        //         << " type: " << svcInfoPtr->svc_type
        //         << " uuid: " << std::hex << svcInfoPtr->svc_id.svc_uuid.svc_uuid << std::dec
        //         << " incarnation: " << svcInfoPtr->incarnationNo
        //         << " status: " << OmExtUtilApi::getInstance()->printSvcStatus(svcInfoPtr->svc_status) << "\n";

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

    /******************************************************************************
     *                      Volume Policy Section
     *****************************************************************************/
    fds_uint32_t createQoSPolicy(const std::string& identifier,
                                 const fds_uint64_t minIops, const fds_uint64_t maxIops, const fds_uint32_t relPrio);
    fds_uint32_t getIdOfQoSPolicy(const std::string& identifier) {
        return 0;
    }
    bool getPolicy(fds_uint32_t volPolicyId, FDS_VolumePolicy& volumePolicy, int localDomain = 0) { //NOLINT
        return false;
    }
    bool addPolicy(const FDS_VolumePolicy& volumePolicy, int localDomain = 0) {
        return false;
    }
    bool updatePolicy(const FDS_VolumePolicy& volumePolicy, int localDomain = 0) {
        return false;
    }
    bool deletePolicy(fds_uint32_t volPolicyId, int localDomain = 0) {
        return false;
    }
    bool getPolicies(std::vector<FDS_VolumePolicy>& policies, int localDomain = 0) {
        return false;
    }

    /******************************************************************************
     *                      Stat Stream Registrations Section
     *****************************************************************************/
    int32_t getNewStreamRegistrationId() {
        static int32_t i = 0;
        return ++i;
    }
    bool addStreamRegistration(apis::StreamingRegistrationMsg& streamReg) {
        return false;
    }
    bool removeStreamRegistration(int regId) {
        return false;
    }
    bool getStreamRegistration(int regId, apis::StreamingRegistrationMsg& streamReg) {
        return false;
    }
    bool getStreamRegistrations(std::vector<apis::StreamingRegistrationMsg>& vecReg) {
        return false;
    }

    /******************************************************************************
     *                      Tenant Section
     *****************************************************************************/
    int64_t createTenant(const std::string& identifier) {
        static int64_t t = 0;
        return ++t;
    }
    bool listTenants(std::vector<fds::apis::Tenant>& tenants) {
        return false;
    }
    int64_t createUser(const std::string& identifier, const std::string& passwordHash, const std::string& secret, bool isAdmin) { //NOLINT
        static int64_t u = 0;
        return ++u;
    }
    bool getUser(int64_t userId, fds::apis::User& user) {
        return false;
    }
    bool listUsers(std::vector<fds::apis::User>& users) {
        return false;
    }
    bool assignUserToTenant(int64_t userId, int64_t tenantId) {
        return false;
    }
    bool revokeUserFromTenant(int64_t userId, int64_t tenantId) {
        return false;
    }
    bool listUsersForTenant(std::vector<fds::apis::User>& users, int64_t tenantId) {
        return false;
    }
    bool updateUser(int64_t  userId, const std::string& identifier, const std::string& passwordHash, const std::string& secret, bool isFdsAdmin) { //NOLINT
        return false;
    }

    /******************************************************************************
     *                      Snapshot Section
     *****************************************************************************/
    bool createSnapshotPolicy(fds::apis::SnapshotPolicy& policy) { //NOLINT
        return false;
    }
    bool getSnapshotPolicy(int64_t policyid, fds::apis::SnapshotPolicy& policy) {
        return false;
    }
    bool listSnapshotPolicies(std::vector<fds::apis::SnapshotPolicy> & _return) { //NOLINT
        return false;
    }
    bool deleteSnapshotPolicy(const int64_t id) { //NOLINT
        return false;
    }
    bool attachSnapshotPolicy(fds_volid_t const volumeId, const int64_t policyId) { //NOLINT
        return false;
    }
    bool listSnapshotPoliciesForVolume(std::vector<fds::apis::SnapshotPolicy> & _return, fds_volid_t const volumeId) { //NOLINT
        return false;
    }
    bool detachSnapshotPolicy(fds_volid_t const volumeId, const int64_t policyId) { //NOLINT
        return false;
    }
    bool listVolumesForSnapshotPolicy(std::vector<int64_t> & _return, const int64_t policyId) { //NOLINT
        return false;
    }

    bool createSnapshot(fpi::Snapshot& snapshot) {
        return false;
    }
    bool updateSnapshot(const fpi::Snapshot& snapshot) {
        return false;
    }

    // volumeid & snapshotid should be set ...
    bool getSnapshot(fpi::Snapshot& snapshot) {
        return false;
    }
    bool deleteSnapshot(fds_volid_t const volumeId, fds_volid_t const snapshotId) {
        return false;
    }
    bool setSnapshotState(fpi::Snapshot& snapshot , fpi::ResourceState state) {
        return false;
    }
    bool setSnapshotState(fds_volid_t const volumeId, fds_volid_t const snapshotId, fpi::ResourceState state) { //NOLINT
        return false;
    }
    bool listSnapshots(std::vector<fpi::Snapshot> & _return, fds_volid_t const volumeId) { //NOLINT
        return false;
    }

    /******************************************************************************
     *                      Subscription Section
     *****************************************************************************/
    fds_subid_t getNewSubscriptionId() {
        static fds_subid_t s = 0;
        return ++s;
    }
    kvstore::ConfigDB::ReturnType setSubscriptionState(fds_subid_t id, fpi::ResourceState state) {
        return kvstore::ConfigDB::ReturnType::NOT_UPDATED;
    }
    fds_subid_t putSubscription(const Subscription &subscription, const bool isNew = true) {
        static fds_subid_t s = 0;
        return ++s;
    }
    kvstore::ConfigDB::ReturnType updateSubscription(const Subscription& subscription) {
        return kvstore::ConfigDB::ReturnType::NOT_UPDATED;
    }
    kvstore::ConfigDB::ReturnType deleteSubscription(fds_subid_t id) {
        return kvstore::ConfigDB::ReturnType::NOT_UPDATED;
    }
    kvstore::ConfigDB::ReturnType subscriptionExists(fds_subid_t id) {
        return kvstore::ConfigDB::ReturnType::NOT_FOUND;
    }
    kvstore::ConfigDB::ReturnType subscriptionExists(const std::string& name, const std::int64_t tenantId) {
        return kvstore::ConfigDB::ReturnType::NOT_FOUND;
    }
    kvstore::ConfigDB::ReturnType getSubscriptionIds(std::vector<fds_subid_t>& ids) {
        return kvstore::ConfigDB::ReturnType::NOT_FOUND;
    }
    fds_subid_t getSubscriptionId(const std::string& name, const std::int64_t tenantId) {
        return 0;
    }
    kvstore::ConfigDB::ReturnType getSubscriptions(std::vector<Subscription>& subscriptions) {
        return kvstore::ConfigDB::ReturnType::NOT_FOUND;
    }
    kvstore::ConfigDB::ReturnType getTenantSubscriptions(std::int64_t tenantId, std::vector<Subscription>& subscriptions) {
        return kvstore::ConfigDB::ReturnType::NOT_FOUND;
    }
    kvstore::ConfigDB::ReturnType getSubscription(fds_subid_t id, Subscription& subscription) {
        return kvstore::ConfigDB::ReturnType::NOT_FOUND;
    }
    kvstore::ConfigDB::ReturnType getSubscription(const std::string& name, const std::int64_t tenantId, Subscription& subscription) {
        return kvstore::ConfigDB::ReturnType::NOT_FOUND;
    }
};

#endif // TESTLIB_FAKEDATASTORE_H_
} // namespace fds
