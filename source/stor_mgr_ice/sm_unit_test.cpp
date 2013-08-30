/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <cstdlib>
#include <Ice/Ice.h>
#include <IceUtil/IceUtil.h>
#include "fdsp/FDSP.h"

#include <iostream>  // NOLINT(*)
#include <unordered_map>
#include <vector>
#include <string>
#include <list>

#include "util/Log.h"
#include "include/fds_types.h"

namespace fds {

/*
 * Ice response communuication class.
 */
class TestResp : public FDS_ProtocolInterface::FDSP_DataPathResp {
 private:
  fds_log *test_log;
  std::unordered_map<ObjectID, std::string, ObjectHash> added_objs;

 public:
  TestResp()
      : test_log(NULL) {
  }

  explicit TestResp(fds_log *log)
      : test_log(log) {
  }
  ~TestResp() {
    
  }

  void SetLog(fds_log *log) {
    assert(test_log == NULL);
    test_log = log;
  }

  void GetObjectResp(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
                     const FDS_ProtocolInterface::FDSP_GetObjTypePtr& get_req,
                     const Ice::Current&) {
    ObjectID oid(get_req->data_obj_id.hash_high,
                 get_req->data_obj_id.hash_low);
    if (msg_hdr->result == FDS_ProtocolInterface::FDSP_ERR_OK) {
      std::string object_data = get_req->data_obj;
      
      if (object_data != added_objs[oid]) {
        FDS_PLOG(test_log) << "Failed get correct object! Got"
                           << object_data << " but expected "
                           << added_objs[oid];
        assert(0);
      }
      FDS_PLOG(test_log) << "Get object " << oid
                         << " response: SUCCESS; " << object_data;
    } else {
      FDS_PLOG(test_log) << "Get object " << oid << " response: FAILURE";
      /*
       * TODO: Just panic for now. Eventually we want to tie
       * the response back to the request in the unit test
       * function and have the unit test return error.
       */
      assert(0);
    }
  }

  void PutObjectResp(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
                     const FDS_ProtocolInterface::FDSP_PutObjTypePtr& put_req,
                     const Ice::Current&) {
    ObjectID oid(put_req->data_obj_id.hash_high,
                 put_req->data_obj_id.hash_low);
    if (msg_hdr->result == FDS_ProtocolInterface::FDSP_ERR_OK) {
      FDS_PLOG(test_log) << "Put object response: SUCCESS";

      added_objs[oid] = put_req->data_obj;
    } else {
      FDS_PLOG(test_log) << "Put object response: FAILURE";
      /*
       * TODO: Just panic for now. Eventually we want to tie
       * the response back to the request in the unit test
       * function and have the unit test return error.
       */
      assert(0);
    }
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

class SmUnitTest {
 private:
  std::list<std::string>  unit_tests;
  FDS_ProtocolInterface::FDSP_DataPathReqPrx& fdspDPAPI;

  fds_log *test_log;

  fds_uint32_t num_updates;

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
    
    fds_uint32_t volume_offset;
    ObjectID oid;
    for (fds_uint32_t i = 0; i < num_updates; i++) {
      volume_offset = i;
      oid = ObjectID(i, i * i);
      std::string object_data = "I'm object number " + i;

      put_req->volume_offset         = volume_offset;
      put_req->data_obj_id.hash_high = oid.GetHigh();
      put_req->data_obj_id.hash_low  = oid.GetLow();
      put_req->data_obj_len          = object_data.size();
      put_req->data_obj              = object_data;
    
      try {
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
    
    fds_uint32_t volume_offset;
    ObjectID oid;
    for (fds_uint32_t i = 0; i < num_updates; i++) {
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
        fdspDPAPI->GetObject(msg_hdr, get_req);
        FDS_PLOG(test_log) << "Sent get obj message to SM"
                           << " with object ID " << oid;
      } catch(...) {
        FDS_PLOG(test_log) << "Failed get obj message to SM"
                           << " with object ID " << oid;
        return -1;
      }
    }    

    FDS_PLOG(test_log) << "Ending test: basic_uq()";

    return 0;
  }

 public:
  /*
   * The non-const refernce is OK.
   */
  explicit SmUnitTest(FDS_ProtocolInterface::FDSP_DataPathReqPrx&
                      fdspDPAPI_arg) // NOLINT(*)
      : fdspDPAPI(fdspDPAPI_arg) {

    test_log = new fds_log("sm_test", "logs");

    unit_tests.push_back("basic_update");
    unit_tests.push_back("basic_uq");

    num_updates = 100;
  }

  explicit SmUnitTest(FDS_ProtocolInterface::FDSP_DataPathReqPrx&
                      fdspDPAPI_arg,
                      fds_uint32_t num_up_arg) // NOLINT(*)
      : fdspDPAPI(fdspDPAPI_arg),
        num_updates(num_up_arg) {

    test_log = new fds_log("sm_test", "logs");

    unit_tests.push_back("basic_update");
    unit_tests.push_back("basic_uq");

    num_updates = num_up_arg;
  }

  ~SmUnitTest() {
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
    fds_uint32_t port_num = 0;
    std::string ip_addr_str;
    for (int i = 1; i < argc; i++) {
      if (strncmp(argv[i], "--testname=", 11) == 0) {
        testname = argv[i] + 11;
      } else if (strncmp(argv[i], "--num_updates=", 14) == 0) {
        num_updates = atoi(argv[i] + 14);
      } else if (strncmp(argv[i], "--port=", 7) == 0) {
        port_num = strtoul(argv[i] + 7, NULL, 0);
      } else {
        std::cout << "Invalid argument " << argv[i] << std::endl;
        return -1;
      }
    }

    /*
     * Setup the network communication. Create a direct connection to
     * a single DM.
     */
    Ice::PropertiesPtr props = communicator()->getProperties();
    Ice::ObjectPrx op;
    if (port_num == 0) {
      op = communicator()->propertyToProxy("ObjectStorMgrSvr.Proxy");
    } else {
      std::ostringstream tcpProxyStr;
      ip_addr_str = props->getProperty("ObjectStorMgrSvr.IPAddress");

      tcpProxyStr << "ObjectStorMgrSvr: tcp -h " << ip_addr_str << " -p " << port_num;
      op = communicator()->stringToProxy(tcpProxyStr.str());
    }

    FDS_ProtocolInterface::FDSP_DataPathReqPrx fdspDPAPI =
        FDS_ProtocolInterface::FDSP_DataPathReqPrx::checkedCast(op); // NOLINT(*)

    Ice::ObjectAdapterPtr adapter = communicator()->createObjectAdapter("");
    Ice::Identity ident;
    ident.name = IceUtil::generateUUID();
    ident.category = "";

    TestResp* fdspDataPathResp;
    fdspDataPathResp = new TestResp();
    if (!fdspDataPathResp) {
      throw "Invalid fdspDataPathRespCback";
    }

    adapter->add(fdspDataPathResp, ident);
    adapter->activate();

    fdspDPAPI->ice_getConnection()->setAdapter(adapter);
    fdspDPAPI->AssociateRespCallback(ident);

    SmUnitTest unittest(fdspDPAPI, num_updates);

    /*
     * This is kinda hackey. Want to
     * pass the logger to the ICE class
     * but it need to be constructed prior
     * to creating the unit test object.
     */
    fdspDataPathResp->SetLog(unittest.GetLogPtr());

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

  return sm_client.main(argc, argv, "stor_mgr.cfg");
}
