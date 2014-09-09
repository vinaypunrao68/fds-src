#define CATCH_CONFIG_MAIN
#include "../catch/catch.hpp"

#include <kvstore/configdb.h>
#include <util/Log.h>
#include <util/stringutils.h>
#include <dlt.h>
#include "../dlt/dlt_helper_funcs.h"
#include <vector>
#include <map>
#include <string>

using namespace fds; //NOLINT
using namespace fds::kvstore; //NOLINT

int globalDomainId = 100;

TEST_CASE("connection"){
    ConfigDB cfg;
    REQUIRE(cfg.isConnected());
}


TEST_CASE("simple"){
    ConfigDB cfg;
    int localDomain  = 1001;

    cfg.setGlobalDomain("fds");
    REQUIRE(cfg.getGlobalDomain()== "fds");

    REQUIRE(cfg.addLocalDomain("local", 1000, globalDomainId));
    REQUIRE(cfg.addLocalDomain("local2", 1001, globalDomainId));

    std::map<int, std::string> mapDomains;

    cfg.getLocalDomains(mapDomains, globalDomainId);

    REQUIRE(mapDomains.size()== 2);
    REQUIRE(mapDomains[1000] == "local");
    REQUIRE(mapDomains[1001] == "local2");
}

TEST_CASE("volume"){
    ConfigDB cfg;
    int localDomain = 1001;

    cfg.deleteVolume(20);

    VolumeDesc vol("vol", 20), vol1("vol1", 21);

    vol.volUUID = 20;
    vol.tennantId = 3;
    vol.localDomainId = localDomain;
    vol.globDomainId = 0;
    vol.volType =(fpi::FDSP_VolType)1;
    vol.capacity= 10.0;

    cfg.addVolume(vol);
    cfg.getVolume(20, vol1);

    REQUIRE(vol1.volUUID == 20);
    REQUIRE(vol1.tennantId == 3);
    REQUIRE(vol1.localDomainId == localDomain);
    REQUIRE(vol1.volType ==(fpi::FDSP_VolType)1);
    REQUIRE(vol1.capacity == 10.0);

    std::vector<VolumeDesc> vecVolumes;

    cfg.getVolumes(vecVolumes, localDomain);

    REQUIRE(vecVolumes.size()== 1);
    REQUIRE(vecVolumes[0].volUUID == 20);
}

TEST_CASE("dlt"){
    int localDomain = 1001;
    ConfigDB cfg;
    DLT dlt(3, 4, 1, true), dlt1(0, 0, 0, false);

    CAPTURE(dlt.getNumBitsForToken());
    CAPTURE(dlt.getDepth());
    CAPTURE(dlt.getNumTokens());

    fillDltNodes(&dlt, 10);

    cfg.storeDlt(dlt, "", localDomain);
    cfg.getDlt(dlt1, 1, localDomain);
    verifyDltNodes(&dlt1, 10);
}

TEST_CASE("tenant"){
    ConfigDB cfg;

    int64_t id1 = cfg.createTenant("testtenant1");
    int64_t id2 = cfg.createTenant("testtenant2");

    std::vector<fds::apis::Tenant> tenants;
    cfg.listTenants(tenants);

    REQUIRE(tenants.size() == 2);
    REQUIRE(tenants[0].identifier == "testtenant1");
}

TEST_CASE("user"){
    ConfigDB cfg;

    int64_t id1 = cfg.createUser("testuser1", "hash", "secret1", false);
    int64_t id2 = cfg.createUser("testuser2", "hash", "secret2", true);

    std::vector<fds::apis::User> users;
    cfg.listUsers(users);

    REQUIRE(users.size() == 2);
    REQUIRE(users[0].identifier == "testuser1");

    fds::apis::User user;
    cfg.getUser(id2, user);

    REQUIRE(user.identifier == "testuser2");

    int64_t tenantid = cfg.createTenant("testtenant1");
    cfg.assignUserToTenant(id1, tenantid);

    users.clear();
    std::vector<fds::apis::Tenant> tenants;
    cfg.listUsersForTenant(users, tenantid);

    REQUIRE(users.size() == 1);
    REQUIRE(users[0].identifier == "testuser1");

    users.clear();

    cfg.revokeUserFromTenant(id1, tenantid);
    cfg.listUsersForTenant(users, tenantid);
    REQUIRE(users.size() == 0);
}

TEST_CASE("utils") {
    std::string name = "Formation Data";
    std::string lower = fds::util::strlower(name);

    REQUIRE(lower == "formation data");

    std::string output;
    output = fds::util::strformat("name is %s", "fds");
    REQUIRE(output == "name is fds");

    output = fds::util::strformat("sum of %d and %d = %ld", 1, 2, 3);
    REQUIRE(output == "sum of 1 and 2 = 3");
}

TEST_CASE("snapshot") {
    fpi::SnapshotPolicy policy;
    policy.policyName = "testsnapshotpolicy";
    ConfigDB cfg;
    cfg.createSnapshotPolicy(policy);

    fpi::Snapshot snapshot;
    snapshot.snapshotName = "testss";
    snapshot.snapshotId = 2;
    snapshot.volumeId =  1;
    cfg.createSnapshot(snapshot);
    std::vector<fpi::Snapshot> vecSnapshots;
    cfg.listSnapshots(vecSnapshots, 1);

    REQUIRE(vecSnapshots.size() == 1);
    REQUIRE(vecSnapshots[0].snapshotName == "testss");
}
