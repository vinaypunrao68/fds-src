/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#include <fdsp/PlatNetSvc.h>
#include <net/PlatNetSvcHandler.h>
#include <VolumeGroupHandle.h>
#include <Am.h>

namespace fds {

AmProcess::AmProcess(int argc, char *argv[], bool initAsModule)
{
    txId_ = 0;

    auto handler = boost::make_shared<PlatNetSvcHandler>(this);
    auto processor = boost::make_shared<fpi::PlatNetSvcProcessor>(handler);
    init(argc, argv, initAsModule, "platform.conf",
         "fds.am.", "am.log", nullptr, handler, processor);
}

void AmProcess::putBlob(const fds_volid_t &volId)
{
    /* Create some random blob start tx */
    fpi::StartTxMsgPtr msg(new fpi::StartTxMsg());
    msg->volumeIoHdr.txId = txId_++;
    volHandle_->sendModifyMsg<fpi::StartTxMsg>(
        FDSP_MSG_TYPEID(fpi::StartTxMsg),
        msg,
        [msg](const Error&, StringPtr) {
            GLOGNOTIFY << "Received response: " << fds::logString(*msg);
        });
}

void AmProcess::attachVolume(const fpi::VolumeGroupInfo &groupInfo)
{
    /* Only one volhanle supported */
    fds_verify(!volHandle_);
    volHandle_.reset(new VolumeGroupHandle(this, groupInfo));
}

int AmProcess::run() {
    LOGNOTIFY << "Doing work";
    readyWaiter.done();
    while (true) {
        sleep(1000);
    }
    return 0;
}

}  // namespace fds

