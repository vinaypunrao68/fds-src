#include <iostream>

#include "StorMgr.h"
#include "DiskMgr.h"

fds_bool_t  stor_mgr_stopping = false;

#define FDS_XPORT_PROTO_TCP 1
#define FDS_XPORT_PROTO_UDP 2
#define FDSP_MAX_MSG_LEN (4*(4*1024 + 128))

ObjectStorMgr *objStorMgr;

// Use a single global levelDB for now

ObjectStorMgrI::ObjectStorMgrI(const Ice::CommunicatorPtr& communicator): _communicator(communicator) {
}

ObjectStorMgrI::~ObjectStorMgrI() {
}

void
ObjectStorMgrI::PutObject(const FDSP_MsgHdrTypePtr &msg_hdr, const FDSP_PutObjTypePtr &put_obj, const Ice::Current&) {
  FDS_PLOG(objStorMgr->GetLog()) << "Putobject()";
  objStorMgr->PutObject(msg_hdr, put_obj);
  msg_hdr->msg_code = FDSP_MSG_PUT_OBJ_RSP;
  objStorMgr->swapMgrId(msg_hdr);
  objStorMgr->fdspDataPathClient->begin_PutObjectResp(msg_hdr, put_obj);
   FDS_PLOG(objStorMgr->GetLog()) << "Sent async PutObj response to Hypervisor";
}

void
ObjectStorMgrI::GetObject(const FDSP_MsgHdrTypePtr &msg_hdr, const FDSP_GetObjTypePtr& get_obj, const Ice::Current&) {
  FDS_PLOG(objStorMgr->GetLog()) << "Getobject()";
  objStorMgr->GetObject(msg_hdr, get_obj);
  msg_hdr->msg_code = FDSP_MSG_GET_OBJ_RSP;
  objStorMgr->swapMgrId(msg_hdr);
  objStorMgr->fdspDataPathClient->begin_GetObjectResp(msg_hdr, get_obj);
  FDS_PLOG(objStorMgr->GetLog()) << "Sent async GetObj response to Hypervisor";
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
//--------------------------------------------------------------------------------------------------
ObjectStorMgr::ObjectStorMgr(fds_uint32_t port,
                             fds_uint32_t control_port,
                             std::string prefix)
    : stor_prefix(prefix) , port_num(port), cp_port_num(control_port) {

  // Init  the log infra  
  sm_log = new fds_log("sm", "logs");
  FDS_PLOG(sm_log) << "Constructing the Data Manager";

  // Create all data structures 
  diskMgr = new DiskMgr();
  std::string filename= stor_prefix + "SNodeObjRepository";

  objStorMutex = new fds_mutex("Object Store Mutex");
  
  // Create leveldb
  objStorDB  = new ObjectDB(filename);
  filename= stor_prefix + "SNodeObjIndex";
  objIndexDB  = new ObjectDB(filename);  
  omClient = new OMgrClient(FDSP_STOR_MGR, "localhost-sm", sm_log);
  omClient->initialize();
  omClient->registerEventHandlerForNodeEvents((node_event_handler_t)nodeEventOmHandler);
  omClient->startAcceptingControlMessages(cp_port_num);
  //omClient->registerNodeWithOM();
  volTbl = new StorMgrVolumeTable(this);
}


ObjectStorMgr::~ObjectStorMgr()
{
  FDS_PLOG(objStorMgr->GetLog()) << " Destructing  the Storage  manager";
  delete objStorDB;
  delete objIndexDB;
  delete diskMgr;
  delete sm_log;
  delete volTbl;
  delete objStorMutex;
}

void ObjectStorMgr::nodeEventOmHandler(int node_id, unsigned int node_ip_addr, int node_state)
{
    switch(node_state) {
       case FDSP_Types::FDS_Node_Up :
           FDS_PLOG(objStorMgr->GetLog()) << "ObjectStorMgr - Node UP event NodeId " << node_id << " Node IP Address " <<  node_ip_addr;
         break;

       case FDSP_Types::FDS_Node_Down:
       case FDSP_Types::FDS_Node_Rmvd:
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
  fds_sm_err_t err;

  err = FDS_SM_OK;

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
  if (err != FDS_SM_OK) {
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

fds_sm_err_t 
ObjectStorMgr::checkDuplicate(FDS_ObjectIdType *object_id, 
                              fds_uint32_t obj_len, 
                              fds_char_t *data_object)
{
ObjectID obj_id(object_id->hash_high, object_id->hash_low);
  ObjectBuf obj;
  obj.size = obj_len;
  fds::Error err(fds::ERR_OK);
  fds_sm_err_t retval = FDS_SM_OK;

  objStorMutex->lock();
  err = objStorDB->Get(obj_id, obj);
  objStorMutex->unlock();
  if (err == fds::ERR_OK) {
     if (memcmp(data_object, obj.data.c_str(), obj_len) == 0 ) { 
         retval =   FDS_SM_ERR_DUPLICATE;
     } else {
      // Handle hash-collision - insert the next collision-id+obj-id 
       retval = FDS_SM_ERR_HASH_COLLISION;
     }
  } 
  
  return retval;
}

fds_sm_err_t 
ObjectStorMgr::writeObject(FDS_ObjectIdType *object_id, 
                           fds_uint32_t obj_len, 
                           fds_char_t *data_object, 
                           FDS_DataLocEntry  *data_loc)
{
   //Hash the object_id to DiskNumber, FileName
fds_uint32_t disk_num = 1;

   // Now append the object to the end of this filename
   diskMgr->writeObject(object_id, obj_len, data_object, data_loc, disk_num);

   return FDS_SM_OK;
}

fds_sm_err_t 
ObjectStorMgr::writeObjLocation(FDS_ObjectIdType *object_id, 
                                fds_uint32_t obj_len, 
                                fds_uint32_t volid, 
                                FDS_DataLocEntry *data_loc)
{
  // fds_uint32_t disk_num = 1;
   // Enqueue the object location entry into the thread that maintains global index file
   //disk_mgr_write_obj_loc(object_id, obj_len, volid, data_loc);
  return FDS_SM_OK;
}


/*------------------------------------------------------------------------- ------------
 * FDSP Protocol internal processing 
 -------------------------------------------------------------------------------------*/
fds_sm_err_t 
ObjectStorMgr::putObjectInternal(FDSP_PutObjTypePtr put_obj_req, 
                                 fds_uint64_t  volid, 
                                 fds_uint32_t num_objs)
{
//fds_char_t *put_obj_buf = put_obj_req._ptr;
fds_uint32_t obj_num=0;
fds_sm_err_t result = FDS_SM_OK;
fds::Error err(fds::ERR_OK);

   for(obj_num = 0; obj_num < num_objs; obj_num++) {
       // Find if this object is a duplicate
       result = checkDuplicate(&put_obj_req->data_obj_id, 
                               put_obj_req->data_obj_len, 
                               (fds_char_t *)put_obj_req->data_obj.data());

       if (result != FDS_SM_ERR_DUPLICATE) {
           // First write the object itself after hashing the objectId to Disknum/filename & obtain an offset entry
         /*
           result = writeObject(&put_obj_req->data_obj_id, 
                                (fds_uint32_t)put_obj_req->data_obj_len,
                                (fds_char_t *)put_obj_req->data_obj.data(), 
                                &object_location_offset);

           // Now write the object location entry in the global index file
           writeObjLocation(&put_obj_req->data_obj_id, 
                            put_obj_req->data_obj_len, volid, 
                            &object_location_offset);
         */
	   /*
	    * This is the levelDB insertion. It's a totally
	    * separate DB from the one above.
	    */
           ObjectID obj_id(put_obj_req->data_obj_id.hash_high, put_obj_req->data_obj_id.hash_low);
           ObjectBuf obj;
           obj.size = put_obj_req->data_obj_len;
	   obj.data  = put_obj_req->data_obj;

	   objStorMutex->lock();
	   err = objStorDB->Put( obj_id, obj);
	   objStorMutex->unlock();

	   if (err != fds::ERR_OK) {
	      FDS_PLOG(objStorMgr->GetLog()) << "Failed to put object " << err;
              return FDS_SM_ERR_DISK_WRITE_FAILED;
	   } else {
	     FDS_PLOG(objStorMgr->GetLog()) << "Successfully put key ";
	   }

       } else {
	   FDS_PLOG(objStorMgr->GetLog()) << "Duplicate object - returning success to put object " << err;
           writeObjLocation(&put_obj_req->data_obj_id, put_obj_req->data_obj_len, volid, NULL);
       }
       volTbl->createVolIndexEntry(volid, 
                                  put_obj_req->volume_offset, 
                                  put_obj_req->data_obj_id,
                                  put_obj_req->data_obj_len);
       // Move the buffer pointer to the next object
       //put_obj_buf += (sizeof(FDSP_PutObjType) + put_obj_req->data_obj_len); 
       //put_obj_req = (FDSP_PutObjType *)put_obj_buf;
   }

   return FDS_SM_OK;
}

void ObjectStorMgr::PutObject(const FDSP_MsgHdrTypePtr& fdsp_msg, const FDSP_PutObjTypePtr& put_obj_req) {

    // Verify the integrity of the FDSP msg using chksums
    // 
    // stor_mgr_verify_msg(fdsp_msg);
    //put_obj_req->data_obj_id.hash_high = ntohl(put_obj_req->data_obj_id.hash_high);
    //put_obj_req->data_obj_id.hash_low = ntohl(put_obj_req->data_obj_id.hash_low);
    //
    ObjectID oid(put_obj_req->data_obj_id.hash_high,
               put_obj_req->data_obj_id.hash_low);

    FDS_PLOG(objStorMgr->GetLog()) << "PutObject Obj ID:" << oid <<"glob_vol_id:" << fdsp_msg->glob_volume_id << "Num Objs:" << fdsp_msg->num_objects;
    putObjectInternal(put_obj_req, fdsp_msg->glob_volume_id, fdsp_msg->num_objects);
}

fds_sm_err_t 
ObjectStorMgr::getObjectInternal(FDSP_GetObjTypePtr get_obj_req, 
                                fds_uint32_t volid, 
                                fds_uint32_t trans_id, 
                                fds_uint32_t num_objs) 
{
    ObjectID obj_id(get_obj_req->data_obj_id.hash_high, get_obj_req->data_obj_id.hash_low);
    ObjectBuf obj;
    obj.size = get_obj_req->data_obj_len;
    obj.data = get_obj_req->data_obj;
    fds::Error err(fds::ERR_OK);

  objStorMutex->lock();
  err = objStorDB->Get(obj_id, obj);
  objStorMutex->unlock();

  if (err != fds::ERR_OK) {
     FDS_PLOG(objStorMgr->GetLog()) << "XID:" << trans_id << "  Failed to get key " << obj_id << " with status " << err;
     get_obj_req->data_obj.assign("");
     return FDS_SM_ERR_OBJ_NOT_EXIST;
  } else {
     FDS_PLOG(objStorMgr->GetLog()) << "XID:" << trans_id << "Successfully got value ";
    get_obj_req->data_obj.assign(obj.data);
  }

  return FDS_SM_OK;
}

void 
ObjectStorMgr::GetObject(const FDSP_MsgHdrTypePtr& fdsp_msg,  
                         const FDSP_GetObjTypePtr& get_obj_req) 
{

    // Verify the integrity of the FDSP msg using chksums
    // 
    // stor_mgr_verify_msg(fdsp_msg);
    //
    int err;
    ObjectID oid(get_obj_req->data_obj_id.hash_high,
               get_obj_req->data_obj_id.hash_low);

    FDS_PLOG(objStorMgr->GetLog()) << "GetObject  XID:" << fdsp_msg->req_cookie << " Obj ID :" << oid << "glob_vol_id:" << fdsp_msg->glob_volume_id << "Num Objs:" << fdsp_msg->num_objects;
   
    if ((err = getObjectInternal(get_obj_req, fdsp_msg->glob_volume_id, fdsp_msg->req_cookie, fdsp_msg->num_objects)) != FDS_SM_OK) {
          fdsp_msg->result = FDSP_ERR_FAILED;
          fdsp_msg->err_code = (FDSP_ErrType)err;
    } else {
          fdsp_msg->result = FDSP_ERR_OK;
    }
}

inline void ObjectStorMgr::swapMgrId(const FDSP_MsgHdrTypePtr& fdsp_msg) {
 FDSP_MgrIdType temp_id;
 long tmp_addr;

 temp_id = fdsp_msg->dst_id;
 fdsp_msg->dst_id = fdsp_msg->src_id;
 fdsp_msg->src_id = temp_id;

 tmp_addr = fdsp_msg->dst_ip_hi_addr;
 fdsp_msg->dst_ip_hi_addr = fdsp_msg->src_ip_hi_addr;
 fdsp_msg->src_ip_hi_addr = tmp_addr;

 tmp_addr = fdsp_msg->dst_ip_lo_addr;
 fdsp_msg->dst_ip_lo_addr = fdsp_msg->src_ip_lo_addr;
 fdsp_msg->src_ip_lo_addr = tmp_addr;

}


/*------------------------------------------------------------------------------------- 
 * Storage Mgr main  processor : Listen on the socket and spawn or assign thread from a pool
 -------------------------------------------------------------------------------------*/
int
ObjectStorMgr::run(int argc, char* argv[])
{
  std::string endPointStr;
  
  Ice::PropertiesPtr props = communicator()->getProperties();
  
  /*
   * Set basic thread properties.
   */
  props->setProperty("ObjectStorMgrSvr.ThreadPool.Size", "50");
  props->setProperty("ObjectStorMgrSvr.ThreadPool.SizeMax", "100");
  props->setProperty("ObjectStorMgrSvr.ThreadPool.SizeWarn", "75");
  
  if (port_num == 0) {
    /*
     * Pull the port from the config file if it wasn't
     * specified on the command line.
     */
    port_num = props->getPropertyAsInt("ObjectStorMgrSvr.PortNumber");
  }
  if (cp_port_num == 0) {
    cp_port_num = props->getPropertyAsInt("ObjStorMgrSvr.ControlPort");
  }
  
  std::ostringstream tcpProxyStr;
  tcpProxyStr << "tcp -p " << port_num;
  
  Ice::ObjectAdapterPtr adapter = communicator()->createObjectAdapterWithEndpoints("ObjectStorMgrSvr", tcpProxyStr.str());
  fdspDataPathServer = new ObjectStorMgrI(communicator());
  adapter->add(fdspDataPathServer, communicator()->stringToIdentity("ObjectStorMgrSvr"));

  callbackOnInterrupt();
  
  adapter->activate();
  
  communicator()->waitForShutdown();
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


int main(int argc, char *argv[])
{
  bool         unit_test;
  fds_uint32_t port, control_port;
  std::string  prefix;
  
  port      = 0;
  unit_test = false;
  
  for (int i = 1; i < argc; i++) {
    std::string arg(argv[i]);
    if (arg == "--unit_test") {
      unit_test = true;
    } else if (strncmp(argv[i], "--cp_port=", 10) == 0) {
      control_port = strtoul(argv[i] + 10, NULL, 0);
    } else if (strncmp(argv[i], "--port=", 7) == 0) {
      port = strtoul(argv[i] + 7, NULL, 0);
    } else if (strncmp(argv[i], "--prefix=", 9) == 0) {
      prefix = argv[i] + 9;
    } else {
       FDS_PLOG(objStorMgr->GetLog()) << "Invalid argument " << argv[i];
      return -1;
    }
  }    
 


  objStorMgr = new ObjectStorMgr(port, control_port, prefix);

  if (unit_test) {
    objStorMgr->unitTest();
    return 0;
  }

  FDS_PLOG(objStorMgr->GetLog()) << "Stor Mgr port_number :" << port;

  objStorMgr->main(argc, argv, "stor_mgr.conf");

  delete objStorMgr;
}


