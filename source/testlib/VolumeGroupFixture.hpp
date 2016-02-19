/*
 * Copyright 2016 Formation Data Systems, Inc.
 */

#ifndef TESTLIB_VOLUMEGROUPFIXTURE_HPP_
#define TESTLIB_VOLUMEGROUPFIXTURE_HPP_

#include <testlib/DmGroupFixture.hpp>
#include <VolumeChecker.h>

struct VolumeGroupFixture : DmGroupFixture {
    using VcHandle = ProcessHandle<VolumeChecker>;
    VcHandle                                vcHandle;

    void initVolumeChecker()
    {
        auto roots = getRootDirectories();
        vcHandle.start({"vcm",
                       roots[0],
                       "--fds.pm.platform_uuid=2059",
                       "--fds.pm.platform_port=9861"
                       });

    }

};



#endif /* TESTLIB_VOLUMEGROUPFIXTURE_HPP_ */
