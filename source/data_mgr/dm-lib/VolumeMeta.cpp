/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <string>

#include <VolumeMeta.h>
#include <fds_process.h>
#include <util/fds_stat.h>
#include "DataMgr.h"

namespace fds {

  extern DataMgr *dataMgr;
/*
 * Currently the dm_log is NEEDED but set to NULL
 * when not passed in to the constructor. We should
 * build our own in class logger if we're not passed
 * one.
 */
/*
VolumeMeta::VolumeMeta()
  : vcat(NULL), tcat(NULL), dm_log(NULL) {

  vol_mtx = new fds_mutex("Volume Meta Mutex");
  vol_desc = new VolumeDesc(0);
}
*/
VolumeCatalog* VolumeMeta::getVcat()
{
      return vcat;
}

VolumeMeta::VolumeMeta(const std::string& _name,
                       fds_int64_t _uuid,
                       VolumeDesc* desc,
                       fds_bool_t crt_catalogs)
        : dm_log(NULL), vcat(NULL), tcat(NULL), fwd_state(VFORWARD_STATE_NONE)
{
    const FdsRootDir *root = g_fdsprocess->proc_fdsroot();

    vol_mtx = new fds_mutex("Volume Meta Mutex");
    vol_desc = new VolumeDesc(_name, _uuid);
    dmCopyVolumeDesc(vol_desc, desc);

    root->fds_mkdir(root->dir_user_repo_dm().c_str());
    if (crt_catalogs) {
        vcat = new VolumeCatalog(root->dir_user_repo_dm() + _name + "_vcat.ldb", crt_catalogs);
        tcat = new TimeCatalog(root->dir_user_repo_dm() + _name + "_tcat.ldb", crt_catalogs);
    }
}

VolumeMeta::VolumeMeta(const std::string& _name,
                       fds_int64_t _uuid,
                       fds_log* _dm_log,
                       VolumeDesc* _desc,
                       fds_bool_t crt_catalogs)
        : VolumeMeta(_name, _uuid, _desc, crt_catalogs) {

  dm_log = _dm_log;
}

VolumeMeta::~VolumeMeta() {
    if (vcat) {
        delete vcat;
    }
    if (tcat) {
        delete tcat;
    }
    delete vol_desc;
    delete vol_mtx;
}

/**
 * If this volume's catalogs were pushed from other DM, this method
 * is called when pusing volume's catalogs is done so they can be
 * now opened and ready for transactions
 */
void VolumeMeta::openCatalogs(fds_volid_t volid)
{
    const std::string vol_name =  dataMgr->getPrefix() +
                              std::to_string(volid);
    const FdsRootDir *root = g_fdsprocess->proc_fdsroot();
    fds_verify((vcat == NULL) && (tcat == NULL));
    vcat = new VolumeCatalog(root->dir_user_repo_dm() + vol_name + "_vcat.ldb", true);
    tcat = new TimeCatalog(root->dir_user_repo_dm() + vol_name + "_tcat.ldb", true);

    // TODO(xxx) unblock this volume's qos queue
}

Error VolumeMeta::OpenTransaction(const std::string blob_name,
                                  const BlobNode*& bnode, VolumeDesc* desc) {
  Error err(ERR_OK);

  /*
   * TODO: Just put it in the vcat for now.
   */


  Record key((const char *)blob_name.c_str(),
             blob_name.size());

  std::string val_string = bnode->ToString();

  Record val(val_string);

  vol_mtx->lock();
  err = vcat->Update(key, val);
  vol_mtx->unlock();

  return err;
}

/**
 * Returns true of the volume does not contain any
 * valid blobs. A valid blob is a non-deleted blob version.
 */
fds_bool_t VolumeMeta::isEmpty() const {
    // Lock the entire DB for now since levelDB's iterator
    // isn't thread-safe
    vol_mtx->lock();
    Catalog::catalog_iterator_t *dbIt = vcat->NewIterator();
    for (dbIt->SeekToFirst(); dbIt->Valid(); dbIt->Next()) {
        Record key = dbIt->key();
        std::string value(dbIt->value().ToString());
        BlobNode bnode(value);
        if (bnode.version != blob_version_deleted) {
            // Found a non-deleted blob in the volume
            return false;
        }
    }
    vol_mtx->unlock();

    return true;
}

Error VolumeMeta::listBlobs(std::list<BlobNode>& bNodeList) {
  Error err(ERR_OK);

  /*
   * Lock the entire DB for now since levelDB's iterator
   * isn't thread-safe
   */
  vol_mtx->lock();
  Catalog::catalog_iterator_t *dbIt = vcat->NewIterator();
  for (dbIt->SeekToFirst(); dbIt->Valid(); dbIt->Next()) {
    Record key = dbIt->key();
    std::string value(dbIt->value().ToString());

    FDS_PLOG_SEV(dm_log, fds::fds_log::normal) << "List blobs iterating over key "
                                               << key.ToString();
    BlobNode bnode(value);
    if (bnode.version != blob_version_deleted) {
        bNodeList.push_back(bnode);
    }
  }
  vol_mtx->unlock();

  return err;
}

Error VolumeMeta::QueryVcat(const std::string blob_name,
                            BlobNode*& bnode) {
  Error err(ERR_OK);

  bnode = NULL;

  Record key((const char *)blob_name.c_str(),
             blob_name.size());

  /*
   * The query will allocate the record.
   * TODO: Don't have the query allocate
   * anything. That's not safe.
   */
  std::string val = "";

  vol_mtx->lock();
  err = vcat->Query(key, &val);
  vol_mtx->unlock();

  if (! err.ok()) {
    FDS_PLOG(dm_log) << "Failed to query for vol " << *vol_desc
                     << " blob " << blob_name << " with err "
                     << err;
    return err;
  } 

  bnode = new BlobNode(val);

  return err;
}

Error VolumeMeta::DeleteVcat(const std::string blob_name) {
  Error err(ERR_OK);


  Record key((const char *)blob_name.c_str(),
             blob_name.size());


  vol_mtx->lock();
  err = vcat->Delete(key);
  vol_mtx->unlock();

  if (! err.ok()) {
    FDS_PLOG(dm_log) << "Failed to delete vol " << *vol_desc
                     << " blob " << blob_name << " with err "
                     << err;
    return err;
  } 

  return err;
}


Error
VolumeMeta::syncVolCat(fds_volid_t volId, NodeUuid node_uuid)
{

  Error err(ERR_OK);
  fds_uint64_t start, end;
  int  returnCode = 0;
  const std::string vol_name =  dataMgr->getPrefix() +
                              std::to_string(volId);
  fds_uint32_t node_ip   = 0;
  fds_uint32_t node_port = 0;
  fds_int32_t node_state = -1;

  FDS_PLOG(dm_log) << " syncVolCat: " << volId;

  const FdsRootDir *root = g_fdsprocess->proc_fdsroot();
  NodeAgent::pointer node = Platform::plf_dm_nodes()->agent_info(node_uuid);
  DmAgent::pointer dm = DmAgent::agt_cast_ptr(node);
  const std::string dst_node = dm->get_node_root() + "user-repo/dm-names/";
  const std::string src_dir_vcat = root->dir_user_repo_dm() + vol_name + "_vcat.ldb";
  const std::string src_dir_tcat = root->dir_user_repo_dm() + vol_name + "_tcat.ldb";
  const std::string dst_dir =  root->dir_user_repo_snap();
  const std::string src_sync_vcat =  root->dir_user_repo_snap() + vol_name + "_vcat.ldb";
  const std::string src_sync_tcat =  root->dir_user_repo_snap() + vol_name + "_tcat.ldb";

  dataMgr->omClient->getNodeInfo(node_uuid.uuid_get_val(), &node_ip, &node_port, &node_state);
  std::string dest_ip = netSessionTbl::ipAddr2String(node_ip);


  const std::string test_cp = "cp -r "+src_dir_vcat+"*  "+dst_dir+" ";
  const std::string test_rsync = "sshpass -p passwd rsync -r "+dst_dir+"  root@"+dest_ip+":"+dst_node+"";
  FDS_PLOG(dm_log) << " rsync: local copy  " << test_cp;
  FDS_PLOG(dm_log) << " rsync:  " << test_rsync;


  vol_mtx->lock();
  //err = vcat->DbSnap(root->dir_user_repo_dm() + "snap" + vol_name + "_vcat.ldb");
  returnCode = std::system((const char *)("cp -r "+src_dir_vcat+"  "+dst_dir+" ").c_str());
  returnCode = std::system((const char *)("cp -r "+src_dir_tcat+"  "+dst_dir+" ").c_str());
  vol_mtx->unlock();

  FDS_PLOG(dm_log) << "system Command  copy return Code : " << returnCode;

  if (! err.ok()) {
    FDS_PLOG(dm_log) << "Failed to create vol snap " << " with err " << err;
    return err;
  }

    start = fds_rdtsc();
    FDS_PLOG(dm_log) << " system Command rsync  start time: " <<  start;
  // rsync the meta data to the new DM nodes 
   // returnCode = std::system((const char *)("sshpass -p passwd rsync -r "+dst_dir+"  root@"+dest_ip+":/tmp").c_str());
   returnCode = std::system((const char *)("sshpass -p passwd rsync -r "+src_sync_vcat+"  root@"+dest_ip+":"+dst_node+"").c_str());
   returnCode = std::system((const char *)("sshpass -p passwd rsync -r "+src_sync_tcat+"  root@"+dest_ip+":"+dst_node+"").c_str());
  // returnCode = std::system((const char *)("rsync -r --rsh='sshpass -p passwd ssh -l root' "+dst+"/  root@"+dest_ip+":"+dst_node+"").c_str());
   
  end = fds_rdtsc();
  if ((end - start))
    FDS_PLOG(dm_log) << " system Command rsync time: " <<  ((end - start) / fds_tck_per_us());

    FDS_PLOG(dm_log) << "system Command :  return Code : " << returnCode;
  return err;
}


Error
VolumeMeta::deltaSyncVolCat(fds_volid_t volId, NodeUuid node_uuid)
{

  Error err(ERR_OK);
  fds_uint64_t start, end;
  int  returnCode = 0;
  const std::string vol_name =  dataMgr->getPrefix() +
                              std::to_string(volId);
  fds_uint32_t node_ip   = 0;
  fds_uint32_t node_port = 0;
  fds_int32_t node_state = -1;

  FDS_PLOG(dm_log) << " syncDeltaVolCat: " << volId;

  const FdsRootDir *root = g_fdsprocess->proc_fdsroot();
  NodeAgent::pointer node = Platform::plf_dm_nodes()->agent_info(node_uuid);
  DmAgent::pointer dm = DmAgent::agt_cast_ptr(node);
  const std::string dst_node = dm->get_node_root() + "user-repo/dm-names/";
  const std::string src_dir_vcat = root->dir_user_repo_dm() + vol_name + "_vcat.ldb";
  const std::string src_dir_tcat = root->dir_user_repo_dm() + vol_name + "_tcat.ldb";
  const std::string dst_dir =  root->dir_user_repo_snap();
  const std::string src_sync_vcat =  root->dir_user_repo_snap() + vol_name + "_vcat.ldb";
  const std::string src_sync_tcat =  root->dir_user_repo_snap() + vol_name + "_tcat.ldb";

  dataMgr->omClient->getNodeInfo(node_uuid.uuid_get_val(), &node_ip, &node_port, &node_state);
  std::string dest_ip = netSessionTbl::ipAddr2String(node_ip);


  const std::string test_cp = "cp -r "+src_dir_vcat+"*  "+dst_dir+" ";
  const std::string test_rsync = "sshpass -p passwd rsync -r "+dst_dir+"  root@"+dest_ip+":"+dst_node+"";
  FDS_PLOG(dm_log) << " rsync: local copy  " << test_cp;
  FDS_PLOG(dm_log) << " rsync:  " << test_rsync;

  /* clean the content of the snap dir. */
  returnCode = std::system((const char *)("rm -rf  "+dst_dir+"* ").c_str());
  returnCode = std::system((const char *)("rm -rf  "+dst_dir+"* ").c_str());

  vol_mtx->lock();
  //err = vcat->DbSnap(root->dir_user_repo_dm() + "snap" + vol_name + "_vcat.ldb");
  returnCode = std::system((const char *)("cp -r "+src_dir_vcat+"  "+dst_dir+" ").c_str());
  returnCode = std::system((const char *)("cp -r "+src_dir_tcat+"  "+dst_dir+" ").c_str());
  // we must set forwarding flag under the same lock we create snapshots
  // so that all in-flight updates waiting for this lock to update DB (those are updates that
  // will not be in the snapshot and will have to be forwarded) will be forwarded on commit
  fwd_state = VFORWARD_STATE_INPROG;
  vol_mtx->unlock();

  FDS_PLOG(dm_log) << "system Command  copy return Code : " << returnCode;

  if (! err.ok()) {
    FDS_PLOG(dm_log) << "Failed to create vol snap " << " with err " << err;
    return err;
  }

    start = fds_rdtsc();
    FDS_PLOG(dm_log) << " system Command rsync  start time: " <<  start;
  // rsync the meta data to the new DM nodes 
   // returnCode = std::system((const char *)("sshpass -p passwd rsync -r "+dst_dir+"  root@"+dest_ip+":/tmp").c_str());
   returnCode = std::system((const char *)("sshpass -p passwd rsync -r "+src_sync_vcat+"  root@"+dest_ip+":"+dst_node+"").c_str());
   returnCode = std::system((const char *)("sshpass -p passwd rsync -r "+src_sync_tcat+"  root@"+dest_ip+":"+dst_node+"").c_str());
  // returnCode = std::system((const char *)("rsync -r --rsh='sshpass -p passwd ssh -l root' "+dst+"/  root@"+dest_ip+":"+dst_node+"").c_str());
   
  end = fds_rdtsc();
  if ((end - start))
    FDS_PLOG(dm_log) << " system Command rsync time: " <<  ((end - start) / fds_tck_per_us());

    FDS_PLOG(dm_log) << "system Command :  return Code : " << returnCode;
  return err;
}

void VolumeMeta::finishForwarding() {
    vol_mtx->lock();
    // TODO(WIN-433) set state to VFORWARD_STATE_FINISHING here
    // so that we are still forwarding untill all queued updates
    // are processed, but for now, we assume no pending in queue
    fwd_state = VFORWARD_STATE_NONE;
    //    fwd_state = VFORWARD_STATE_FINISHING;
    vol_mtx->unlock();
}

void VolumeMeta::dmCopyVolumeDesc(VolumeDesc *v_desc, VolumeDesc *pVol) {
  v_desc->name = pVol->name;
  v_desc->volUUID = pVol->volUUID;
  v_desc->tennantId = pVol->tennantId;
  v_desc->localDomainId = pVol->localDomainId;
  v_desc->globDomainId = pVol->globDomainId;

  v_desc->capacity = pVol->capacity;
  v_desc->volType = pVol->volType;
  v_desc->maxQuota = pVol->maxQuota;
  v_desc->replicaCnt = pVol->replicaCnt;

  v_desc->consisProtocol = FDS_ProtocolInterface::
      FDSP_ConsisProtoType(pVol->consisProtocol);
  v_desc->appWorkload = pVol->appWorkload;
  v_desc->mediaPolicy = pVol->mediaPolicy;

  v_desc->volPolicyId = pVol->volPolicyId;
  v_desc->iops_max = pVol->iops_max;
  v_desc->iops_min = pVol->iops_min;
  v_desc->relativePrio = pVol->relativePrio;
}

std::ostream&
operator<<(std::ostream& out, const MetadataPair& mdPair) {
    out << "Blob metadata key=" << mdPair.key << " value="
        << mdPair.value;
    return out;
}

std::ostream&
operator<<(std::ostream& out, const BlobNode& bnode) {
    out << "Blob " << bnode.blob_name << ", version " << bnode.version
        << ", size " << bnode.blob_size << ", volume " << bnode.vol_id
        << ", entries: ";

    for (BlobObjectList::const_iterator it = bnode.obj_list.cbegin();
         it != bnode.obj_list.cend();
         it++) {
        out << "offset " << (*it).offset << " and size "
            << (*it).size << ", ";
    }

    return out;
}

std::ostream&
operator<<(std::ostream& out, const BlobObjectInfo& binfo) {
    out << "Blob info offset " << binfo.offset
        << ", object id " << binfo.data_obj_id
        << ", size " << binfo.size
        << std::boolalpha << ", sparse is "
        << binfo.sparse << ", blob end is "
        << binfo.blob_end;

    return out;
}

}  // namespace fds
