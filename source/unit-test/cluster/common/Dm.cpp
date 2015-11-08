/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#include "dm.h"
#include "lib/QoSWFQDispatcher.h"

namespace fds {

#define FdsDmSysTaskId      fds_volid_t(0x8fffffff)
#define FdsDmSysTaskPrio    5

#define REGISTER_VOLUMEMSG_HANDLER(ReqMsgT, RespMsgT, cbfunc)  \
    asyncReqHandlers_[FDSP_MSG_TYPEID(ReqMsgT)] = \
    [this] (boost::shared_ptr<FDS_ProtocolInterface::AsyncHdr>& asyncHdr, \
        boost::shared_ptr<std::string>& payloadBuf) \
    { \
        DBG(fiu_do_on("svc.dropincoming."#ReqMsgT, return;)); \
        boost::shared_ptr<ReqMsgT> payload; \
        fds::deserializeFdspMsg(payloadBuf, payload); \
        auto &volumeIoHdr = getVolumeIoHdrRef(*payload) \
        auto qosMsg = new QosVolumeIo<ReqMsgT, RespMsgT>(volumeIoHdr.groupId, \
                                                         FDSP_MSG_TYPEID(ReqMsgT), \
                                                         payload, cbfunc); \
        auto enqRet = dm->qosCtrl->enqueMsg(volumeIoHdr.groupId, qosMsg); \
        if (enqRet != ERR_OK) { \
            GLOGWARN << "Failed to enqueue " << *qosMsg; \
            delete qosMsg; \
        } \
    }


template <class ReqMsgT, class RespMsgT>
std::ostream& operator<<(std::ostream &out, const QosVolumeIo<ReqMsgT, RespMsgT>& io) {
    out << " msgType: " << io.msgType
        << " volumeid: " << io.io_vol_id;
        // TODO: Print message
}

DmHandler::DmHandler(DmProcess* dmProc)
    : PlatNetSvcHandler(dmProc),
    dm(dmProc)
{
    // REGISTER_FDSP_MSG_HANDLER(fpi::StatStreamRegistrationMsg, registerStreaming);
    REGISTER_QOS_HANDLER(fpi::StartTxMsg, &dm::handleStartTxMsg);
}

dmQosCtrl::dmQosCtrl(DmProcess *parent,
                     uint32_t _max_thrds,
                     dispatchAlgoType algo,
                     fds_int64_t scheduleRate,
                     fds_uint32_t qosOutstandingTasks,
                     fds_log *log)
    : FDS_QoSControl(_max_thrds, algo, log, "DM")
{
    parentDm = parent;
    dispatcher = new QoSWFQDispatcher(this, scheduleRate, qosOutstandingTasks, log);
}

Error dmQosCtrl::processIO(FDS_IOType* io) {
    return parentDm->processIO(io);
}

Error dmQosCtrl::markIODone(FDS_IOType* io) {
    return dispatcher->markIODone(io);
}

dmQosCtrl::~dmQosCtrl() {
    delete dispatcher;
}

DmProcess::DmProcess(int argc, char *argv[])
{
    auto handler = boost::make_shared<DmHandler>(this);
    auto processor = boost::make_shared<fpi::PlatNetSvcProcessor>(handler);
    init(argc, argv, "platform.conf", "fds.dm.", "dm.log", nullptr, handler, processor);

    /* Init qos handler */
    qosCtrl.reset(new dmQosCtrl(this,
                            10,
                            FDS_QoSControl::FDS_DISPATCH_WFQ,
                            2 * 60 * 1000,
                            20,
                            GetLog()));
    qosCtrl->runScheduler();
    sysTaskQueue.reset(new FDS_VolumeQueue(1024, 10000, 20, FdsDmSysTaskPrio));
    sysTaskQueue->activate();
    auto err = qosCtrl->registerVolume(FdsDmSysTaskId, sysTaskQueue.get());
    fds_verify(err == ERR_OK);
}

Error DmProcess::processIO(FDS_IOType* io) {
    Error err(ERR_OK);
    fds_verify(io->io_type == FDS_DM_VOLUME_IO);

    auto volIo = static_cast<QosVolumeIo<void, void>*>(io);
    switch (volIo->msgType){
        case fpi::StartTxMsg:
            runSynchronizedVolumeIoHandler(&Volume::handleStartTx,
                                           static_cast<StartTxIo*>(volIo));
            break;
        case fpi::UpdateTxMsg:
        case fpi::CommitTxMsg:
        default:
            fds_verify("Unknown message");
            break;
    }
    
    return err;
}

int DmProcess::run() {
    LOGNOTIFY << "Doing work";
    while (true) {
        sleep(1000);
    }
    return 0;
}

}  // namespace fds

#if 0
int main(int argc, char* argv[]) {
    std::unique_ptr<fds::cluster::DmProcess> process(new fds::cluster::DmProcess(argc, argv));
    process->main();
    return 0;
}
#endif
