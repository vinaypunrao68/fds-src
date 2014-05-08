/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#define GTEST_USE_OWN_TR1_TUPLE 0

#include <iostream>
#include <chrono>
#include <thread>
#include <functional>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <util/Log.h>
#include <fdsp/FDSP_DataPathReq.h>
#include <fds_typedefs.h>
#include <fdsp/AMSvc.h>
#include <fdsp/SMSvc.h>
#include <net/net-service.h>
#include <RpcRequest.h>
#include <RpcRequestPool.h>
#include <RpcEp.h>

using ::testing::AtLeast;
using ::testing::Return;
using namespace fds;  // NOLINT
using namespace FDS_ProtocolInterface;  // NOLINT

/**
 * Helper macro for invoking an rpc
 * req - RpcRequest context object
 * SvcType - class type of the service
 * method - method to invoke
 * ... - arguments to the method
 */
#define INVOKE(epMgr, src_uuid, dst_uuid, req, SvcType, method, arg)        \
    do {                                                                    \
        auto client = (epMgr)->getEpClient<SvcType>((req).getEndPointId());   \
        arg.header.msg_src_id = req->getRequestId();                            \
        arg.header.msg_src_uuid = src_uuid;                                     \
        arg.header.msg_dst_uuid = dst_uuid;                                     \
        client->method(arg);                                                    \
    } while(false)

namespace fds_test {
RpcEpMgr gAmEpMgr;
RpcRequestPool gAmReqPool;

RpcEpMgr gSmEpMgr;
RpcRequestPool gSmReqPool;

class AsyncRspHandler : virtual public AsyncRspSvcIf {
public:
  void asyncResponse(const AsyncHdr& header, const std::string& payload)
  {
      std::cout << __FUNCTION__ << "\n";
  }
  void asyncResponse(boost::shared_ptr<AsyncHdr>& header,
          boost::shared_ptr<std::string>& payload)
  {
      std::cout << __FUNCTION__ << "\n";
      auto asyncReq = epMgr_->getAsyncRpcRequestTracker()->\
              getAsyncRpcRequest(static_cast<AsyncRpcRequestId>(header->msg_src_id));
      if (!asyncReq) {
          GLOGWARN << "Request " << header->msg_src_id << " doesn't exist";
          return;
      }
      asyncReq->handleResponse(header, payload);
  }
  void setEpMgr(RpcEpMgr *mgr) {
      epMgr_ = mgr;
  }
protected:
  RpcEpMgr *epMgr_;
};

class AMSvcHandler : virtual public AMSvcIf, virtual public AsyncRspHandler {
 public:
  AMSvcHandler() {
      std::cout << __FUNCTION__ << "\n";
  }
  ~AMSvcHandler() {
      std::cout << __FUNCTION__ << "\n";
  }
};

class SMSvcHandler : virtual public SMSvcIf, virtual public AsyncRspHandler {
 public:
  SMSvcHandler() {
      std::cout << __FUNCTION__ << "\n";
  }
  ~SMSvcHandler() {
      std::cout << __FUNCTION__ << "\n";
  }

  void getObject(const GetObjReq& req) {
      gSmEpMgr.sendAsyncResponse(req.header, GetObjRsp());
      std::cout << __FUNCTION__ << "\n";
  }


  void getObject(boost::shared_ptr<GetObjReq>& req) {
      std::cout << __FUNCTION__ << "\n";

  }

  void putObject(const PutObjReq& req) {
      std::cout << __FUNCTION__ << "\n";
  }


  void putObject(boost::shared_ptr<PutObjReq>& req) {
      std::cout << __FUNCTION__ << "\n";
  }

};

TEST(RpcEp, test) {
    try {

        fpi::SvcUuid amSvcId;
        amSvcId.svc_uuid = 2;
        fpi::SvcUuid smSvcId;
        smSvcId.svc_uuid = 1;

        RpcServer<AMSvcProcessor, AMSvcHandler> amServer(9191);
        boost::shared_ptr<RpcEp<SMSvcClient>> amToSmEp(new RpcEp<SMSvcClient>(smSvcId.svc_uuid, "127.0.0.1", 9192));
        gAmEpMgr.addEp(amToSmEp->getId(), amToSmEp);
        amServer.setEpMgr(&gAmEpMgr);
        amServer.start();


        RpcServer<SMSvcProcessor, SMSvcHandler> smServer(9192);
        boost::shared_ptr<RpcEp<AMSvcClient>> smToAmEp(new RpcEp<AMSvcClient>(amSvcId.svc_uuid, "127.0.0.1", 9191));
        gSmEpMgr.addEp(smToAmEp->getId(), smToAmEp);
        smServer.setEpMgr(&gSmEpMgr);
        smServer.start();

        std::this_thread::sleep_for(std::chrono::seconds(2));
        amToSmEp->connect();
        smToAmEp->connect();

        auto asyncReq = gAmReqPool.newEPAsyncRpcRequest(smSvcId);
        INVOKE(&gAmEpMgr, amSvcId, smSvcId, asyncReq, SMSvcClient, getObject, GetObjReq());

        // gAmEpMgr.getEpClient<SMSvcClient>(1)->getObject(GetObjReq());
        // gAmEpMgr.getEpClient<SMSvcClient>(1)->putObject(PutObjReq());

        std::this_thread::sleep_for(std::chrono::seconds(2));
    } catch (...) {

    }
}

#if 0
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
#endif
}  // namespace fds_test

int main(int argc, char** argv) {
    // The following line must be executed to initialize Google Mock
    // (and Google Test) before running the tests.
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}
