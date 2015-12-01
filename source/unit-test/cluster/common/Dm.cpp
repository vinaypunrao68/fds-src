/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#include <lib/QoSWFQDispatcher.h>
#include <Volume.h>
#include <Dm.h>

namespace fds {

#define ASSERT_IN_DISPATCHER_CONTEXT()
#define FdsDmSysTaskId      fds_volid_t(0x8fffffff)
#define FdsDmSysTaskPrio    5

DmHandler::DmHandler(DmProcess* dmProc)
    : PlatNetSvcHandler(dmProc),
    dm(dmProc)
{
    initHandlers();
}

void DmHandler::initHandlers()
{
    registerHandler<StartTxIo>(getVolIdFromSvcMsgWithIoHdr<StartTxIo::ReqMsgT>);
    registerHandler<UpdateTxIo>(getVolIdFromSvcMsgWithIoHdr<UpdateTxIo::ReqMsgT>);
    registerHandler<CommitTxIo>(getVolIdFromSvcMsgWithIoHdr<CommitTxIo::ReqMsgT>);

    registerHandler<PullActiveTxsIo>(getVolIdFromSvcMsg<PullActiveTxsIo::ReqMsgT>);
    registerHandler<PullCommitLogEntriesIo>(getVolIdFromSvcMsg<PullCommitLogEntriesIo::ReqMsgT>);
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
    dispatcher = new QoSWFQDispatcher(this, scheduleRate, qosOutstandingTasks, false, log);
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

DmProcess::DmProcess(int argc, char *argv[], bool initAsModule)
{
    auto handler = boost::make_shared<DmHandler>(this);
    auto processor = boost::make_shared<fpi::PlatNetSvcProcessor>(handler);
    init(argc, argv, initAsModule, "platform.conf",
         "fds.dm.", "dm.log", nullptr, handler, processor);

    /* Init qos handler */
    qosCtrl.reset(new dmQosCtrl(this,
                            3,
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

    /* NOTE: After this point we only deal with shared pointers */
    auto volIo = (static_cast<VolumeIoBase*>(io));
    auto volId = volIo->getVolumeId();
    auto itr = volumeTbl.find(volId);
    fds_verify(itr != volumeTbl.end());
    auto &volume = itr->second;
    qosCtrl->threadPool->scheduleWithAffinity(volId.get(), [volume, volIo]() {
        /* Make volIo shared_ptr from this point */
        auto volIoPtr = SHPTR<VolumeIoBase>(volIo);
        /* Handle */
        volume->getCurrentBehavior()->handle(volIo->msgType, volIoPtr);
    });
#if 0
    switch (volIo->msgType){
        case FDSP_MSG_TYPEID(fpi::StartTxMsg):
            runSynchronizedVolumeIoHandler(&Volume::handleStartTx,
                                           SHPTR_CAST(StartTxIo, volIo));
            break;
        case FDSP_MSG_TYPEID(fpi::UpdateTxMsg):
            runSynchronizedVolumeIoHandler(&Volume::handleUpdateTx,
                                           SHPTR_CAST(UpdateTxIo, volIo));
            break;
        case FDSP_MSG_TYPEID(fpi::CommitTxMsg):
            runSynchronizedVolumeIoHandler(&Volume::handleCommitTx,
                                           SHPTR_CAST(CommitTxIo, volIo));
            break;
        case FDSP_MSG_TYPEID(fpi::SyncPullLogEntriesMsg):
            runSynchronizedVolumeIoHandler(&Volume::handleSyncPullLogEntries,
                                           SHPTR_CAST(SyncPullLogEntriesIo, volIo));
            break;
        case FDSP_MSG_TYPEID(fpi::QosFunction):
            runSynchronizedVolumeIoHandler(&Volume::handleQosFunctionIo,
                                           SHPTR_CAST(QosFunctionIo, volIo));
            break;
        default:
            fds_panic("Unknown message");
            break;
    }
#endif
    
    return err;
}

Error DmProcess::addVolume(const fds_volid_t &volId)
{
    ASSERT_IN_DISPATCHER_CONTEXT();
    fds_verify(volumeTbl.find(volId) == volumeTbl.end());
    auto volume = new Volume(this, qosCtrl.get(), volId);
    volumeTbl[volId].reset(volume);
    volume->init();
    return ERR_OK;
}
void DmProcess::shutdown_modules()
{
    qosCtrl->stop();
    SvcProcess::shutdown_modules();
}

int DmProcess::run() {
    LOGNOTIFY << "Doing work";
    readyWaiter.done();
    shutdownGate_.waitUntilOpened();
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
