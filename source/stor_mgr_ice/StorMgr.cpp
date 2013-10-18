/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <iostream>

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
ObjectStorMgrI::PutObject(const FDSP_MsgHdrTypePtr& msg_hdr,
                          const FDSP_PutObjTypePtr& put_obj,
                          const Ice::Current&) {
  FDS_PLOG(objStorMgr->GetLog()) << "Putobject()";
  objStorMgr->PutObject(msg_hdr, put_obj);
  /*
   * TODO: At the moment, this only acknowledges the I/O was put
   * on the queue properly, NOT that it was successfully processed!
   */
  msg_hdr->msg_code = FDSP_MSG_PUT_OBJ_RSP;
  objStorMgr->swapMgrId(msg_hdr);
  objStorMgr->fdspDataPathClient->begin_PutObjectResp(msg_hdr, put_obj);
  FDS_PLOG(objStorMgr->GetLog()) << "Sent async PutObj response to Hypervisor";
}

void
ObjectStorMgrI::GetObject(const FDSP_MsgHdrTypePtr& msgHdr,
                          const FDSP_GetObjTypePtr& getObj,
                          const Ice::Current&) {
  FDS_PLOG(objStorMgr->GetLog()) << "Received a Getobject() network request";

  /*
   * Track the outstanding get request.
   * TODO: This is a total hack. We're overloading the msg_hdr's
   * req_cookie field to track the outstanding request Id that
   * we pass into the SM.
   *
   * TODO: We should check if this value has rolled at some point.
   * Though it's big enough for us to not care right now.
   */
  fds_uint64_t reqId;
  reqId = std::atomic_fetch_add(&(objStorMgr->nextReqId), (fds_uint64_t)1);
  msgHdr->req_cookie = reqId;
  objStorMgr->getsMutex->lock();
  objStorMgr->waitingGets[reqId] = ObjectStorMgr::OutStandingGet(msgHdr, getObj);
  objStorMgr->getsMutex->unlock();

  /*
   * Submit the request to be enqueued
   */
  objStorMgr->GetObject(msgHdr, getObj);

  /*
   * If we failed to enqueue the I/O return the error response
   * now as there is no more processing to do.
   */
  if (msgHdr->result != FDSP_ERR_OK) {
    objStorMgr->getsMutex->lock();
    objStorMgr->waitingGets.erase(reqId);
    objStorMgr->getsMutex->unlock();
    

    msgHdr->msg_code = FDSP_MSG_GET_OBJ_RSP;
    objStorMgr->swapMgrId(msgHdr);
    objStorMgr->fdspDataPathClient->begin_GetObjectResp(msgHdr, getObj);

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
ObjectStorMgrI::AssociateRespCallback(const Ice::Identity& ident, const Ice::Current& current) {
  FDS_PLOG(objStorMgr->GetLog()) << "Associating response Callback client to ObjStorMgr  :" << _communicator->identityToString(ident);

  objStorMgr->fdspDataPathClient = FDSP_DataPathRespPrx::uncheckedCast(current.con->createProxy(ident));
}

/**
 * Storage manager member functions
 */
ObjectStorMgr::ObjectStorMgr() :
    totalRate(10000) {
  /*
   * TODO: Fix the totalRate above to not
   * be hard coded.
   */

  // Init  the log infra  
  sm_log = new fds_log("sm", "logs");
  FDS_PLOG(sm_log) << "Constructing the Object Storage Manager";

  // Create all data structures 
  diskMgr = new DiskMgr();

  objStorMutex = new fds_mutex("Object Store Mutex");
  getsMutex = new fds_mutex("Object Store Mutex");

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
   * TODO: Totally just making variables
   * fill the in correctly at some point.
   * Note the 5 is hard coded since this
   * is the volId the unit test generates
   * for I/O.
   */
  testVolQueue = new SmVolQueue(5,
                                5,
                                5,
                                5,
                                5);
  qosCtrl = new SmQosCtrl(this,
                          5,
                          FDS_QoSControl::FDS_DISPATCH_HIER_TOKEN_BUCKET,
                          sm_log);
  qosCtrl->registerVolume(testVolQueue->getVolUuid(),
                          dynamic_cast<FDS_VolumeQueue*>(testVolQueue));

  qosCtrl->runScheduler();
}

ObjectStorMgr::~ObjectStorMgr()
{
  FDS_PLOG(objStorMgr->GetLog()) << " Destructing  the Storage  manager";
  if (objStorDB)
    delete objStorDB;
  if (objIndexDB)
    delete objIndexDB;

  /*
   * TODO: Should not need to do. Not sure that qosctrl
   * cleans itself up properly.
   */
  qosCtrl->deregisterVolume(testVolQueue->getVolUuid());

  delete getsMutex;

  delete qosCtrl;
  delete testVolQueue;

  delete diskMgr;
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


void ObjectStorMgr::volEventOmHandler(fds::fds_volid_t volume_id, fds::VolumeDesc *vdb, int vol_action)
{
    switch(vol_action) {
       case FDS_VOL_ACTION_CREATE :
	 FDS_PLOG(objStorMgr->GetLog()) << "ObjectStorMgr - Volume Create " << volume_id << " Volume Name " <<  (*vdb);
         break;

       case FDS_VOL_ACTION_DELETE:
	 FDS_PLOG(objStorMgr->GetLog()) << " ObjectStorMgr - Volume Delete :" << volume_id << " Volume Name " << (*vdb);
	 break;
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
   */
  err = putObjectInternal(put_obj_req, vol_id, num_objs);
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
ObjectStorMgr::readObject(const ObjectID& objId, 
			  ObjectBuf& objData)
{
  Error err(ERR_OK);
  
  diskio::DataIO& dio_mgr = diskio::DataIO::disk_singleton();
  SMDiskReq     *disk_req;
  meta_vol_io_t   vio;
  meta_obj_id_t   oid;
  vadr_set_inval(vio.vol_adr);
  oid.oid_hash_hi = objId.GetHigh();
  oid.oid_hash_lo = objId.GetLow();
  disk_req = new SMDiskReq(vio, oid, (ObjectBuf *)&objData, true); // blocking call
  dio_mgr.disk_read(disk_req);
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

  objGetData.size = 4096;
  objGetData.data = "";
  objGetData.data.reserve(4096);

  objStorMutex->lock();
  err = readObject(objId, objGetData);
  objStorMutex->unlock();
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
ObjectStorMgr::writeObject(const ObjectID& objId, 
                           const ObjectBuf& objData)
{
  Error err(ERR_OK);
  
  diskio::DataIO& dio_mgr = diskio::DataIO::disk_singleton();
  SMDiskReq     *disk_req;
  meta_vol_io_t   vio;
  meta_obj_id_t   oid;
  vadr_set_inval(vio.vol_adr);
  oid.oid_hash_hi = objId.GetHigh();
  oid.oid_hash_lo = objId.GetLow();
  disk_req = new SMDiskReq(vio, oid, (ObjectBuf *)&objData, true); // blocking call
  dio_mgr.disk_write(disk_req);
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
ObjectStorMgr::putObjectInternal(const SmIoReq& putReq) {
  Error err(ERR_OK);
  const ObjectID&  objId   = putReq.getObjId();
  const ObjectBuf& objData = putReq.getObjData();


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
    return err;
  } else if (err != ERR_OK) {
    objStorMutex->unlock();
    FDS_PLOG(objStorMgr->GetLog()) << "Failed to put object: " << err;
    return err;
  }

  err = writeObject(objId, objData);
  objStorMutex->unlock();

  if (err != fds::ERR_OK) {
    FDS_PLOG(objStorMgr->GetLog()) << "Failed to put object " << err;
    return err;
  } else {
    FDS_PLOG(objStorMgr->GetLog()) << "Successfully put key " << objId;
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

  return err;
}

Error
ObjectStorMgr::putObjectInternal(FDSP_PutObjTypePtr putObjReq, 
                                 fds_volid_t        volId, 
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
                                 FDS_IO_WRITE);

    err = qosCtrl->enqueueIO(ioReq->getVolId(), static_cast<FDS_IOType*>(ioReq));
    if (err != ERR_OK) {
      /*
       * Just return if we see an error. The way the function is setup
       * it's a bit unnatrual to return errors for multiple IOs so
       * we'll just stop at the first error we see to make sure it
       * doesn't get lost.
       */
      FDS_PLOG(objStorMgr->GetLog()) << "Unable to enqueue putObject request";
      return err;
    }
    FDS_PLOG(objStorMgr->GetLog()) << "Successfully enqueued putObject request";
   }

   return err;
}

void
ObjectStorMgr::PutObject(const FDSP_MsgHdrTypePtr& fdsp_msg,
                         const FDSP_PutObjTypePtr& put_obj_req) {
  // Verify the integrity of the FDSP msg using chksums
  // 
  // stor_mgr_verify_msg(fdsp_msg);
  //
  ObjectID oid(put_obj_req->data_obj_id.hash_high,
               put_obj_req->data_obj_id.hash_low);

  FDS_PLOG(objStorMgr->GetLog()) << "PutObject Obj ID: " << oid
                                 << ", glob_vol_id: " << fdsp_msg->glob_volume_id
                                 << ", Num Objs: " << fdsp_msg->num_objects;
  putObjectInternal(put_obj_req, fdsp_msg->glob_volume_id, fdsp_msg->num_objects);
}

Error
ObjectStorMgr::getObjectInternal(const SmIoReq& getReq) {
  Error           err(ERR_OK);
  const ObjectID& objId  = getReq.getObjId();
  ObjectBuf       objData;
  /*
   * We need to fix this once diskmanager keeps track of object size
   * and allocates buffer automatically.
   * For now, we will pass the fixed block size for size and preallocate
   * memory for that size.
   */
  objData.size = 4096;
  objData.data = "";
  objData.data.reserve(4096);

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
                                   << " and data " << objData.data
                                   << " for request ID " << getReq.io_req_id;
  }

  /*
   * Prepare a response to send back.
   */
  getsMutex->lock();
  assert(waitingGets.count(getReq.io_req_id) > 0);
  OutStandingGet waitingGet = waitingGets[getReq.io_req_id];
  waitingGets.erase(getReq.io_req_id);
  getsMutex->unlock();

  FDS_ProtocolInterface::FDSP_MsgHdrTypePtr msgHdr = waitingGet.first;
  FDS_ProtocolInterface::FDSP_GetObjTypePtr getObj = waitingGet.second;

  /*
   * This does an additional copy in to the network buffer.
   */
  getObj->data_obj     = objData.data;
  getObj->data_obj_len = objData.size;
  msgHdr->msg_code     = FDSP_MSG_GET_OBJ_RSP;
  swapMgrId(msgHdr);
  fdspDataPathClient->begin_GetObjectResp(msgHdr, getObj);
  FDS_PLOG(objStorMgr->GetLog()) << "Sent async GetObj response after processing";

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

  FDS_PLOG(objStorMgr->GetLog()) << "GetObject XID: " << fdsp_msg->req_cookie
                                 << ", Obj ID: " << oid
                                 << ", glob_vol_id: " << fdsp_msg->glob_volume_id
                                 << ", Num Objs: " << fdsp_msg->num_objects;
   
  err = getObjectInternal(get_obj_req,
                          fdsp_msg->glob_volume_id,
                          fdsp_msg->req_cookie,
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
ObjectStorMgr::run(int argc, char* argv[])
{

  bool         unit_test;
  std::string endPointStr;
  fds::DmDiskInfo     *info;
  fds::DmDiskQuery     in;
  fds::DmDiskQueryOut  out;
  
  unit_test = false;
  std::string  omIpStr;
  fds_uint32_t omConfigPort;

  omConfigPort = 0;

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
    } else {
       FDS_PLOG(objStorMgr->GetLog()) << "Invalid argument " << argv[i];
      return -1;
    }
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

  volTbl = new StorMgrVolumeTable(this);

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
   * Query  Disk Manager  for disk parameter details 
   */
    fds::DmQuery        &query = fds::DmQuery::dm_query();
    in.dmq_mask = fds::dmq_disk_info;
    query.dm_disk_query(in, &out);
    /* we should be bundling multiple disk  parameters  into one message to OM TBD */ 
    dInfo = new  FDSP_AnnounceDiskCapability(); 
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
  omClient->initialize();
  omClient->registerEventHandlerForNodeEvents((node_event_handler_t)nodeEventOmHandler);
  omClient->registerEventHandlerForVolEvents((volume_event_handler_t)volEventOmHandler);
  omClient->startAcceptingControlMessages(cp_port_num);
  omClient->registerNodeWithOM(dInfo);

  communicator()->waitForShutdown();

  if (ifAddrStruct != NULL) {
    freeifaddrs(ifAddrStruct);
  }

  return EXIT_SUCCESS;
}

fds_log* ObjectStorMgr::GetLog() {
  return sm_log;
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

  objStorMgr->main(argc, argv, "stor_mgr.conf");

  delete objStorMgr;
}

