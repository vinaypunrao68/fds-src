/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <string>

#include <VolumeMeta.h>
#include <fds_process.h>

namespace fds {

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

VolumeMeta::VolumeMeta(const std::string& _name,
                       fds_uint64_t _uuid,VolumeDesc* desc)
    : dm_log(NULL)
{
    const FdsRootDir *root = g_fdsprocess->proc_fdsroot();

    vol_mtx = new fds_mutex("Volume Meta Mutex");
    vol_desc = new VolumeDesc(_name, _uuid);
    dmCopyVolumeDesc(vol_desc, desc);

    root->fds_mkdir(root->dir_user_repo_dm().c_str());
    vcat = new VolumeCatalog(root->dir_user_repo_dm() + _name + "_vcat.ldb");
    tcat = new TimeCatalog(root->dir_user_repo_dm() + _name + "_tcat.ldb");
}

VolumeMeta::VolumeMeta(const std::string& _name,
                       fds_uint64_t _uuid,
                       fds_log* _dm_log,VolumeDesc* _desc)
    : VolumeMeta(_name, _uuid, _desc) {

  dm_log = _dm_log;
}

VolumeMeta::~VolumeMeta() {
  delete vcat;
  delete tcat;
  delete vol_desc;
  delete vol_mtx;
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
                                               << key.ToString() << " and value "
                     << value;
    bNodeList.push_back(BlobNode(value));
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

  FDS_PLOG(dm_log) << "Query for " << blob_name << " returned string: " << val
		   << " .  Constructed bnode: " << bnode->ToString();

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

  v_desc->volPolicyId = pVol->volPolicyId;
  v_desc->iops_max = pVol->iops_max;
  v_desc->iops_min = pVol->iops_min;
  v_desc->relativePrio = pVol->relativePrio;
}


}  // namespace fds
