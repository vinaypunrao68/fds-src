/*
 * Copyright 2014-2015 Formation Data Systems, Inc.
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

struct AmVolumeTable;
struct CommonModuleProviderIf;
struct WaitQueue;

class AmQoSCtrl :
    public FDS_QoSControl,
    public AmDataProvider
{
    using processor_cb_type = std::function<void(AmRequest*, Error const&)>;
    processor_cb_type processor_cb;

    QoSHTBDispatcher *htb_dispatcher;

 public:
    AmQoSCtrl(uint32_t max_thrds, dispatchAlgoType algo, CommonModuleProviderIf* provider, fds_log *log);
    virtual ~AmQoSCtrl();

    Error updateQoS(long int const* rate, float const* throttle);

    Error processIO(FDS_IOType *io) override;
    void init(processor_cb_type const& cb);
    fds_uint32_t waitForWorkers() { return 1; }
    Error modifyVolumePolicy(fds_volid_t vol_uuid, const VolumeDesc& vdesc);
    Error enqueueRequest(AmRequest *amReq);
    bool shutdown();

    /**
     * These are the QoS specific DataProvider routines.
     * Everything else is pass-thru.
     */
    Error registerVolume(VolumeDesc const& volDesc) override;
    Error removeVolume(VolumeDesc const& volDesc) override;

 protected:

    /**
     * These are here cause AmProcessor is not a DataProvider yet.
     */
    void openVolumeCb(AmRequest * amReq, Error const error) override
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
    /// Unique ptr to the volume table
    AmVolumeTable* volTable;

    std::unique_ptr<WaitQueue> wait_queue;

    void detachVolume(AmRequest *amReq);
    void execRequest(FDS_IOType* io);
    void completeRequest(AmRequest* amReq, Error const error);
    Error markIODone(AmRequest *io);
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMQOSCTRL_H_
