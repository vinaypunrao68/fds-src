/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_AMVOLUMETABLE_H_
#define SOURCE_ACCESS_MGR_INCLUDE_AMVOLUMETABLE_H_

#include <string>
#include <deque>
#include <mutex>
#include <unordered_map>

#include <fds_error.h>
#include <fds_types.h>
#include <qos_ctrl.h>
#include <blob/BlobTypes.h>
#include <fds_qos.h>
#include <util/Log.h>
#include <concurrency/RwLock.h>

namespace fds {

/* Forward declarations */
struct AmQoSCtrl;
struct AmRequest;
struct AmVolume;
struct WaitQueue;

struct AmVolumeTable : public HasLogger {
    static constexpr fds_volid_t fds_default_vol_uuid { 1 };

    using volume_ptr_type = std::shared_ptr<AmVolume>;

    /// Use logger that passed in to the constructor
    AmVolumeTable(size_t const qos_threads, fds_log *parent_log);
    ~AmVolumeTable();
    
    /// Registers the callback we make to the transaction layer
    using tx_callback_type = std::function<void(AmRequest*)>;
    void registerCallback(tx_callback_type cb);

    Error registerVolume(const VolumeDesc& vdesc, fds_int64_t token);
    Error removeVolume(const VolumeDesc& volDesc);

    /**
     * Returns NULL is volume does not exist
     */
    volume_ptr_type getVolume(fds_volid_t vol_uuid) const;

    /**
     * Returns the volumes max object size
     */
    fds_uint32_t getVolMaxObjSize(fds_volid_t volUuid) const;

    /**
     * Returns volume uuid if found in volume map.
     * if volume does not exist, returns 'invalid_vol_id'
     */
    fds_volid_t getVolumeUUID(const std::string& vol_name) const;

    Error modifyVolumePolicy(fds_volid_t vol_uuid,
                             const VolumeDesc& vdesc);

    Error enqueueRequest(AmRequest* amReq);
    Error markIODone(AmRequest* amReq);
    bool drained();

    Error updateQoS(long int const* rate,
                    float const* throttle);

  private:
    /// volume uuid -> AmVolume map
    std::unordered_map<fds_volid_t, volume_ptr_type> volume_map;

    std::unique_ptr<WaitQueue> wait_queue;

    /// QoS Module
    std::unique_ptr<AmQoSCtrl> qos_ctrl;

    /// Protects volume_map
    mutable fds_rwlock map_rwlock;
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMVOLUMETABLE_H_
