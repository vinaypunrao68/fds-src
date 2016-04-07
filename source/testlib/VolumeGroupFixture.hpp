/*
 * Copyright 2016 Formation Data Systems, Inc.
 */

#ifndef TESTLIB_VOLUMEGROUPFIXTURE_HPP_
#define TESTLIB_VOLUMEGROUPFIXTURE_HPP_

#include <testlib/DmGroupFixture.hpp>
#include <VolumeChecker.h>

struct VolumeGroupFixture : DmGroupFixture {
    using VcHandle = ProcessHandle<VolumeChecker>;
    VolumeGroupFixture() {
    }

    void startAm2()
    {
        amHandle2.start({"am",
                        roots[1],
                        util::strformat("--fds.pm.platform_uuid=%d", getPlatformUuid(1)),
                        util::strformat("--fds.pm.platform_port=%d", getPlatformPort(1)),
                        "--fds.am.threadpool.num_threads=3"
                        });
    }

    SHPTR<VolumeGroupHandle> setupVolumeGroupHandleOnAm2(uint32_t quorumCnt)
    {
        /* Set up the volume on om and dms */
        if (!v1Desc) {
            createVolumeV1();
        }
        Error e = amHandle2.proc->getSvcMgr()->getDmtManager()->addSerializedDMT(dmtData,
                                                                                 nullptr,
                                                                                 DMT_COMMITTED);
        EXPECT_TRUE(e == ERR_OK);

        /* Create a volumegroup handle with quorum of quorumCnt */
        auto volumeHandle = MAKE_SHARED<VolumeGroupHandle>(amHandle2.proc, v1Id, quorumCnt);
        amHandle2.proc->setVolumeHandle(volumeHandle.get());

        return volumeHandle;
    }

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
        fds_volid_t volId0(volIdList[0]);

        if (g_fdsprocess == NULL) {
            g_fdsprocess = omHandle.proc;
        }
        // As volume checker, we init as an AM
        vcHandle.start({"checker",
                       roots[0],
                       "--fds.pm.platform_uuid=2059",
                       "--fds.pm.platform_port=9861",
                       volListString
                       });

        // Phase 1 test
        ASSERT_FALSE(vcHandle.proc->getStatus() == fds::VolumeChecker::VC_NOT_STARTED);

        // vgCheckerList should have 1 volume element in it
        ASSERT_TRUE(vcHandle.proc->testGetVgCheckerListSize() == 1);
        // That one should have "clusterSize" in it
        ASSERT_TRUE(vcHandle.proc->testGetVgCheckerListSize(0) == clusterSize);

        // Sleep for seconds for msgs to be sent and to do work
        sleep(3);

        // Each DM in the cluster should have received the command
        // Check if all DMs have responded (NS_FINISHED)
        ASSERT_TRUE(vcHandle.proc->testVerifyCheckerListStatus(2));
        ASSERT_TRUE(vcHandle.proc->getStatus() == fds::VolumeChecker::VC_DM_DONE);
    }

    void stopVolumeChecker() {
        vcHandle.stop();
    }

    VcHandle   vcHandle;
    AmHandle   amHandle2;
};



#endif /* TESTLIB_VOLUMEGROUPFIXTURE_HPP_ */
