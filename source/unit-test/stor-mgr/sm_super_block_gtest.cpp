/**
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <ostream>
#include <set>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <fds_process.h>
#include <object-store/SmSuperblock.h>

using ::testing::AtLeast;
using ::testing::Return;

namespace fds {

static std::string logname = "smsuperblock";

static std::string test_path = "/tmp/smdisks/";

class SmSuperblockTestDriver {



};  // class SmSuperblockTestDriver



}  // fds





int
main(int argc, char *argv[])
{
    fds::init_process_globals(fds::logname);

    ::testing::InitGoogleMock(&argc, argv);

    return RUN_ALL_TESTS();
}


