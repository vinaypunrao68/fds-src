/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <iostream>
#include <thread>

#include <policy_rpc.h>
#include <policy_tier.h>
#include "StorMgr.h"
#include "fds_obj_cache.h"

namespace fds {

fds_bool_t  stor_mgr_stopping = false;

#define FDS_XPORT_PROTO_TCP 1
#define FDS_XPORT_PROTO_UDP 2
#define FDSP_MAX_MSG_LEN (4*(4*1024 + 128))

ObjectStorMgr *objStorMgr;

/*
 * SM's Ice interface member functions
 */
ObjectStorMgrI::ObjectStorMgrI(const Ice::CommunicatorPtr& communicator) :
    _communicator(communicator) {
}

ObjectStorMgrI::~ObjectStorMgrI() {
}

void
ObjectStorMgrI::PutObject(const FDSP_MsgHdrTypePtr& msgHdr,
                          const FDSP_PutObjTypePtr& putObj,
                          const Ice::Current&) {
  FDS_PLOG(objStorMgr->GetLog()) << "Received a Putobject() network request";

#ifdef FDS_TEST_SM_NOOP
  msgHdr->msg_code = FDSP_MSG_PUT_OBJ_RSP;
  msgHdr->result = FDSP_ERR_OK;
  objStorMgr->swapMgrId(msgHdr);
  objStorMgr->fdspDataPathClient[msgHdr->src_node_name]->begin_PutObjectResp(msgHdr, putObj);
  FDS_PLOG(objStorMgr->GetLog()) << "FDS_TEST_SM_NOOP defined. Sent async PutObj response right after receiving req.";
  return;
#endif /* FDS_TEST_SM_NOOP */

  /*
   * Track the outstanding get request.
   * TODO: This is a total hack. We're overloading the msg_hdr's
   * msg_chksum field to track the outstanding request Id that
   * we pass into the SM.
   *
   * TODO: We should check if this value has rolled at some point.
   * Though it's big enough for us to not care right now.
   */
  fds_uint64_t reqId;
  reqId = std::atomic_fetch_add(&(objStorMgr->nextReqId), (fds_uint64_t)1);
  msgHdr->msg_chksum = reqId;
  objStorMgr->waitingReqMutex->lock();
  objStorMgr->waitingReqs[reqId] = msgHdr;
  objStorMgr->waitingReqMutex->unlock();

  objStorMgr->PutObject(msgHdr, putObj);

  /*
   * If we failed to enqueue the I/O return the error response
   * now as there is no more processing to do.
   */
  if (msgHdr->result != FDSP_ERR_OK) {
    objStorMgr->waitingReqMutex->lock();
    objStorMgr->waitingReqs.erase(reqId);
    objStorMgr->waitingReqMutex->unlock();

    msgHdr->msg_code = FDSP_MSG_PUT_OBJ_RSP;
    objStorMgr->swapMgrId(msgHdr);
    objStorMgr->fdspDataPathClient[msgHdr->src_node_name]->begin_PutObjectResp(msgHdr, putObj);

    FDS_PLOG(objStorMgr->GetLog()) << "Sent async PutObj response after receiving";
  }
}

void
ObjectStorMgrI::GetObject(const FDSP_MsgHdrTypePtr& msgHdr,
                          const FDSP_GetObjTypePtr& getObj,
                          const Ice::Current&) {
  FDS_PLOG(objStorMgr->GetLog()) << "Received a Getobject() network request";

#ifdef FDS_TEST_SM_NOOP
  msgHdr->msg_code = FDSP_MSG_GET_OBJ_RSP;
  msgHdr->result = FDSP_ERR_OK;
  objStorMgr->swapMgrId(msgHdr);
  objStorMgr->fdspDataPathClient[msgHdr->src_node_name]->begin_GetObjectResp(msgHdr, getObj);
  FDS_PLOG(objStorMgr->GetLog()) << "FDS_TEST_SM_NOOP defined. Sent async GetObj response right after receiving req.";
  return;
#endif /* FDS_TEST_SM_NOOP */

  /*
   * Track the outstanding get request.
   * TODO: This is a total hack. We're overloading the msg_hdr's
   * msg_chksum field to track the outstanding request Id that
   * we pass into the SM.
   *
   * TODO: We should check if this value has rolled at some point.
   * Though it's big enough for us to not care right now.
   */
  fds_uint64_t reqId;
  reqId = std::atomic_fetch_add(&(objStorMgr->nextReqId), (fds_uint64_t)1);
  msgHdr->msg_chksum = reqId;
  objStorMgr->waitingReqMutex->lock();
  objStorMgr->waitingReqs[reqId] = msgHdr;
  objStorMgr->waitingReqMutex->unlock();

  /*
   * Submit the request to be enqueued
   */
  objStorMgr->GetObject(msgHdr, getObj);

  /*
   * If we failed to enqueue the I/O return the error response
   * now as there is no more processing to do.
   */
  if (msgHdr->result != FDSP_ERR_OK) {
    objStorMgr->waitingReqMutex->lock();
    objStorMgr->waitingReqs.erase(reqId);
    objStorMgr->waitingReqMutex->unlock();

    msgHdr->msg_code = FDSP_MSG_GET_OBJ_RSP;
    objStorMgr->swapMgrId(msgHdr);
    objStorMgr->fdspDataPathClient[msgHdr->src_node_name]->begin_GetObjectResp(msgHdr, getObj);

    FDS_PLOG(objStorMgr->GetLog()) << "Sent async GetObj response after receiving";
  }
}

void
ObjectStorMgrI::DeleteObject(const FDSP_MsgHdrTypePtr& msgHdr,
                          const FDSP_DeleteObjTypePtr& delObj,
                          const Ice::Current&) {
  FDS_PLOG(objStorMgr->GetLog()) << "Received a Putobject() network request";

#ifdef FDS_TEST_SM_NOOP
  msgHdr->msg_code = FDSP_MSG_PUT_OBJ_RSP;
  msgHdr->result = FDSP_ERR_OK;
  objStorMgr->swapMgrId(msgHdr);
  objStorMgr->fdspDataPathClient[msgHdr->src_node_name]->begin_DeleteObjectResp(msgHdr, delObj);
  FDS_PLOG(objStorMgr->GetLog()) << "FDS_TEST_SM_NOOP defined. Sent async PutObj response right after receiving req.";
  return;
#endif /* FDS_TEST_SM_NOOP */

  /*
   * Track the outstanding get request.
   * TODO: This is a total hack. We're overloading the msg_hdr's
   * msg_chksum field to track the outstanding request Id that
   * we pass into the SM.
   *
   * TODO: We should check if this value has rolled at some point.
   * Though it's big enough for us to not care right now.
   */
  fds_uint64_t reqId;
  reqId = std::atomic_fetch_add(&(objStorMgr->nextReqId), (fds_uint64_t)1);
  msgHdr->msg_chksum = reqId;
  objStorMgr->waitingReqMutex->lock();
  objStorMgr->waitingReqs[reqId] = msgHdr;
  objStorMgr->waitingReqMutex->unlock();

  objStorMgr->DeleteObject(msgHdr, delObj);

  /*
   * If we failed to enqueue the I/O return the error response
   * now as there is no more processing to do.
   */
  if (msgHdr->result != FDSP_ERR_OK) {
    objStorMgr->waitingReqMutex->lock();
    objStorMgr->waitingReqs.erase(reqId);
    objStorMgr->waitingReqMutex->unlock();

    msgHdr->msg_code = FDSP_MSG_DELETE_OBJ_RSP;
    objStorMgr->swapMgrId(msgHdr);
    objStorMgr->fdspDataPathClient[msgHdr->src_node_name]->begin_DeleteObjectResp(msgHdr, delObj);

    FDS_PLOG(objStorMgr->GetLog()) << "Sent async PutObj response after receiving";
  }
}

void
ObjectStorMgrI::UpdateCatalogObject(const FDSP_MsgHdrTypePtr &msg_hdr, const FDSP_UpdateCatalogTypePtr& update_catalog , const Ice::Current&) {
  FDS_PLOG(objStorMgr->GetLog()) << "Wrong Interface Call: In the interface updatecatalog()";
}

void
ObjectStorMgrI::QueryCatalogObject(const FDSP_MsgHdrTypePtr &msg_hdr, const FDSP_QueryCatalogTypePtr& query_catalog , const Ice::Current&) {
  FDS_PLOG(objStorMgr->GetLog())<< "Wrong Interface Call: In the interface QueryCatalogObject()";
}

void
ObjectStorMgrI::DeleteCatalogObject(const FDSP_MsgHdrTypePtr &msg_hdr, const FDSP_DeleteCatalogTypePtr& query_catalog , const Ice::Current&) {
  FDS_PLOG(objStorMgr->GetLog())<< "Wrong Interface Call: In the interface DeleteCatalogObject()";
}

void
ObjectStorMgrI::OffsetWriteObject(const FDSP_MsgHdrTypePtr& msg_hdr, const FDSP_OffsetWriteObjTypePtr& offset_write_obj, const Ice::Current&) {
  FDS_PLOG(objStorMgr->GetLog()) << "In the interface offsetwrite()";
}

void
ObjectStorMgrI::RedirReadObject(const FDSP_MsgHdrTypePtr &msg_hdr, const FDSP_RedirReadObjTypePtr& redir_read_obj, const Ice::Current&) {
  FDS_PLOG(objStorMgr->GetLog()) << "In the interface redirread()";
}

void
ObjectStorMgrI::AssociateRespCallback(const Ice::Identity& ident, const std::string& src_node_name, const Ice::Current& current) {
  FDS_PLOG(objStorMgr->GetLog()) << "Associating response Callback client to ObjStorMgr for node " 
				 << src_node_name << " : " << _communicator->identityToString(ident);

  objStorMgr->fdspDataPathClient[src_node_name] = FDSP_DataPathRespPrx::uncheckedCast(current.con->createProxy(ident));
}

/**
 * Storage manager member functions
 * 
 * TODO: The number of test vols, the
 * totalRate, and number of qos threads
 * are being hard coded in the initializer
 * list below.
 */
ObjectStorMgr::ObjectStorMgr() :
    runMode(NORMAL_MODE),
    numTestVols(10),
    totalRate(500),
    qosThrds(10),
    port_num(0),
    shuttingDown(false),
    numWBThreads(1),
    maxDirtyObjs(10000),
    cp_port_num(0) {
  /*
   * TODO: Fix the totalRate above to not
   * be hard coded.
   */

  // Init  the log infra  
  sm_log = new fds_log("sm", "logs");
  FDS_PLOG(sm_log) << "Constructing the Object Storage Manager";
  objStorMutex = new fds_mutex("Object Store Mutex");
  waitingReqMutex = new fds_mutex("Object Store Mutex");

  /*
   * Init the outstanding request count to 0.
   */
  nextReqId = ATOMIC_VAR_INIT(0);

  /*
   * Will setup OM comm during run()
   */
  omClient = NULL;

  /*
   * Setup the tier related members
   */
  dirtyFlashObjs = new ObjQueue(maxDirtyObjs);
  fds_verify(dirtyFlashObjs->is_lock_free() == true);
  writeBackThreads = new fds_threadpool(numWBThreads);

  /* Set up the journal */
  //omJrnl = new TransJournal<ObjectID, ObjectIdJrnlEntry>();

  /*
   * Setup QoS related members.
   */
  qosCtrl = new SmQosCtrl(this,
                          qosThrds,
                          FDS_QoSControl::FDS_DISPATCH_WFQ,
                          sm_log);
  qosCtrl->runScheduler();

  /*
   * stats class init 
   */
  objStats =  new ObjStatsTracker(sm_log);

  /*
   * Performance stats recording
   * TODO: This is *not* a unique name, so
   * multiple SMs will clobber each other.
   * The prefix isn't parsed until later.
   * We should fix that.
   */
  Error err;
  perfStats = new PerfStats("migratorSmStats");
  err = perfStats->enable();
  fds_verify(err == ERR_OK);

  sysParams = NULL;
}

ObjectStorMgr::~ObjectStorMgr() {
  FDS_PLOG(objStorMgr->GetLog()) << " Destructing  the Storage  manager";
  shuttingDown = true;

  if (objStorDB)
    delete objStorDB;
  if (objIndexDB)
    delete objIndexDB;

  /*
   * Clean up the QoS system. Need to wait for I/Os to
   * complete and deregister each volume. The volume info
   * is freed when the table is deleted.
   * TODO: We should prevent further volume registration and
   * accepting network I/Os while shutting down.
   */
  std::list<fds_volid_t> volIds = volTbl->getVolList();
  for (std::list<fds_volid_t>::iterator vit = volIds.begin();
       vit != volIds.end();
       vit++) {
    qosCtrl->quieseceIOs((*vit));
    qosCtrl->deregisterVolume((*vit));
  }

  delete perfStats;

  /*
   * TODO: Assert that the waiting req map is empty.
   */

  delete waitingReqMutex;
  delete qosCtrl;

  delete writeBackThreads;
  delete dirtyFlashObjs;
  delete tierEngine;
  delete rankEngine;

  delete sm_log;
  delete volTbl;
  delete objStorMutex;
  //delete omJrnl;
}

void ObjectStorMgr::nodeEventOmHandler(int node_id,
                                       unsigned int node_ip_addr,
                                       int node_state,
                                       fds_uint32_t node_port,
                                       FDS_ProtocolInterface::FDSP_MgrIdType node_type)
{
    switch(node_state) {
       case FDS_Node_Up :
           FDS_PLOG(objStorMgr->GetLog()) << "ObjectStorMgr - Node UP event NodeId " << node_id << " Node IP Address " <<  node_ip_addr;
         break;

       case FDS_Node_Down:
       case FDS_Node_Rmvd:
           FDS_PLOG(objStorMgr->GetLog()) << " ObjectStorMgr - Node Down event NodeId :" << node_id << " node IP addr" << node_ip_addr ;
        break;
    }
}

/*
 * Note this function is generally run in the context
 * of an Ice thread.
 */
void
ObjectStorMgr::volEventOmHandler(fds_volid_t  volumeId,
                                 VolumeDesc  *vdb,
                                 int          action,
				 FDSP_ResultType result) {
  StorMgrVolume* vol = NULL;
  Error err = ERR_OK;

  fds_assert(vdb != NULL);

  switch(action) {
    case FDS_VOL_ACTION_CREATE :
      FDS_PLOG_SEV(objStorMgr->GetLog(), fds::fds_log::notification) << "Received create for vol "
                                     << "[" << volumeId << ", "
                                     << vdb->getName() << "]";
      /*
       * Needs to reference the global SM object
       * since this is a static function.
       */
      objStorMgr->volTbl->registerVolume(*vdb);
      vol = objStorMgr->volTbl->getVolume(volumeId);
      fds_assert(vol != NULL);
      err = objStorMgr->qosCtrl->registerVolume(vol->getVolId(),
                                          dynamic_cast<FDS_VolumeQueue*>(vol->getQueue()));
      objStorMgr->objCache->vol_cache_create(volumeId, 1024 * 1024 * 8, 1024 * 1024 * 256);
      fds_assert(err == ERR_OK);
      if (err != ERR_OK) {
	FDS_PLOG_SEV(objStorMgr->GetLog(), fds::fds_log::error) << "registration failed for vol id " << volumeId << " error: "
    			  << err;
      }
      break;

    case FDS_VOL_ACTION_DELETE:
      FDS_PLOG_SEV(objStorMgr->GetLog(), fds::fds_log::notification) << "Received delete for vol "
                                     << "[" << volumeId << ", "
                                     << vdb->getName() << "]";
      break;
  case fds_notify_vol_mod:
    FDS_PLOG_SEV(objStorMgr->GetLog(), fds::fds_log::notification) << "Received modify for vol "
                                     << "[" << volumeId << ", "
                                     << vdb->getName() << "]";

      vol = objStorMgr->volTbl->getVolume(volumeId);
      fds_assert(vol != NULL);
      vol->voldesc->modifyPolicyInfo(vdb->iops_min, vdb->iops_max, vdb->relativePrio);
      err = objStorMgr->qosCtrl->modifyVolumeQosParams(vol->getVolId(),
						       vdb->iops_min, vdb->iops_max, vdb->relativePrio);
      if ( !err.ok() )  {
	FDS_PLOG_SEV(objStorMgr->GetLog(), fds::fds_log::error) << "Modify volume policy failed for vol " << vdb->getName() << " error: "
								<< err.GetErrstr();
      }
      break;
    default:
      fds_panic("Unknown (corrupt?) volume event recieved!");
  }
}

void ObjectStorMgr::writeBackFunc(ObjectStorMgr *parent) {
  ObjectID   *objId;
  fds_bool_t  nonEmpty;
  Error       err;

  while (1) {

    if (parent->isShuttingDown()) {
      break;
    }

    /*
     * Find an object to write back.
     * TODO: Should add a bulk-object interface.
     */
    objId = NULL;
    nonEmpty = parent->popDirtyFlash(&objId);
    if (nonEmpty == false) {
      /*
       * If the queue is empty sleep for some
       * period of time.
       * Note I have no idea if this is the
       * correct amount of time or not.
       */
      FDS_PLOG(objStorMgr->GetLog()) << "Nothing dirty in flash, going to sleep...";
      sleep(5);
      continue;
    }
    fds_verify(objId != NULL);

    /*
     * Blocking call to write back (mirror) this object
     * to disk.
     * TODO: Mark the object as being accessed to ensure
     * it's not evicted by another thread in the meantime.
     */
    err = parent->writeBackObj(*objId);
    fds_verify(err == ERR_OK);

    delete objId;
  }
}

Error ObjectStorMgr::writeBackObj(const ObjectID &objId) {
  Error err(ERR_OK);

  FDS_PLOG(objStorMgr->GetLog()) << "Writing back object " << objId
                                 << " from flash to disk";

  /*
   * Read back the object from flash.
   * TODO: We should pin the object in cache when writing
   * to flash so that we can read it back from memory rather
   * than flash.
   */
  ObjectBuf objData;
  err = readObject(objId, objData);
  if (err != ERR_OK) {
    return err;
  }

  /*
   * Write object back to disk tier.
   */
  err = writeObject(objId, objData, diskio::diskTier);
  if (err != ERR_OK) {
    return err;
  }

  /*
   * Mark the object as 'clean' somewhere.
   */

  return err;
}

void ObjectStorMgr::unitTest() {
  Error err(ERR_OK);

  FDS_PLOG(objStorMgr->GetLog()) << "Running unit test";
  
  /*
   * Create fake objects
   */
  std::string object_data("Hi, I'm object data.");
  FDSP_PutObjType *put_obj_req;
  put_obj_req = new FDSP_PutObjType();

  put_obj_req->volume_offset = 0;
  put_obj_req->data_obj_id.hash_high = 0x00;
  put_obj_req->data_obj_id.hash_low = 0x101;
  put_obj_req->data_obj_len = object_data.length() + 1;
  put_obj_req->data_obj = object_data;

  /*
   * Create fake volume ID
   */
  fds_uint64_t vol_id   = 0xbeef;
  fds_uint32_t num_objs = 1;

  /*
   * Write fake object.
   * Note: we're just adding a hard coded 0 for
   * the request ID.
   */
  err = putObjectInternal(put_obj_req, vol_id, 0, num_objs);
  if (err != ERR_OK) {
    FDS_PLOG(objStorMgr->GetLog()) << "Failed to put object ";
    // delete put_obj_req;
    return;
  }
  // delete put_obj_req;

  FDSP_GetObjType *get_obj_req = new FDSP_GetObjType();
  memset((char *)&(get_obj_req->data_obj_id),
	 0x00,
	 sizeof(get_obj_req->data_obj_id));
  get_obj_req->data_obj_id.hash_low = 0x101;
  err = getObjectInternal(get_obj_req, vol_id, 1, num_objs);
  // delete get_obj_req;
}

Error
ObjectStorMgr::writeObjectLocation(const ObjectID& objId,
				   meta_obj_map_t *obj_map,
                                   fds_bool_t      append) {

  Error err(ERR_OK);

  diskio::MetaObjMap objMap;
  ObjectBuf          objData;

  if (append == true) {
    FDS_PLOG(objStorMgr->GetLog()) << "Appending new location for object " << objId;

    /*
     * Get existing object locations
     * TODO: We need a better way to update this
     * location DB with a new location. This requires
     * reading the existing locations, updating the entry,
     * and re-writing it. We often just want to append.
     */
    err = readObjectLocations(objId, objMap);
    if (err != ERR_OK && err != ERR_DISK_READ_FAILED) {
      FDS_PLOG(objStorMgr->GetLog()) << "Failed to read existing object locations"
                                     << " during location write";
      return err;
    } else if (err == ERR_DISK_READ_FAILED) {
      /*
       * Assume this error means the key just did not exist.
       * TODO: Add an err to differention "no key" from "failed read".
       */
      FDS_PLOG(objStorMgr->GetLog()) << "Not able to read existing object locations"
                                     << ", assuming no prior entry existed";
      err = ERR_OK;
    }
  }

  /*
   * Add new location to existing locations
   */
  objMap.updateMap(*obj_map);

  objData.size = objMap.marshalledSize();
  objData.data = std::string(objMap.marshalling(), objMap.marshalledSize());
  err = objStorDB->Put(objId, objData);
  if (err == ERR_OK) {
    FDS_PLOG(GetLog()) << "Updating object location for object "
                       << objId << " to " << objMap;
  } else {
    FDS_PLOG(GetLog()) << "Failed to put object " << objId
                       << " into odb with error " << err;
  }

  return err;
}

/*
 * Reads all object locations
 */
Error
ObjectStorMgr::readObjectLocations(const ObjectID     &objId,
                                   diskio::MetaObjMap &objMaps) {
  Error     err(ERR_OK);
  ObjectBuf objData;

  objData.size = 0;
  objData.data = "";
  err = objStorDB->Get(objId, objData);
  if (err == ERR_OK) {
    objData.size = objData.data.size();
    objMaps.unmarshalling(objData.data, objData.size);
  }

  return err;
}

Error
ObjectStorMgr::readObjectLocations(const ObjectID &objId,
                                   meta_obj_map_t *objMap) {
  Error err(ERR_OK);
  ObjectBuf objData;

  objData.size = 0;
  objData.data = "";
  err = objStorDB->Get(objId, objData);
  if (err == ERR_OK) {
    string_to_obj_map(objData.data, objMap);
    FDS_PLOG(GetLog()) << "Retrieving object location for object "
                       << objId << " as " << objData.data;
  } else {
    FDS_PLOG(GetLog()) << "No object location found for object " << objId << " in index DB";
  }
  return err;
}

Error
ObjectStorMgr::deleteObjectLocation(const ObjectID& objId) { 

  Error err(ERR_OK);
  meta_obj_map_t *obj_map;

  diskio::MetaObjMap objMap;
  ObjectBuf          objData;

    /*
     * Get existing object locations
     */
    err = readObjectLocations(objId, objMap);
    if (err != ERR_OK && err != ERR_DISK_READ_FAILED) {
      FDS_PLOG(objStorMgr->GetLog()) << "Failed to read existing object locations"
                                     << " during location write";
      return err;
    } else if (err == ERR_DISK_READ_FAILED) {
      /*
       * Assume this error means the key just did not exist.
       * TODO: Add an err to differention "no key" from "failed read".
       */
      FDS_PLOG(objStorMgr->GetLog()) << "Not able to read existing object locations"
                                     << ", assuming no prior entry existed";
      err = ERR_OK;
    }

  /*
   * Set the ref_cnt to 0, which will be the delete marker for this object and Garbage collector feeds on these objects
   */
  obj_map->obj_refcnt = -1;
  objData.size = objMap.marshalledSize();
  objData.data = std::string(objMap.marshalling(), objMap.marshalledSize());
  err = objStorDB->Put(objId, objData);
  if (err == ERR_OK) {
    FDS_PLOG(GetLog()) << "Setting the delete marker for object "
                       << objId << " to " << objMap;
  } else {
    FDS_PLOG(GetLog()) << "Failed to put object " << objId
                       << " into odb with error " << err;
  }

  return err;
}

Error
ObjectStorMgr::readObject(const ObjectID& objId,
			  ObjectBuf& objData) {
  diskio::DataTier tier;
  return readObject(objId, objData, tier);
}

/*
 * Note the tierUsed parameter is an output param.
 * It gets set in the function and the prior
 * value is unused.
 * It is only guaranteed to be set if success
 * is returned
 */
Error 
ObjectStorMgr::readObject(const ObjectID   &objId,
			  ObjectBuf        &objData,
                          diskio::DataTier &tierUsed) {
  Error err(ERR_OK);
  
  diskio::DataIO& dio_mgr = diskio::DataIO::disk_singleton();
  SmPlReq     *disk_req;
  meta_vol_io_t   vio;
  meta_obj_id_t   oid;
  vadr_set_inval(vio.vol_adr);

  /*
   * TODO: Why to we create another oid structure here?
   * Just pass a ref to objId?
   */
  oid.oid_hash_hi = objId.GetHigh();
  oid.oid_hash_lo = objId.GetLow();

  // create a blocking request object
  disk_req = new SmPlReq(vio, oid, (ObjectBuf *)&objData, true);

  /*
   * Read all of the object's locations
   */
  diskio::MetaObjMap objMap;
  err = readObjectLocations(objId, objMap);
  if (err == ERR_OK) {
    /*
     * Read obj from flash if we can
     */
    if (objMap.hasFlashMap() == true) {
      err = objMap.getFlashMap(*(disk_req->req_get_vmap()));
      if(err != ERR_OK) {
        delete disk_req;
        return err;
      }
    } else {
      /*
       * Read obj from disk
       */
      fds_verify(objMap.hasDiskMap() == true);
      err = objMap.getDiskMap(*(disk_req->req_get_vmap()));
      if(err != ERR_OK) {
        delete disk_req;
        return err;
      }
    }
    // Update the request with the tier info from disk
    disk_req->setTierFromMap();
    tierUsed = disk_req->getTier();

    FDS_PLOG(objStorMgr->GetLog()) << "Reading object " << objId << " from "
                                   << ((disk_req->getTier() == diskio::diskTier) ? "disk" : "flash")
                                   << " tier";
    objData.size = disk_req->req_get_vmap()->obj_size;
    objData.data.resize(objData.size, 0);
    dio_mgr.disk_read(disk_req);
  }
  delete disk_req;
  return err;
}

/**
 * Checks if an object ID already exists. An object
 * data buffer is passed so that inline buffer
 * comparison needs to be made.
 *
 * An err value of ERR_DUPLICATE is returned if the object
 * is a duplicate, ERR_OK if it is not a duplicate, and an
 * error if something else when wrong during the lookup.
 */
Error
ObjectStorMgr::checkDuplicate(const ObjectID&  objId,
                              const ObjectBuf& objCompData) {
  Error err(ERR_OK);

  ObjectBuf objGetData;

  /*
   * We need to fix this once diskmanager keeps track of object size
   * and allocates buffer automatically.
   * For now, we will pass the fixed block size for size and preallocate
   * memory for that size.
   */

  objGetData.size = 0;
  objGetData.data = "";
 
  err = readObject(objId, objGetData);
  
  if (err == ERR_OK) {
    /*
     * Perform an inline check that the data is the same.
     * This uses the std::string comparison function.
     * TODO: We need a better solution. This is going to
     * be crazy slow to always do an inline check.
     */
    if (objGetData.data == objCompData.data) { 
      err = ERR_DUPLICATE;
    } else {
      /*
       * Handle hash-collision - insert the next collision-id+obj-id 
       */
      err = ERR_HASH_COLLISION;
      fds_panic("Encountered a hash collision checking object %s. Bailing out now!",
                objId.ToHex().c_str());
    }
  } else if (err == ERR_DISK_READ_FAILED) {
    /*
     * This error indicates the DB entry was empty
     * so we can reset the error to OK.
     */
    err = ERR_OK;
  }

  return err;
}

/*
 * Note the tier parameter is an output param.
 * It gets set in the function and the prior
 * value is unused.
 * It is only guaranteed to be set if success
 * is returned.
 */
Error
ObjectStorMgr::writeObject(const ObjectID   &objId, 
                           const ObjectBuf  &objData,
                           fds_volid_t       volId,
                           diskio::DataTier &tier) {
  /*
   * Ask the tiering engine which tier to place this object
   */
  tier = tierEngine->selectTier(objId, volId);
  return writeObject(objId, objData, tier);
}

Error 
ObjectStorMgr::writeObject(const ObjectID  &objId, 
                           const ObjectBuf &objData,
                           diskio::DataTier tier) {
  Error err(ERR_OK);

  fds_verify((tier == diskio::diskTier) ||
             (tier == diskio::flashTier));
  
  diskio::DataIO& dio_mgr = diskio::DataIO::disk_singleton();
  SmPlReq     *disk_req;
  meta_vol_io_t   vio;
  meta_obj_id_t   oid;
  fds_bool_t      pushOk;
  vadr_set_inval(vio.vol_adr);

  /*
   * TODO: Why to we create another oid structure here?
   * Just pass a ref to objId?
   */
  oid.oid_hash_hi = objId.GetHigh();
  oid.oid_hash_lo = objId.GetLow();

  FDS_PLOG(objStorMgr->GetLog()) << "Writing object " << objId << " into the "
                                 << ((tier == diskio::diskTier) ? "disk" : "flash")
                                 << " tier";
  disk_req = new SmPlReq(vio, oid, (ObjectBuf *)&objData, true, tier); // blocking call
  dio_mgr.disk_write(disk_req);
  err = writeObjectLocation(objId, disk_req->req_get_vmap(), true);
  if ((err == ERR_OK) &&
      (tier == diskio::flashTier)) {
    /*
     * If written to flash, add to dirty flash list
     */
    pushOk = dirtyFlashObjs->push(new ObjectID(objId));
    fds_verify(pushOk == true);
  }

  delete disk_req;
  return err;
}


Error 
ObjectStorMgr::relocateObject(const ObjectID &objId,
                              diskio::DataTier from_tier,
                              diskio::DataTier to_tier) {
  
  Error err(ERR_OK);
  ObjectBuf objGetData;

  objGetData.size = 0;
  objGetData.data = "";
 
  err = readObject(objId, objGetData);
  
  diskio::DataIO& dio_mgr = diskio::DataIO::disk_singleton();
  SmPlReq     *disk_req;
  meta_vol_io_t   vio;
  meta_obj_id_t   oid;
  vadr_set_inval(vio.vol_adr);

  oid.oid_hash_hi = objId.GetHigh();
  oid.oid_hash_lo = objId.GetLow();

  disk_req = new SmPlReq(vio, oid, (ObjectBuf *)&objGetData, true, to_tier); 
  dio_mgr.disk_write(disk_req);
  err = writeObjectLocation(objId, disk_req->req_get_vmap(), false);

  if (to_tier == diskio::diskTier) {
    perfStats->recordIO(flashToDisk, 0, diskio::diskTier, FDS_IO_WRITE);
  } else {
    fds_verify(to_tier == diskio::flashTier);
    perfStats->recordIO(diskToFlash, 0, diskio::flashTier, FDS_IO_WRITE);
  }
  FDS_PLOG(objStorMgr->GetLog()) << "relocateObject " << objId << " into the "
                                 << ((to_tier == diskio::diskTier) ? "disk" : "flash")
                                 << " tier";
  delete disk_req;
  // Delete the object
  return err;
}

/*------------------------------------------------------------------------- ------------
 * FDSP Protocol internal processing 
 -------------------------------------------------------------------------------------*/

/**
 * Process a single object put.
 */
Error
ObjectStorMgr::putObjectInternal(SmIoReq* putReq) {
  Error err(ERR_OK);
  const ObjectID&  objId    = putReq->getObjId();
  fds_volid_t volId         = putReq->getVolId();
  diskio::DataTier tierUsed = diskio::maxTier;
  ObjBufPtrType objBufPtr = NULL;
  const FDSP_PutObjTypePtr& putObjReq = putReq->getPutObjReq();
  bool new_buff_allocated = false;

  //ObjectIdJrnlEntry* jrnlEntry =  omJrnl->get_journal_entry_for_key(objId);
  objStorMutex->lock();

  objBufPtr = objCache->object_retrieve(volId, objId);
  if (objBufPtr != NULL) {
    fds_verify(!(objCache->is_object_io_in_progress(volId, objId, objBufPtr)));

    // Now check for dedup here.
    if (objBufPtr->data == putObjReq->data_obj) {
      err = ERR_DUPLICATE;
    } else {
	/*
	 * Handle hash-collision - insert the next collision-id+obj-id 
	 */
      err = ERR_HASH_COLLISION;
    }
    objCache->object_release(volId, objId, objBufPtr);
  } 

  // Find if this object is a duplicate
  if (err == ERR_OK) {
    // Nothing in cache. Let's allocate a new buf from cache mgr and copy over the data

    objBufPtr = objCache->object_alloc(volId, objId, putObjReq->data_obj.size());
    memcpy((void *)objBufPtr->data.c_str(), (void *)putObjReq->data_obj.c_str(), putObjReq->data_obj.size()); 
    new_buff_allocated = true;

    err = checkDuplicate(objId,
			 *objBufPtr);

  }
  
  if (err == ERR_DUPLICATE) {
    if (new_buff_allocated) {
      objCache->object_add(volId, objId, objBufPtr, false);
      objCache->object_release(volId, objId, objBufPtr); 
    }
    objStorMutex->unlock();
    FDS_PLOG(objStorMgr->GetLog()) << "Put dup:  " << err
                                   << ", returning success";
    /*
     * Reset the err to OK to ack the metadata update.
     */
    err = ERR_OK;
  } else if (err != ERR_OK) {
    if (new_buff_allocated) {
      objCache->object_release(volId, objId, objBufPtr); 
      objCache->object_delete(volId, objId);
    }
    objStorMutex->unlock();
    FDS_PLOG(objStorMgr->GetLog()) << "Failed to check object duplicate status on put: "
                                   << err;
  } else {

    /*
     * Write the object and record which tier it went to
     */
    err = writeObject(objId, *objBufPtr, volId, tierUsed);

    if (err != fds::ERR_OK) {
      objCache->object_release(volId, objId, objBufPtr);       
      objCache->object_delete(volId, objId);
      objStorMutex->unlock();
      FDS_PLOG(objStorMgr->GetLog()) << "Failed to put object " << err;
    } else {
      objCache->object_add(volId, objId, objBufPtr, false);
      objCache->object_release(volId, objId, objBufPtr); 
      objStorMutex->unlock();
      FDS_PLOG(objStorMgr->GetLog()) << "Successfully put object " << objId;
      /* if we successfully put to flash -- notify ranking engine */
      if (tierUsed == diskio::flashTier) {
	StorMgrVolume *vol = volTbl->getVolume(volId);
	fds_verify(vol);
	rankEngine->rankAndInsertObject(objId, *(vol->voldesc)); 
      }
    }



    /*
     * Stores a reverse mapping from the volume's disk location
     * to the oid at that location.
     */
    /*
      TODO: Comment this back in!
      volTbl->createVolIndexEntry(putReq.io_vol_id,
      put_obj_req->volume_offset,
      put_obj_req->data_obj_id,
      put_obj_req->data_obj_len);
    */
  }

  //omJrnl->release_journal_entry_with_notify(jrnlEntry);
  qosCtrl->markIODone(*putReq,
                      tierUsed);

  /*
   * Prepare a response to send back.
   */
  waitingReqMutex->lock();
  fds_verify(waitingReqs.count(putReq->io_req_id) > 0);
  FDS_ProtocolInterface::FDSP_MsgHdrTypePtr msgHdr = waitingReqs[putReq->io_req_id];
  waitingReqs.erase(putReq->io_req_id);
  waitingReqMutex->unlock();

  FDS_ProtocolInterface::FDSP_PutObjTypePtr putObj =
      new FDS_ProtocolInterface::FDSP_PutObjType();
  putObj->data_obj_id.hash_high = objId.GetHigh();
  putObj->data_obj_id.hash_low  = objId.GetLow();
  putObj->data_obj_len          = putObjReq->data_obj_len;

  if (err == ERR_OK) {
    msgHdr->result = FDS_ProtocolInterface::FDSP_ERR_OK;
  } else {
    msgHdr->result = FDS_ProtocolInterface::FDSP_ERR_FAILED;
  }

  msgHdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_PUT_OBJ_RSP;
  swapMgrId(msgHdr);
  fdspDataPathClient[msgHdr->src_node_name]->begin_PutObjectResp(msgHdr, putObj);
  FDS_PLOG(objStorMgr->GetLog()) << "Sent async PutObj response after processing";

  /*
   * Free the IO request structure that
   * was allocated when the request was
   * enqueued.
   */
  delete putReq;

  return err;
}

Error
ObjectStorMgr::putObjectInternal(FDSP_PutObjTypePtr putObjReq, 
                                 fds_volid_t        volId, 
                                 fds_uint32_t       transId,
                                 fds_uint32_t       numObjs) {
  fds::Error err(fds::ERR_OK);
  ObjectID obj_id(putObjReq->data_obj_id.hash_high,
		  putObjReq->data_obj_id.hash_low);

  for(fds_uint32_t obj_num = 0; obj_num < numObjs; obj_num++) {
    
    /*
     * Allocate the ioReq structure. The pointer will get copied
     * into the queue and freed later once the IO completes.
     */
    SmIoReq *ioReq = new SmIoReq(putObjReq->data_obj_id.hash_high,
                                 putObjReq->data_obj_id.hash_low,
                                 // putObjReq->data_obj,
				 putObjReq,
                                 volId,
                                 FDS_IO_WRITE,
                                 transId);

    err = qosCtrl->enqueueIO(ioReq->getVolId(), static_cast<FDS_IOType*>(ioReq));
    if (err != ERR_OK) {
      /*
       * Just return if we see an error. The way the function is setup
       * it's a bit unnatrual to return errors for multiple IOs so
       * we'll just stop at the first error we see to make sure it
       * doesn't get lost.
       */
      FDS_PLOG(objStorMgr->GetLog()) << "Unable to enqueue putObject request "
                                     << transId;
      return err;
    }
    FDS_PLOG(objStorMgr->GetLog()) << "Successfully enqueued putObject request "
                                   << transId;
  }

   return err;
}

Error
ObjectStorMgr::deleteObjectInternal(SmIoReq* delReq) {
  Error err(ERR_OK);
  const ObjectID&  objId    = delReq->getObjId();
  fds_volid_t volId         = delReq->getVolId();
  ObjBufPtrType objBufPtr = NULL;
  const FDSP_DeleteObjTypePtr& delObjReq = delReq->getDeleteObjReq();


  objBufPtr = objCache->object_retrieve(volId, objId);
  if (objBufPtr != NULL) {
    objCache->object_release(volId, objId, objBufPtr);
    objCache->object_delete(volId, objId);
  } 

  //ObjectIdJrnlEntry* jrnlEntry =  omJrnl->get_journal_entry_for_key(objId);
  objStorMutex->lock();
  /*
   * Delete the object 
   */
  err = deleteObjectLocation(objId);
  objStorMutex->unlock();
  if (err != fds::ERR_OK) {
     FDS_PLOG(objStorMgr->GetLog()) << "Failed to delete object " << err;
  } else {
     FDS_PLOG(objStorMgr->GetLog()) << "Successfully delete object " << objId;
  }

  //omJrnl->release_journal_entry_with_notify(jrnlEntry);
  qosCtrl->markIODone(*delReq,
                      diskio::diskTier);

  /*
   * Prepare a response to send back.
   */
  waitingReqMutex->lock();
  fds_verify(waitingReqs.count(delReq->io_req_id) > 0);
  FDS_ProtocolInterface::FDSP_MsgHdrTypePtr msgHdr = waitingReqs[delReq->io_req_id];
  waitingReqs.erase(delReq->io_req_id);
  waitingReqMutex->unlock();

  FDS_ProtocolInterface::FDSP_DeleteObjTypePtr delObj =
      new FDS_ProtocolInterface::FDSP_DeleteObjType();
  delObj->data_obj_id.hash_high = objId.GetHigh();
  delObj->data_obj_id.hash_low  = objId.GetLow();
  delObj->data_obj_len          = objBufPtr->size;

  if (err == ERR_OK) {
    msgHdr->result = FDS_ProtocolInterface::FDSP_ERR_OK;
  } else {
    msgHdr->result = FDS_ProtocolInterface::FDSP_ERR_FAILED;
  }

  msgHdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_DELETE_OBJ_RSP;
  swapMgrId(msgHdr);
  fdspDataPathClient[msgHdr->src_node_name]->begin_DeleteObjectResp(msgHdr, delObj);
  FDS_PLOG(objStorMgr->GetLog()) << "Sent async DelObj response after processing";

  /*
   * Free the IO request structure that
   * was allocated when the request was
   * enqueued.
   */
  delete delReq;

  return err;
}

void
ObjectStorMgr::PutObject(const FDSP_MsgHdrTypePtr& fdsp_msg,
                         const FDSP_PutObjTypePtr& put_obj_req) {
  Error err(ERR_OK);

  // Verify the integrity of the FDSP msg using chksums
  // 
  // stor_mgr_verify_msg(fdsp_msg);
  //
  ObjectID oid(put_obj_req->data_obj_id.hash_high,
               put_obj_req->data_obj_id.hash_low);

  FDS_PLOG(objStorMgr->GetLog()) << "PutObject Obj ID: " << oid
                                 << ", glob_vol_id: " << fdsp_msg->glob_volume_id
                                 << ", for request ID: " << fdsp_msg->msg_chksum
                                 << ", Num Objs: " << fdsp_msg->num_objects;
  err = putObjectInternal(put_obj_req,
                          fdsp_msg->glob_volume_id,
                          fdsp_msg->msg_chksum,
                          fdsp_msg->num_objects);
  if (err != ERR_OK) {
    fdsp_msg->result = FDSP_ERR_FAILED;
    fdsp_msg->err_code = err.getIceErr();
  } else {
    fdsp_msg->result = FDSP_ERR_OK;
  }
}


void
ObjectStorMgr::DeleteObject(const FDSP_MsgHdrTypePtr& fdsp_msg,
                         const FDSP_DeleteObjTypePtr& del_obj_req) {
  Error err(ERR_OK);

  // Verify the integrity of the FDSP msg using chksums
  // 
  // stor_mgr_verify_msg(fdsp_msg);
  //
  ObjectID oid(del_obj_req->data_obj_id.hash_high,
               del_obj_req->data_obj_id.hash_low);

  FDS_PLOG(objStorMgr->GetLog()) << "PutObject Obj ID: " << oid
                                 << ", glob_vol_id: " << fdsp_msg->glob_volume_id
                                 << ", for request ID: " << fdsp_msg->msg_chksum
                                 << ", Num Objs: " << fdsp_msg->num_objects;
  err = deleteObjectInternal(del_obj_req,
                             fdsp_msg->glob_volume_id, 
                             fdsp_msg->msg_chksum);
  if (err != ERR_OK) {
    fdsp_msg->result = FDSP_ERR_FAILED;
    fdsp_msg->err_code = err.getIceErr();
  } else {
    fdsp_msg->result = FDSP_ERR_OK;
  }
}

Error
ObjectStorMgr::getObjectInternal(SmIoReq *getReq) {
  Error            err(ERR_OK);
  const ObjectID  &objId = getReq->getObjId();
  fds_volid_t volId      = getReq->getVolId();
  diskio::DataTier tierUsed = diskio::maxTier;
  ObjBufPtrType objBufPtr = NULL;

  /*
   * We need to fix this once diskmanager keeps track of object size
   * and allocates buffer automatically.
   * For now, we will pass the fixed block size for size and preallocate
   * memory for that size.
   */

  //ObjectIdJrnlEntry* jrnlEntry =  omJrnl->get_journal_entry_for_key(objId);
  objStorMutex->lock();
  objBufPtr = objCache->object_retrieve(volId, objId);

  if (!objBufPtr) {
    ObjectBuf objData;
    objData.size = 0;
    objData.data = "";
    err = readObject(objId, objData, tierUsed);
    objBufPtr = objCache->object_alloc(volId, objId, objData.size);
    memcpy((void *)objBufPtr->data.c_str(), (void *)objData.data.c_str(), objData.size);
    objCache->object_add(volId, objId, objBufPtr, false); // read data is always clean
  } else {
    fds_verify(!(objCache->is_object_io_in_progress(volId, objId, objBufPtr)));
  }

  objStorMutex->unlock();

  if (err != fds::ERR_OK) {
    FDS_PLOG(objStorMgr->GetLog()) << "Failed to get object " << objId
                                   << " with error " << err;
    /*
     * Set the data to empty so we don't return
     * garbage.
     */
  } else {
    FDS_PLOG(objStorMgr->GetLog()) << "Successfully got object " << objId
      // << " and data " << objData.data
                                   << " for request ID " << getReq->io_req_id;
  }

  //omJrnl->release_journal_entry_with_notify(jrnlEntry);
  qosCtrl->markIODone(*getReq, tierUsed);

  /*
   * Prepare a response to send back.
   */
  waitingReqMutex->lock();
  fds_assert(waitingReqs.count(getReq->io_req_id) > 0);
  FDS_ProtocolInterface::FDSP_MsgHdrTypePtr msgHdr = waitingReqs[getReq->io_req_id];
  waitingReqs.erase(getReq->io_req_id);
  waitingReqMutex->unlock();
  
  /*
   * This does an additional object copy into the network buffer.
   */
  FDS_ProtocolInterface::FDSP_GetObjTypePtr getObj =
    new FDS_ProtocolInterface::FDSP_GetObjType();
  fds_uint64_t oidHigh = objId.GetHigh();
  fds_uint64_t oidLow = objId.GetLow();
  getObj->data_obj_id.hash_high    = objId.GetHigh();
  getObj->data_obj_id.hash_low     = objId.GetLow();
  getObj->data_obj                 = (err == ERR_OK)? objBufPtr->data:"";
  getObj->data_obj_len             = (err == ERR_OK)? objBufPtr->size:0;

  if (err == ERR_OK) {
    msgHdr->result = FDS_ProtocolInterface::FDSP_ERR_OK;
  } else {
    msgHdr->result = FDS_ProtocolInterface::FDSP_ERR_FAILED;
  }
  msgHdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_GET_OBJ_RSP;
  swapMgrId(msgHdr);
  fdspDataPathClient[msgHdr->src_node_name]->begin_GetObjectResp(msgHdr, getObj);
  FDS_PLOG(objStorMgr->GetLog()) << "Sent async GetObj response after processing";
  
  objStats->updateIOpathStats(getReq->getVolId(), getReq->getObjId());
  volTbl->updateVolStats(getReq->getVolId());

  objStats->updateIOpathStats(getReq->getVolId(),getReq->getObjId());
  volTbl->updateVolStats(getReq->getVolId());

  objCache->object_release(volId, objId, objBufPtr);  

  /*
   * Free the IO request structure that
   * was allocated when the request was
   * enqueued.
   */
  delete getReq;

  return err;
}

Error
ObjectStorMgr::deleteObjectInternal(FDSP_DeleteObjTypePtr delObjReq, 
                                 fds_volid_t        volId,
                                 fds_uint32_t       transId) {
  Error err(ERR_OK);

  /*
   * Allocate and enqueue an IO request. The request is freed
   * when the IO completes.
   */
  SmIoReq *ioReq = new SmIoReq(delObjReq->data_obj_id.hash_high,
                               delObjReq->data_obj_id.hash_low,
                               // "",
			       delObjReq,
                               volId,
                               FDS_DELETE_BLOB,
                               transId);

  err = qosCtrl->enqueueIO(ioReq->getVolId(), static_cast<FDS_IOType*>(ioReq));

  if (err != fds::ERR_OK) {
    FDS_PLOG(objStorMgr->GetLog()) << "Unable to enqueue delObject request "
                                   << transId;
    return err;
  }
  FDS_PLOG(objStorMgr->GetLog()) << "Successfully enqueued delObject request "
                                 << transId;

  return err;
}

/*
 * TODO: The error response is currently returned via the msgHdr.
 * We should change the function to return an error. That's a
 * clearer interface.
 */
void 
ObjectStorMgr::GetObject(const FDSP_MsgHdrTypePtr& fdsp_msg,  
                         const FDSP_GetObjTypePtr& get_obj_req) {

    // Verify the integrity of the FDSP msg using chksums
    // 
    // stor_mgr_verify_msg(fdsp_msg);
    //
  Error err(ERR_OK);
  ObjectID oid(get_obj_req->data_obj_id.hash_high,
               get_obj_req->data_obj_id.hash_low);

  FDS_PLOG(objStorMgr->GetLog()) << "GetObject XID: " << fdsp_msg->msg_chksum
                                 << ", Obj ID: " << oid
                                 << ", glob_vol_id: " << fdsp_msg->glob_volume_id
                                 << ", Num Objs: " << fdsp_msg->num_objects;
   
  err = getObjectInternal(get_obj_req,
                          fdsp_msg->glob_volume_id,
                          fdsp_msg->msg_chksum,
                          fdsp_msg->num_objects);
  if (err != ERR_OK) {
    fdsp_msg->result = FDSP_ERR_FAILED;
    fdsp_msg->err_code = err.getIceErr();
  } else {
    fdsp_msg->result = FDSP_ERR_OK;
  }
}

/*------------------------------------------------------------------------------------- 
 * Storage Mgr main  processor : Listen on the socket and spawn or assign thread from a pool
 -------------------------------------------------------------------------------------*/
int
ObjectStorMgr::run(int argc, char* argv[]) {

  fds_bool_t      unit_test;
  fds_bool_t      useTestMode;
  std::string     endPointStr;
  DmDiskInfo     *info;
  DmDiskQuery     in;
  DmDiskQueryOut  out;
  std::string     omIpStr;
  fds_uint32_t    omConfigPort;
  
  unit_test    = false;
  useTestMode  = false;
  omConfigPort = 0;
  runMode = NORMAL_MODE;


  for (int i = 1; i < argc; i++) {
    std::string arg(argv[i]);
    if (arg == "--unit_test") {
      unit_test = true;
    } else if (strncmp(argv[i], "--cp_port=", 10) == 0) {
      cp_port_num = strtoul(argv[i] + 10, NULL, 0);
    } else if (strncmp(argv[i], "--port=", 7) == 0) {
      port_num = strtoul(argv[i] + 7, NULL, 0);
    } else if (strncmp(argv[i], "--om_ip=", 8) == 0) {
      omIpStr = argv[i] + 8;
    } else if (strncmp(argv[i], "--om_port=", 10) == 0) {
      omConfigPort = strtoul(argv[i] + 10, NULL, 0);
    } else if (strncmp(argv[i], "--prefix=", 9) == 0) {
      stor_prefix = argv[i] + 9;
    } else if (strncmp(argv[i], "--test_mode", 11) == 0) {
      useTestMode = true;
    }
  }

  GetLog()->setSeverityFilter((fds_log::severity_level) (getSysParams()->log_severity));

  if (useTestMode == true) {
    runMode = TEST_MODE;
  }

  // Create leveldb
  std::string filename= stor_prefix + "SNodeObjRepository";
  objStorDB  = new ObjectDB(filename);
  filename= stor_prefix + "SNodeObjIndex";
  objIndexDB  = new ObjectDB(filename); 

  Ice::PropertiesPtr props = communicator()->getProperties();

  if (cp_port_num == 0) {
    cp_port_num = props->getPropertyAsInt("ObjectStorMgrSvr.ControlPort");
  }

  if (port_num == 0) {
    /*
     * Pull the port from the config file if it wasn't
     * specified on the command line.
     */
    port_num = props->getPropertyAsInt("ObjectStorMgrSvr.PortNumber");
  }

  if (unit_test) {
    objStorMgr->unitTest();
    return 0;
  }

  FDS_PLOG(objStorMgr->GetLog()) << "Stor Mgr port_number :" << port_num;
  
  /*
   * Set basic thread properties.
   */
  props->setProperty("ObjectStorMgrSvr.ThreadPool.Client.Size", "200");
  props->setProperty("ObjectStorMgrSvr.ThreadPool.Client.SizeMax", "400");
  props->setProperty("ObjectStorMgrSvr.ThreadPool.Client.SizeWarn", "300");

  props->setProperty("ObjectStorMgrSvr.ThreadPool.Server.Size", "200");
  props->setProperty("ObjectStorMgrSvr.ThreadPool.Server.SizeMax", "400");
  props->setProperty("ObjectStorMgrSvr.ThreadPool.Server.SizeWarn", "300");

  std::ostringstream tcpProxyStr;
  tcpProxyStr << "tcp -p " << port_num;
  
  Ice::ObjectAdapterPtr adapter = communicator()->createObjectAdapterWithEndpoints("ObjectStorMgrSvr", tcpProxyStr.str());
  fdspDataPathServer = new ObjectStorMgrI(communicator());
  adapter->add(fdspDataPathServer, communicator()->stringToIdentity("ObjectStorMgrSvr"));

  callbackOnInterrupt();

  adapter->activate();

  struct ifaddrs *ifAddrStruct = NULL;
  struct ifaddrs *ifa          = NULL;
  void   *tmpAddrPtr           = NULL;

  /*
   * Get the local IP of the host.
   * This is needed by the OM.
   */
  getifaddrs(&ifAddrStruct);
  for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
    if (ifa->ifa_addr->sa_family == AF_INET) { // IPv4
      if (strncmp(ifa->ifa_name, "lo", 2) != 0) {
          tmpAddrPtr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
          char addrBuf[INET_ADDRSTRLEN];
          inet_ntop(AF_INET, tmpAddrPtr, addrBuf, INET_ADDRSTRLEN);
          myIp = std::string(addrBuf);
	  if (myIp.find("10.1") != std::string::npos)
	    break; /* TODO: more dynamic */
      }
    }
  }
  assert(myIp.empty() == false);
  FDS_PLOG(objStorMgr->GetLog()) << "Stor Mgr IP:" << myIp;

  /*
   * Query persistent layer for disk parameter details 
   */
  fds::DmQuery &query = fds::DmQuery::dm_query();
  in.dmq_mask = fds::dmq_disk_info;
  query.dm_disk_query(in, &out);
  /* we should be bundling multiple disk parameters into one message to OM TBD */ 
  FDS_ProtocolInterface::FDSP_AnnounceDiskCapabilityPtr dInfo =
      new FDS_ProtocolInterface::FDSP_AnnounceDiskCapability(); 
  while (1) {
    info = out.query_pop();
    if (info != nullptr) {
      FDS_PLOG(objStorMgr->GetLog()) << "Max blks capacity: " << info->di_max_blks_cap
                                     << ", Disk type........: " << info->di_disk_type
                                     << ", Max iops.........: " << info->di_max_iops
                                     << ", Min iops.........: " << info->di_min_iops
                                     << ", Max latency (us).: " << info->di_max_latency
                                     << ", Min latency (us).: " << info->di_min_latency;

      if ( info->di_disk_type == FDS_DISK_SATA) {
        dInfo->disk_iops_max =  info->di_max_iops; /*  max avarage IOPS */
        dInfo->disk_iops_min =  info->di_min_iops; /* min avarage IOPS */
        dInfo->disk_capacity = info->di_max_blks_cap;  /* size in blocks */
        dInfo->disk_latency_max = info->di_max_latency; /* in us second */
        dInfo->disk_latency_min = info->di_min_latency; /* in us second */
      } else if (info->di_disk_type == FDS_DISK_SSD) {
        dInfo->ssd_iops_max =  info->di_max_iops; /*  max avarage IOPS */
        dInfo->ssd_iops_min =  info->di_min_iops; /* min avarage IOPS */
        dInfo->ssd_capacity = info->di_max_blks_cap;  /* size in blocks */
        dInfo->ssd_latency_max = info->di_max_latency; /* in us second */
        dInfo->ssd_latency_min = info->di_min_latency; /* in us second */
      } else 
        FDS_PLOG(objStorMgr->GetLog()) << "Unknown Disk Type " << info->di_disk_type;

      delete info;
      continue;
    }
    break;
  }

  /*
   * Register this node with OM.
   */
  omClient = new OMgrClient(FDSP_STOR_MGR,
                            omIpStr,
                            omConfigPort,
                            myIp,
                            port_num,
                            stor_prefix + "localhost-sm",
                            sm_log);
  
  /*
   * Create local volume table. Create after omClient
   * is initialized, because it needs to register with the
   * omClient. Create before register with OM because
   * the OM vol event receivers depend on this table.
   */
  volTbl = new StorMgrVolumeTable(this);

  /* Create tier related classes -- has to be after volTbl is created */
  rankEngine = new ObjectRankEngine(stor_prefix, 100000, volTbl, objStats, objStorMgr->GetLog());
  tierEngine = new TierEngine(TierEngine::FDS_TIER_PUT_ALGO_BASIC_RANK, volTbl, rankEngine, objStorMgr->GetLog());
  objCache = new FdsObjectCache(1024 * 1024 * 256, 
				slab_allocator_type_default, 
				eviction_policy_type_default,
				objStorMgr->GetLog());

  std::thread *stats_thread = new std::thread(log_ocache_stats);

  /*
   * Register/boostrap from OM
   */
  omClient->initialize();
  omClient->registerEventHandlerForNodeEvents((node_event_handler_t)nodeEventOmHandler);
  omClient->registerEventHandlerForVolEvents((volume_event_handler_t)volEventOmHandler);
  omClient->omc_srv_pol = &sg_SMVolPolicyServ;
  omClient->startAcceptingControlMessages(cp_port_num);
  omClient->registerNodeWithOM(dInfo);

  /*
   * Create local variables for test mode
   */
  if (runMode == TEST_MODE) {
    /*
     * Create test volumes.
     */
    VolumeDesc*  testVdb;
    std::string testVolName;
    for (fds_uint32_t testVolId = 1; testVolId < numTestVols + 1; testVolId++) {
      testVolName = "testVol" + std::to_string(testVolId);
      /*
       * We're using the ID as the min/max/priority
       * for the volume QoS.
       */
      testVdb = new VolumeDesc(testVolName,
                               testVolId,
                               8+ testVolId,
                               10000,       /* high max iops so that unit tests does not take forever to finish */
                               testVolId);
      fds_assert(testVdb != NULL);
      if ( (testVolId % 3) == 0)
	testVdb->volType = FDSP_VOL_BLKDEV_DISK_TYPE;
      else if ( (testVolId % 3) == 1)
	testVdb->volType = FDSP_VOL_BLKDEV_SSD_TYPE;
      else 
	testVdb->volType = FDSP_VOL_BLKDEV_HYBRID_TYPE;

      volEventOmHandler(testVolId,
                        testVdb,
                        FDS_VOL_ACTION_CREATE,
			FDS_ProtocolInterface::FDSP_ERR_OK);

      delete testVdb;
    }
  }

  /*
   * Kick off the writeback thread(s)
   */
  writeBackThreads->schedule(writeBackFunc, this);
  
  communicator()->waitForShutdown();

  if (ifAddrStruct != NULL) {
    freeifaddrs(ifAddrStruct);
  }

  return EXIT_SUCCESS;
}

fds_log* ObjectStorMgr::GetLog() {
  return sm_log;
}

Error
ObjectStorMgr::getObjectInternal(FDSP_GetObjTypePtr getObjReq, 
                                 fds_volid_t        volId, 
                                 fds_uint32_t       transId, 
                                 fds_uint32_t       numObjs) {
  Error err(ERR_OK);

  /*
   * Allocate and enqueue an IO request. The request is freed
   * when the IO completes.
   */
  SmIoReq *ioReq = new SmIoReq(getObjReq->data_obj_id.hash_high,
                               getObjReq->data_obj_id.hash_low,
                               // "",
			       getObjReq,
                               volId,
                               FDS_IO_READ,
                               transId);

  err = qosCtrl->enqueueIO(ioReq->getVolId(), static_cast<FDS_IOType*>(ioReq));

  if (err != fds::ERR_OK) {
    FDS_PLOG(objStorMgr->GetLog()) << "Unable to enqueue getObject request "
                                   << transId;
    getObjReq->data_obj_len = 0;
    getObjReq->data_obj.assign("");
    return err;
  }
  FDS_PLOG(objStorMgr->GetLog()) << "Successfully enqueued getObject request "
                                 << transId;

  return err;
}

inline void ObjectStorMgr::swapMgrId(const FDSP_MsgHdrTypePtr& fdsp_msg) {
 FDSP_MgrIdType temp_id;
 long tmp_addr;
 fds_uint32_t tmp_port;

 temp_id = fdsp_msg->dst_id;
 fdsp_msg->dst_id = fdsp_msg->src_id;
 fdsp_msg->src_id = temp_id;

 tmp_addr = fdsp_msg->dst_ip_hi_addr;
 fdsp_msg->dst_ip_hi_addr = fdsp_msg->src_ip_hi_addr;
 fdsp_msg->src_ip_hi_addr = tmp_addr;

 tmp_addr = fdsp_msg->dst_ip_lo_addr;
 fdsp_msg->dst_ip_lo_addr = fdsp_msg->src_ip_lo_addr;
 fdsp_msg->src_ip_lo_addr = tmp_addr;

 tmp_port = fdsp_msg->dst_port;
 fdsp_msg->dst_port = fdsp_msg->src_port;
 fdsp_msg->src_port = tmp_port;
}


void getObjectExt(SmIoReq* getReq) {
     objStorMgr->getObjectInternal(getReq);
}

void putObjectExt(SmIoReq* putReq) {
     objStorMgr->putObjectInternal(putReq);
}

void delObjectExt(SmIoReq* putReq) {
     objStorMgr->deleteObjectInternal(putReq);
}


Error ObjectStorMgr::SmQosCtrl::processIO(FDS_IOType* _io) {
   Error err(ERR_OK);
   SmIoReq *io = static_cast<SmIoReq*>(_io);

   if (io->io_type == FDS_IO_READ) {
          FDS_PLOG(FDS_QoSControl::qos_log) << "Processing a get request";
          threadPool->schedule(getObjectExt,io);
   } else if (io->io_type == FDS_IO_WRITE) {
          FDS_PLOG(FDS_QoSControl::qos_log) << "Processing a put request";
          threadPool->schedule(putObjectExt,io);
   } else if (io->io_type == FDS_DELETE_BLOB) {
          FDS_PLOG(FDS_QoSControl::qos_log) << "Processing a put request";
          threadPool->schedule(delObjectExt,io);
   } else {
          assert(0);
   }

   return err;
}



void
ObjectStorMgr::interruptCallback(int) {
  communicator()->shutdown();
}

void log_ocache_stats() {

  while(1) {
    usleep(500000);
    objStorMgr->getObjCache()->log_stats_to_file("ocache_stats.dat");    
  }

}

}  // namespace fds

