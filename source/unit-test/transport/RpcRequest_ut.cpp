/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#define GTEST_USE_OWN_TR1_TUPLE 0

#include <iostream>
#include <functional>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <fdsp/FDSP_DataPathReq.h>
#include <RpcRequest.h>
#include <RpcRequestPool.h>

using ::testing::AtLeast;
using ::testing::Return;
using namespace fds;  // NOLINT
using namespace FDS_ProtocolInterface;  // NOLINT

namespace fds_test {
/**
 * Tests async request invoke
 */
TEST(EPRpcRequest, invoke) {
    using namespace std::placeholders;

    RpcRequestPool pool;
    fpi::SvcUuid epId;
    FDSP_MsgHdrType fdsp_msg;
    FDSP_GetObjType get_obj_req;

    auto asyncReq = pool.newEPAsyncRpcRequest(epId);
    INVOKE_RPC(*asyncReq, FDSP_DataPathReqClient, GetObject, fdsp_msg, get_obj_req);
    EXPECT_TRUE(asyncReq->getError() != ERR_OK);
}

/**
 * Tests async request invoke
 */
TEST(EPAsyncRpcRequest, invoke) {

    RpcRequestPool pool;
    fpi::SvcUuid epId;
    FDSP_MsgHdrType fdsp_msg;
    FDSP_GetObjType get_obj_req;

    auto asyncReq = pool.newEPAsyncRpcRequest(epId);
    /*asyncReq->invoke<FDSP_DataPathReqClient>(&FDSP_DataPathReqClient::GetObject,
                                            fdsp_msg, get_obj_req);*/

    EXPECT_TRUE(asyncReq->getError() != ERR_OK);
    // int d1 = asyncReq->invoke(&Animal::run);
    // int d2 = asyncReq->invoke(&Animal::run, 10);

    
}
}  // namespace fds_test

int main(int argc, char** argv) {
    // The following line must be executed to initialize Google Mock
    // (and Google Test) before running the tests.
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}
