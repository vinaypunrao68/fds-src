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
#include "Volume.h"

namespace fds {

struct DmProcess;

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
    DmProcess(int argc, char *argv[], bool initAsModule);
    Error processIO(FDS_IOType* io);

    virtual int run() override;

    Error addVolume(const fds_volid_t &volId);

    template <class Functor, class IoType, class... Args>
    void runSynchronizedVolumeIoHandler(Functor&& f, IoType *io) {
        auto volId = io->getVolumeId();
        auto itr = volumeTbl.find(io->getVolumeId());
        fds_verify(itr != volumeTbl.end());
        // TODO(Rao): Handle volume not found
        auto volume = itr->second;
        qosCtrl->threadPool->scheduleWithAffinity(
            volId.get(),
            [f, volume, io]() {
                // TODO(Rao): handle volume state check
                auto thisVol = volume.get();
                (thisVol->*f)(io);
            });
    }


    std::unique_ptr<dmQosCtrl>                  qosCtrl;
    std::unique_ptr<FDS_VolumeQueue>            sysTaskQueue;
    std::unordered_map<fds_volid_t, VolumePtr>  volumeTbl;
};

struct DmHandler: PlatNetSvcHandler {
    explicit DmHandler(DmProcess* dmProc);

    void initHandlers();

    template <class QosVolumeIoT>
    void registerHandler() {
        asyncReqHandlers_[QosVolumeIoT::reqMsgTypeId] =
            [this] (SHPTR<fpi::AsyncHdr>& asyncHdr,
                    SHPTR<std::string>& payloadBuf)
            {
                SHPTR<typename QosVolumeIoT::ReqMsgT> payload;
                fds::deserializeFdspMsg(payloadBuf, payload);
                auto &volumeIoHdr = getVolumeIoHdrRef(*payload);
                auto cbfunc = std::bind(&DmHandler::responseCb<QosVolumeIoT>,
                                        this,
                                        asyncHdr,
                                        std::placeholders::_1);
                fds_volid_t volId(volumeIoHdr.groupId);
                auto qosMsg = new QosVolumeIoT(volId,
                                               payload,
                                               cbfunc);
                auto enqRet = dm->qosCtrl->enqueueIO(volId, qosMsg);
                if (enqRet != ERR_OK) {
                    fds_panic("Not handled");
                    // TODO(Rao): Fix it
                    // GLOGWARN << "Failed to enqueue " << *qosMsg;
                    delete qosMsg;
                }
            };
    }

    template <class QosVolumeIoT>
    void responseCb(fpi::AsyncHdrPtr& asyncHdr,
                    QosVolumeIoT *io) {
        asyncHdr->msg_code = static_cast<int32_t>(io->respStatus.GetErrno());
        if (io->respStatus == ERR_OK) {
            sendAsyncResp(*asyncHdr, QosVolumeIoT::respMsgTypeId, *(io->respMsg));
        } else {
            GLOGWARN << "Returning error response: " << io->respStatus
                << " header: " << fds::logString(*asyncHdr);
            sendAsyncResp(*asyncHdr, FDSP_MSG_TYPEID(fpi::EmptyMsg), fpi::EmptyMsg());
        }
    }
    DmProcess*      dm;
};


}  // namespace fds

#endif
