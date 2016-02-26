/*
 * Copyright 2016 Formation Data Systems, Inc.
 */

#ifndef TESTLIB_VOLUMEGROUPFIXTURE_HPP_
#define TESTLIB_VOLUMEGROUPFIXTURE_HPP_

#include <testlib/DmGroupFixture.hpp>
#include <VolumeChecker.h>

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

    void runVolumeChecker(std::vector<unsigned> volIdList, unsigned clusterSize) {
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
        ASSERT_TRUE(vcHandle.proc->getStatus() == fds::VolumeChecker::VC_DM_HASHING);

        // vgCheckerList should have 1 volume element in it
        ASSERT_TRUE(vcHandle.proc->testGetVgCheckerListSize() == 1);
        // That one should have "clusterSize" in it
        ASSERT_TRUE(vcHandle.proc->testGetVgCheckerListSize(0) == clusterSize);

        // The checker should have sent msgs to all the volumes
        // See the chkNodeStatus code for match
        ASSERT_TRUE(vcHandle.proc->testVerifyCheckerListStatus(1));
    }

    void stopVolumeChecker() {
        vcHandle.stop();
    }

};



#endif /* TESTLIB_VOLUMEGROUPFIXTURE_HPP_ */
