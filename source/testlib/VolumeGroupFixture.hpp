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

    void addDMTToVC(DMTPtr DMT) {
        ASSERT_TRUE(vcHandle.isRunning());
        std::string dmtData;
        dmt->getSerialized(dmtData);
        Error e = vcHandle.proc->getSvcMgr()->getDmtManager()->addSerializedDMT(dmtData,
                                                                          nullptr,
                                                                          DMT_COMMITTED);
        ASSERT_TRUE(e == ERR_OK);
    }

    void initVolumeChecker() {
        auto roots = getRootDirectories();
        // As volume checker, we init as an AM
        vcHandle.start({"checker",
                       roots[0],
                       "--fds.pm.platform_uuid=2059",
                       "--fds.pm.platform_port=9861"
                       });

    }

    void stopVolumeChecker() {
        vcHandle.stop();
    }

};



#endif /* TESTLIB_VOLUMEGROUPFIXTURE_HPP_ */
