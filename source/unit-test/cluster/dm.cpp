/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#include <fds_process.h>
#include <fdsp/PlatNetSvc.h>
#include <net/SvcMgr.h>
#include <net/SvcProcess.h>
#include <net/PlatNetSvcHandler.h>

namespace fds {
namespace cluster {
struct DmProcess;

struct DmHandler: PlatNetSvcHandler {
    DmHandler::DmHandler(DmProcess* dmProc)
        : PlatNetSvcHandler(dmProc),
        dm(dmProc)
    {
        REGISTER_FDSP_MSG_HANDLER(fpi::StatStreamRegistrationMsg, registerStreaming);
    }
    
    DmProcess*      dm;
};

struct dmQosCtrl : FDS_QoSControl {
    DmProcess *parentDm;
    dmQosCtrl(DmProcess *parent,
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

    Error processIO(FDS_IOType* io) {
        return parentDm->processIO(static_cast<DmRequest*>(_io));
    }

    Error markIODone(const FDS_IOType& _io) {
        dispatcher->markIODone(const_cast<FDS_IOType *>(&_io));
        return ERR_OK;
    }

    virtual ~dmQosCtrl() {
        delete dispatcher;
    }
};

struct Volume : Replica {
    
};

struct DmProcess : SvcProcess {
    DmProcess(int argc, char *argv[])
    {
        auto handler = boost::make_shared<DmHandler>(this, this);
        auto processor = boost::make_shared<fpi::PlatNetSvcProcessor>(handler);
        init(argc, argv, "platform.conf", "fds.dm.", "dm.log", nullptr, handler, processor);

        /* Init qos handler */
        qosCtrl = new dmQosCtrl(this,
                                10,
                                FDS_QoSControl::FDS_DISPATCH_WFQ,
                                2 * 60 * 1000,
                                20,
                                GetLog());
        qosCtrl->runScheduler();
        sysTaskQueue = new FDS_VolumeQueue(1024, 10000, 20, FdsDmSysTaskPrio);
        sysTaskQueue->activate();
        auto err = qosCtrl->registerVolume(FdsDmSysTaskId, sysTaskQueue);
        fds_verify(err == ERR_OK);
    }
    Error processIO(FDS_IOType* io) {
        Error err(ERR_OK);
        switch (io->io_type){
            /* TODO(Rao): Add the new refactored DM messages types here */
            case FDS_DM_SNAP_VOLCAT:
            default:
                assert(0);
                break;
        }
        return err;
    }

    virtual int run() override {
        LOGNOTIFY << "Doing work";
        while (true) {
            sleep(1000);
        }
        return 0;
    }

    std::unique_ptr<dmQosCtrl>                  qosCtrl;
    std::unique_ptr<FDS_VolumeQueue>            sysTaskQueue;
}

}
}
