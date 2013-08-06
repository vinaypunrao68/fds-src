#include <iostream>

#include "StorMgr.h"
#include "DiskMgr.h"

fds_bool_t  stor_mgr_stopping = false;

#define FDS_XPORT_PROTO_TCP 1
#define FDS_XPORT_PROTO_UDP 2
#define FDSP_MAX_MSG_LEN (4*(4*1024 + 128))

ObjectStorMgr *ObjStorMgr;

// Use a single global levelDB for now
leveldb::DB* db;

ObjectStorMgrI::ObjectStorMgrI() {
}

ObjectStorMgrI::~ObjectStorMgrI() {
}

void
ObjectStorMgrI::PutObject(const FDSP_MsgHdrTypePtr &msg_hdr, const FDSP_PutObjTypePtr &put_obj, const Ice::Current&) {
  std::cout << "In the interface putobject()" << std::endl;
  ObjStorMgr->PutObject(msg_hdr, put_obj);
}

void
ObjectStorMgrI::GetObject(const FDSP_MsgHdrTypePtr &msg_hdr, const FDSP_GetObjTypePtr& get_obj, const Ice::Current&) {
  std::cout << "In the interface getobject()" << std::endl;
  ObjStorMgr->GetObject(msg_hdr, get_obj);
}

void
ObjectStorMgrI::UpdateCatalogObject(const FDSP_MsgHdrTypePrx &msg_hdr, const FDSP_UpdateCatalogTypePrx& update_catalog , const Ice::Current&) {
  std::cout << "In the interface updatecatalog()" << std::endl;
}

void
ObjectStorMgrI::OffsetWriteObject(const FDSP_MsgHdrTypePrx& msg_hdr, const FDSP_OffsetWriteObjTypePrx& offset_write_obj, const Ice::Current&) {
  std::cout << "In the interface offsetwrite()" << std::endl;
}

void
ObjectStorMgrI::RedirReadObject(const FDSP_MsgHdrTypePrx &msg_hdr, const FDSP_RedirReadObjTypePrx& redir_read_obj, const Ice::Current&) {
  std::cout << "In the interface redirread()" << std::endl;
}

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
//fds_char_t *put_obj_buf = reinterpret_cast<fds_char_t *> (put_obj_req);
FDSDataLocEntryType object_location_offset;
fds_uint32_t obj_num=0;
fds_sm_err_t result = FDS_SM_OK;

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
	   leveldb::Slice key((char *)&(put_obj_req->data_obj_id),
			      sizeof(put_obj_req->data_obj_id));
	   leveldb::Slice value((char*)put_obj_req->data_obj.data(),
				put_obj_req->data_obj_len);
	   leveldb::WriteOptions writeopts;
	   writeopts.sync = true;
	   leveldb::Status status = db->Put(writeopts,
					    key,
					    value);

	   if (! status.ok()) {
	     std::cout << "Failed to put object "
		       << status.ToString() << std::endl;
	   } else {
	     std::cout << "Successfully put key "
		       << key.ToString() << std::endl;
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

    printf("StorageHVisor --> StorMgr : FDSP_MSG_PUT_OBJ_REQ ObjectId %016llx:%016llx \n",put_obj_req->data_obj_id.hash_high, put_obj_req->data_obj_id.hash_low);
    putObjectInternal(put_obj_req, fdsp_msg->glob_volume_id, fdsp_msg->num_objects);
}

fds_sm_err_t 
ObjectStorMgr::getObjectInternal(FDSP_GetObjTypePtr get_obj_req, 
                                fds_uint32_t volid, 
                                fds_uint32_t num_objs) 
{
     // stor_mgr_read_object(get_obj_req->data_obj_id, get_obj_req->data_obj_len, &get_obj_req->data_obj);

  leveldb::Slice key((char *)&(get_obj_req->data_obj_id),
		     sizeof(get_obj_req->data_obj_id));
  std::string value((char *)get_obj_req->data_obj.data());
  leveldb::Status status = db->Get(leveldb::ReadOptions(), key, &value);

  if (! status.ok()) {
    std::cout << "Failed to get key " << key.ToString() << " with status "
	      << status.ToString() << std::endl;
  } else {
    std::cout << "Successfully got value " << value << std::endl;
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

inline void ObjectStorMgr::swapMgrId(FDSP_MsgHdrType *fdsp_msg) {
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
    string udpEndPoint = "udp -p 9600";
    string tcpEndPoint = "tcp -p 6900";

    Ice::ObjectAdapterPtr adapter = communicator()->createObjectAdapterWithEndpoints("ObjectStorMgrSvr", tcpEndPoint);
    //ObjectStorMgrIPtr  *objStorSvr = new ObjectStorMgrI();
    // FDS_ProtocolInterface::ObjectStorMgrIPtr  objStorSvr = new ObjectStorMgrI();
    FDS_ProtocolInterface::FDSP_DataPathReqPtr objStorSrv = new ObjectStorMgrI();
    // ObjectStorMgrI objStorSrv;
    adapter->add(objStorSrv, communicator()->stringToIdentity("ObjectStorMgrSvr"));

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
  ObjectStorMgr *objStorMgr = new ObjectStorMgr();

  if (unit_test) {
    objStorMgr->unitTest();
    return 0;
  }

  printf("Stor Mgr port_number : %d\n", port_number);

  objStorMgr->main(argc, argv, "config.server");
}


