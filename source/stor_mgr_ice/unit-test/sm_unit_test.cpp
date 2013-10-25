/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <cstdlib>
#include <Ice/Ice.h>
#include <IceUtil/IceUtil.h>
#include <fdsp/FDSP.h>
#include <hash/MurmurHash3.h>

#include <iostream>  // NOLINT(*)
#include <unordered_map>
#include <vector>
#include <string>
#include <list>
#include <atomic>

#include <util/Log.h>
#include <fds_assert.h>
#include <fds_types.h>
#include <concurrency/Mutex.h>

namespace fds {

class SmUnitTest {
 private:
  /*
   * Ice response communuication class.
   */
  class TestResp : public FDS_ProtocolInterface::FDSP_DataPathResp {
   private:
    SmUnitTest *parentUt;

   public:
    explicit TestResp(SmUnitTest* parentUt_arg) :
        parentUt(parentUt_arg) {
    }
    ~TestResp() {
    }

    void GetObjectResp(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
                       const FDS_ProtocolInterface::FDSP_GetObjTypePtr& get_req,
                       const Ice::Current&) {
      /*
       * TODO: May want to sanity check the other response fields.
       */
      fds_verify(msg_hdr->result == FDS_ProtocolInterface::FDSP_ERR_OK);

      ObjectID oid(get_req->data_obj_id.hash_high,
                   get_req->data_obj_id.hash_low);
      std::string objData = get_req->data_obj;
      fds_verify(parentUt->checkGetObj(oid, objData) == true);
    }

    void PutObjectResp(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
                       const FDS_ProtocolInterface::FDSP_PutObjTypePtr& put_req,
                       const Ice::Current&) {
      /*
       * TODO: May want to sanity check the other response fields.
       */
      fds_verify(msg_hdr->result == FDS_ProtocolInterface::FDSP_ERR_OK);
    }

    void UpdateCatalogObjectResp(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr&
                                 fdsp_msg,
                                 const
                                 FDS_ProtocolInterface::FDSP_UpdateCatalogTypePtr&
                                 update_req,
                                 const Ice::Current &) {
    }

    void QueryCatalogObjectResp(const
                                FDS_ProtocolInterface::FDSP_MsgHdrTypePtr&
                                fdsp_msg,
                                const
                                FDS_ProtocolInterface::FDSP_QueryCatalogTypePtr&
                                cat_obj_req,
                                const Ice::Current &) {
    }

    void OffsetWriteObjectResp(const
                               FDS_ProtocolInterface::FDSP_MsgHdrTypePtr&
                             fdsp_msg,
                               const
                               FDS_ProtocolInterface::FDSP_OffsetWriteObjTypePtr&
                               offset_write_obj_req,
                               const Ice::Current &) {
    }
    void RedirReadObjectResp(const
                             FDS_ProtocolInterface::FDSP_MsgHdrTypePtr&
                             fdsp_msg,
                             const
                             FDS_ProtocolInterface::FDSP_RedirReadObjTypePtr&
                             redir_write_obj_req,
                             const Ice::Current &) {
    }
  };

  std::list<std::string>  unit_tests;
  FDS_ProtocolInterface::FDSP_DataPathReqPrx& fdspDPAPI;
  TestResp* fdspDataPathResp;
  Ice::ObjectAdapterPtr adapter;

  fds_log *test_log;

  fds_uint32_t num_updates;

  std::unordered_map<ObjectID, std::string, ObjectHash> added_objs;
  fds_mutex *objMapLock;
  std::atomic<fds_uint64_t>  ackedGets;

  /*
   * Helper functions.
   */
  fds_bool_t checkGetObj(const ObjectID& oid,
                         const std::string& objData) {
    ackedGets++;
    objMapLock->lock();
    if (objData != added_objs[oid]) {
      FDS_PLOG(test_log) << "Failed get correct object! For object "
                         << oid << " Got ["
                         << objData << "] but expected ["
                         << added_objs[oid] << "]";
      objMapLock->unlock();
      return false;
    }
    FDS_PLOG(test_log) << "Get object " << oid
                       << " check: SUCCESS; " << objData;
    objMapLock->unlock();
    return true;
  }

  void updatePutObj(const ObjectID& oid,
                    const std::string& objData) {
    objMapLock->lock();
    /*
     * Note that this overwrites whatever was previously
     * cached at that oid.
     */
    added_objs[oid] = objData;
    FDS_PLOG(test_log) << "Put object " << oid << " with data "
                       << objData << " into map ";
    objMapLock->unlock();
  }

  /*
   * Unit test funtions
   */
  fds_int32_t basic_update() {

    FDS_PLOG(test_log) << "Starting test: basic_update()";
    
    /*
     * Send lots of put requests.
     */
    FDS_ProtocolInterface::FDSP_MsgHdrTypePtr msg_hdr =
        new FDS_ProtocolInterface::FDSP_MsgHdrType;
    FDS_ProtocolInterface::FDSP_PutObjTypePtr put_req =
        new FDS_ProtocolInterface::FDSP_PutObjType;
    
    msg_hdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_PUT_OBJ_REQ;    
    msg_hdr->src_id   = FDS_ProtocolInterface::FDSP_STOR_HVISOR;
    msg_hdr->dst_id   = FDS_ProtocolInterface::FDSP_STOR_MGR;    
    msg_hdr->result   = FDS_ProtocolInterface::FDSP_ERR_OK;
    msg_hdr->err_code = FDS_ProtocolInterface::FDSP_ERR_SM_NO_SPACE;
    /*
     * TODO: Change this! We should reg a volume.
     */
    msg_hdr->glob_volume_id = 5;
    
    fds_uint32_t volume_offset;
    ObjectID oid;
    for (fds_uint32_t i = 1; i < num_updates + 1; i++) {
      volume_offset = i;
      oid = ObjectID(i, i * i);
      std::ostringstream convert;
      convert << i;
      std::string object_data = "I am object number " + convert.str();

      put_req->volume_offset         = volume_offset;
      put_req->data_obj_id.hash_high = oid.GetHigh();
      put_req->data_obj_id.hash_low  = oid.GetLow();
      put_req->data_obj_len          = object_data.size();
      put_req->data_obj              = object_data;
      msg_hdr->num_objects           = 1;
    
      try {
        updatePutObj(oid, object_data);
        fdspDPAPI->PutObject(msg_hdr, put_req);
        FDS_PLOG(test_log) << "Sent put obj message to SM"
                           << " for volume offset " << put_req->volume_offset
                           << " with object ID " << oid << " and data "
                           << object_data;
      } catch(...) {
        FDS_PLOG(test_log) << "Failed to put obj message to SM"
                           << " for volume offset" << put_req->volume_offset
                           << " with object ID " << oid << " and data "
                           << object_data;
        return -1;
      }
    }
    
    FDS_PLOG(test_log) << "Ending test: basic_update()";
    return 0;
  }

  fds_int32_t basic_uq() {

    FDS_PLOG(test_log) << "Starting test: basic_uq()";
    
    FDS_ProtocolInterface::FDSP_MsgHdrTypePtr msg_hdr =
        new FDS_ProtocolInterface::FDSP_MsgHdrType;
    FDS_ProtocolInterface::FDSP_PutObjTypePtr put_req =
        new FDS_ProtocolInterface::FDSP_PutObjType;
    
    msg_hdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_PUT_OBJ_REQ;    
    msg_hdr->src_id   = FDS_ProtocolInterface::FDSP_STOR_HVISOR;
    msg_hdr->dst_id   = FDS_ProtocolInterface::FDSP_STOR_MGR;    
    msg_hdr->result   = FDS_ProtocolInterface::FDSP_ERR_OK;
    msg_hdr->err_code = FDS_ProtocolInterface::FDSP_ERR_SM_NO_SPACE;
    /*
     * TODO: Change this! We should reg a volume.
     */
    msg_hdr->glob_volume_id = 5;
    
    fds_uint32_t volume_offset;
    std::list<ObjectID> objIdsPut;
    for (fds_uint32_t i = 0; i < num_updates; i++) {
      volume_offset = i;
      std::ostringstream convert;
      convert << i;
      std::string object_data = "I am object number " + convert.str();
      ObjectID oid;
      MurmurHash3_x64_128(object_data.c_str(),
                          object_data.size() + 1,
                          0,
                          &oid);

      put_req->volume_offset         = volume_offset;
      put_req->data_obj_id.hash_high = oid.GetHigh();
      put_req->data_obj_id.hash_low  = oid.GetLow();
      put_req->data_obj_len          = object_data.size();
      put_req->data_obj              = object_data;
      msg_hdr->num_objects           = 1;

      try {
        updatePutObj(oid, object_data);
        fdspDPAPI->PutObject(msg_hdr, put_req);
        FDS_PLOG(test_log) << "Sent put obj message to SM"
                           << " for volume offset " << put_req->volume_offset
                           << " with object ID " << oid << " and data "
                           << object_data;
      } catch(...) {
        FDS_PLOG(test_log) << "Failed to put obj message to SM"
                           << " for volume offset" << put_req->volume_offset
                           << " with object ID " << oid << " and data "
                           << object_data;
        return -1;
      }
      objIdsPut.push_back(oid);
    }

    ackedGets = 0;
    FDS_ProtocolInterface::FDSP_GetObjTypePtr get_req =
        new FDS_ProtocolInterface::FDSP_GetObjType;
    
    msg_hdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_GET_OBJ_REQ;
    msg_hdr->src_id   = FDS_ProtocolInterface::FDSP_STOR_HVISOR;
    msg_hdr->dst_id   = FDS_ProtocolInterface::FDSP_STOR_MGR;
    msg_hdr->result   = FDS_ProtocolInterface::FDSP_ERR_OK;
    msg_hdr->err_code = FDS_ProtocolInterface::FDSP_ERR_SM_NO_SPACE;
    /*
     * TODO: Change this! We should reg a volume.
     */
    msg_hdr->glob_volume_id = 5;

    ObjectID oid;
    for (fds_uint32_t i = 0; i < objIdsPut.size(); i++) {
      oid = objIdsPut.front();
      get_req->data_obj_id.hash_high = oid.GetHigh();
      get_req->data_obj_id.hash_low  = oid.GetLow();
      get_req->data_obj_len          = 0;
    
      try {
        fdspDPAPI->GetObject(msg_hdr, get_req);
        FDS_PLOG(test_log) << "Sent get obj message to SM"
                           << " with object ID " << oid;
      } catch(...) {
        FDS_PLOG(test_log) << "Failed get obj message to SM"
                           << " with object ID " << oid;
        return -1;
      }

      objIdsPut.pop_front();
    }

    /*
     * Spin and wait for the gets to complete.
     */
    fds_uint64_t acks = ackedGets.load();
    while (acks != objIdsPut.size()) {
      FDS_PLOG(test_log) << "Received " << acks
                         << " of " << objIdsPut.size()
                         << " get response";
      sleep(1);
      acks = ackedGets.load();
    }
    FDS_PLOG(test_log) << "Received all " << acks << " of "
                       << objIdsPut.size() << " get responses";

    FDS_PLOG(test_log) << "Ending test: basic_uq()";

    return 0;
  }

  fds_int32_t basic_dedupe() {

    FDS_PLOG(test_log) << "Starting test: basic_dedupe()";
    
    FDS_ProtocolInterface::FDSP_MsgHdrTypePtr msg_hdr =
        new FDS_ProtocolInterface::FDSP_MsgHdrType;
    FDS_ProtocolInterface::FDSP_PutObjTypePtr put_req =
        new FDS_ProtocolInterface::FDSP_PutObjType;
    
    msg_hdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_PUT_OBJ_REQ;    
    msg_hdr->src_id   = FDS_ProtocolInterface::FDSP_STOR_HVISOR;
    msg_hdr->dst_id   = FDS_ProtocolInterface::FDSP_STOR_MGR;    
    msg_hdr->result   = FDS_ProtocolInterface::FDSP_ERR_OK;
    msg_hdr->err_code = FDS_ProtocolInterface::FDSP_ERR_SM_NO_SPACE;
    /*
     * TODO: Change this! We should reg a volume.
     */
    msg_hdr->glob_volume_id = 5;
    
    fds_uint32_t volume_offset;
    ObjectID oid;
    for (fds_uint32_t i = 1; i < num_updates + 1; i++) {
      volume_offset = i;
      oid = ObjectID(i, i * i);
      std::ostringstream convert;
      convert << i;
      std::string object_data = "I am object number " + convert.str();

      put_req->volume_offset         = volume_offset;
      put_req->data_obj_id.hash_high = oid.GetHigh();
      put_req->data_obj_id.hash_low  = oid.GetLow();
      put_req->data_obj_len          = object_data.size();
      put_req->data_obj              = object_data;
      msg_hdr->num_objects           = 1;
    
      try {
        updatePutObj(oid, object_data);
        fdspDPAPI->PutObject(msg_hdr, put_req);
        FDS_PLOG(test_log) << "Sent put obj message to SM"
                           << " for volume offset " << put_req->volume_offset
                           << " with object ID " << oid << " and data "
                           << object_data;
        //Put the object again and expect no errors just logs of a dupe
        fdspDPAPI->PutObject(msg_hdr, put_req);
      } catch(...) {
        FDS_PLOG(test_log) << "Failed to put obj message to SM"
                           << " for volume offset" << put_req->volume_offset
                           << " with object ID " << oid << " and data "
                           << object_data;
        return -1;
      }
    }

    ackedGets = 0;
    FDS_ProtocolInterface::FDSP_GetObjTypePtr get_req =
        new FDS_ProtocolInterface::FDSP_GetObjType;
    
    msg_hdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_GET_OBJ_REQ;
    msg_hdr->src_id   = FDS_ProtocolInterface::FDSP_STOR_HVISOR;
    msg_hdr->dst_id   = FDS_ProtocolInterface::FDSP_STOR_MGR;
    msg_hdr->result   = FDS_ProtocolInterface::FDSP_ERR_OK;
    msg_hdr->err_code = FDS_ProtocolInterface::FDSP_ERR_SM_NO_SPACE;
    /*
     * TODO: Change this! We should reg a volume.
     */
    msg_hdr->glob_volume_id = 5;
    
    for (fds_uint32_t i = 1; i < num_updates + 1; i++) {
      oid = ObjectID(i, i * i);
      get_req->data_obj_id.hash_high = oid.GetHigh();
      get_req->data_obj_id.hash_low  = oid.GetLow();
      get_req->data_obj_len          = 0;
    
      try {
        fdspDPAPI->GetObject(msg_hdr, get_req);
        FDS_PLOG(test_log) << "Sent get obj message to SM"
                           << " with object ID " << oid;
      } catch(...) {
        FDS_PLOG(test_log) << "Failed get obj message to SM"
                           << " with object ID " << oid;
        return -1;
      }
    }

    /*
     * Spin and wait for the gets to complete.
     */
    fds_uint64_t acks = ackedGets.load();
    while (acks != num_updates) {
      FDS_PLOG(test_log) << "Received " << acks
                         << " of " << num_updates
                         << " get response";
      sleep(1);
      acks = ackedGets.load();
    }
    FDS_PLOG(test_log) << "Received all " << acks << " of "
                       << num_updates << " get responses";

    FDS_PLOG(test_log) << "Ending test: basic_dedupe()";

    return 0;
  }


  fds_int32_t basic_query() {

    FDS_PLOG(test_log) << "Starting test: basic_query()";
    
    FDS_ProtocolInterface::FDSP_MsgHdrTypePtr msg_hdr =
        new FDS_ProtocolInterface::FDSP_MsgHdrType;

    ObjectID oid;

    FDS_ProtocolInterface::FDSP_GetObjTypePtr get_req =
        new FDS_ProtocolInterface::FDSP_GetObjType;
    
    msg_hdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_GET_OBJ_REQ;
    msg_hdr->src_id   = FDS_ProtocolInterface::FDSP_STOR_HVISOR;
    msg_hdr->dst_id   = FDS_ProtocolInterface::FDSP_STOR_MGR;
    msg_hdr->result   = FDS_ProtocolInterface::FDSP_ERR_OK;
    msg_hdr->err_code = FDS_ProtocolInterface::FDSP_ERR_SM_NO_SPACE;
    
    for (fds_uint32_t i = 0; i < num_updates; i++) {
      oid = ObjectID(i, i * i);
      get_req->data_obj_id.hash_high = oid.GetHigh();
      get_req->data_obj_id.hash_low  = oid.GetLow();
      get_req->data_obj_len          = 0;
    
      try {
        fdspDPAPI->begin_GetObject(msg_hdr, get_req);
        FDS_PLOG(test_log) << "Sent get obj message to SM"
                           << " with object ID " << oid;
      } catch(...) {
        FDS_PLOG(test_log) << "Failed get obj message to SM"
                           << " with object ID " << oid;
        return -1;
      }
    }    

    FDS_PLOG(test_log) << "Ending test: basic_query()";

    return 0;
  }

 public:
  /*
   * The non-const refernce is OK.
   */
  explicit SmUnitTest(FDS_ProtocolInterface::FDSP_DataPathReqPrx&
                      fdspDPAPI_arg,
                      Ice::ObjectAdapterPtr adapter_arg)  // NOLINT(*)
      : fdspDPAPI(fdspDPAPI_arg),
        adapter(adapter_arg) {

    test_log   = new fds_log("sm_test", "logs");
    objMapLock = new fds_mutex("Added object map lock");

    fdspDataPathResp = new TestResp(this);
    fds_assert(fdspDataPathResp != NULL);

    Ice::Identity ident;
    ident.name = IceUtil::generateUUID();
    ident.category = "";

    adapter->add(fdspDataPathResp, ident);
    adapter->activate();

    fdspDPAPI->ice_getConnection()->setAdapter(adapter);
    fdspDPAPI->AssociateRespCallback(ident, "sm_test_client");

    unit_tests.push_back("basic_update");
    unit_tests.push_back("basic_uq");
    unit_tests.push_back("basic_dedupe");

    num_updates = 100;
  }

  explicit SmUnitTest(FDS_ProtocolInterface::FDSP_DataPathReqPrx&
                      fdspDPAPI_arg,
                      Ice::ObjectAdapterPtr adapter_arg,
                      fds_uint32_t num_up_arg)  // NOLINT(*)
      : SmUnitTest(fdspDPAPI_arg, adapter_arg) {
    num_updates = num_up_arg;
  }

  ~SmUnitTest() {
    /*
     * TODO: The test_log can be deleted while
     * Ice messages are still incoming and using
     * the log, which is a problem.
     * Leak the log for now and fix this soon using
     * coordination so that outstanding I/Os are
     * accounted for prior to destruction.
     */
    // delete fdspDataPathResp;
    delete objMapLock;
    delete test_log;
  }

  fds_log* GetLogPtr() {
    return test_log;
  }

  fds_int32_t Run(const std::string& testname) {
    fds_int32_t result = 0;
    std::cout << "Running unit test \"" << testname << "\"" << std::endl;

    if (testname == "basic_update") {
      result = basic_update();
    } else if (testname == "basic_uq") {
      result = basic_uq();
    } else if (testname == "basic_query") {
      result = basic_query();
    } else if (testname == "basic_dedupe") {
      result = basic_dedupe();
    } else {
      std::cout << "Unknown unit test " << testname << std::endl;
    }

    if (result == 0) {
      std::cout << "Unit test \"" << testname << "\" PASSED"  << std::endl;
    } else {
      std::cout << "Unit test \"" << testname << "\" FAILED" << std::endl;
    }
    std::cout << std::endl;

    return result;
  }

  void Run() {
    fds_int32_t result = 0;
    for (std::list<std::string>::iterator it = unit_tests.begin();
         it != unit_tests.end();
         ++it) {
      result = Run(*it);
      if (result != 0) {
        std::cout << "Unit test FAILED" << std::endl;
        break;
      }
    }

    if (result == 0) {
      std::cout << "Unit test PASSED" << std::endl;
    }
  }
};

/*
 * Ice request communication class.
 */
class TestClient : public Ice::Application {
 public:
  TestClient() {
  }
  ~TestClient() {
  }

  /*
   * Ice will execute the application via run()
   */
  int run(int argc, char* argv[]) {
    /*
     * Process the cmdline args.
     */
    std::string testname;
    fds_uint32_t num_updates = 100;
    fds_uint32_t port_num = 0, cp_port_num=0;
    std::string ip_addr_str;
    for (int i = 1; i < argc; i++) {
      if (strncmp(argv[i], "--testname=", 11) == 0) {
        testname = argv[i] + 11;
      } else if (strncmp(argv[i], "--num_updates=", 14) == 0) {
        num_updates = atoi(argv[i] + 14);
      } else if (strncmp(argv[i], "--port=", 7) == 0) {
        port_num = strtoul(argv[i] + 7, NULL, 0);
      } else if (strncmp(argv[i], "--cp_port=", 10) == 0) {
        cp_port_num = strtoul(argv[i] + 10, NULL, 0);
      } else {
        std::cout << "Invalid argument " << argv[i] << std::endl;
        return -1;
      }
    }

    /*
     * Setup the network communication. Create a direct connection to
     */
    Ice::PropertiesPtr props = communicator()->getProperties();
    Ice::ObjectPrx op;
    if (port_num == 0) {
      op = communicator()->propertyToProxy("ObjectStorMgrSvr.Proxy");
    } else if (cp_port_num == 0) { 
      cp_port_num = communicator()->propertyToProxy("ObjectStorMgrSvr.ControlPort");
    } else {
      std::ostringstream tcpProxyStr;
      ip_addr_str = props->getProperty("ObjectStorMgrSvr.IPAddress");

      tcpProxyStr << "ObjectStorMgrSvr: tcp -h " << ip_addr_str << " -p " << port_num;
      op = communicator()->stringToProxy(tcpProxyStr.str());
    }

    FDS_ProtocolInterface::FDSP_DataPathReqPrx fdspDPAPI =
        FDS_ProtocolInterface::FDSP_DataPathReqPrx::checkedCast(op); // NOLINT(*)

    Ice::ObjectAdapterPtr adapter = communicator()->createObjectAdapter("");
    SmUnitTest unittest(fdspDPAPI, adapter, num_updates);

    if (testname.empty()) {
      unittest.Run();
    } else {
      unittest.Run(testname);
    }

    return 0;
  }

 private:
};

}  // namespace fds

int main(int argc, char* argv[]) {
  fds::TestClient sm_client;

  return sm_client.main(argc, argv, "stor_mgr.conf");
}
