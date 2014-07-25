/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

/*
 * Volume metadata class. Describes the per-volume metada
 * that is maintained by a data manager.
 */

#ifndef SOURCE_DATA_MGR_INCLUDE_VOLUMEMETA_H_
#define SOURCE_DATA_MGR_INCLUDE_VOLUMEMETA_H_

#include <string>
#include <map>
#include <fds_types.h>
#include <fds_error.h>
#include <dm-platform.h>
#include <util/Log.h>

#include <concurrency/Mutex.h>
#include <fds_volume.h>
#include <fdsp/fds_service_types.h>

namespace fds {

class VolumeMeta : public HasLogger {
 public:
    boost::posix_time::ptime dmtclose_time;  // timestamp when dmt_close  arrived
    /**
     * volume  meta forwarding state
     */
    typedef enum {
        VFORWARD_STATE_NONE,      // not fowarding
        VFORWARD_STATE_INPROG,    // forwarding
        VFORWARD_STATE_FINISHING  // forwarding queued updates before finish
    } fwdStateType;

  private:
    fds_mutex  *vol_mtx;

    /**
     * volume meta forwarding state
     */
    fwdStateType fwd_state;  // write protected by vol_mtx

    /*
     * This class is non-copyable.
     * Disable copy constructor/assignment operator.
     */
    VolumeMeta(const VolumeMeta& _vm);
    VolumeMeta& operator= (const VolumeMeta rhs);

 public:
    fds_bool_t isForwarding() const {
        return (fwd_state != VFORWARD_STATE_NONE);
    }

    fds_bool_t isForwardFinish() const {
        return (fwd_state == VFORWARD_STATE_FINISHING);
    }
    void setForwardInProgress() {
        fwd_state = VFORWARD_STATE_INPROG;
    }
    void setForwardFinish() {
        fwd_state = VFORWARD_STATE_NONE;
    }
    /**
     * If state is 'in progress', will move to 'finishing'
     */
    void finishForwarding();

    VolumeDesc *vol_desc;
    /*
     * Default constructor should NOT be called
     * directly. It is needed for STL data struct
     * support.
     */
    VolumeMeta();
    VolumeMeta(const std::string& _name,
               fds_volid_t _uuid,
               VolumeDesc *v_desc);
    VolumeMeta(const std::string& _name,
               fds_volid_t _uuid,
               fds_log* _dm_log,
               VolumeDesc *v_desc);
    ~VolumeMeta();

    void dmCopyVolumeDesc(VolumeDesc *v_desc, VolumeDesc *pVol);
    /*
     * per volume queue
     */
    FDS_VolumeQueue*  dmVolQueue;
};

}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_VOLUMEMETA_H_
