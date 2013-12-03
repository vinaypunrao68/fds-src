/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <Ice/Ice.h>
#include <IceUtil/IceUtil.h>

#include <cstdlib>
#include <iostream>  // NOLINT(*)
#include <vector>
#include <string>
#include <list>
#include <atomic>

#include <util/Log.h>
#include <fds_types.h>
#include <fdsp/FDSP.h>
#include "../../unit-test/lib/test_stat.h"

#define DEF_NUM_CONC_REQS 3
int num_conc_reqs = DEF_NUM_CONC_REQS;

std::atomic<unsigned int> num_ios_outstanding;
fds::StatIOPS *iops_stats;


namespace fds {

class DmUnitTest {
 private:
  std::list<std::string>  unit_tests;
  FDS_ProtocolInterface::FDSP_DataPathReqPrx& fdspDPAPI;
  FDS_ProtocolInterface::FDSP_ControlPathReqPrx& fdspCPAPI;

  fds_log *test_log;

  fds_uint32_t num_updates;
  bool test_mode_perf;
  int max_outstanding_ios;

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
    msg_hdr->src_node_name = "dm_test_client";
    msg_hdr->glob_volume_id = 1; /* TODO: Don't hard code to 1 */

    fds_uint32_t block_id;
    ObjectID oid;
    Ice::AsyncResultPtr *rp;
	
    rp = new Ice::AsyncResultPtr[num_conc_reqs];

    for (fds_uint32_t i = 0; i < num_updates; i++) {
      block_id = i;
      oid = ObjectID(i, i * i);

      if ((test_mode_perf) && (max_outstanding_ios > 0)){
	fds_uint32_t n_ios_outstanding = atomic_load(&num_ios_outstanding);
	while (n_ios_outstanding >= max_outstanding_ios)
	  {
	    usleep(1000);
	    n_ios_outstanding = atomic_load(&num_ios_outstanding);
	  }
      }

      update_req->blob_name         = std::to_string(block_id);
      update_req->dm_transaction_id     = 1;
      update_req->dm_operation          =
          FDS_ProtocolInterface::FDS_DMGR_TXN_STATUS_OPEN;
      
      update_req->obj_list.clear();
      FDS_ProtocolInterface::FDSP_BlobObjectInfo upd_obj_info;
      upd_obj_info.offset = 0; upd_obj_info.size = i*100+1;
      upd_obj_info.data_obj_id.hash_high = oid.GetHigh();
      upd_obj_info.data_obj_id.hash_low  = oid.GetLow();
      update_req->obj_list.push_back(upd_obj_info);
      update_req->meta_list.clear();

      try {
        rp[i%num_conc_reqs] = fdspDPAPI->begin_UpdateCatalogObject(
            msg_hdr, update_req);
        FDS_PLOG(test_log) << "Sent trans open message to DM"
                          << " for volume offset" << update_req->blob_name
                          << " and object " << oid;
	fds_uint32_t n_ios_outstanding = atomic_fetch_add(&num_ios_outstanding, (unsigned int)1);

      } catch(...) {
        FDS_PLOG(test_log) << "Failed to send trans open message to DM"
                           << " for volume offsete "
                           << update_req->blob_name
                           << " an object " << oid;
      }

      if (!test_mode_perf) {

	if (i % num_conc_reqs == num_conc_reqs-1) {
	  int j;
	  for (j = 0; j < num_conc_reqs; j++) {
	    rp[j]->waitForCompleted();
	  }
	}
      }

    }

    if (!test_mode_perf) {
    /*
     * Send commits for each open.
     */
    for (fds_uint32_t i = 0; i < num_updates; i++) {
      block_id = i;
      oid = ObjectID(i, i * i);

      update_req->blob_name         = std::to_string(block_id);
      update_req->dm_transaction_id     = 1;
      update_req->dm_operation          =
          FDS_ProtocolInterface::FDS_DMGR_TXN_STATUS_COMMITED;

      update_req->obj_list.clear();
      FDS_ProtocolInterface::FDSP_BlobObjectInfo upd_obj_info;
      upd_obj_info.offset = 0; upd_obj_info.size = i*100+1;
      upd_obj_info.data_obj_id.hash_high = oid.GetHigh();
      upd_obj_info.data_obj_id.hash_low  = oid.GetLow();
      update_req->obj_list.push_back(upd_obj_info);
      update_req->meta_list.clear();

      try {
        Ice::AsyncResultPtr rp = fdspDPAPI->begin_UpdateCatalogObject(
            msg_hdr, update_req);
        FDS_PLOG(test_log) << "Sent trans commit message to DM"
                           << " for volume offset"
                           << update_req->blob_name
                           << " and object " << oid;
	if (!test_mode_perf) {
	  rp->waitForCompleted();
	}
      } catch(...) {
        FDS_PLOG(test_log) << "Failed to send trans commit message to DM"
                           << " for volume offsete "
                           << update_req->blob_name
                           << " an object " << oid;
      }
    }
    }

    fds_uint32_t n_ios_outstanding = 0;

    do {
      sleep(10);
      n_ios_outstanding = atomic_load(&num_ios_outstanding);
    } while (n_ios_outstanding > 0);

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
    msg_hdr->src_node_name = "dm_test_client";
    msg_hdr->glob_volume_id = 1; /* TODO: Don't hard code to 1 */

    fds_uint32_t block_id;
    ObjectID oid;
    for (fds_uint32_t i = 0; i < num_updates; i++) {
      block_id = i;
      oid = ObjectID(i, i * i);

      update_req->blob_name         = std::to_string(block_id);
      update_req->dm_transaction_id     = 1;
      update_req->dm_operation          =
          FDS_ProtocolInterface::FDS_DMGR_TXN_STATUS_OPEN;

      update_req->obj_list.clear();
      FDS_ProtocolInterface::FDSP_BlobObjectInfo upd_obj_info;
      upd_obj_info.offset = 0; upd_obj_info.size = i*100+1;
      upd_obj_info.data_obj_id.hash_high = oid.GetHigh();
      upd_obj_info.data_obj_id.hash_low  = oid.GetLow();
      update_req->obj_list.push_back(upd_obj_info);
      update_req->meta_list.clear();

      try {
        fdspDPAPI->UpdateCatalogObject(msg_hdr, update_req);
        FDS_PLOG(test_log) << "Sent trans open message to DM"
                          << " for volume offset"
                           << update_req->blob_name
                          << " and object " << oid;
      } catch(...) {
        FDS_PLOG(test_log) << "Failed to send trans open message to DM"
                           << " for volume offsete "
                           << update_req->blob_name
                           << " an object " << oid;
      }
    }

    /*
     * Send commits for each open.
     */
    for (fds_uint32_t i = 0; i < num_updates; i++) {
      block_id = i;
      oid = ObjectID(i, i * i);

      update_req->blob_name         = std::to_string(block_id);
      update_req->dm_transaction_id     = 1;
      update_req->dm_operation          =
          FDS_ProtocolInterface::FDS_DMGR_TXN_STATUS_COMMITED;

      update_req->obj_list.clear();
      FDS_ProtocolInterface::FDSP_BlobObjectInfo upd_obj_info;
      upd_obj_info.offset = 0; upd_obj_info.size = i*100+1;
      upd_obj_info.data_obj_id.hash_high = oid.GetHigh();
      upd_obj_info.data_obj_id.hash_low  = oid.GetLow();
      update_req->obj_list.push_back(upd_obj_info);
      update_req->meta_list.clear();


      try {
        fdspDPAPI->UpdateCatalogObject(msg_hdr, update_req);
        FDS_PLOG(test_log) << "Sent trans commit message to DM"
                           << " for volume offset "
                           << update_req->blob_name
                           << " and object " << oid;
      } catch(...) {
        FDS_PLOG(test_log) << "Failed to send trans commit message to DM"
                           << " for volume offset "
                           << update_req->blob_name
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

      query_req->blob_name = std::to_string(block_id);

      /*
       * Just set defaults for the other fields
       * They will be set on the response.
       */
      query_req->dm_transaction_id     = 1;
      query_req->dm_operation          =
          FDS_ProtocolInterface::FDS_DMGR_TXN_STATUS_OPEN;
      query_req->obj_list.clear();
      query_req->meta_list.clear();

      try {
        fdspDPAPI->QueryCatalogObject(msg_hdr, query_req);
        FDS_PLOG(test_log) << "Sent query message to DM"
                           << " for volume offset "
                           << update_req->blob_name;
      } catch(...) {
        FDS_PLOG(test_log) << "Failed to send query message to DM"
                           << " for volume offset "
                           << update_req->blob_name;
      }
    }

    FDS_PLOG(test_log) << "Ending test: basic_uq()";

    return 0;
  }

  fds_uint32_t basic_bloblist() {
    FDS_PLOG(test_log) << "Starting test: basic_bloblist()";

    /*
     * Send lots of transaction open requests.
     */
    FDS_ProtocolInterface::FDSP_MsgHdrTypePtr msg_hdr =
        new FDS_ProtocolInterface::FDSP_MsgHdrType;
    FDS_ProtocolInterface::FDSP_GetVolumeBlobListReqTypePtr listReq =
        new FDS_ProtocolInterface::FDSP_GetVolumeBlobListReqType;

    msg_hdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_GET_VOL_BLOB_LIST_REQ;
    msg_hdr->src_id   = FDS_ProtocolInterface::FDSP_STOR_HVISOR;
    msg_hdr->dst_id   = FDS_ProtocolInterface::FDSP_DATA_MGR;
    msg_hdr->result   = FDS_ProtocolInterface::FDSP_ERR_OK;
    msg_hdr->err_code = FDS_ProtocolInterface::FDSP_ERR_SM_NO_SPACE;
    msg_hdr->src_node_name = "dm_test_client";
    msg_hdr->glob_volume_id = 2; /* TODO: Don't hard code to 1 */

    fds_int32_t blobsToReturn = 10;
    fds_uint64_t cookie = 1;
    Ice::AsyncResultPtr *rp;
	
    rp = new Ice::AsyncResultPtr[num_conc_reqs];

    for (fds_uint32_t i = 0; i < num_updates; i++) {
      listReq->max_blobs_to_return = blobsToReturn;
      listReq->iterator_cookie = cookie;
      msg_hdr->req_cookie = i;

      try {
        rp[i%num_conc_reqs] = fdspDPAPI->begin_GetVolumeBlobList(msg_hdr, listReq);
        FDS_PLOG(test_log) << "Sent list blobs message to DM";

      } catch(...) {
        FDS_PLOG(test_log) << "Failed to send list blobs message to DM";
      }

      if (i % num_conc_reqs == num_conc_reqs-1) {
        for (fds_uint32_t j = 0; j < num_conc_reqs; j++) {
          rp[j]->waitForCompleted();
        }
      }      
    }

    FDS_PLOG(test_log) << "Ending test: basic_bloblist()";
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
    msg_hdr->src_node_name = "dm_test_client";
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

      query_req->blob_name = std::to_string(block_id);

      /*
       * Just set defaults for the other fields
       * They will be set on the response.
       */
      query_req->dm_transaction_id     = 1;
      query_req->dm_operation          =
          FDS_ProtocolInterface::FDS_DMGR_TXN_STATUS_OPEN;
      query_req->obj_list.clear();
      query_req->meta_list.clear();

      try {
        fdspDPAPI->begin_QueryCatalogObject(msg_hdr, query_req);
        FDS_PLOG(test_log) << "Sent query message to DM"
                           << " for volume offset "
                           << query_req->blob_name;
      } catch(...) {
        FDS_PLOG(test_log) << "Failed to send query message to DM"
                           << " for volume offset "
                           << query_req->blob_name;
      }
    }

    sleep(5);

    FDS_PLOG(test_log) << "Ending test: basic_uq()";

    return 0;
  }

  int basic_multivol() {
    FDS_PLOG(test_log) << "Starting test: basic_multivol()";

    fds_uint32_t num_vols = 100;

    /*
     * Send lots of create vol requests.
     */
    FDS_ProtocolInterface::FDSP_MsgHdrTypePtr msg_hdr =
        new FDS_ProtocolInterface::FDSP_MsgHdrType;
    FDS_ProtocolInterface::FDSP_NotifyVolTypePtr vol_msg =
        new FDS_ProtocolInterface::FDSP_NotifyVolType;
    vol_msg->vol_desc = new FDS_ProtocolInterface::FDSP_VolumeDescType();

    msg_hdr->minor_ver = 0;
    msg_hdr->msg_code =
        FDS_ProtocolInterface::FDSP_MSG_NOTIFY_VOL;
    msg_hdr->msg_id =  1;

    msg_hdr->major_ver = 0xa5;
    msg_hdr->minor_ver = 0x5a;

    msg_hdr->num_objects = 1;
    msg_hdr->frag_len = 0;
    msg_hdr->frag_num = 0;

    msg_hdr->tennant_id = 0;
    msg_hdr->local_domain_id = 0;

    msg_hdr->src_id   = FDS_ProtocolInterface::FDSP_ORCH_MGR;
    msg_hdr->dst_id   = FDS_ProtocolInterface::FDSP_DATA_MGR;
    msg_hdr->result   = FDS_ProtocolInterface::FDSP_ERR_OK;
    msg_hdr->err_code = FDS_ProtocolInterface::FDSP_ERR_SM_NO_SPACE;
    msg_hdr->src_node_name = "dm_test_client";

    vol_msg->type = FDS_ProtocolInterface::FDSP_NOTIFY_ADD_VOL;

    vol_msg->vol_desc->rel_prio = 2;
    vol_msg->vol_desc->capacity = 0x1 << 30; // 1 Gig
    vol_msg->vol_desc->volType = FDS_ProtocolInterface::FDSP_VOL_BLKDEV_TYPE;
    vol_msg->vol_desc->defConsisProtocol = FDS_ProtocolInterface::FDSP_CONS_PROTO_STRONG;
    vol_msg->vol_desc->appWorkload = FDS_ProtocolInterface::FDSP_APP_WKLD_FILESYS;

    for (fds_uint32_t i = 0; i < num_vols; i++) {

      msg_hdr->glob_volume_id = i + 1;
      vol_msg->vol_desc->volUUID = i + 1;

      vol_msg->vol_name = "Vol_" + std::to_string(msg_hdr->glob_volume_id);
      vol_msg->vol_desc->vol_name = vol_msg->vol_name;

      fdspCPAPI->NotifyAddVol(msg_hdr, vol_msg);
    }

    /*
     * Send lots of create vol requests.
     */
    vol_msg->type = FDS_ProtocolInterface::FDSP_NOTIFY_RM_VOL;
    for (fds_uint32_t i = 0; i < num_vols; i++) {
      msg_hdr->glob_volume_id = i + 1;
      vol_msg->vol_desc->volUUID = i + 1;
      vol_msg->vol_name = "Vol_" + std::to_string(msg_hdr->glob_volume_id);
      vol_msg->vol_desc->vol_name = vol_msg->vol_name;

      fdspCPAPI->NotifyRmVol(msg_hdr, vol_msg);
    }

    FDS_PLOG(test_log) << "Ending test: basic_multivol()";

    return 0;
  }

 public:
  /*
   * The non-const refernce is OK.
   */
  explicit DmUnitTest(FDS_ProtocolInterface::FDSP_DataPathReqPrx&
                      fdspDPAPI_arg,
                      FDS_ProtocolInterface::FDSP_ControlPathReqPrx&
                      fdspCPAPI_arg) // NOLINT(*)
      : fdspDPAPI(fdspDPAPI_arg),
        fdspCPAPI(fdspCPAPI_arg) {
    test_log = new fds_log("dm_test", "logs");

    unit_tests.push_back("basic_update");
    unit_tests.push_back("basic_uq");
    unit_tests.push_back("basic_query");
    unit_tests.push_back("basic_multivol");

    num_updates = 100;
    max_outstanding_ios = 100;
    test_mode_perf = false;

  }

  explicit DmUnitTest(FDS_ProtocolInterface::FDSP_DataPathReqPrx&
                      fdspDPAPI_arg,
                      FDS_ProtocolInterface::FDSP_ControlPathReqPrx&
                      fdspCPAPI_arg,
                      fds_uint32_t num_up_arg,
		      bool testperf,
		      fds_uint32_t max_oios) // NOLINT(*)
      : fdspDPAPI(fdspDPAPI_arg),
        fdspCPAPI(fdspCPAPI_arg) {
    test_log = new fds_log("dm_test", "logs");

    unit_tests.push_back("basic_update");
    unit_tests.push_back("basic_uq");
    unit_tests.push_back("basic_query");
    unit_tests.push_back("basic_multivol");
    // TODO: Add back...needs some cleanup
    // unit_tests.push_back("basic_bloblist");

    num_updates = num_up_arg;
    max_outstanding_ios = max_oios;;
    test_mode_perf = testperf;
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

    /*
     * Wait before running test for server to be ready.
     * TODO: Remove this and make sure the test isn't
     * kicked off until the server is ready and have server
     * properly wait until it's up.
     */
    sleep(5);
    if (testname == "basic_update") {
      result = basic_update();
    } else if (testname == "basic_uq") {
      result = basic_uq();
    } else if (testname == "basic_query") {
      result = basic_query();
    } else if (testname == "basic_multivol") {
      result = basic_multivol();
    } else if (testname == "basic_bloblist") {
      result = basic_bloblist();
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
      fds_uint32_t n_ios_outstanding;
      n_ios_outstanding = atomic_fetch_sub(&num_ios_outstanding, (unsigned int)1);
      FDS_PLOG(test_log) << "Received DM catalog trans open response ("
			 << n_ios_outstanding << "): ";

      if (iops_stats) {
	boost::posix_time::ptime comp_time = boost::posix_time::microsec_clock::universal_time();
	iops_stats->handleIOCompletion(1, comp_time);
      }

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
      FDS_ProtocolInterface::FDSP_BlobObjectInfo& cat_obj_info = cat_obj_req->obj_list[0];
      ObjectID oid(cat_obj_info.data_obj_id.hash_high,
                   cat_obj_info.data_obj_id.hash_low);
      FDS_PLOG(test_log) << "Received query response success with object "
                         << oid << " for volume offset "
                         << cat_obj_req->blob_name;
      if ((cat_obj_info.data_obj_id.hash_high *
           cat_obj_info.data_obj_id.hash_high !=
           cat_obj_info.data_obj_id.hash_low)
          || (strtoull(cat_obj_req->blob_name.c_str(), NULL, 0) !=
              cat_obj_info.data_obj_id.hash_high)) {
        FDS_PLOG(test_log) << "****** Received object ID seems to be incorrect";
        assert(0);
      }
    } else {
      FDS_PLOG(test_log) << "Received query response failure for offset "
                         << cat_obj_req->blob_name;
      /*
       * TODO: Just panic for now. Eventually we want to tie
       * the response back to the request in the unit test
       * function and have the unit test return error.
       */
      assert(0);
    }
  }

  void DeleteCatalogObjectResp(const
                              FDS_ProtocolInterface::FDSP_MsgHdrTypePtr&
                              fdsp_msg,
                               const
                              FDS_ProtocolInterface::FDSP_DeleteCatalogTypePtr&
                              del_cat_obj,
                              const Ice::Current &) {
  }

  void DeleteObjectResp(const
                              FDS_ProtocolInterface::FDSP_MsgHdrTypePtr&
                              fdsp_msg,
                               const
                              FDS_ProtocolInterface::FDSP_DeleteObjTypePtr&
                              del_cat_obj,
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
  void GetVolumeBlobListResp(const FDSP_MsgHdrTypePtr& fds_msg, 
			     const FDSP_GetVolumeBlobListRespTypePtr& blob_list_rsp, 
			     const Ice::Current &){
    FDS_PLOG_SEV(test_log, fds::fds_log::normal) << "Received listblob response";
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
    fds_uint32_t port_num    = 0;
    fds_uint32_t cp_port_num = 0;
    std::string ip_addr_str;
    bool testperf = false;
    fds_uint32_t max_oios = 0;

    for (int i = 1; i < argc; i++) {
      if (strncmp(argv[i], "--testname=", 11) == 0) {
        testname = argv[i] + 11;
      } else if (strncmp(argv[i], "--num_updates=", 14) == 0) {
        num_updates = atoi(argv[i] + 14);
      } else if (strncmp(argv[i], "--port=", 7) == 0) {
        port_num = strtoul(argv[i] + 7, NULL, 0);
      } else if (strncmp(argv[i], "--cp_port=", 10) == 0) {
        cp_port_num = strtoul(argv[i] + 10, NULL, 0);
      } else if (strncmp(argv[i], "--conc=", 7) == 0) {
	num_conc_reqs = strtoul(argv[i] + 7, NULL, 0);
      } else if (strncmp(argv[i], "--testperf", 10) == 0) {
	testperf = true;
      } else if (strncmp(argv[i], "--max_out=", 10) == 0) {
	max_oios = strtoul(argv[i] + 10, NULL, 0);
      } else {
        std::cout << "Invalid argument " << argv[i] << std::endl;
        return -1;
      }
    }

    /*
     * Setup the network communication. Create a direct data path
     * connection to a single DM.
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
    fdspDPAPI->AssociateRespCallback(ident, "dm_test_client");

    /*
     * Determine control path port number.
     */
    std::string cp_port_str;
    if (cp_port_num == 0) {
      cp_port_str = props->getProperty("DataMgr.ControlPort");
      cp_port_num = strtoul(cp_port_str.c_str(), NULL, 0);
    }
    std::ostringstream omProxyStr;
    ip_addr_str = props->getProperty("DataMgr.IPAddress");
    omProxyStr << "OrchMgrClient: tcp -h " << ip_addr_str
               << " -p  " << cp_port_num;
    op = communicator()->stringToProxy(omProxyStr.str());
    FDS_ProtocolInterface::FDSP_ControlPathReqPrx fdspCPAPI =
        FDS_ProtocolInterface::FDSP_ControlPathReqPrx::checkedCast(op);

    num_ios_outstanding = ATOMIC_VAR_INIT(0);
    if (testperf) {
      std::vector<fds_uint32_t> qids; 
      qids.push_back(1);
      iops_stats = new StatIOPS("dm_unit_test", qids);
    } else {
      iops_stats = NULL;
    }

    DmUnitTest unittest(fdspDPAPI, fdspCPAPI, num_updates, testperf, max_oios);

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
