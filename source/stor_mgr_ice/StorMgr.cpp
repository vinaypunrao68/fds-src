/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <iostream>

#include <policy_rpc.h>
#include <policy_tier.h>
#include "StorMgr.h"
#include "DiskMgr.h"

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
ObjectStorMgrI::UpdateCatalogObject(const FDSP_MsgHdrTypePtr &msg_hdr, const FDSP_UpdateCatalogTypePtr& update_catalog , const Ice::Current&) {
  FDS_PLOG(objStorMgr->GetLog()) << "Wrong Interface Call: In the interface updatecatalog()";
}

void
ObjectStorMgrI::QueryCatalogObject(const FDSP_MsgHdrTypePtr &msg_hdr, const FDSP_QueryCatalogTypePtr& query_catalog , const Ice::Current&) {
  FDS_PLOG(objStorMgr->GetLog())<< "Wrong Interface Call: In the interface QueryCatalogObject()";
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
    totalRate(200),
    qosThrds(10),
    port_num(0),
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
}

ObjectStorMgr::~ObjectStorMgr()
{
  FDS_PLOG(objStorMgr->GetLog()) << " Destructing  the Storage  manager";
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

  /*
   * TODO: Assert that the waiting req map is empty.
   */

  delete waitingReqMutex;
  delete qosCtrl;

  delete tierEngine;
  delete rankEngine;

  delete sm_log;
  delete volTbl;
  delete objStorMutex;
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
                                 int          action) {
  StorMgrVolume* vol = NULL;

  switch(action) {
    case FDS_VOL_ACTION_CREATE :
      FDS_PLOG(objStorMgr->GetLog()) << "Received create for vol "
                                     << "[" << volumeId << ", "
                                     << vdb->getName() << "]";
      fds_assert(vdb != NULL);

      /*
       * Needs to reference the global SM object
       * since this is a static function.
       */
      objStorMgr->volTbl->registerVolume(*vdb);
      vol = objStorMgr->volTbl->getVolume(volumeId);
      fds_assert(vol != NULL);
      objStorMgr->qosCtrl->registerVolume(vol->getVolId(),
                                          dynamic_cast<FDS_VolumeQueue*>(vol->getQueue()));
      break;

    case FDS_VOL_ACTION_DELETE:
      FDS_PLOG(objStorMgr->GetLog()) << "Received delete for vol "
                                     << "[" << volumeId << ", "
                                     << vdb->getName() << "]";
      break;
    default:
      fds_panic("Unknown (corrupt?) volume event recieved!");
  }
}

void ObjectStorMgr::unitTest()
{
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
				   meta_obj_map_t *obj_map) {

  Error err(ERR_OK);

  ObjectBuf objData;

  string obj_map_string = obj_map_to_string(obj_map);
  objData.size = obj_map_string.size();
  objData.data = obj_map_string;
  err = objStorDB->Put(objId, objData);
  FDS_PLOG(GetLog()) << "Updating object location for object " << objId << " to " << objData.data;
  return err;

}

Error
ObjectStorMgr::readObjectLocation(const ObjectID& objId,
				  meta_obj_map_t *obj_map) {

  Error err(ERR_OK);
  ObjectBuf objData;

  objData.size = 0;
  objData.data = "";
  err = objStorDB->Get(objId, objData);
  if (err == ERR_OK) {
    string_to_obj_map(objData.data, obj_map);
    FDS_PLOG(GetLog()) << "Retrieving object location for object "
                       << objId << " as " << objData.data;
  } else {
    FDS_PLOG(GetLog()) << "No object location found for object " << objId << " in index DB";
  }
  return err;
}

Error 
ObjectStorMgr::readObject(const ObjectID& objId, 
			  ObjectBuf& objData)
{
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

  disk_req = new SmPlReq(vio, oid, (ObjectBuf *)&objData, true); // blocking call
  err = readObjectLocation(objId, disk_req->req_get_vmap());
  if (err == ERR_OK) {
    // Update the request with the tier info from disk
    disk_req->setTierFromMap();
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

Error 
ObjectStorMgr::writeObject(const ObjectID  &objId, 
                           const ObjectBuf &objData,
                           fds_volid_t      volId) {
  Error err(ERR_OK);
  
  diskio::DataIO& dio_mgr = diskio::DataIO::disk_singleton();
  SmPlReq     *disk_req;
  meta_vol_io_t   vio;
  meta_obj_id_t   oid;
  diskio::DataTier tier;
  vadr_set_inval(vio.vol_adr);

  /*
   * TODO: Why to we create another oid structure here?
   * Just pass a ref to objId?
   */
  oid.oid_hash_hi = objId.GetHigh();
  oid.oid_hash_lo = objId.GetLow();

  tier = tierEngine->selectTier(objId, volId);
  disk_req = new SmPlReq(vio, oid, (ObjectBuf *)&objData, true, tier); // blocking call
  dio_mgr.disk_write(disk_req);
  err = writeObjectLocation(objId, disk_req->req_get_vmap());

  FDS_PLOG(objStorMgr->GetLog()) << "Writing object " << objId << " into the "
                                 << ((tier == diskio::diskTier) ? "disk" : "flash")
                                 << " tier";
  delete disk_req;
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
  const ObjectID&  objId   = putReq->getObjId();
  const ObjectBuf& objData = putReq->getObjData();
  fds_volid_t volId        = putReq->getVolId();

  objStorMutex->lock();

  // Find if this object is a duplicate
  err = checkDuplicate(objId,
		       objData);
  
  if (err == ERR_DUPLICATE) {
    objStorMutex->unlock();
    FDS_PLOG(objStorMgr->GetLog()) << "Put dup:  " << err
                                   << ", returning success";
    /*
     * Reset the err to OK to ack the metadata update.
     */
    err = ERR_OK;
  } else if (err != ERR_OK) {
    objStorMutex->unlock();
    FDS_PLOG(objStorMgr->GetLog()) << "Failed to check object duplicate status on put: "
                                   << err;
  } else {
    /*
     * This is the levelDB insertion. It's a totally
     * separate DB from the one above.
     */
    err = writeObject(objId, objData, volId);
    objStorMutex->unlock();

    if (err != fds::ERR_OK) {
      FDS_PLOG(objStorMgr->GetLog()) << "Failed to put object " << err;
    } else {
      FDS_PLOG(objStorMgr->GetLog()) << "Successfully put object " << objId;
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

  qosCtrl->markIODone(*putReq);

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
  putObj->data_obj_len          = objData.size;

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

  for(fds_uint32_t obj_num = 0; obj_num < numObjs; obj_num++) {
    
    /*
     * Allocate the ioReq structure. The pointer will get copied
     * into the queue and freed later once the IO completes.
     */
    SmIoReq *ioReq = new SmIoReq(putObjReq->data_obj_id.hash_high,
                                 putObjReq->data_obj_id.hash_low,
                                 putObjReq->data_obj,
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

Error
ObjectStorMgr::getObjectInternal(SmIoReq *getReq) {
  Error           err(ERR_OK);
  const ObjectID& objId  = getReq->getObjId();
  ObjectBuf       objData;
  /*
   * We need to fix this once diskmanager keeps track of object size
   * and allocates buffer automatically.
   * For now, we will pass the fixed block size for size and preallocate
   * memory for that size.
   */
  objData.size = 0;
  objData.data = "";

  objStorMutex->lock();
  err = readObject(objId, objData);
  objStorMutex->unlock();
  objData.size = objData.data.size();

  if (err != fds::ERR_OK) {
    FDS_PLOG(objStorMgr->GetLog()) << "Failed to get object " << objId
                                   << " with error " << err;
    /*
     * Set the data to empty so we don't return
     * garbage.
     */
    objData.size = 0;
    objData.data = "";
  } else {
    FDS_PLOG(objStorMgr->GetLog()) << "Successfully got object " << objId
      // << " and data " << objData.data
                                   << " for request ID " << getReq->io_req_id;
  }

  qosCtrl->markIODone(*getReq);

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
  getObj->data_obj                 = objData.data;
  getObj->data_obj_len             = objData.size;

  if (err == ERR_OK) {
    msgHdr->result = FDS_ProtocolInterface::FDSP_ERR_OK;
  } else {
    msgHdr->result = FDS_ProtocolInterface::FDSP_ERR_FAILED;
  }
  msgHdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_GET_OBJ_RSP;
  swapMgrId(msgHdr);
  fdspDataPathClient[msgHdr->src_node_name]->begin_GetObjectResp(msgHdr, getObj);
  FDS_PLOG(objStorMgr->GetLog()) << "Sent async GetObj response after processing";

  /*
   * Free the IO request structure that
   * was allocated when the request was
   * enqueued.
   */
  delete getReq;

  return err;
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
                               "",
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
  rankEngine = new ObjectRankEngine(stor_prefix, 1000000, objStats, objStorMgr->GetLog());
  tierEngine = new TierEngine(TierEngine::FDS_TIER_PUT_ALGO_BASIC_RANK, volTbl, rankEngine, objStorMgr->GetLog());

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
                               testVolId,
                               testVolId * 2,
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
                        FDS_VOL_ACTION_CREATE);

      delete testVdb;
    }
  }

  communicator()->waitForShutdown();

  if (ifAddrStruct != NULL) {
    freeifaddrs(ifAddrStruct);
  }

  return EXIT_SUCCESS;
}

fds_log* ObjectStorMgr::GetLog() {
  return sm_log;
}

void getObjectExt(SmIoReq* getReq) {
     objStorMgr->getObjectInternal(getReq);
}

void putObjectExt(SmIoReq* putReq) {
     objStorMgr->putObjectInternal(putReq);
}


Error ObjectStorMgr::SmQosCtrl::processIO(FDS_IOType* _io) {
   Error err(ERR_OK);
   SmIoReq *io = static_cast<SmIoReq*>(_io);

   if (io->io_type == FDS_IO_READ) {
          FDS_PLOG(FDS_QoSControl::qos_log) << "Processing a get request";
          threadPool->schedule(getObjectExt,io);
          objStorMgr->objStats->updateIOpathStats(io->getVolId(),io->getObjId());
   } else if (io->io_type == FDS_IO_WRITE) {
          FDS_PLOG(FDS_QoSControl::qos_log) << "Processing a put request";
          threadPool->schedule(putObjectExt,io);
   } else {
          assert(0);
   }

   return err;
}



void
ObjectStorMgr::interruptCallback(int)
{
    communicator()->shutdown();
}


}  // namespace fds

int main(int argc, char *argv[])
{
  objStorMgr = new ObjectStorMgr();

    /* Instantiate a DiskManager Module instance */
    fds::Module *io_dm_vec[] = {
        &diskio::gl_dataIOMod,
        &fds::gl_tierPolicy,
        nullptr
    };
    fds::ModuleVector  io_dm(argc, argv, io_dm_vec);
    io_dm.mod_execute();

  objStorMgr->main(argc, argv, "stor_mgr.conf");

  delete objStorMgr;
}

