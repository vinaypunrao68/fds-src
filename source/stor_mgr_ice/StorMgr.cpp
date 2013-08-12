#include <iostream>

#include "StorMgr.h"
#include "DiskMgr.h"

fds_bool_t  stor_mgr_stopping = false;

#define FDS_XPORT_PROTO_TCP 1
#define FDS_XPORT_PROTO_UDP 2
#define FDSP_MAX_MSG_LEN (4*(4*1024 + 128))

ObjectStorMgr *objStorMgr;

// Use a single global levelDB for now
leveldb::DB* db;

ObjectStorMgrI::ObjectStorMgrI() {
}

ObjectStorMgrI::~ObjectStorMgrI() {
}

void
ObjectStorMgrI::PutObject(const FDSP_MsgHdrTypePtr &msg_hdr, const FDSP_PutObjTypePtr &put_obj, const Ice::Current&) {
  std::cout << "In the interface putobject()" << std::endl;
  objStorMgr->PutObject(msg_hdr, put_obj);
  msg_hdr->msg_code = FDSP_MSG_PUT_OBJ_RSP;
  objStorMgr->swapMgrId(msg_hdr);
  objStorMgr->fdspDataPathClient->PutObjectResp(msg_hdr, put_obj);
}

void
ObjectStorMgrI::GetObject(const FDSP_MsgHdrTypePtr &msg_hdr, const FDSP_GetObjTypePtr& get_obj, const Ice::Current&) {
  std::cout << "In the interface getobject()" << std::endl;
  objStorMgr->GetObject(msg_hdr, get_obj);
  msg_hdr->msg_code = FDSP_MSG_GET_OBJ_RSP;
  objStorMgr->swapMgrId(msg_hdr);
  objStorMgr->fdspDataPathClient->GetObjectResp(msg_hdr, get_obj);
}

void
ObjectStorMgrI::UpdateCatalogObject(const FDSP_MsgHdrTypePtr &msg_hdr, const FDSP_UpdateCatalogTypePtr& update_catalog , const Ice::Current&) {
  std::cout << "Wrong Interface Call: In the interface updatecatalog()" << std::endl;
}

void
ObjectStorMgrI::OffsetWriteObject(const FDSP_MsgHdrTypePtr& msg_hdr, const FDSP_OffsetWriteObjTypePtr& offset_write_obj, const Ice::Current&) {
  std::cout << "In the interface offsetwrite()" << std::endl;
}

void
ObjectStorMgrI::RedirReadObject(const FDSP_MsgHdrTypePtr &msg_hdr, const FDSP_RedirReadObjTypePtr& redir_read_obj, const Ice::Current&) {
  std::cout << "In the interface redirread()" << std::endl;
}
//--------------------------------------------------------------------------------------------------
ObjectStorMgrClientI::ObjectStorMgrClientI() 
{
}

ObjectStorMgrClientI::~ObjectStorMgrClientI() 
{
}

void
ObjectStorMgrClientI::PutObjectResp(const FDSP_MsgHdrTypePtr &msg_hdr, const FDSP_PutObjTypePtr &put_obj, const Ice::Current&) 
{
}

void
ObjectStorMgrClientI::GetObjectResp(const FDSP_MsgHdrTypePtr &msg_hdr, const FDSP_GetObjTypePtr& get_obj, const Ice::Current&) {
}

void
ObjectStorMgrClientI::UpdateCatalogObjectResp(const FDSP_MsgHdrTypePtr &msg_hdr, 
                                              const FDSP_UpdateCatalogTypePtr& update_catalog , const Ice::Current&) 
{
}

void
ObjectStorMgrClientI::OffsetWriteObjectResp(const FDSP_MsgHdrTypePtr& msg_hdr, const FDSP_OffsetWriteObjTypePtr& offset_write_obj, const Ice::Current&) 
{
}

void
ObjectStorMgrClientI::RedirReadObjectResp(const FDSP_MsgHdrTypePtr &msg_hdr, const FDSP_RedirReadObjTypePtr& redir_read_obj, const Ice::Current&) {
}

//--------------------------------------------------------------------------------------------------
ObjectStorMgr::ObjectStorMgr() 
{
  // Create all data structures 
  diskMgr = new DiskMgr();
  std::string filename= "SNodeObjRepository";
  
  // Create leveldb
  objStorDB  = new ObjectDB(filename);
  filename= "SNodeObjIndex";
  objIndexDB  = new ObjectDB(filename);
}


ObjectStorMgr::~ObjectStorMgr()
{
  delete objStorDB;
  delete objIndexDB;
}

void ObjectStorMgr::unitTest()
{
  fds_sm_err_t err;

  err = FDS_SM_OK;

  std::cout << "Running unit test" << std::endl;
  
  /*
   * Create fake objects
   */
  std::string object_data("Hi, I'm object data.");
  FDSP_PutObjType *put_obj_req;
  put_obj_req = (FDSP_PutObjType *)malloc(sizeof(FDSP_PutObjType) +
					    object_data.length() + 1);
  memset(&(put_obj_req->data_obj_id),
	 0x00,
	 sizeof(put_obj_req->data_obj_id));
  put_obj_req->data_obj_id.hash_low = 0x101;
  put_obj_req->data_obj_len = object_data.length() + 1;
  memcpy((char *)put_obj_req->data_obj.data(),
	 object_data.c_str(),
	 put_obj_req->data_obj_len);

  /*
   * Create fake volume ID
   */
  fds_uint32_t vol_id   = 0xbeef;
  fds_uint32_t num_objs = 1;

  /*
   * Write fake object.
   */
  err = putObjectInternal(put_obj_req, vol_id, num_objs);
  if (err != FDS_SM_OK) {
    std::cout << "Failed to put object " << std::endl;
    free(put_obj_req);
    return;
  }

  FDSP_GetObjType *get_obj_req = new FDSP_GetObjType();
  memset((char *)&(get_obj_req->data_obj_id),
	 0x00,
	 sizeof(get_obj_req->data_obj_id));
  get_obj_req->data_obj_id.hash_low = 0x101;
  err = getObjectInternal(get_obj_req, vol_id, num_objs);
}

fds_sm_err_t 
ObjectStorMgr::checkDuplicate(FDS_ObjectIdType *object_id, 
                              fds_uint32_t obj_len, 
                              fds_char_t *data_object)
{
  return FDS_SM_OK;
}

fds_sm_err_t 
ObjectStorMgr::writeObject(FDS_ObjectIdType *object_id, 
                           fds_uint32_t obj_len, 
                           fds_char_t *data_object, 
                           FDSDataLocEntryType *data_loc)
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
                                FDSDataLocEntryType *data_loc)
{
  // fds_uint32_t disk_num = 1;
   // Enqueue the object location entry into the thread that maintains global index file
   //disk_mgr_write_obj_loc(object_id, obj_len, volid, data_loc);
  return FDS_SM_OK;
}


/*------------------------------------------------------------------------- ------------
 * FDSP Protocol message processing 
 -------------------------------------------------------------------------------------*/
fds_sm_err_t 
ObjectStorMgr::putObjectInternal(FDSP_PutObjTypePtr put_obj_req, 
                                 fds_uint32_t volid, 
                                 fds_uint32_t num_objs)
{
//fds_char_t *put_obj_buf = put_obj_req._ptr;
FDSDataLocEntryType object_location_offset;
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
           result = writeObject(&put_obj_req->data_obj_id, 
                                (fds_uint32_t)put_obj_req->data_obj_len,
                                (fds_char_t *)put_obj_req->data_obj.data(), 
                                &object_location_offset);

           // Now write the object location entry in the global index file
           writeObjLocation(&put_obj_req->data_obj_id, 
                            put_obj_req->data_obj_len, volid, 
                            &object_location_offset);

	   /*
	    * This is the levelDB insertion. It's a totally
	    * separate DB from the one above.
	    */
           DiskLoc dl;
           dl.vol_id = volid;
           dl.file_id = 0;
           dl.offset = put_obj_req->volume_offset;
           ObjectBuf obj;
           obj.size = put_obj_req->data_obj_len;
	   obj.data  = put_obj_req->data_obj;
	   err = objStorDB->Put( dl, obj);

	   if (err != fds::ERR_OK) {
	     std::cout << "Failed to put object "
		       << err << std::endl;
	   } else {
	     std::cout << "Successfully put key " << std::endl;
	   }

       } else {
           writeObjLocation(&put_obj_req->data_obj_id, put_obj_req->data_obj_len, volid, NULL);
       }
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

  printf("StorageHVisor --> StorMgr : FDSP_MSG_PUT_OBJ_REQ ObjectId %016llx:%016llx \n",(long long unsigned int) put_obj_req->data_obj_id.hash_high, (long long unsigned int)put_obj_req->data_obj_id.hash_low);
    putObjectInternal(put_obj_req, fdsp_msg->glob_volume_id, fdsp_msg->num_objects);

}

fds_sm_err_t 
ObjectStorMgr::getObjectInternal(FDSP_GetObjTypePtr get_obj_req, 
                                fds_uint32_t volid, 
                                fds_uint32_t num_objs) 
{
     DiskLoc dl;
     dl.vol_id = volid;
     dl.file_id = 0;
     ObjectBuf obj;
     obj.size = get_obj_req->data_obj_len;
     obj.data = get_obj_req->data_obj;
     fds::Error err(fds::ERR_OK);
     // stor_mgr_read_object(get_obj_req->data_obj_id, get_obj_req->data_obj_len, &get_obj_req->data_obj);

  err = objStorDB->Get(dl, obj);

  if (err != fds::ERR_OK) {
    std::cout << "Failed to get key " << dl.vol_id << ":" << dl.file_id << ":" << dl.offset << " with status "
	      << err << std::endl;
  } else {
    std::cout << "Successfully got value " << obj.data.c_str() << std::endl;
    get_obj_req->data_obj = obj.data;
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

    std::cout << "StorageHVisor --> StorMgr : FDSP_MSG_GET_OBJ_REQ ObjectId %016llx:%016llx \n" << get_obj_req->data_obj_id.hash_high <<  get_obj_req->data_obj_id.hash_low << std::endl;
    getObjectInternal(get_obj_req, fdsp_msg->glob_volume_id, fdsp_msg->num_objects);
}

inline void ObjectStorMgr::swapMgrId(const FDSP_MsgHdrTypePtr& fdsp_msg) {
 FDSP_MgrIdType temp_id;
 temp_id = fdsp_msg->dst_id;
 fdsp_msg->dst_id = fdsp_msg->src_id;
 fdsp_msg->src_id = temp_id;
}


/*------------------------------------------------------------------------------------- 
 * Storage Mgr main  processor : Listen on the socket and spawn or assign thread from a pool
 -------------------------------------------------------------------------------------*/
int
ObjectStorMgr::run(int argc, char*[])
{
std::string endPointStr;
    if(argc > 1)
    {
        cerr << appName() << ": too many arguments" << endl;
        return EXIT_FAILURE;
    }

    callbackOnInterrupt();
    string udpEndPoint = "udp -p 9601";
    string tcpEndPoint = "tcp -p 6901";

    Ice::ObjectAdapterPtr adapter = communicator()->createObjectAdapterWithEndpoints("ObjectStorMgrSvr", tcpEndPoint);
    fdspDataPathServer = new ObjectStorMgrI();
    adapter->add(fdspDataPathServer, communicator()->stringToIdentity("ObjectStorMgrSvr"));

    fdspDataPathClient = new ObjectStorMgrClientI();
    adapter->add(fdspDataPathClient, communicator()->stringToIdentity("ObjectStorMgrClient")); 
    //_workQueue->start();
    adapter->activate();

    communicator()->waitForShutdown();
    //_workQueue->getThreadControl().join();
    return EXIT_SUCCESS;
}


void
ObjectStorMgr::interruptCallback(int)
{
    communicator()->shutdown();
}


int main(int argc, char *argv[])
{
  bool unit_test;
  int port_number = FDS_STOR_MGR_DGRAM_PORT;
  

  unit_test = 0;

  if (argc > 1) {
    for (int i = 1; i < argc; i++) {
      std::string arg(argv[i]);
      if (arg == "--unit_test") {
	unit_test = true;
      } else if (arg == "-p") {
	port_number = atoi(argv[i+1]);
	i++;
      } else {
	std::cerr << "Unknown option" << std::endl;
	return 0;
      }
    }
    
  }
  objStorMgr = new ObjectStorMgr();

  if (unit_test) {
    objStorMgr->unitTest();
    return 0;
  }

  printf("Stor Mgr port_number : %d\n", port_number);

  objStorMgr->main(argc, argv, "config.server");
}


