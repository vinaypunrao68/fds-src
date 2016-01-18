/*
 * Copyright 2014-2016 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_AMQOSCTRL_H_
#define SOURCE_ACCESS_MGR_INCLUDE_AMQOSCTRL_H_

#include "qos_ctrl.h"
#include "fds_qos.h"
#include "../lib/qos_htb.h"
#include "AmDataProvider.h"

#include <util/Log.h>
#include <concurrency/ThreadPool.h>
#include <fds_types.h>
#include <fds_error.h>

namespace fds {

class AmQoSCtrl :
    public FDS_QoSControl,
    public AmDataProvider
{
    QoSHTBDispatcher *htb_dispatcher;

 public:
    AmQoSCtrl(AmDataProvider* prev, uint32_t max_thrds, fds_log *log);
    ~AmQoSCtrl() override;


    Error processIO(FDS_IOType *io) override;
    fds_uint32_t waitForWorkers() { return 1; }

    /**
     * These are the QoS specific DataProvider routines.
     * Everything else is pass-thru.
     */
    bool done() override;
    void start() override;
    void stop() override;
    Error modifyVolumePolicy(const VolumeDesc& vdesc) override;
    void registerVolume(VolumeDesc const& volDesc) override;
    void removeVolume(VolumeDesc const& volDesc) override;
    Error updateQoS(int64_t const* rate, float const* throttle) override;
    void closeVolume(AmRequest *amReq) override         { enqueueRequest(amReq); }
    void statVolume(AmRequest *amReq) override          { enqueueRequest(amReq); }
    void setVolumeMetadata(AmRequest *amReq) override   { enqueueRequest(amReq); }
    void volumeContents(AmRequest *amReq) override      { enqueueRequest(amReq); }
    void startBlobTx(AmRequest *amReq) override         { enqueueRequest(amReq); }
    void commitBlobTx(AmRequest *amReq) override        { enqueueRequest(amReq); }
    void statBlob(AmRequest *amReq) override            { enqueueRequest(amReq); }
    void setBlobMetadata(AmRequest *amReq) override     { enqueueRequest(amReq); }
    void deleteBlob(AmRequest *amReq) override          { enqueueRequest(amReq); }
    void renameBlob(AmRequest *amReq) override          { enqueueRequest(amReq); }
    void getBlob(AmRequest *amReq) override             { enqueueRequest(amReq); }
    void putBlob(AmRequest *amReq) override             { enqueueRequest(amReq); }
    void putBlobOnce(AmRequest *amReq) override         { enqueueRequest(amReq); }

 protected:

    /**
     * These are the response we are interested in
     */
    void closeVolumeCb(AmRequest * amReq, Error const error) override
    { completeRequest(amReq, error); }

    void statVolumeCb(AmRequest * amReq, Error const error) override
    { completeRequest(amReq, error); }

    void setVolumeMetadataCb(AmRequest * amReq, Error const error) override
    { completeRequest(amReq, error); }

    void getVolumeMetadataCb(AmRequest * amReq, Error const error) override
    { completeRequest(amReq, error); }

    void volumeContentsCb(AmRequest * amReq, Error const error) override
    { completeRequest(amReq, error); }

    void startBlobTxCb(AmRequest * amReq, Error const error) override
    { completeRequest(amReq, error); }

    void commitBlobTxCb(AmRequest * amReq, Error const error) override
    { completeRequest(amReq, error); }

    void abortBlobTxCb(AmRequest * amReq, Error const error) override
    { completeRequest(amReq, error); }

    void statBlobCb(AmRequest * amReq, Error const error) override
    { completeRequest(amReq, error); }

    void setBlobMetadataCb(AmRequest * amReq, Error const error) override
    { completeRequest(amReq, error); }

    void deleteBlobCb(AmRequest * amReq, Error const error) override
    { completeRequest(amReq, error); }

    void renameBlobCb(AmRequest * amReq, Error const error) override
    { completeRequest(amReq, error); }

    void getBlobCb(AmRequest * amReq, Error const error) override
    { completeRequest(amReq, error); }

    void getOffsetsCb(AmRequest * amReq, Error const error) override
    { completeRequest(amReq, error); }

    void getObjectCb(AmRequest * amReq, Error const error) override
    { completeRequest(amReq, error); }

    void putBlobCb(AmRequest * amReq, Error const error) override
    { completeRequest(amReq, error); }

    void putBlobOnceCb(AmRequest * amReq, Error const error) override
    { completeRequest(amReq, error); }

 private:
    mutable fds_rwlock queue_lock;
    bool stopping {false};

    void enqueueRequest(AmRequest *amReq);
    void completeRequest(AmRequest* amReq, Error const error);
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMQOSCTRL_H_
