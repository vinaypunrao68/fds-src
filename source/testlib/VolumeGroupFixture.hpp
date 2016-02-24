/*
 * Copyright 2016 Formation Data Systems, Inc.
 */

#ifndef TESTLIB_VOLUMEGROUPFIXTURE_HPP_
#define TESTLIB_VOLUMEGROUPFIXTURE_HPP_

#include <testlib/DmGroupFixture.hpp>

// Hack to allow VolumeGroupFixture to access private members
// FRIEND_TEST() macro doesn't work for non-test functions
#define private public
#include <VolumeChecker.h>
#undef private

struct VolumeGroupFixture : DmGroupFixture {

    VolumeGroupFixture() {}
    using VcHandle = ProcessHandle<VolumeChecker>;
    VcHandle   vcHandle;

    void addDMTToVC(DMTPtr DMT, unsigned clusterSize) {
        ASSERT_TRUE(vcHandle.isRunning());
        std::string dmtData;
        dmt->getSerialized(dmtData);
        Error e = vcHandle.proc->getSvcMgr()->getDmtManager()->addSerializedDMT(dmtData,
                                                                          nullptr,
                                                                          DMT_COMMITTED);
        ASSERT_TRUE(e == ERR_OK);
        auto dmt = vcHandle.proc->getSvcMgr()->getDmtManager()->getDMT(DMT_COMMITTED);
        auto svcUuidVector = dmt->getSvcUuids(fds_volid_t(v1Id));
        ASSERT_TRUE(svcUuidVector.size() == clusterSize);
    }

    void initVolumeChecker(std::vector<unsigned> volIdList, unsigned clusterSize) {
        auto roots = getRootDirectories();

        std::string volListString = "-v=";

        // For now, we only have one volume
        volListString += std::to_string(volIdList[0]);

        // As volume checker, we init as an AM
        vcHandle.start({"checker",
                       roots[0],
                       "--fds.pm.platform_uuid=2059",
                       "--fds.pm.platform_port=9861",
                       volListString
                       });

        ASSERT_FALSE(vcHandle.proc->getStatus() == fds::VolumeChecker::VC_NOT_STARTED);

        // Phase 1 test
        ASSERT_TRUE(vcHandle.proc->getStatus() == fds::VolumeChecker::VC_PHASE_1);

        // dmCheckerList should have 1 volume element in it
        ASSERT_TRUE(vcHandle.proc->dmCheckerList.size() == 1);
        // That one should have "clusterSize" in it
        ASSERT_TRUE(vcHandle.proc->dmCheckerList[0].second.size() == clusterSize);

        // The checker should have sent msgs to all the volumes
        for (auto dmChecker : vcHandle.proc->dmCheckerList) {
            for (auto oneDMtoCheck : dmChecker.second) {
                ASSERT_TRUE(oneDMtoCheck.status == fds::VolumeChecker::dmCheckerMetaData::NS_CONTACTED);
            }
        }

    }

    void stopVolumeChecker() {
        vcHandle.stop();
    }

};



#endif /* TESTLIB_VOLUMEGROUPFIXTURE_HPP_ */
