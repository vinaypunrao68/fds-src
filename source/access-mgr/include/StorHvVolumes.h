/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_STORHVVOLUMES_H_
#define SOURCE_ACCESS_MGR_INCLUDE_STORHVVOLUMES_H_

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <unordered_map>
#include <boost/atomic.hpp>
#include <boost/thread/thread.hpp>
#include <boost/lockfree/queue.hpp>
#include <concurrency/ThreadPool.h>

#include <fds_error.h>
#include <fds_types.h>
#include <fds_volume.h>
#include <qos_ctrl.h>
#include <blob/BlobTypes.h>
#include <fds_qos.h>
#include <util/Log.h>
#include <concurrency/RwLock.h>
#include "PerfTrace.h"

/*
 * Forward declaration of SH control class
 */
class StorHvCtrl;

namespace fds {

/* Forward declarations */
struct AmRequest;

struct StorHvVolume : public FDS_Volume , public HasLogger
{
    StorHvVolume(const VolumeDesc& vdesc, StorHvCtrl *sh_ctrl, fds_log *parent_log);
    StorHvVolume(StorHvVolume const& rhs) = delete;
    StorHvVolume& operator=(StorHvVolume const& rhs) = delete;

    ~StorHvVolume();

    /* Reference to parent SH instance */
    StorHvCtrl *parent_sh;

    /*
     * per volume queue
     */
    std::unique_ptr<FDS_VolumeQueue> volQueue;
};

typedef std::vector<fds_volid_t> StorHvVolVec;

class StorHvVolumeTable : public HasLogger {
    static constexpr fds_volid_t fds_default_vol_uuid { 1 };

  public:
    using volume_ptr_type = std::shared_ptr<StorHvVolume>;

    /// Use logger that passed in to the constructor
    explicit StorHvVolumeTable(StorHvCtrl *sh_ctrl, fds_log *parent_log = nullptr);
    ~StorHvVolumeTable() {
        map_rwlock.write_lock();
        volume_map.clear();
    }

    Error registerVolume(const VolumeDesc& vdesc);
    Error removeVolume(fds_volid_t vol_uuid);

    /**
     * Returns NULL is volume does not exist
     */
    volume_ptr_type getVolume(fds_volid_t vol_uuid);

    /**
     * Returns list of volume ids currently in the table
     */
    StorHvVolVec getVolumeIds() const;

    /**
     * Returns the volumes max object size
     */
    fds_uint32_t getVolMaxObjSize(fds_volid_t volUuid) const;

    /**
     * Returns volume uuid if found in volume map.
     * if volume does not exist, returns 'invalid_vol_id'
     */
    fds_volid_t getVolumeUUID(const std::string& vol_name) const;

    /**
     * Returns the base volume id for DMT lookup.
     * for regular volumes , returns the same uuid
     * for snapshots, returns the parent uuid
     */
    fds_volid_t getBaseVolumeId(fds_volid_t volId) const;

    /**
     * Returns volume name if found in volume map.
     * if volume does not exist, returns empty string
     */
    std::string getVolumeName(fds_volid_t volId) const;

    /** returns true if volume exists, otherwise retuns false */
    fds_bool_t volumeExists(const std::string& vol_name) const;

    /**
     * Add blob request to wait queue -- those are blobs that
     * are waiting for OM to attach buckets to AM; once
     * vol table receives vol attach event, it will move
     * all requests waiting in the queue for that bucket to
     * appropriate qos queue
     */
    void addBlobToWaitQueue(const std::string& bucket_name,
                            AmRequest* blob_req);

    /**
     * Complete blob requests that are waiting in wait queue
     * with error (because e.g. volume not found, etc).
     */
    void completeWaitBlobsWithError(const std::string& bucket_name,
                                    Error err) {
        moveWaitBlobsToQosQueue(invalid_vol_id, bucket_name, err);
    }

    Error modifyVolumePolicy(fds_volid_t vol_uuid,
                             const VolumeDesc& vdesc);
    void moveWaitBlobsToQosQueue(fds_volid_t vol_uuid,
                                 const std::string& vol_name,
                                 Error error);

  private:
    /// print volume map, other detailed state to log
    void dump();

  private:
    /// volume uuid -> StorHvVolume map
    std::unordered_map<fds_volid_t, volume_ptr_type> volume_map;

    /// Protects volume_map
    mutable fds_rwlock map_rwlock;

    /**
     * list of blobs that are waiting for OM to attach appropriate
     * bucket to AM if it exists/ or return 'does not exist error
     */
    typedef std::vector<AmRequest*> bucket_wait_vec_t;
    typedef std::map<std::string, bucket_wait_vec_t> wait_blobs_t;
    typedef std::map<std::string, bucket_wait_vec_t>::iterator wait_blobs_it_t;
    wait_blobs_t wait_blobs;
    fds_rwlock wait_rwlock;

    /// Reference to parent SH instance
    StorHvCtrl *parent_sh;
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_STORHVVOLUMES_H_
