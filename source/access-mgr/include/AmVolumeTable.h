/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_AMVOLUMETABLE_H_
#define SOURCE_ACCESS_MGR_INCLUDE_AMVOLUMETABLE_H_

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
#include <qos_ctrl.h>
#include <blob/BlobTypes.h>
#include <fds_qos.h>
#include <util/Log.h>
#include <concurrency/RwLock.h>
#include "PerfTrace.h"

namespace fds {

/* Forward declarations */
struct AmRequest;
struct AmVolume;

typedef std::vector<fds_volid_t> StorHvVolVec;

struct AmVolumeTable : public HasLogger {
    static constexpr fds_volid_t fds_default_vol_uuid { 1 };

    using volume_ptr_type = std::shared_ptr<AmVolume>;

    /// Use logger that passed in to the constructor
    explicit AmVolumeTable(fds_log *parent_log = nullptr);
    ~AmVolumeTable() {
        map_rwlock.write_lock();
        volume_map.clear();
    }

    Error registerVolume(const VolumeDesc& vdesc);
    Error removeVolume(fds_volid_t vol_uuid);

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

  private:
    /// volume uuid -> AmVolume map
    std::unordered_map<fds_volid_t, volume_ptr_type> volume_map;

    /// Protects volume_map
    mutable fds_rwlock map_rwlock;
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMVOLUMETABLE_H_
