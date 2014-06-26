/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <string>
#include <list>
#include <VolumeMeta.h>
#include <fds_process.h>
#include <util/timeutils.h>
#include "DataMgr.h"

namespace fds {

extern DataMgr *dataMgr;

MetadataPair::MetadataPair() {
}
MetadataPair::MetadataPair(const std::string &_key,
                           const std::string &_value) {
    key   = _key;
    value = _value;
}

MetadataPair::MetadataPair(fpi::FDSP_MetaDataPair mpair) {
    key = mpair.key;
    value = mpair.value;
}

uint32_t MetadataPair::write(serialize::Serializer* s) const {
    uint32_t b = 0;
    b += s->writeString(key);
    b += s->writeString(value);
    return b;
}

uint32_t MetadataPair::read(serialize::Deserializer* d) {
    uint32_t b = 0;
    key.clear(); value.clear();
    b += d->readString(key);
    b += d->readString(value);
    return b;
}
//------------------------------------------

BlobObjectInfo::BlobObjectInfo()
        : offset(0),
          size(0),
          sparse(false),
          blob_end(false) {
}

BlobObjectInfo::BlobObjectInfo(const fpi::FDSP_BlobObjectInfo& blob_obj_info)
        : BlobObjectInfo() {
    offset = blob_obj_info.offset;
    data_obj_id.SetId((const char *)blob_obj_info.data_obj_id.digest.c_str(),
                      blob_obj_info.data_obj_id.digest.length());
    size = blob_obj_info.size;
    sparse = false;
    blob_end = blob_obj_info.blob_end;
}

BlobObjectInfo::BlobObjectInfo(const BlobObjectInfo &rhs) {
    *this = rhs;
}

BlobObjectInfo::BlobObjectInfo(fds_uint64_t off,
                               fds_uint64_t _size)
        : BlobObjectInfo() {
    offset = off;
    size   = _size;
    sparse = true;
}

BlobObjectInfo::~BlobObjectInfo() {
}


BlobObjectInfo& BlobObjectInfo::operator= (const BlobObjectInfo &rhs) {
    offset      = rhs.offset;
    data_obj_id = rhs.data_obj_id;
    size        = rhs.size;
    sparse      = rhs.sparse;
    blob_end    = rhs.blob_end;
    return *this;
}

uint32_t BlobObjectInfo::write(serialize::Serializer* s) const {
    uint32_t bytes = 0;

    bytes += s->writeI64(offset);
    bytes += s->writeI64(size);
    bytes += s->writeI32(sparse);
    bytes += s->writeI32(blob_end);
    bytes += data_obj_id.write(s);
    return bytes;
}

uint32_t BlobObjectInfo::read(serialize::Deserializer* d) {
    uint32_t bytes = 0;

    bytes += d->readI64(offset);
    bytes += d->readI64(size);
    bytes += d->readI32(reinterpret_cast<int32_t&>(sparse));
    bytes += d->readI32(reinterpret_cast<int32_t&>(blob_end));
    bytes += data_obj_id.read(d);
    return bytes;
}

//================================================================================

BlobObjectList::BlobObjectList() {
}

BlobObjectList::BlobObjectList(fpi::FDSP_BlobObjectList& blob_obj_list)
        : BlobObjectList() {
    initFromFDSPObjList(blob_obj_list);
}

BlobObjectList::~BlobObjectList() {
}

bool BlobObjectList::hasObjectAtOffset(fds_uint64_t offset) const {
    const auto iter = find(offset);
    return iter != end();
}

const BlobObjectInfo& BlobObjectList::objectAtOffset(const fds_uint64_t offset) const {
    return at(offset);
}

void BlobObjectList::initFromFDSPObjList(fpi::FDSP_BlobObjectList& blob_obj_list) {
    clear();
    for (uint i = 0; i < blob_obj_list.size(); i++) {
        (*this)[blob_obj_list[i].offset] =  blob_obj_list[i];
    }
}

void BlobObjectList::ToFDSPObjList(fpi::FDSP_BlobObjectList& fdsp_obj_list) const {
    fdsp_obj_list.clear();
    for (const auto& blob : *this) {
        fpi::FDSP_BlobObjectInfo obj_info;
        obj_info.offset = blob.second.offset;
        obj_info.size = blob.second.size;
        obj_info.data_obj_id.digest = std::string((const char *)(blob.second.data_obj_id.GetId()),
                                                  (size_t)blob.second.data_obj_id.GetLen());
        fdsp_obj_list.push_back(obj_info);
    }
}

fds_uint32_t BlobObjectList::write(serialize::Serializer*  s) const {
    uint32_t bytes = 0;
    bytes += s->writeI64(size());
    for (const auto& item : *this){
        // prem: NO need to write the offset ..
        bytes += item.second.write(s);
    }
    return bytes;
}

fds_uint32_t BlobObjectList::read(serialize::Deserializer* d) {
    uint32_t bytes = 0;
    uint64_t sz;
    clear();
    bytes += d->readI64(reinterpret_cast<int64_t&>(sz));

    for (; sz > 0; --sz) {
        BlobObjectInfo blob;
        bytes += blob.read(d);
        insert(value_type(blob.offset, blob));
    }
    return bytes;
}

// ==========================================================
BlobNode::BlobNode() {
    // Init the new blob to an invalid version
    // until someone actually inits valid data.
    version = blob_version_invalid;
    blob_size = 0;
    meta_list.clear();
}

BlobNode::~BlobNode() {
}

BlobNode::BlobNode(fds_volid_t volId, const std::string &name) {
    vol_id = volId;
    blob_name = name;
}

BlobNode::BlobNode(fpi::FDSP_UpdateCatalogTypePtr cat_msg, fds_volid_t _vol_id) {
    initFromFDSPPayload(cat_msg, _vol_id);
}

void BlobNode::updateMetadata(const std::string &key, const std::string &value) {
    for (auto& meta : meta_list) {
        if (meta.key == key) {
            meta.value = value;
            return;
        }
    }

    // We don't already have the key, append it
    meta_list.push_back(MetadataPair(key, value));
}

uint32_t BlobNode::write(serialize::Serializer*  s) const {
    uint32_t bytes = 0;

    bytes += s->writeString(blob_name);
    bytes += s->writeI64(version);
    bytes += s->writeI64(vol_id);
    bytes += s->writeI64(blob_size);
    bytes += s->writeI32(blob_mime_type);
    bytes += s->writeI32(replicaCnt);
    bytes += s->writeI32(writeQuorum);
    bytes += s->writeI32(readQuorum);
    bytes += s->writeI32(consisProtocol);

    uint32_t sz = meta_list.size();
    bytes += s->writeI32(sz);
    for (const MetadataPair& meta : meta_list) {
        bytes += meta.write(s);
    }
    bytes += obj_list.write(s);
    return bytes;
}

uint32_t BlobNode::read(serialize::Deserializer* d) {
    uint32_t bytes = 0;
    blob_name.clear();
    bytes += d->readString(blob_name);
    bytes += d->readI64(version);
    bytes += d->readI64(reinterpret_cast<int64_t&>(vol_id));
    bytes += d->readI64(blob_size);
    bytes += d->readI32(blob_mime_type);
    bytes += d->readI32(replicaCnt);
    bytes += d->readI32(writeQuorum);
    bytes += d->readI32(readQuorum);
    bytes += d->readI32(consisProtocol);
    uint32_t sz = 0;
    bytes += d->readI32(sz);
    meta_list.clear();
    meta_list.reserve(sz);
    for ( ; sz > 0 ; --sz) {
        MetadataPair meta;
        bytes += meta.read(d);
        meta_list.push_back(meta);
    }
    bytes += obj_list.read(d);

    return bytes;
}

void BlobNode::initMetaListFromFDSPMetaList(fpi::FDSP_MetaDataList& mlist) {
    meta_list.clear();
    uint i = 0;
    for (i = 0; i < mlist.size(); i++) {
        meta_list.push_back(mlist[i]);
    }
}

void BlobNode::metaListToFDSPMetaList(fpi::FDSP_MetaDataList& mlist) const {
    mlist.clear();
    uint i = 0;
    for (i = 0; i < meta_list.size(); i++) {
        fpi::FDSP_MetaDataPair mpair;
        mpair.key = meta_list[i].key;
        mpair.value = meta_list[i].value;
        mlist.push_back(mpair);
    }
}


void BlobNode::initFromFDSPPayload(const fpi::FDSP_UpdateCatalogTypePtr cat_msg,
                                   fds_volid_t _vol_id) {
    // Since we're updaing the blob's contents,
    // bumps its version.
    // TODO(Andrew): This should be based on the vols versioning
    version++;
    blob_mime_type = 0;
    replicaCnt = writeQuorum = readQuorum = 0;
    consisProtocol = 0;
    vol_id = _vol_id;
    blob_name = cat_msg->blob_name;

    initMetaListFromFDSPMetaList(cat_msg->meta_list);
    obj_list.initFromFDSPObjList(cat_msg->obj_list);

    // TODO(Andrew): This calculation assumes that we're
    // setting the entire blobs contents. This needs to
    // change when we allow partial updates or sparse blobs.
    blob_size = 0;
    for (const auto& iter : obj_list) {
        blob_size += iter.second.size;
    }
}

void BlobNode::ToFdspPayload(fpi::FDSP_QueryCatalogTypePtr& query_msg) const {
    query_msg->blob_name = blob_name;
    query_msg->blob_size = blob_size;
    query_msg->blob_version = version;
    metaListToFDSPMetaList(query_msg->meta_list);
    obj_list.ToFDSPObjList(query_msg->obj_list);
}

// ==========================================================

VolumeCatalog* VolumeMeta::getVcat() {
    return vcat;
}

VolumeMeta::VolumeMeta(const std::string& _name,
                       fds_int64_t _uuid,
                       VolumeDesc* desc,
                       fds_bool_t crt_catalogs)
              : vcat(NULL), tcat(NULL), fwd_state(VFORWARD_STATE_NONE),
                dmtclose_time(boost::posix_time::min_date_time)
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
}

Error VolumeMeta::OpenTransaction(const std::string blob_name,
                                  const BlobNode*& bnode, VolumeDesc* desc) {
    Error err(ERR_OK);

    /*
     * TODO: Just put it in the vcat for now.
     */


    Record key(blob_name);

    std::string serializedData;
    LOGDEBUG << "storing bnode";
    bnode->getSerialized(serializedData);

    Record val(serializedData);

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

    // it's possible that vcat is NULL, if we just starting
    // to push meta for this volume from another DM -- in that
    // case return TRUE for isEmpty (if it's not empty on
    // other DMs, they will return false)
    if (!vcat) {
        vol_mtx->unlock();
        return true;
    }

    Catalog::catalog_iterator_t *dbIt = vcat->NewIterator();
    for (dbIt->SeekToFirst(); dbIt->Valid(); dbIt->Next()) {
        Record key = dbIt->key();
        BlobNode bnode;
        LOGDEBUG << "loading bnode";
        fds_verify(bnode.loadSerialized(dbIt->value().ToString()) == ERR_OK);
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
        LOGNORMAL << "List blobs iterating over key " << key.ToString();
        BlobNode bnode;
        LOGDEBUG << "loading bnode";
        fds_verify(bnode.loadSerialized(dbIt->value().ToString()) == ERR_OK);
        if (bnode.version != blob_version_deleted) {
            bNodeList.push_back(bnode);
        }
    }
    vol_mtx->unlock();

    return err;
}

Error VolumeMeta::QueryVcat(const std::string blob_name, BlobNode*& bnode) {
    Error err(ERR_OK);

    bnode = NULL;

    Record key(blob_name);

    /*
     * The query will allocate the record.
     * TODO: Don't have the query allocate
     * anything. That's not safe.
     */
    std::string val = "";

    vol_mtx->lock();
    err = vcat->Query(key, &val);
    vol_mtx->unlock();

    if (!err.ok()) {
        LOGERROR << "Failed to query for vol " << *vol_desc
                 << " blob " << blob_name << " with err "
                 << err;
        return err;
    }

    bnode = new BlobNode();
    LOGDEBUG << "loading bnode";
    fds_verify(bnode->loadSerialized(val) == ERR_OK);

    return err;
}

Error VolumeMeta::DeleteVcat(const std::string blob_name) {
    Error err(ERR_OK);


    Record key(blob_name);

    vol_mtx->lock();
    err = vcat->Delete(key);
    vol_mtx->unlock();

    if (!err.ok()) {
        LOGERROR << "Failed to delete vol " << *vol_desc
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
  const std::string node_name =  dataMgr->getPrefix() +
                              std::to_string(node_uuid.uuid_get_val());
  fds_uint32_t node_ip   = 0;
  fds_uint32_t node_port = 0;
  fds_int32_t node_state = -1;

  LOGDEBUG << " syncVolCat: " << volId;

  const FdsRootDir *root = g_fdsprocess->proc_fdsroot();
  NodeAgent::pointer node = Platform::plf_dm_nodes()->agent_info(node_uuid);
  DmAgent::pointer dm = DmAgent::agt_cast_ptr(node);
  const std::string dst_node = dm->get_node_root() + "user-repo/dm-names/";
  const std::string src_dir_vcat = root->dir_user_repo_dm() + vol_name + "_vcat.ldb";
  const std::string src_dir_tcat = root->dir_user_repo_dm() + vol_name + "_tcat.ldb";
  const std::string dst_dir = root->dir_user_repo_snap() + node_name + std::string("/");;
  const std::string src_sync_vcat =  root->dir_user_repo_snap() + node_name + std::string("/") + vol_name + "_vcat.ldb";
  const std::string src_sync_tcat =  root->dir_user_repo_snap() + node_name + std::string("/") + vol_name + "_tcat.ldb";

    dataMgr->omClient->getNodeInfo(node_uuid.uuid_get_val(), &node_ip, &node_port, &node_state);
    std::string dest_ip = netSessionTbl::ipAddr2String(node_ip);

    const std::string test_cp = "cp -r "+src_dir_vcat+"*  "+dst_dir+" ";
    const std::string test_rsync = "sshpass -p passwd rsync -r "
            + dst_dir + "  root@" + dest_ip + ":" + dst_node + "";
    LOGNORMAL << " rsync: local copy  " << test_cp;
    LOGNORMAL << " rsync:  " << test_rsync;

    vol_mtx->lock();
    // err = vcat->DbSnap(root->dir_user_repo_dm() + "snap" + vol_name + "_vcat.ldb");
    returnCode = std::system((const char *)("cp -r "+src_dir_vcat+"  "+dst_dir+" ").c_str());
    returnCode = std::system((const char *)("cp -r "+src_dir_tcat+"  "+dst_dir+" ").c_str());
    vol_mtx->unlock();

    LOGNORMAL << "system Command  copy return Code : " << returnCode;

    if (!err.ok()) {
        LOGNORMAL << "Failed to create vol snap " << " with err " << err;
        return err;
    }

    start = fds::util::rdtsc();
    LOGDEBUG << " system Command rsync  start time: " <<  start;
  // rsync the meta data to the new DM nodes 
   returnCode = std::system((const char *)("sshpass -p passwd rsync -r "+src_sync_vcat+"  root@"+dest_ip+":"+dst_node+"").c_str());
   returnCode = std::system((const char *)("sshpass -p passwd rsync -r "+src_sync_tcat+"  root@"+dest_ip+":"+dst_node+"").c_str());
   
  end = fds::util::rdtsc();
  if ((end - start))
    LOGDEBUG << " system Command rsync time: " <<  ((end - start) / fds::util::getClockTicks());

    LOGNORMAL << "system Command :  return Code : " << returnCode;
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
  const std::string node_name =  dataMgr->getPrefix() +
                              std::to_string(node_uuid.uuid_get_val());
  fds_uint32_t node_ip   = 0;
  fds_uint32_t node_port = 0;
  fds_int32_t node_state = -1;

  LOGDEBUG << " syncDeltaVolCat: " << volId;

  const FdsRootDir *root = g_fdsprocess->proc_fdsroot();
  NodeAgent::pointer node = Platform::plf_dm_nodes()->agent_info(node_uuid);
  DmAgent::pointer dm = DmAgent::agt_cast_ptr(node);
  const std::string dst_node = dm->get_node_root() + "user-repo/dm-names/";
  const std::string src_dir_vcat = root->dir_user_repo_dm() + vol_name + "_vcat.ldb";
  const std::string src_dir_tcat = root->dir_user_repo_dm() + vol_name + "_tcat.ldb";
  const std::string dst_dir = root->dir_user_repo_snap() + node_name + std::string("/");;
  const std::string src_sync_vcat =  root->dir_user_repo_snap() + node_name + std::string("/") + vol_name + "_vcat.ldb";
  const std::string src_sync_tcat =  root->dir_user_repo_snap() + node_name + std::string("/") + vol_name + "_tcat.ldb";

  dataMgr->omClient->getNodeInfo(node_uuid.uuid_get_val(), &node_ip, &node_port, &node_state);
  std::string dest_ip = netSessionTbl::ipAddr2String(node_ip);


  const std::string test_cp = "cp -r "+src_dir_vcat+"*  "+dst_dir+" ";
  const std::string test_rsync = "sshpass -p passwd rsync -r "+dst_dir+"  root@"+dest_ip+":"+dst_node+"";
  LOGDEBUG << " rsync: local copy  " << test_cp;
  LOGDEBUG << " rsync:  " << test_rsync;


  vol_mtx->lock();
  /* clean the content of the snap dir. */
  returnCode = std::system((const char *)("rm -rf  "+dst_dir+"* ").c_str());
  returnCode = std::system((const char *)("rm -rf  "+dst_dir+"* ").c_str());
  returnCode = std::system((const char *)("cp -r "+src_dir_vcat+"  "+dst_dir+" ").c_str());
  returnCode = std::system((const char *)("cp -r "+src_dir_tcat+"  "+dst_dir+" ").c_str());
  // we must set forwarding flag under the same lock we create snapshots
  // so that all in-flight updates waiting for this lock to update DB (those are updates that
  // will not be in the snapshot and will have to be forwarded) will be forwarded on commit
  fwd_state = VFORWARD_STATE_INPROG;
  vol_mtx->unlock();

  LOGNORMAL << "system Command  copy return Code : " << returnCode;

  if (! err.ok()) {
    LOGERROR << "Failed to create vol snap " << " with err " << err;
    return err;
  }

    start = fds::util::rdtsc();
    LOGDEBUG << " system Command rsync  start time: " <<  start;
  // rsync the meta data to the new DM nodes 
   // returnCode = std::system((const char *)("sshpass -p passwd rsync -r "+dst_dir+"  root@"+dest_ip+":/tmp").c_str());
   returnCode = std::system((const char *)("sshpass -p passwd rsync -r "+src_sync_vcat+"  root@"+dest_ip+":"+dst_node+"").c_str());
   returnCode = std::system((const char *)("sshpass -p passwd rsync -r "+src_sync_tcat+"  root@"+dest_ip+":"+dst_node+"").c_str());
  // returnCode = std::system((const char *)("rsync -r --rsh='sshpass -p passwd ssh -l root' "+dst+"/  root@"+dest_ip+":"+dst_node+"").c_str());
   
  end = fds::util::rdtsc();
  if ((end - start))
    LOGDEBUG << " system Command rsync time: " <<  ((end - start) / fds::util::getClockTicks());

    LOGDEBUG << "system Command :  return Code : " << returnCode;
  return err;
}

//
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
    }
    else {
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

std::ostream& operator<<(std::ostream& out, const MetadataPair& mdPair) {
    out << "{meta:" << mdPair.key << "=" << mdPair.value <<"}";
    return out;
}

std::ostream& operator<<(std::ostream& out, const BlobNode& bnode) {
    out << "["
        << " blob:" << bnode.blob_name
        << " version:" << bnode.version
        << " size:" << bnode.blob_size
        << " volume:" << bnode.vol_id
        << " entries: ("
        << bnode.obj_list << ")";


    out << " meta: ";
    for (const auto& meta :  bnode.meta_list) {
        out << meta << " ";
    }

    out << "]";

    return out;
}

std::ostream& operator<<(std::ostream& out, const BlobObjectInfo& binfo) {
    out << "["
        << " offset:" << binfo.offset
        << " objectid:" << binfo.data_obj_id
        << " size:" << binfo.size
        << " sparse:" << std::boolalpha << binfo.sparse
        << " end:" << binfo.blob_end
        << "]";

    return out;
}

std::ostream& operator<<(std::ostream& out, const BlobObjectList& blobObjectList) {
    for (const auto& blobObject : blobObjectList) {
        out << blobObject.second << " ";
    }
    return out;
}
}  // namespace fds
