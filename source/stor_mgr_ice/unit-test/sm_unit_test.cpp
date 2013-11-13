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
#include <fds-probe/fds_probe.h>

namespace fds {

class SmUnitTest {
 private:
  /*
   * FDS Probe workload gen class.
   */
  class SmUtProbe : public ProbeMod {
   private:
    SmUnitTest *parentUt;
    FDS_ProtocolInterface::FDSP_MsgHdrTypePtr putHdr;

   public:
    explicit SmUtProbe(const std::string &name,
                       probe_mod_param_t &param,
                       Module            *owner,
                       SmUnitTest        *parentUt_arg) :
        ProbeMod(name.c_str(), param, owner),
        parentUt(parentUt_arg) {

      putHdr = new FDS_ProtocolInterface::FDSP_MsgHdrType;
      putHdr->msg_code      = FDS_ProtocolInterface::FDSP_MSG_PUT_OBJ_REQ;    
      putHdr->src_id        = FDS_ProtocolInterface::FDSP_STOR_HVISOR;
      putHdr->dst_id        = FDS_ProtocolInterface::FDSP_STOR_MGR;    
      putHdr->result        = FDS_ProtocolInterface::FDSP_ERR_OK;
      putHdr->err_code      = FDS_ProtocolInterface::FDSP_ERR_SM_NO_SPACE;
      putHdr->src_node_name = "sm_test_client";
      /*
       * TODO: Change this! We should reg a volume.
       */
      putHdr->glob_volume_id = 5;
      putHdr->num_objects    = 1;
    }
    ~SmUtProbe() {
    }

    void pr_intercept_request(ProbeRequest &req) {
    }
    void pr_put(ProbeRequest &req) {
      ProbeIORequest &ioReq = dynamic_cast<ProbeIORequest&>(req);

      FDS_ProtocolInterface::FDSP_PutObjTypePtr putReq =
          new FDS_ProtocolInterface::FDSP_PutObjType;

      putReq->volume_offset         = ioReq.pr_voff;
      putReq->data_obj_len          = ioReq.pr_wr_size;
      putReq->data_obj              = std::string(ioReq.pr_wr_buf, ioReq.pr_wr_size);
      MurmurHash3_x64_128(putReq->data_obj.c_str(),
                          putReq->data_obj_len,
                          0,
                          &ioReq.pr_oid);
      putReq->data_obj_id.hash_high = ioReq.pr_oid.GetHigh();
      putReq->data_obj_id.hash_low  = ioReq.pr_oid.GetLow();
      parentUt->fdspDPAPI->begin_PutObject(putHdr, putReq);
      FDS_PLOG(parentUt->test_log) << "Sent put obj message to SM"
                                   << " for volume offset " << putReq->volume_offset
                                   << " with object ID " << ioReq.pr_oid << " and data size "
                                   << putReq->data_obj_len;
      req.req_complete();
    }
    void pr_get(ProbeRequest &req) {
      std::cout << "Got a get print" << std::endl;
      req.req_complete();
    }
    void pr_delete(ProbeRequest &req) {
      req.req_complete();
    }
    void pr_verify_request(ProbeRequest &req) {
    }
    void pr_gen_report(std::string &out) {
    }
    void mod_startup() {
    }
    void mod_shutdown() {
    }
  };

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
    msg_hdr->src_node_name = "sm_test_client";
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
    msg_hdr->src_node_name = "sm_test_client";
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
    msg_hdr->src_node_name = "sm_test_client";
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
    msg_hdr->src_node_name = "sm_test_client";
    
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

  fds_uint32_t basic_migration() {
    FDS_PLOG(test_log) << "Starting test: basic_migration()";

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
    msg_hdr->src_node_name = "sm_test_client";

    /* storMgr creates 10 volumes, and volume ids = 2, 5, 8 are hybrid 
     * so lets use those for this test; lower volume ids have higher prio  */
    int num_vols = 3;
    int vols[num_vols];
    int volid = 2;
    for (int v = 0; v < num_vols; ++v) {
      vols[v] = volid;
      volid += 3;
    } 

    /* step 1 --  populate all volumes with 'num_updates' objects */
    fds_uint32_t volume_offset;
    ObjectID oid;
    int id = 0;
    for (int v = 0; v < num_vols; ++v) {
      msg_hdr->glob_volume_id = vols[v];
      
      for (fds_uint32_t i = 1; i < num_updates + 1; i++) {
	volume_offset = i;
	id = i+v*(num_updates+1);
	oid = ObjectID(id, id * id);
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
    }

    /* wait a bit so that we finish most of the writes, and fill in the flash
     * so that when we do lots of reads, it will be something to kick out from
     * flash */
    sleep(30);

    /* step 2 -- do lots of reads for volume 2 so we should see volume 2
     * gets to use the flash and kick out other vols objects -- should first
     * kick out objects of vol 8 (lowest prio) and then objects of vol 5 */
    FDS_ProtocolInterface::FDSP_GetObjTypePtr get_req =
        new FDS_ProtocolInterface::FDSP_GetObjType;
    
    msg_hdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_GET_OBJ_REQ;
    msg_hdr->src_id   = FDS_ProtocolInterface::FDSP_STOR_HVISOR;
    msg_hdr->dst_id   = FDS_ProtocolInterface::FDSP_STOR_MGR;
    msg_hdr->result   = FDS_ProtocolInterface::FDSP_ERR_OK;
    msg_hdr->err_code = FDS_ProtocolInterface::FDSP_ERR_SM_NO_SPACE;
    msg_hdr->src_node_name = "sm_test_client";

    int v = 2;
    msg_hdr->glob_volume_id = vols[v];
    int num_reads = 20;
    for (int k = 0; k < num_reads; ++k)
      {
	for (fds_uint32_t i = 1; i < (num_updates+1); i++) {
	  id = i+v*(num_updates+1);

	  oid = ObjectID(id, id * id);
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
      }

    system("../bin/fdscli --auto-tier-migration on --domain-id 1");
    /*
     * Spin and wait for the gets to complete.
     */
    /*    fds_uint64_t acks = ackedGets.load();
    while (acks != num_updates) {
      FDS_PLOG(test_log) << "Received " << acks
                         << " of " << num_updates
                         << " get response";
      sleep(1);
      acks = ackedGets.load();
    }
    FDS_PLOG(test_log) << "Received all " << acks << " of "
                       << num_updates << " get responses";
    */

    /* step 3 -- we need to wait for at least 30 seconds to let
     * ranking engine to get the hot objects from the stat tracker.
     * Ranking engine processes hot objects from the stat tracker on 
     * repeating timer every 30 seconds (can change in ObjectRankEngine 
     * constructor.  */
    sleep(40);
    system("../bin/fdscli --auto-tier-migration off --domain-id 1");

    /* step 4 -- can either start migrator or can hack ranking engine 
     * to call migrate as soon it promoted hot objs/ demotes existing objs
     * in the rank table. */
    //TODO

    v = 0;
    msg_hdr->glob_volume_id = vols[v];
    num_reads = 1;
    for (int k = 0; k < num_reads; ++k)
      {
	for (fds_uint32_t i = 1; i < (num_updates+1); i++) {
	  id = i+v*(num_updates+1);

	  oid = ObjectID(id, id * id);
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
      }
    /* we should look at perf stat output to actually see if migrations happened */

    FDS_PLOG(test_log) << "Ending test: basic_migration()";

    return 0;
  }

  fds_uint32_t basic_probe() {
    FDS_PLOG(test_log) << "Starting test: basic_probe()";

    probe_mod_param_t probe_param;
    SmUtProbe probe("SM unit test probe", probe_param, NULL, this);

    Module *sm_probe_vec[] = {
      &probe,
      &gl_probeMainLib,
      NULL
    };

    /*
     * Create some default cmdline args to pass in.
     * TODO: Actually connect with fds module to use
     * the command line stuff from there.
     */
    int argc = 2;
    char *binName = new char[13];
    strcpy(binName, "sm_unit_test\0");
    char *argName = new char[3];
    strcpy(argName, "-f\0");
    char *argv[2] = {binName, argName};
    ModuleVector probeVec(argc, argv, sm_probe_vec);

    probeVec.mod_execute();

    gl_probeMainLib.probe_run_main(&probe);

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
    unit_tests.push_back("basic_migration");

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

  friend SmUtProbe;

  fds_int32_t Run(const std::string& testname) {
    fds_int32_t result = 0;
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
    } else if (testname == "basic_dedupe") {
      result = basic_dedupe();
    } else if (testname == "basic_migration") {
      result = basic_migration();
    } else if (testname == "basic_probe") {
      result = basic_probe();
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
