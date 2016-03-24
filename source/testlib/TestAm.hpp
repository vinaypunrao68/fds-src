#ifndef TESTAM_HPP_
#define TESTAM_HPP_

/*
 * Copyright 2016 Formation Data Systems, Inc.
 */
#include <fds_process.h>
#include <fdsp/PlatNetSvc.h>
#include <net/SvcMgr.h>
#include <net/SvcProcess.h>
#include <net/PlatNetSvcHandler.h>
#include <testlib/ProcessHandle.hpp>
#include <testlib/SvcMsgFactory.h>
#include <testlib/TestUtils.h>
#include <net/VolumeGroupHandle.h>
#include <boost/filesystem.hpp>
#include "fdsp/common_constants.h"


namespace fds {

struct TestAm : SvcProcess {
    TestAm(int argc, char *argv[], bool initAsModule)
    {
        handler_ = MAKE_SHARED<PlatNetSvcHandler>(this);
        auto processor = boost::make_shared<fpi::PlatNetSvcProcessor>(handler_);

        /**
         * Note on Thrift service compatibility:
         *
         * For service that extends PlatNetSvc, add the processor twice using
         * Thrift service name as the key and again using 'PlatNetSvc' as the
         * key. Only ONE major API version is supported for PlatNetSvc.
         *
         * All other services:
         * Add Thrift service name and a processor for each major API version
         * supported.
         */
        TProcessorMap processors;
        processors.insert(std::make_pair<std::string,
            boost::shared_ptr<apache::thrift::TProcessor>>(
                fpi::commonConstants().PLATNET_SERVICE_NAME, processor));

        init(argc, argv, initAsModule, "platform.conf",
             "fds.am.", "am.log", nullptr, handler_, processors);
        REGISTER_FDSP_MSG_HANDLER_GENERIC(handler_, \
                                          fpi::AddToVolumeGroupCtrlMsg, addToVolumeGroup);
        REGISTER_FDSP_MSG_HANDLER_GENERIC(handler_, \
                                          fpi::SwitchCoordinatorMsg, switchCoordinator);
    }
    virtual int run() override
    {
        readyWaiter.done();
        shutdownGate_.waitUntilOpened();
        return 0;
    }
    void setVolumeHandle(VolumeGroupHandle *h) {
        vc_ = h;
    }
    void addToVolumeGroup(fpi::AsyncHdrPtr& asyncHdr,
                          fpi::AddToVolumeGroupCtrlMsgPtr& addMsg)
    {
        fds_volid_t volId(addMsg->groupId);
        vc_->handleAddToVolumeGroupMsg(
            addMsg,
            [this, asyncHdr](const Error& e,
                             const fpi::AddToVolumeGroupRespCtrlMsgPtr &payload) {
            asyncHdr->msg_code = e.GetErrno();
            handler_->sendAsyncResp(*asyncHdr,
                                    FDSP_MSG_TYPEID(fpi::AddToVolumeGroupRespCtrlMsg),
                                    *payload);
            });
    }
    void switchCoordinator(fpi::AsyncHdrPtr& asyncHdr,
                          fpi::SwitchCoordinatorMsgPtr& msg) {
        vc_->close([]() {});
        auto resp = MAKE_SHARED<fpi::SwitchCoordinatorRespMsg>();
        handler_->sendAsyncResp(*asyncHdr,
                                FDSP_MSG_TYPEID(fpi::SwitchCoordinatorRespMsg),
                                *resp);
    }
    PlatNetSvcHandlerPtr handler_;
    VolumeGroupHandle *vc_{nullptr};
};


} // namespace fds

#endif
