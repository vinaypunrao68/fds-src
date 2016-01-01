/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <string>
#include <list>
#include <VolumeMeta.h>
#include <fds_process.h>
#include <util/timeutils.h>

namespace fds {

#if 0
VolumeMeta::VolumeMeta(const std::string& _name,
                       fds_volid_t _uuid,
                       VolumeDesc* desc)
              : fwd_state(VFORWARD_STATE_NONE),
                dmVolQueue(0),
                dataManager(nullptr),
                cbToVGMgr(NULL)
{
    const FdsRootDir *root = g_fdsprocess->proc_fdsroot();

    vol_mtx = new fds_mutex("Volume Meta Mutex");
    vol_desc = new VolumeDesc(_name, _uuid);
    dmCopyVolumeDesc(vol_desc, desc);

    root->fds_mkdir(root->dir_sys_repo_dm().c_str());
    root->fds_mkdir(root->dir_user_repo_dm().c_str());

    // this should be overwritten when volume add triggers read of the persisted value
    sequence_id = 0;
}
#endif

VolumeMeta::VolumeMeta(const std::string& _name,
                       fds_volid_t _uuid,
                       fds_log* _dm_log,
                       VolumeDesc* _desc,
                       DataMgr &_dm)
          : fwd_state(VFORWARD_STATE_NONE),
            dmVolQueue(0),
            dataManager(_dm),
            cbToVGMgr(NULL) {
    const FdsRootDir *root = g_fdsprocess->proc_fdsroot();

    vol_mtx = new fds_mutex("Volume Meta Mutex");
    vol_desc = new VolumeDesc(_name, _uuid);
    dmCopyVolumeDesc(vol_desc, _desc);

    root->fds_mkdir(root->dir_sys_repo_dm().c_str());
    root->fds_mkdir(root->dir_user_repo_dm().c_str());

    // this should be overwritten when volume add triggers read of the persisted value
    sequence_id = 0;
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
    setForwardFinish();
    vol_mtx->unlock();
    LOGMIGRATE << "finishForwarding for volume " << *vol_desc
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
    v_desc->redundancyCnt = pVol->redundancyCnt;

    v_desc->consisProtocol = fpi::FDSP_ConsisProtoType(pVol->consisProtocol);
    v_desc->appWorkload = pVol->appWorkload;
    v_desc->mediaPolicy = pVol->mediaPolicy;

    v_desc->volPolicyId = pVol->volPolicyId;
    v_desc->iops_throttle = pVol->iops_throttle;
    v_desc->iops_assured = pVol->iops_assured;
    v_desc->relativePrio = pVol->relativePrio;
    v_desc->fSnapshot = pVol->fSnapshot;
    v_desc->srcVolumeId = pVol->srcVolumeId;
    v_desc->qosQueueId = pVol->qosQueueId;
    v_desc->contCommitlogRetention = pVol->contCommitlogRetention;
    v_desc->timelineTime = pVol->timelineTime;
}

void VolumeMeta::setSequenceId(sequence_id_t new_seq_id){
    fds_mutex::scoped_lock l(sequence_lock);

    if (new_seq_id > sequence_id) {
        sequence_id = new_seq_id;
    }
}

sequence_id_t VolumeMeta::getSequenceId(){
    fds_mutex::scoped_lock l(sequence_lock);

    return sequence_id;
}


Error VolumeMeta::startMigration(NodeUuid& srcDmUuid,
                                 fpi::FDSP_VolumeDescType &vol,
                                 migrationCb doneCb) {
    Error err(ERR_OK);
    fds_assert(migrationDest == nullptr);
    cbToVGMgr = doneCb;
    uint32_t deltaBlobTimeout = uint32_t(MODULEPROVIDER()->get_fds_config()->
                                get<int32_t>("fds.dm.migration.migration_max_delta_blobs_to", 5));


    // DataMgr *nonConstDm = dataManager;

    auto dummyId = 0;
    migrationDest.reset(new DmMigrationDest(dummyId,
                                            dataManager,
                                            srcDmUuid,
                                            vol,
                                            deltaBlobTimeout,
                                            std::bind(&VolumeMeta::cleanUpMigrationDestination,
                                                      this,
                                                      std::placeholders::_1,
                                                      std::placeholders::_2,
                                                      std::placeholders::_3)));

    // TODO : once processInitialBlobFilterSet has been fixed to be
    // iterative, then we won't block here.
    err = migrationDest->start();

    return err;
}

Error VolumeMeta::serveMigration(DmRequest *dmRequest) {
    Error err(ERR_OK);
    NodeUuid mySvcUuid(MODULEPROVIDER()->getSvcMgr()->getSelfSvcUuid().svc_uuid);
    DmIoResyncInitialBlob* typedRequest = static_cast<DmIoResyncInitialBlob*>(dmRequest);
    NodeUuid destDmUuid(typedRequest->destNodeUuid);
    fpi::CtrlNotifyInitialBlobFilterSetMsgPtr migReqMsg = typedRequest->message;
    migrationCb cleanupCb = typedRequest->localCb;

    LOGNOTIFY << "migrationid: " << migReqMsg->DMT_version
        <<" received msg for volume " << migReqMsg->volumeId;

    err = createMigrationSource(destDmUuid, mySvcUuid, migReqMsg, cleanupCb);

    return err;
}

Error VolumeMeta::createMigrationSource(NodeUuid destDmUuid,
                                        const NodeUuid &mySvcUuid,
                                        fpi::CtrlNotifyInitialBlobFilterSetMsgPtr filterSet,
                                        migrationCb cleanup) {
    Error err(ERR_OK);
    auto maxNumBlobs = uint64_t(MODULEPROVIDER()->get_fds_config()->
                           get<int64_t>("fds.dm.migration.migration_max_delta_blobs"));
    auto maxNumBlobDesc = uint64_t(MODULEPROVIDER()->get_fds_config()->
                              get<int64_t>("fds.dm.migration.migration_max_delta_blob_desc"));


    auto fds_volid = fds_volid_t(filterSet->volumeId);
    fds_verify(fds_volid == vol_desc->GetID());

    auto search = migrationSrcMap.find(destDmUuid);
    if (search != migrationSrcMap.end()) {
        LOGERROR << "migrationid: " << filterSet->DMT_version
            << " Source received request for destination node: " << destDmUuid
            << " volume " << filterSet->volumeId << " but it already exists";
        err = ERR_DUPLICATE;
    } else {
        DmMigrationSrc::shared_ptr source;
        {
            SCOPEDWRITE(migrationSrcMapLock);
            source = DmMigrationSrc::shared_ptr(
                    new DmMigrationSrc(dataManager,
                                       mySvcUuid,
                                       destDmUuid,
                                       filterSet->DMT_version,
                                       filterSet,
                                       std::bind(&VolumeMeta::cleanUpMigrationSource,
                                                 this,
                                                 std::placeholders::_1,
                                                 std::placeholders::_2,
                                                 destDmUuid),
                                       cleanup,
                                       maxNumBlobs,
                                       maxNumBlobDesc));
            migrationSrcMap.insert(std::make_pair(destDmUuid, source));
        }
        source->run();
    }
    return err;
}

void VolumeMeta::cleanUpMigrationSource(fds_volid_t volId,
                                        const Error &err,
                                        const NodeUuid destDmUuid) {

    if (!err.ok()) {
        LOGERROR << "Cleaning up for vol: " << volId << " dest node: " << destDmUuid <<
            " with error: " << err;
    } else {
        LOGNORMAL << "[migrate] Cleaning up for vol: " << volId << " dest node: " << destDmUuid <<
            " with error: " << err;

    }
    DmMigrationSrc::shared_ptr source;
    {
        SCOPEDWRITE(migrationSrcMapLock);
        auto search = migrationSrcMap.find(destDmUuid);
        if (search == migrationSrcMap.end()) {
            LOGERROR << "Volume " << volId << " Unable to find entry " << destDmUuid;
            return;
        } else {
            source = search->second;
            source->finish();
            migrationSrcMap.erase(search);
        }
    }
}

void VolumeMeta::cleanUpMigrationDestination(NodeUuid srcNodeUuid,
                                             fds_volid_t volId,
                                             const Error &err) {

    /** volId and srcNodeUuid is there for formality */
    fds_assert(volId == vol_desc->volUUID);
    if (!err.ok()) {
        LOGERROR << "Cleaning up for vol: " << volId << " dest node: " << srcNodeUuid <<
            " with error: " << err;
    } else {
        LOGNORMAL << "[migrate] Cleaning up for vol: " << volId << " dest node: " << srcNodeUuid <<
            " with error: " << err;
    }

    migrationDest.reset();
}

}  // namespace fds

