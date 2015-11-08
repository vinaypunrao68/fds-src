#ifndef DM_H_
#define DM_H_

/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#include <fds_process.h>
#include <fdsp/PlatNetSvc.h>
#include <net/SvcMgr.h>
#include <net/SvcProcess.h>
#include <net/PlatNetSvcHandler.h>

namespace fds {
namespace dm {
struct DmProcess;

struct DmHandler: PlatNetSvcHandler {
    explicit DmHandler(DmProcess* dmProc);
    template <class ReqT, class RespT>
    void responseCb(fpi::AsyncHdrPtr& asyncHdr,
                    const fpi::FDSPMsgTypeId &respMsgType,
                    const Error &err,
                    QosVolumeIo<ReqT, RespT> *io) {
        asyncHdr->msg_code = static_cast<int32_t>(err.GetErrno());
        if (err == ERR_OK) {
            sendAsyncResp(*asyncHdr, respMsgType, *(io->respMsg));
        } else {
            GLOWARN << "Returning error response: " << err
            " header: " << fds::logString(*asyncHdr);
            sendAsyncResp(*asyncHdr, FDSP_MSG_TYPEID(fpi::EmptyMsg), fpi::EmptyMsg());
        }
    }
    DmProcess*      dm;
};

struct dmQosCtrl : FDS_QoSControl {
    dmQosCtrl(DmProcess *parent,
              uint32_t _max_thrds,
              dispatchAlgoType algo,
              fds_int64_t scheduleRate,
              fds_uint32_t qosOutstandingTasks,
              fds_log *log);
    virtual ~dmQosCtrl();

    Error processIO(FDS_IOType* io) override;
    Error markIODone(FDS_IOType* io) override;

    DmProcess *parentDm;
};

struct DmProcess : SvcProcess {
    DmProcess(int argc, char *argv[]);
    Error processIO(FDS_IOType* io);

    virtual int run() override;

    template <class Functor, class IoType, class... Args>
    void runSynchronizedVolumeIoHandler(Functor&& f, IoType *io) {
        auto volId = io->getVolumeId();
        auto itr = volumeTbl.find(io->getVolumeId());
        fds_verify(itr != volumeTbl.end());
        // TODO(Rao): Handle volume not found
        auto volume = itr->second;
        qostCtrl->threadPool->scheduleWithAffinity(
            volId.get(),
            [f, volume, io]() {
                // TODO(Rao): handle volume state check
                f(volume.get(), io);
            });
    }


    std::unique_ptr<dmQosCtrl>                  qosCtrl;
    std::unique_ptr<FDS_VolumeQueue>            sysTaskQueue;
    std::unordered_map<fds_volid_t, VolumePtr>  volumeTbl;
};

}  // namespace dm 
}  // namespace fds

#endif
