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
        // TODO
    }

};



#endif /* TESTLIB_VOLUMEGROUPFIXTURE_HPP_ */
