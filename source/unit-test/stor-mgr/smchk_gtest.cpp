/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <object-store/ObjectStore.h>

#include "sm_util_lib.h"
#include "sm_dataset.h"

class SMChkTest : public ::testing::Test {
public:
    SMChkTest() {}
    ~SMChkTest() {}
    virtual void SetUp() override;
    virtual void TearDown() override;

private:
    std::shared_ptr<StandaloneSM> sm;

};


void SMChkTest::SetUp() {
    // Bring up standalone SM
    StandaloneSM::objectStore = ObjectStore::unique_ptr(
            new ObjectStore("Standalone Object Store", NULL, volTbl));

    sm = new std::shared_ptr<StandaloneSM>(new StandaloneSM(0, nullptr, "platform.conf",
            "fds", StandaloneSM::smVec));
    sm.main();

    // Now write a dataset

}

void SMChkTest::TearDown() {
    // Remove mock data from /fds/dev
}


int main(int argc, char * argv[]) {
    fds::init_process_globals(fds::logname);
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}
