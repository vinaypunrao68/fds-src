/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <cstdlib>
#include <Ice/Ice.h>
#include <IceUtil/IceUtil.h>
#include "fdsp/FDSP.h"

#include <iostream>  // NOLINT(*)
#include <vector>
#include <string>
#include <list>

#include "util/Log.h"
#include "include/fds_types.h"

#define NUM_CONC_REQS 3

namespace fds {

class DmUnitTest {
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
     * Send lots of transaction open requests.
     */
    FDS_ProtocolInterface::FDSP_MsgHdrTypePtr msg_hdr =
        new FDS_ProtocolInterface::FDSP_MsgHdrType;
    FDS_ProtocolInterface::FDSP_UpdateCatalogTypePtr update_req =
        new FDS_ProtocolInterface::FDSP_UpdateCatalogType;
    
    msg_hdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_UPDATE_CAT_OBJ_REQ;    
    msg_hdr->src_id   = FDS_ProtocolInterface::FDSP_STOR_HVISOR;
    msg_hdr->dst_id   = FDS_ProtocolInterface::FDSP_DATA_MGR;    
    msg_hdr->result   = FDS_ProtocolInterface::FDSP_ERR_OK;
    msg_hdr->err_code = FDS_ProtocolInterface::FDSP_ERR_SM_NO_SPACE;
    msg_hdr->glob_volume_id = 1; /* TODO: Don't hard code to 1 */
    
    fds_uint32_t block_id;
    ObjectID oid;
    Ice::AsyncResultPtr rp[NUM_CONC_REQS];

    for (fds_uint32_t i = 0; i < num_updates; i++) {
      block_id = i;
      oid = ObjectID(i, i * i);

      update_req->volume_offset         = block_id;
      update_req->dm_transaction_id     = 1;
      update_req->dm_operation          =
          FDS_ProtocolInterface::FDS_DMGR_TXN_STATUS_OPEN;
      update_req->data_obj_id.hash_high = oid.GetHigh();
      update_req->data_obj_id.hash_low  = oid.GetLow();
    
      try {
        rp[i%NUM_CONC_REQS] = fdspDPAPI->begin_UpdateCatalogObject(msg_hdr, update_req);
        FDS_PLOG(test_log) << "Sent trans open message to DM"
                          << " for volume offset" << update_req->volume_offset
                          << " and object " << oid;
      } catch(...) {
        FDS_PLOG(test_log) << "Failed to send trans open message to DM"
                           << " for volume offsete " << update_req->volume_offset
                           << " an object " << oid;
      }

      if (i % NUM_CONC_REQS == NUM_CONC_REQS-1) {
	int j;
	for (j = 0; j < NUM_CONC_REQS; j++) {
	  rp[j]->waitForCompleted();
	}
      }
      
    }

    /*
     * Send commits for each open.
     */
    for (fds_uint32_t i = 0; i < num_updates; i++) {
      block_id = i;
      oid = ObjectID(i, i * i);

      update_req->volume_offset         = block_id;
      update_req->dm_transaction_id     = 1;
      update_req->dm_operation          =
          FDS_ProtocolInterface::FDS_DMGR_TXN_STATUS_COMMITED;
      update_req->data_obj_id.hash_high = oid.GetHigh();
      update_req->data_obj_id.hash_low  = oid.GetLow();

      try {
        Ice::AsyncResultPtr rp = fdspDPAPI->begin_UpdateCatalogObject(msg_hdr, update_req);
        FDS_PLOG(test_log) << "Sent trans commit message to DM"
                           << " for volume offset" << update_req->volume_offset
                           << " and object " << oid;
	rp->waitForCompleted();
      } catch(...) {
        FDS_PLOG(test_log) << "Failed to send trans commit message to DM"
                           << " for volume offsete " << update_req->volume_offset
                           << " an object " << oid;
      }
    }
    
    sleep(10);

    FDS_PLOG(test_log) << "Ending test: basic_update()";
    return 0;
  }

  fds_int32_t basic_uq() {

    FDS_PLOG(test_log) << "Starting test: basic_uq()";

    /*
     * Send lots of transaction open requests.
     */
    FDS_ProtocolInterface::FDSP_MsgHdrTypePtr msg_hdr =
        new FDS_ProtocolInterface::FDSP_MsgHdrType;
    FDS_ProtocolInterface::FDSP_UpdateCatalogTypePtr update_req =
        new FDS_ProtocolInterface::FDSP_UpdateCatalogType;
    
    msg_hdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_UPDATE_CAT_OBJ_REQ;    
    msg_hdr->src_id   = FDS_ProtocolInterface::FDSP_STOR_HVISOR;
    msg_hdr->dst_id   = FDS_ProtocolInterface::FDSP_DATA_MGR;    
    msg_hdr->result   = FDS_ProtocolInterface::FDSP_ERR_OK;
    msg_hdr->err_code = FDS_ProtocolInterface::FDSP_ERR_SM_NO_SPACE;
    msg_hdr->glob_volume_id = 1; /* TODO: Don't hard code to 1 */
    
    fds_uint32_t block_id;
    ObjectID oid;
    for (fds_uint32_t i = 0; i < num_updates; i++) {
      block_id = i;
      oid = ObjectID(i, i * i);

      update_req->volume_offset         = block_id;
      update_req->dm_transaction_id     = 1;
      update_req->dm_operation          =
          FDS_ProtocolInterface::FDS_DMGR_TXN_STATUS_OPEN;
      update_req->data_obj_id.hash_high = oid.GetHigh();
      update_req->data_obj_id.hash_low  = oid.GetLow();
    
      try {
        fdspDPAPI->UpdateCatalogObject(msg_hdr, update_req);
        FDS_PLOG(test_log) << "Sent trans open message to DM"
                          << " for volume offset" << update_req->volume_offset
                          << " and object " << oid;
      } catch(...) {
        FDS_PLOG(test_log) << "Failed to send trans open message to DM"
                           << " for volume offsete " << update_req->volume_offset
                           << " an object " << oid;
      }
    }

    /*
     * Send commits for each open.
     */
    for (fds_uint32_t i = 0; i < num_updates; i++) {
      block_id = i;
      oid = ObjectID(i, i * i);

      update_req->volume_offset         = block_id;
      update_req->dm_transaction_id     = 1;
      update_req->dm_operation          =
          FDS_ProtocolInterface::FDS_DMGR_TXN_STATUS_COMMITED;
      update_req->data_obj_id.hash_high = oid.GetHigh();
      update_req->data_obj_id.hash_low  = oid.GetLow();

      try {
        fdspDPAPI->UpdateCatalogObject(msg_hdr, update_req);
        FDS_PLOG(test_log) << "Sent trans commit message to DM"
                           << " for volume offset" << update_req->volume_offset
                           << " and object " << oid;
      } catch(...) {
        FDS_PLOG(test_log) << "Failed to send trans commit message to DM"
                           << " for volume offsete " << update_req->volume_offset
                           << " an object " << oid;
      }
    }
    
    /*
     * Send queries for the newly put DM entires.
     */
    FDS_ProtocolInterface::FDSP_QueryCatalogTypePtr query_req =
        new FDS_ProtocolInterface::FDSP_QueryCatalogType;
    msg_hdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_QUERY_CAT_OBJ_REQ;
    
    for (fds_uint32_t i = 0; i < num_updates; i++) {
      block_id = i;
      
      query_req->volume_offset = block_id;
      
      /*
       * Just set defaults for the other fields
       * They will be set on the response.
       */
      query_req->dm_transaction_id     = 1;
      query_req->dm_operation          =
          FDS_ProtocolInterface::FDS_DMGR_TXN_STATUS_OPEN;
      query_req->data_obj_id.hash_high = 0;
      query_req->data_obj_id.hash_low  = 0;

      try {
        fdspDPAPI->QueryCatalogObject(msg_hdr, query_req);
        FDS_PLOG(test_log) << "Sent query message to DM"
                           << " for volume offset" << update_req->volume_offset;
      } catch(...) {
        FDS_PLOG(test_log) << "Failed to send query message to DM"
                           << " for volume offsete " << update_req->volume_offset;
      }
    }

    FDS_PLOG(test_log) << "Ending test: basic_uq()";

    return 0;
  }
  
  fds_int32_t basic_query() {
    
    FDS_PLOG(test_log) << "Starting test: basic_query()";
    
    /*
     * Send lots of transaction open requests.
     */
    FDS_ProtocolInterface::FDSP_MsgHdrTypePtr msg_hdr =
        new FDS_ProtocolInterface::FDSP_MsgHdrType;
    
    msg_hdr->src_id   = FDS_ProtocolInterface::FDSP_STOR_HVISOR;
    msg_hdr->dst_id   = FDS_ProtocolInterface::FDSP_DATA_MGR;    
    msg_hdr->result   = FDS_ProtocolInterface::FDSP_ERR_OK;
    msg_hdr->err_code = FDS_ProtocolInterface::FDSP_ERR_SM_NO_SPACE;
    msg_hdr->glob_volume_id = 1; /* TODO: Don't hard code to 1 */
    
    fds_uint32_t block_id;
    ObjectID oid;
 
    /*
     * Send queries for the newly put DM entires.
     */
    FDS_ProtocolInterface::FDSP_QueryCatalogTypePtr query_req =
        new FDS_ProtocolInterface::FDSP_QueryCatalogType;
    msg_hdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_QUERY_CAT_OBJ_REQ;
    
    for (fds_uint32_t i = 0; i < num_updates; i++) {
      block_id = i;
      
      query_req->volume_offset = block_id;
      
      /*
       * Just set defaults for the other fields
       * They will be set on the response.
       */
      query_req->dm_transaction_id     = 1;
      query_req->dm_operation          =
          FDS_ProtocolInterface::FDS_DMGR_TXN_STATUS_OPEN;
      query_req->data_obj_id.hash_high = 0;
      query_req->data_obj_id.hash_low  = 0;
      
      try {
        fdspDPAPI->begin_QueryCatalogObject(msg_hdr, query_req);
        FDS_PLOG(test_log) << "Sent query message to DM"
                           << " for volume offset" << query_req->volume_offset;
      } catch(...) {
        FDS_PLOG(test_log) << "Failed to send query message to DM"
                           << " for volume offsete " << query_req->volume_offset;
      }
    }

    sleep(5);

    FDS_PLOG(test_log) << "Ending test: basic_uq()";

    return 0;
  }
  
  int basic_multivol() {

    FDS_PLOG(test_log) << "Starting test: basic_multivol()";

    FDS_PLOG(test_log) << "Ending test: basic_multivol()";

    return 0;
  }

 public:
  /*
   * The non-const refernce is OK.
   */
  explicit DmUnitTest(FDS_ProtocolInterface::FDSP_DataPathReqPrx&
                       fdspDPAPI_arg) // NOLINT(*)
      : fdspDPAPI(fdspDPAPI_arg) {

    test_log = new fds_log("dm_test", "logs");

    unit_tests.push_back("basic_update");
    unit_tests.push_back("basic_uq");
    unit_tests.push_back("basic_query");
    unit_tests.push_back("basic_multivol");

    num_updates = 100;
  }

  explicit DmUnitTest(FDS_ProtocolInterface::FDSP_DataPathReqPrx&
                      fdspDPAPI_arg,
                      fds_uint32_t num_up_arg) // NOLINT(*)
      : fdspDPAPI(fdspDPAPI_arg) {

    test_log = new fds_log("dm_test", "logs");

    unit_tests.push_back("basic_update");
    unit_tests.push_back("basic_uq");
    unit_tests.push_back("basic_query");
    unit_tests.push_back("basic_multivol");

    num_updates = num_up_arg;
  }

  ~DmUnitTest() {
    delete test_log;
  }

  fds_log* GetLogPtr() {
    return test_log;
  }

  fds_int32_t Run(const std::string& testname) {
    int result = 0;
    std::cout << "Running unit test \"" << testname << "\"" << std::endl;

    if (testname == "basic_update") {
      result = basic_update();
    } else if (testname == "basic_uq") {
      result = basic_uq();
    } else if (testname == "basic_query") {
      result = basic_query();
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
 * Ice communication classes
 */
class TestResp : public FDS_ProtocolInterface::FDSP_DataPathResp {
 private:
  fds_log *test_log;

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

  void GetObjectResp(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr&,
                     const FDS_ProtocolInterface::FDSP_GetObjTypePtr&,
                     const Ice::Current&) {
  }

  void PutObjectResp(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr&,
                     const FDS_ProtocolInterface::FDSP_PutObjTypePtr&,
                     const Ice::Current&) {
  }

  void UpdateCatalogObjectResp(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr&
                               fdsp_msg,
                               const
                               FDS_ProtocolInterface::FDSP_UpdateCatalogTypePtr&
                               update_req,
                               const Ice::Current &) {
    if (update_req->dm_operation ==
        FDS_ProtocolInterface::FDS_DMGR_TXN_STATUS_OPEN) {
      FDS_PLOG(test_log) << "Received DM catalog trans open response: ";
    } else if (update_req->dm_operation ==
               FDS_ProtocolInterface::FDS_DMGR_TXN_STATUS_COMMITED) {
      FDS_PLOG(test_log) << "Received DM catalog trans commit response: ";
    }

    if (fdsp_msg->result == FDS_ProtocolInterface::FDSP_ERR_OK) {
      FDS_PLOG(test_log) << "success";
    } else {
      FDS_PLOG(test_log) << "failure";
      /*
       * TODO: Just panic for now. Eventually we want to tie
       * the response back to the request in the unit test
       * function and have the unit test return error.
       */
      assert(0);
    }
  }

  void QueryCatalogObjectResp(const
                              FDS_ProtocolInterface::FDSP_MsgHdrTypePtr&
                              fdsp_msg,
                               const
                              FDS_ProtocolInterface::FDSP_QueryCatalogTypePtr&
                              cat_obj_req,
                              const Ice::Current &) {
    if (fdsp_msg->result == FDS_ProtocolInterface::FDSP_ERR_OK) {
      ObjectID oid(cat_obj_req->data_obj_id.hash_high,
                   cat_obj_req->data_obj_id.hash_low);
      FDS_PLOG(test_log) << "Received query response success with object "
                         << oid << " for volume offset " << cat_obj_req->volume_offset;
      if ((cat_obj_req->data_obj_id.hash_high * cat_obj_req->data_obj_id.hash_high != cat_obj_req->data_obj_id.hash_low)
	  || (cat_obj_req->volume_offset != cat_obj_req->data_obj_id.hash_high)) {
	FDS_PLOG(test_log) << "****** Received object ID seems to be incorrect";
      }
    } else {
      FDS_PLOG(test_log) << "Received query response failure for offset "
                         << cat_obj_req->volume_offset;
      /*
       * TODO: Just panic for now. Eventually we want to tie
       * the response back to the request in the unit test
       * function and have the unit test return error.
       */
      assert(0);
    }
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
      op = communicator()->propertyToProxy("StorHvisorClient.Proxy");
    } else {
      std::ostringstream tcpProxyStr;
      ip_addr_str = props->getProperty("DataMgr.IPAddress");

      tcpProxyStr << "DataMgr: tcp -h " << ip_addr_str << " -p " << port_num;
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

    DmUnitTest unittest(fdspDPAPI, num_updates);

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
  fds::TestClient dm_client;

  return dm_client.main(argc, argv, "dm_test.conf");
}
