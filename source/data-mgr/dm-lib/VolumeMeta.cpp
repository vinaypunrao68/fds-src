/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <string>
#include <list>
#include <VolumeMeta.h>
#include <fds_process.h>
#include <util/timeutils.h>
#include <DataMgr.h>

namespace fds {

extern DataMgr *dataMgr;

VolumeMeta::VolumeMeta(const std::string& _name,
                       fds_int64_t _uuid,
                       VolumeDesc* desc)
              : fwd_state(VFORWARD_STATE_NONE),
                dmtclose_time(boost::posix_time::min_date_time)
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

Error
VolumeMeta::syncVolCat(fds_volid_t volId, NodeUuid node_uuid) {
    Error err(ERR_OK);
    fds_uint64_t start, end;
    int  returnCode = 0;
    const std::string vol_name =  dataMgr->getPrefix() +
            std::to_string(volId);
    const std::string node_name =  dataMgr->getPrefix() +
            std::to_string(node_uuid.uuid_get_val());
    fds_uint32_t node_ip   = 0;
    fds_uint32_t node_port = 0;
    fds_int32_t node_state = -1;

    LOGDEBUG << " syncVolCat: " << volId;

    const FdsRootDir *root = g_fdsprocess->proc_fdsroot();
    NodeAgent::pointer node = Platform::plf_dm_nodes()->agent_info(node_uuid);
    DmAgent::pointer dm = agt_cast_ptr<DmAgent>(node);
    const std::string dst_node = dm->get_node_root() + "user-repo/dm-names/";
    const std::string src_dir_vcat = root->dir_user_repo_dm() + vol_name + "_vcat.ldb";
    const std::string src_dir_tcat = root->dir_user_repo_dm() + vol_name + "_tcat.ldb";
    const std::string dst_dir = root->dir_user_repo_snap() + node_name + std::string("/");
    const std::string src_sync_vcat =  root->dir_user_repo_snap() +\
            node_name + std::string("/") + vol_name + "_vcat.ldb";
    const std::string src_sync_tcat =  root->dir_user_repo_snap() +\
            node_name + std::string("/") + vol_name + "_tcat.ldb";

    dataMgr->omClient->getNodeInfo(node_uuid.uuid_get_val(), &node_ip, &node_port, &node_state);
    std::string dest_ip = netSessionTbl::ipAddr2String(node_ip);

    const std::string test_cp = "cp -r "+src_dir_vcat+"*  "+dst_dir+" ";
    const std::string test_rsync = "sshpass -p passwd rsync -r "
            + dst_dir + "  root@" + dest_ip + ":" + dst_node + "";
    LOGDEBUG << " rsync: local copy  " << test_cp;
    LOGDEBUG << " rsync:  " << test_rsync;

    vol_mtx->lock();
    // err = vcat->DbSnap(root->dir_user_repo_dm() + "snap" + vol_name + "_vcat.ldb");
    returnCode = std::system((const char *)("cp -r "+src_dir_vcat+"  "+dst_dir+" ").c_str());
    returnCode = std::system((const char *)("cp -r "+src_dir_tcat+"  "+dst_dir+" ").c_str());
    vol_mtx->unlock();

    LOGDEBUG << "system Command  copy return Code : " << returnCode;

    if (!err.ok()) {
        LOGERROR << "Failed to create vol snap " << " with err " << err;
        return err;
    }

    start = fds::util::rdtsc();
    LOGDEBUG << " system Command rsync  start time: " <<  start;
    // rsync the meta data to the new DM nodes
    returnCode = std::system((const char *)("sshpass -p passwd rsync -r "+\
                                            src_sync_vcat+"  root@"+dest_ip+\
                                            ":"+dst_node+"").c_str());
    returnCode = std::system((const char *)("sshpass -p passwd rsync -r "+\
                                            src_sync_tcat+"  root@"+dest_ip+\
                                            ":"+dst_node+"").c_str());

    end = fds::util::rdtsc();
    if ((end - start)) {
        LOGDEBUG << " system Command rsync time: " <<  ((end - start) / fds::util::getClockTicks());
    }

    LOGNORMAL << "system Command :  return Code : " << returnCode;
    return err;
}

Error
VolumeMeta::deltaSyncVolCat(fds_volid_t volId, NodeUuid node_uuid) {
    Error err(ERR_OK);
    fds_uint64_t start, end;
    int  returnCode = 0;
    const std::string vol_name =  dataMgr->getPrefix() +
            std::to_string(volId);
    const std::string node_name =  dataMgr->getPrefix() +
            std::to_string(node_uuid.uuid_get_val());
    fds_uint32_t node_ip   = 0;
    fds_uint32_t node_port = 0;
    fds_int32_t node_state = -1;

    LOGDEBUG << " syncDeltaVolCat: " << volId;

    const FdsRootDir *root = g_fdsprocess->proc_fdsroot();
    NodeAgent::pointer node = Platform::plf_dm_nodes()->agent_info(node_uuid);
    DmAgent::pointer dm = agt_cast_ptr<DmAgent>(node);
    const std::string dst_node = dm->get_node_root() + "user-repo/dm-names/";
    const std::string src_dir_vcat = root->dir_user_repo_dm() + vol_name + "_vcat.ldb";
    const std::string src_dir_tcat = root->dir_user_repo_dm() + vol_name + "_tcat.ldb";
    const std::string dst_dir = root->dir_user_repo_snap() + node_name + std::string("/");;
    const std::string src_sync_vcat =  root->dir_user_repo_snap() + node_name +\
            std::string("/") + vol_name + "_vcat.ldb";
    const std::string src_sync_tcat =  root->dir_user_repo_snap() + node_name +\
            std::string("/") + vol_name + "_tcat.ldb";

    dataMgr->omClient->getNodeInfo(node_uuid.uuid_get_val(), &node_ip, &node_port, &node_state);
    std::string dest_ip = netSessionTbl::ipAddr2String(node_ip);


    const std::string test_cp = "cp -r "+src_dir_vcat+"*  "+dst_dir+" ";
    const std::string test_rsync = "sshpass -p passwd rsync -r "+dst_dir+\
            "  root@"+dest_ip+":"+dst_node+"";
    LOGDEBUG << " rsync: local copy  " << test_cp;
    LOGDEBUG << " rsync:  " << test_rsync;

    vol_mtx->lock();
    // err = vcat->DbSnap(root->dir_user_repo_dm() + "snap" + vol_name + "_vcat.ldb");
    /* clean the vcat and tcat ldb in the snap dir. */
    returnCode = std::system((const char *)("rm -rf  "+dst_dir+vol_name+"_vcat.ldb").c_str());
    returnCode = std::system((const char *)("rm -rf  "+dst_dir+vol_name+"_tcat.ldb").c_str());
    returnCode = std::system((const char *)("cp -r "+src_dir_vcat+"  "+dst_dir+" ").c_str());
    returnCode = std::system((const char *)("cp -r "+src_dir_tcat+"  "+dst_dir+" ").c_str());
    // we must set forwarding flag under the same lock we create snapshots
    // so that all in-flight updates waiting for this lock to update DB (those are updates that
    // will not be in the snapshot and will have to be forwarded) will be forwarded on commit
    fwd_state = VFORWARD_STATE_INPROG;
    vol_mtx->unlock();

    LOGNORMAL << "system Command  copy return Code : " << returnCode;

    if (!err.ok()) {
        LOGERROR << "Failed to create vol snap " << " with err " << err;
        return err;
    }

    start = fds::util::rdtsc();
    LOGDEBUG << " system Command rsync  start time: " <<  start;
    // rsync the meta data to the new DM nodes
    // returnCode = std::system((const char *)("sshpass -p passwd rsync -r "+
    // dst_dir+"  root@"+dest_ip+":/tmp").c_str());
    returnCode = std::system((const char *)("sshpass -p passwd rsync -r "+ \
                                            src_sync_vcat+"  root@"+dest_ip+ \
                                            ":"+dst_node+"").c_str());
    returnCode = std::system((const char *)("sshpass -p passwd rsync -r "+ \
                                            src_sync_tcat+"  root@"+dest_ip+":" \
                                            +dst_node+"").c_str());
    // returnCode = std::system((const char *)(
    // "rsync -r --rsh='sshpass -p passwd ssh -l root' "+dst+"/  root@"+
    // dest_ip+":"+dst_node+"").c_str());

    end = fds::util::rdtsc();
    if ((end - start))
        LOGDEBUG << " system Command rsync time: " <<  ((end - start) / fds::util::getClockTicks());

    LOGDEBUG << "system Command :  return Code : " << returnCode;
    return err;
}

// Returns true if volume is in forwarding state, and qos queue is empty;
// otherwise returns false (volume not in forwarding state or qos queue
// is not empty
//
void VolumeMeta::finishForwarding() {
    LOGNORMAL << "finishForwarding for volume " << *vol_desc
              << ", state " << fwd_state;

    vol_mtx->lock();
    if (fwd_state == VFORWARD_STATE_INPROG) {
        // set close time to now + small(ish) time interval, so that we also include
        // IO that are in fligt (sent from AM but did not reach DM's qos queue)
        dmtclose_time = boost::posix_time::microsec_clock::universal_time() +
                boost::posix_time::milliseconds(1600);
        fwd_state = VFORWARD_STATE_FINISHING;
        LOGDEBUG << "finishForwarding:  close time: " << dmtclose_time;
    } else {
        fwd_state = VFORWARD_STATE_NONE;
    }
    vol_mtx->unlock();
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
}

}  // namespace fds
