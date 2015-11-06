/*
 * Copyright 2014-2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_AMQOSCTRL_H_
#define SOURCE_ACCESS_MGR_INCLUDE_AMQOSCTRL_H_

#include "qos_ctrl.h"
#include "fds_qos.h"
#include "../lib/qos_htb.h"
#include <util/Log.h>
#include <concurrency/ThreadPool.h>
#include "fds_error.h"
#include <fds_types.h>
#include "fds_table.h"
#include <fds_error.h>
#include <fds_volume.h>

namespace fds {
struct AmRequest;
struct AmVolume;
struct AmVolumeTable;
struct CommonModuleProviderIf;
struct WaitQueue;

class AmQoSCtrl : public FDS_QoSControl {
    using processor_cb_type = std::function<void(AmRequest*, Error const&)>;
    processor_cb_type processor_cb;
 public:
    QoSHTBDispatcher *htb_dispatcher;

    AmQoSCtrl(uint32_t max_thrds, dispatchAlgoType algo, CommonModuleProviderIf* provider, fds_log *log);
    virtual ~AmQoSCtrl();

    Error updateQoS(long int const* rate, float const* throttle);

    Error processIO(FDS_IOType *io) override;
    void init(processor_cb_type const& cb);
    fds_uint32_t waitForWorkers();
    Error registerVolume(VolumeDesc const& volDesc);
    Error modifyVolumePolicy(fds_volid_t vol_uuid, const VolumeDesc& vdesc);
    Error removeVolume(std::string const& name, fds_volid_t const vol_uuid);
    Error enqueueRequest(AmRequest *amReq);
    bool stop();

    /** These are here as a pass-thru to vol manager until we have stackable
     * interfaces */
    Error updateDlt(bool dlt_type, std::string& dlt_data, FDS_Table::callback_type const& cb);
    Error updateDmt(bool dmt_type, std::string& dmt_data, FDS_Table::callback_type const& cb);
    Error getDMT();
    Error getDLT();

 private:
    /// Unique ptr to the volume table
    std::unique_ptr<AmVolumeTable> volTable;

    std::unique_ptr<WaitQueue> wait_queue;

    void detachVolume(AmRequest *amReq);
    void execRequest(FDS_IOType* io);
    void completeRequest(AmRequest* amReq, Error const& error);
    Error markIODone(AmRequest *io);
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMQOSCTRL_H_
