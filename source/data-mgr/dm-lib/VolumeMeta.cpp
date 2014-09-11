/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <string>
#include <list>
#include <VolumeMeta.h>
#include <fds_process.h>
#include <util/timeutils.h>

namespace fds {

VolumeMeta::VolumeMeta(const std::string& _name,
                       fds_int64_t _uuid,
                       VolumeDesc* desc)
              : fwd_state(VFORWARD_STATE_NONE)
{
    const FdsRootDir *root = g_fdsprocess->proc_fdsroot();

    vol_mtx = new fds_mutex("Volume Meta Mutex");
    vol_desc = new VolumeDesc(_name, _uuid);
    dmCopyVolumeDesc(vol_desc, desc);

    root->fds_mkdir(root->dir_user_repo_dm().c_str());
}

VolumeMeta::VolumeMeta(const std::string& _name,
                       fds_int64_t _uuid,
                       fds_log* _dm_log,
                       VolumeDesc* _desc)
        : VolumeMeta(_name, _uuid, _desc) {
}

VolumeMeta::~VolumeMeta() {
    delete vol_desc;
    delete vol_mtx;
}

// Returns true if volume is in forwarding state, and qos queue is empty;
// otherwise returns false (volume not in forwarding state or qos queue
// is not empty
//
void VolumeMeta::finishForwarding() {
    vol_mtx->lock();
    if (fwd_state == VFORWARD_STATE_INPROG) {
        // set close time to now + small(ish) time interval to include IOs that
        // being queued right now
        // dmtclose_time = boost::posix_time::microsec_clock::universal_time() +
        //        boost::posix_time::milliseconds(10);
        fwd_state = VFORWARD_STATE_FINISHING;
    } else {
        fwd_state = VFORWARD_STATE_NONE;
    }
    vol_mtx->unlock();

    LOGNORMAL << "finishForwarding for volume " << *vol_desc
              << ", state " << fwd_state;
}

void VolumeMeta::dmCopyVolumeDesc(VolumeDesc *v_desc, VolumeDesc *pVol) {
    v_desc->name = pVol->name;
    v_desc->volUUID = pVol->volUUID;
    v_desc->tennantId = pVol->tennantId;
    v_desc->localDomainId = pVol->localDomainId;
    v_desc->globDomainId = pVol->globDomainId;

    v_desc->maxObjSizeInBytes = pVol->maxObjSizeInBytes;
    v_desc->capacity = pVol->capacity;
    v_desc->volType = pVol->volType;
    v_desc->maxQuota = pVol->maxQuota;
    v_desc->replicaCnt = pVol->replicaCnt;

    v_desc->consisProtocol = fpi::FDSP_ConsisProtoType(pVol->consisProtocol);
    v_desc->appWorkload = pVol->appWorkload;
    v_desc->mediaPolicy = pVol->mediaPolicy;

    v_desc->volPolicyId = pVol->volPolicyId;
    v_desc->iops_max = pVol->iops_max;
    v_desc->iops_min = pVol->iops_min;
    v_desc->relativePrio = pVol->relativePrio;
    v_desc->fSnapshot = pVol->fSnapshot;
    v_desc->srcVolumeId = pVol->srcVolumeId;
}

}  // namespace fds
