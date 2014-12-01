/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <cstdlib>
#include <hash/MurmurHash3.h>

#include <iostream>  // NOLINT(*)
#include <unordered_map>
#include <vector>
#include <string>
#include <list>
#include <atomic>
#include <boost/shared_ptr.hpp>

#include <util/Log.h>
#include <fds_assert.h>
#include <fds_process.h>
#include <fdsp/FDSP_types.h>
#include <fdsp/FDSP_DataPathReq.h>
#include <fdsp/FDSP_DataPathResp.h>
#include <concurrency/Mutex.h>
#include <fds-probe/fds_probe.h>
#include <NetSession.h>
#include <ObjStats.h>
#include <ObjectId.h>

namespace fds {
typedef boost::shared_ptr<FDS_ProtocolInterface::FDSP_DataPathReqClient>
    FDSP_DataPathReqClientPtr;

#ifdef SM_PROBE_TEST
/*
 * FDS Probe workload gen class.
 */
class SmUtProbe : public ProbeMod {
 private:
  SmUnitTest *parentUt;
  fds_mutex  *offMapMtx;
  std::unordered_map<fds_uint64_t, ObjectID> offsetMap;

 public:
  explicit SmUtProbe(const std::string &name,
                     probe_mod_param_t &param,
                     Module            *owner,
                     SmUnitTest        *parentUt_arg) :
      ProbeMod(name.c_str(), &param, owner),
      parentUt(parentUt_arg) {
          offMapMtx = new fds_mutex("offset map mutex");
      }
  ~SmUtProbe() {
      delete offMapMtx;
  }

  void pr_intercept_request(ProbeRequest &req) {
  }
  void pr_put(ProbeRequest &req) {
      ProbeIORequest &ioReq = dynamic_cast<ProbeIORequest&>(req);

      FDS_ProtocolInterface::FDSP_MsgHdrTypePtr putHdr(
          new FDS_ProtocolInterface::FDSP_MsgHdrType);
      putHdr->msg_code       = FDS_ProtocolInterface::FDSP_MSG_PUT_OBJ_REQ;
      putHdr->src_id         = FDS_ProtocolInterface::FDSP_STOR_HVISOR;
      putHdr->dst_id         = FDS_ProtocolInterface::FDSP_STOR_MGR;
      putHdr->result         = FDS_ProtocolInterface::FDSP_ERR_OK;
      putHdr->err_code       = ERR_OK;
      putHdr->src_node_name  = "sm_test_client";
      putHdr->glob_volume_id = 5;
      putHdr->num_objects    = 1;

      FDS_ProtocolInterface::FDSP_PutObjTypePtr putReq(
          new FDS_ProtocolInterface::FDSP_PutObjType);
      putReq->volume_offset         = ioReq.pr_voff;
      putReq->data_obj_len          = ioReq.pr_wr_size;
      putReq->data_obj              = std::string(ioReq.pr_wr_buf, ioReq.pr_wr_size);
      MurmurHash3_x64_128(putReq->data_obj.c_str(),
                          putReq->data_obj_len,
                          0,
                          &ioReq.pr_oid);
      putReq->data_obj_id.digest =
              std::string((const char *)ioReq.pr_oid.GetId(), (size_t)ioReq.pr_oid.GetLen());
//      putReq->data_obj_id.hash_high = ioReq.pr_oid.GetHigh();
//      putReq->data_obj_id.hash_low  = ioReq.pr_oid.GetLow();

      putHdr->req_cookie = parentUt->addPending(ioReq);

      parentUt->fdspDPAPI->PutObject(putHdr, putReq);
      FDS_PLOG(g_fdslog) << "Sent put obj message to SM"
          << " for volume offset " << putReq->volume_offset
          << " with object ID " << ioReq.pr_oid << " and data size "
          << putReq->data_obj_len;

      /*
       * Cache the object ID and data so we can read and verify later.
       */
      offMapMtx->lock();
      offsetMap[ioReq.pr_voff] = ioReq.pr_oid;
      offMapMtx->unlock();
      parentUt->updatePutObj(ioReq.pr_oid, putReq->data_obj);
  }
  void pr_get(ProbeRequest &req) {
      ProbeIORequest &ioReq = dynamic_cast<ProbeIORequest&>(req);

      FDS_ProtocolInterface::FDSP_MsgHdrTypePtr getHdr(
          new FDS_ProtocolInterface::FDSP_MsgHdrType);
      getHdr->msg_code       = FDS_ProtocolInterface::FDSP_MSG_GET_OBJ_REQ;
      getHdr->src_id         = FDS_ProtocolInterface::FDSP_STOR_HVISOR;
      getHdr->dst_id         = FDS_ProtocolInterface::FDSP_STOR_MGR;
      getHdr->result         = FDS_ProtocolInterface::FDSP_ERR_OK;
      getHdr->err_code       = ERR_OK;
      getHdr->src_node_name  = "sm_test_client";
      getHdr->glob_volume_id = 5;
      getHdr->num_objects    = 1;

      FDS_ProtocolInterface::FDSP_GetObjTypePtr getReq(
          new FDS_ProtocolInterface::FDSP_GetObjType);
      if (offsetMap.count(ioReq.pr_voff) == 0) {
          std::cout << "Recieved read for unknown offset " << ioReq.pr_voff
              << " so just ending the read here!"<< std::endl;
          ioReq.req_complete();
          return;
      }
      offMapMtx->lock();
      ioReq.pr_oid = offsetMap[ioReq.pr_voff];
      offMapMtx->unlock();
      getReq->data_obj_id.digest =
              std::string((const char *)ioReq.pr_oid.GetId(), (size_t)ioReq.pr_oid.GetLen());
//      getReq->data_obj_id.hash_high = ioReq.pr_oid.GetHigh();
//      getReq->data_obj_id.hash_low = ioReq.pr_oid.GetLow();

      size_t reqSize;
      const char* buf_ptr = ioReq.pr_rd_buf(&reqSize);
      getReq->data_obj_len = reqSize;

      getHdr->req_cookie = parentUt->addPending(ioReq);

      parentUt->fdspDPAPI->GetObject(getHdr, getReq);
      FDS_PLOG(g_fdslog) << "Sent get obj message to SM"
          << " for object ID " << ioReq.pr_oid << " and data size "
          << getReq->data_obj_len;
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
#endif


class SmUnitTest {
 private:
  /*
   * Response communuication class.
   */
  class TestResp : public FDS_ProtocolInterface::FDSP_DataPathRespIf {
   private:
    SmUnitTest *parentUt;

   public:
    explicit TestResp(SmUnitTest* parentUt_arg) :
        parentUt(parentUt_arg) {
        }
    ~TestResp() {
    }

    void GetObjectResp(const FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
                       const FDS_ProtocolInterface::FDSP_GetObjType& get_obj_req) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }

    void PutObjectResp(const FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
                       const FDS_ProtocolInterface::FDSP_PutObjType& put_obj_req) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }

    void DeleteObjectResp(const FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
                          const FDS_ProtocolInterface::FDSP_DeleteObjType& del_obj_req) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }


    void OffsetWriteObjectResp(const FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
                               const FDS_ProtocolInterface::FDSP_OffsetWriteObjType& offset_write_obj_req) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }


    void RedirReadObjectResp(const FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
                             const FDS_ProtocolInterface::FDSP_RedirReadObjType& redir_write_obj_req) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }

    void GetObjectResp(FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
                       FDS_ProtocolInterface::FDSP_GetObjTypePtr& get_req) {
        /*
         * TODO: May want to sanity check the other response fields.
         */
        fds_verify(msg_hdr->result == FDS_ProtocolInterface::FDSP_ERR_OK);

        ObjectID oid(get_req->data_obj_id.digest);
        std::string objData = get_req->data_obj;
        fds_verify(parentUt->checkGetObj(oid, objData) == true);

        ProbeIORequest *ioReq = parentUt->getAndRmPending(msg_hdr->req_cookie);
        if (ioReq != NULL) {
            ioReq->req_complete();
        }
    }

    void PutObjectResp(FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
                       FDS_ProtocolInterface::FDSP_PutObjTypePtr& put_req) {
        /*
         * TODO: May want to sanity check the other response fields.
         */
        fds_verify(msg_hdr->result == FDS_ProtocolInterface::FDSP_ERR_OK);
        parentUt->updateAckedPuts();
        ProbeIORequest *ioReq = parentUt->getAndRmPending(msg_hdr->req_cookie);
        if (ioReq != NULL) {
            ioReq->req_complete();
        }
    }
    void DeleteObjectResp(FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
                          FDS_ProtocolInterface::FDSP_DeleteObjTypePtr& del_req) {
    }

    void OffsetWriteObjectResp(FDS_ProtocolInterface::FDSP_MsgHdrTypePtr&
                               fdsp_msg,
                               FDS_ProtocolInterface::FDSP_OffsetWriteObjTypePtr&
                               offset_write_obj_req) {
    }
    void RedirReadObjectResp(FDS_ProtocolInterface::FDSP_MsgHdrTypePtr&
                             fdsp_msg,
                             FDS_ProtocolInterface::FDSP_RedirReadObjTypePtr&
                             redir_write_obj_req) {
    }
    void GetObjectMetadataResp(
            boost::shared_ptr<FDSP_GetObjMetadataResp>& metadata_resp) {
    }
    void GetObjectMetadataResp(const FDSP_GetObjMetadataResp& metadata_resp) {
    }
  };

 private:
  std::list<std::string>  unit_tests;
  boost::shared_ptr<netSessionTbl> nst_;
  FDSP_DataPathReqClientPtr fdspDPAPI;
  boost::shared_ptr<TestResp> fdspDataPathResp;
  netDataPathClientSession* dp_session_;
  std::string node_name_;
  std::string session_uuid_;
  FdsConfigAccessor conf_helper_;

  /*
   * Members used for probe request tracking
   */
  fds_mutex   *reqMutex;
  fds_uint64_t nextReqId;
  std::map<fds_uint64_t, ProbeIORequest*> pendingIoMap;
  fds_uint64_t addPending(ProbeIORequest &ioReq) {
      reqMutex->lock();
      fds_uint64_t reqId = nextReqId;
      nextReqId++;
      pendingIoMap[reqId] = &ioReq;
      reqMutex->unlock();
      return reqId;
  }
  ProbeIORequest* getAndRmPending(fds_uint64_t reqId) {
      ProbeIORequest *ioReq = NULL;
      reqMutex->lock();
      if (pendingIoMap.count(reqId) > 0) {
          ioReq = pendingIoMap[reqId];
          pendingIoMap.erase(reqId);
      }
      reqMutex->unlock();
      return ioReq;
  }

  fds_uint32_t num_updates;

  std::unordered_map<ObjectID, std::string, ObjectHash> added_objs;
  fds_mutex *objMapLock;
  std::atomic<fds_uint64_t>  ackedGets;
  std::atomic<fds_uint64_t>  ackedPuts;

  /*
   * Helper functions.
   */
  fds_bool_t checkGetObj(const ObjectID& oid,
                         const std::string& objData) {
      ackedGets++;
      objMapLock->lock();
      if (objData != added_objs[oid]) {
          FDS_PLOG(g_fdslog) << "Failed get correct object! For object "
              << oid << " Got ["
              << objData << "] but expected ["
              << added_objs[oid] << "]";
          objMapLock->unlock();
          return false;
      }
      FDS_PLOG(g_fdslog) << "Get check for object " << oid
          << ": SUCCESS";
      objMapLock->unlock();
      return true;
  }

  void updateAckedPuts() {
      ackedPuts++;
      FDS_PLOG(g_fdslog) << "Receiver PutObj response for object cnt: " << ackedPuts;
  }

  void updatePutObj(const ObjectID& oid,
                    const std::string& objData) {
      objMapLock->lock();
      /*
       * Note that this overwrites whatever was previously
       * cached at that oid.
       */
      added_objs[oid] = objData;
      FDS_PLOG(g_fdslog) << "Put object " << oid << " into map ";
      objMapLock->unlock();
  }

  /*
   * Unit test funtions
   */
  fds_int32_t basic_update() {

      FDS_PLOG(g_fdslog) << "Starting test: basic_update()";

      /*
       * Send lots of put requests.
       */
      FDS_ProtocolInterface::FDSP_MsgHdrTypePtr msg_hdr(
          new FDS_ProtocolInterface::FDSP_MsgHdrType);
      FDS_ProtocolInterface::FDSP_PutObjTypePtr put_req(
          new FDS_ProtocolInterface::FDSP_PutObjType);

      msg_hdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_PUT_OBJ_REQ;
      msg_hdr->src_id   = FDS_ProtocolInterface::FDSP_STOR_HVISOR;
      msg_hdr->dst_id   = FDS_ProtocolInterface::FDSP_STOR_MGR;
      msg_hdr->result   = FDS_ProtocolInterface::FDSP_ERR_OK;
      msg_hdr->err_code = ERR_OK;
      msg_hdr->src_node_name = node_name_;
      msg_hdr->session_uuid = session_uuid_;
      /*
       * TODO: Change this! We should reg a volume.
       */
      msg_hdr->glob_volume_id = 5;

      fds_uint32_t volume_offset;
      ObjectID oid;
      for (fds_uint32_t i = 1; i < num_updates + 1; i++) {
          volume_offset = i;
//          oid = ObjectID(i, i * i);
          oid = ObjectID(0x5a5a);
          std::ostringstream convert;
          convert << i;
          std::string object_data = "I am object number " + convert.str();

          put_req->volume_offset         = volume_offset;
          put_req->data_obj_id.digest =
              std::string((const char *)oid.GetId(), (size_t)oid.GetLen());
//          put_req->data_obj_id.hash_high = oid.GetHigh();
//          put_req->data_obj_id.hash_low  = oid.GetLow();
          put_req->data_obj_len          = object_data.size();
          put_req->data_obj              = object_data;
          msg_hdr->num_objects           = 1;

          try {
              updatePutObj(oid, object_data);
              fdspDPAPI->PutObject(msg_hdr, put_req);
              FDS_PLOG(g_fdslog) << "Sent put obj message to SM"
                  << " for volume offset " << put_req->volume_offset
                  << " with object ID " << oid << " and data "
                  << object_data;
          } catch(...) {
              FDS_PLOG(g_fdslog) << "Failed to put obj message to SM"
                  << " for volume offset" << put_req->volume_offset
                  << " with object ID " << oid << " and data "
                  << object_data;
              return -1;
          }
      }

      /* Wait for puts to finish */
      while (ackedPuts.load() != num_updates) {sleep(1);}

      return 0;
  }

  fds_int32_t basic_uq() {

      FDS_PLOG(g_fdslog) << "Starting test: basic_uq()";

      ackedPuts = 0;
      ackedGets = 0;

      FDS_ProtocolInterface::FDSP_MsgHdrTypePtr msg_hdr(
          new FDS_ProtocolInterface::FDSP_MsgHdrType);
      FDS_ProtocolInterface::FDSP_PutObjTypePtr put_req(
          new FDS_ProtocolInterface::FDSP_PutObjType);

      msg_hdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_PUT_OBJ_REQ;
      msg_hdr->src_id   = FDS_ProtocolInterface::FDSP_STOR_HVISOR;
      msg_hdr->dst_id   = FDS_ProtocolInterface::FDSP_STOR_MGR;
      msg_hdr->result   = FDS_ProtocolInterface::FDSP_ERR_OK;
      msg_hdr->err_code = ERR_OK;
      msg_hdr->src_node_name = node_name_;
      msg_hdr->session_uuid = session_uuid_;
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
         /*
          * Hash the data to obtain the ID
          */
          ObjectID oid = ObjIdGen::genObjectId(object_data.c_str(),
                                          object_data.size() + 1);

          put_req->volume_offset         = volume_offset;
          put_req->data_obj_id.digest =
              std::string((const char *)oid.GetId(), (size_t)oid.GetLen());
//          put_req->data_obj_id.hash_high = oid.GetHigh();
//          put_req->data_obj_id.hash_low  = oid.GetLow();
          put_req->data_obj_len          = object_data.size();
          put_req->data_obj              = object_data;
          msg_hdr->num_objects           = 1;

          try {
              updatePutObj(oid, object_data);
              fdspDPAPI->PutObject(msg_hdr, put_req);
              FDS_PLOG(g_fdslog) << "Sent put obj message to SM"
                  << " for volume offset " << put_req->volume_offset
                  << " with object ID " << oid << " and data "
                  << object_data;
          } catch(...) {
              FDS_PLOG(g_fdslog) << "Failed to put obj message to SM"
                  << " for volume offset" << put_req->volume_offset
                  << " with object ID " << oid << " and data "
                  << object_data;
              return -1;
          }
          objIdsPut.push_back(oid);
      }

      /* Wait for puts to finish */
      while (ackedPuts.load() != num_updates) {sleep(1);}
      std::cout << "Received responses for all " << num_updates
          << " Puts" << std::endl;

      ackedGets = 0;
      FDS_ProtocolInterface::FDSP_GetObjTypePtr get_req(
          new FDS_ProtocolInterface::FDSP_GetObjType);

      msg_hdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_GET_OBJ_REQ;
      msg_hdr->src_id   = FDS_ProtocolInterface::FDSP_STOR_HVISOR;
      msg_hdr->dst_id   = FDS_ProtocolInterface::FDSP_STOR_MGR;
      msg_hdr->result   = FDS_ProtocolInterface::FDSP_ERR_OK;
      msg_hdr->err_code = ERR_OK;
      /*
       * TODO: Change this! We should reg a volume.
       */
      msg_hdr->glob_volume_id = 5;

      ObjectID oid;
      for (fds_uint32_t i = 0; i < objIdsPut.size(); i++) {
          oid = objIdsPut.front();
          get_req->data_obj_id.digest =
              std::string((const char *)oid.GetId(), (size_t)oid.GetLen());
//          get_req->data_obj_id.hash_high = oid.GetHigh();
//          get_req->data_obj_id.hash_low  = oid.GetLow();
          get_req->data_obj_len          = 0;

          try {
              fdspDPAPI->GetObject(msg_hdr, get_req);
              FDS_PLOG(g_fdslog) << "Sent get obj message to SM"
                  << " with object ID " << oid;
          } catch(...) {
              FDS_PLOG(g_fdslog) << "Failed get obj message to SM"
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
          FDS_PLOG(g_fdslog) << "Received " << acks
              << " of " << objIdsPut.size()
              << " get response";
          sleep(1);
          acks = ackedGets.load();
      }
      FDS_PLOG(g_fdslog) << "Received all " << acks << " of "
          << objIdsPut.size() << " get responses";

      FDS_PLOG(g_fdslog) << "Ending test: basic_uq()";

      return 0;
  }

  fds_int32_t basic_dedupe() {

      FDS_PLOG(g_fdslog) << "Starting test: basic_dedupe()";

      FDS_ProtocolInterface::FDSP_MsgHdrTypePtr msg_hdr(
          new FDS_ProtocolInterface::FDSP_MsgHdrType);
      FDS_ProtocolInterface::FDSP_PutObjTypePtr put_req(
          new FDS_ProtocolInterface::FDSP_PutObjType);

      msg_hdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_PUT_OBJ_REQ;
      msg_hdr->src_id   = FDS_ProtocolInterface::FDSP_STOR_HVISOR;
      msg_hdr->dst_id   = FDS_ProtocolInterface::FDSP_STOR_MGR;
      msg_hdr->result   = FDS_ProtocolInterface::FDSP_ERR_OK;
      msg_hdr->err_code = ERR_OK;
      msg_hdr->src_node_name = node_name_;
      msg_hdr->session_uuid = session_uuid_;
      /*
       * TODO: Change this! We should reg a volume.
       */
      msg_hdr->glob_volume_id = 5;

      fds_uint32_t volume_offset;
      ObjectID oid;
      for (fds_uint32_t i = 1; i < num_updates + 1; i++) {
          volume_offset = i;
          oid = ObjectID(0x5a5a);
          std::ostringstream convert;
          convert << i;
          std::string object_data = "I am object number " + convert.str();

          put_req->volume_offset         = volume_offset;
          put_req->data_obj_id.digest =
              std::string((const char *)oid.GetId(), (size_t)oid.GetLen());
//          put_req->data_obj_id.hash_high = oid.GetHigh();
//          put_req->data_obj_id.hash_low  = oid.GetLow();
          put_req->data_obj_len          = object_data.size();
          put_req->data_obj              = object_data;
          msg_hdr->num_objects           = 1;

          try {
              updatePutObj(oid, object_data);
              fdspDPAPI->PutObject(msg_hdr, put_req);
              FDS_PLOG(g_fdslog) << "Sent put obj message to SM"
                  << " for volume offset " << put_req->volume_offset
                  << " with object ID " << oid << " and data "
                  << object_data;
              //Put the object again and expect no errors just logs of a dupe
              fdspDPAPI->PutObject(msg_hdr, put_req);
          } catch(...) {
              FDS_PLOG(g_fdslog) << "Failed to put obj message to SM"
                  << " for volume offset" << put_req->volume_offset
                  << " with object ID " << oid << " and data "
                  << object_data;
              return -1;
          }
      }

      ackedGets = 0;
      FDS_ProtocolInterface::FDSP_GetObjTypePtr get_req(
          new FDS_ProtocolInterface::FDSP_GetObjType);

      msg_hdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_GET_OBJ_REQ;
      msg_hdr->src_id   = FDS_ProtocolInterface::FDSP_STOR_HVISOR;
      msg_hdr->dst_id   = FDS_ProtocolInterface::FDSP_STOR_MGR;
      msg_hdr->result   = FDS_ProtocolInterface::FDSP_ERR_OK;
      msg_hdr->err_code = ERR_OK;
      /*
       * TODO: Change this! We should reg a volume.
       */
      msg_hdr->glob_volume_id = 5;

      for (fds_uint32_t i = 1; i < num_updates + 1; i++) {
          oid = ObjectID(0x5a5a);
          get_req->data_obj_id.digest =
              std::string((const char *)oid.GetId(), (size_t)oid.GetLen());
//          get_req->data_obj_id.hash_high = oid.GetHigh();
//          get_req->data_obj_id.hash_low  = oid.GetLow();
          get_req->data_obj_len          = 0;

          try {
              fdspDPAPI->GetObject(msg_hdr, get_req);
              FDS_PLOG(g_fdslog) << "Sent get obj message to SM"
                  << " with object ID " << oid;
          } catch(...) {
              FDS_PLOG(g_fdslog) << "Failed get obj message to SM"
                  << " with object ID " << oid;
              return -1;
          }
      }

      /*
       * Spin and wait for the gets to complete.
       */
      fds_uint64_t acks = ackedGets.load();
      while (acks != num_updates) {
          FDS_PLOG(g_fdslog) << "Received " << acks
              << " of " << num_updates
              << " get response";
          sleep(1);
          acks = ackedGets.load();
      }
      FDS_PLOG(g_fdslog) << "Received all " << acks << " of "
          << num_updates << " get responses";

      FDS_PLOG(g_fdslog) << "Ending test: basic_dedupe()";

      return 0;
  }


  fds_int32_t basic_query() {

      FDS_PLOG(g_fdslog) << "Starting test: basic_query()";

      FDS_ProtocolInterface::FDSP_MsgHdrTypePtr msg_hdr(
          new FDS_ProtocolInterface::FDSP_MsgHdrType);

      ObjectID oid;

      FDS_ProtocolInterface::FDSP_GetObjTypePtr get_req(
          new FDS_ProtocolInterface::FDSP_GetObjType);

      msg_hdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_GET_OBJ_REQ;
      msg_hdr->src_id   = FDS_ProtocolInterface::FDSP_STOR_HVISOR;
      msg_hdr->dst_id   = FDS_ProtocolInterface::FDSP_STOR_MGR;
      msg_hdr->result   = FDS_ProtocolInterface::FDSP_ERR_OK;
      msg_hdr->err_code = ERR_OK;
      msg_hdr->src_node_name = node_name_;
      msg_hdr->session_uuid = session_uuid_;

      for (fds_uint32_t i = 0; i < num_updates; i++) {
          oid = ObjectID(0x5a5a);
          get_req->data_obj_id.digest =
              std::string((const char *)oid.GetId(), (size_t)oid.GetLen());
//          get_req->data_obj_id.hash_high = oid.GetHigh();
//          get_req->data_obj_id.hash_low  = oid.GetLow();
          get_req->data_obj_len          = 0;

          try {
              fdspDPAPI->GetObject(msg_hdr, get_req);
              FDS_PLOG(g_fdslog) << "Sent get obj message to SM"
                  << " with object ID " << oid;
          } catch(...) {
              FDS_PLOG(g_fdslog) << "Failed get obj message to SM"
                  << " with object ID " << oid;
              return -1;
          }
      }

      FDS_PLOG(g_fdslog) << "Ending test: basic_query()";

      return 0;
  }

  fds_uint32_t basic_migration() {
      FDS_PLOG(g_fdslog) << "Starting test: basic_migration()";

      /*
       * Send lots of put requests.
       */
      FDS_ProtocolInterface::FDSP_MsgHdrTypePtr msg_hdr(
          new FDS_ProtocolInterface::FDSP_MsgHdrType);
      FDS_ProtocolInterface::FDSP_PutObjTypePtr put_req(
          new FDS_ProtocolInterface::FDSP_PutObjType);

      msg_hdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_PUT_OBJ_REQ;
      msg_hdr->src_id   = FDS_ProtocolInterface::FDSP_STOR_HVISOR;
      msg_hdr->dst_id   = FDS_ProtocolInterface::FDSP_STOR_MGR;
      msg_hdr->result   = FDS_ProtocolInterface::FDSP_ERR_OK;
      msg_hdr->err_code = ERR_OK;
      msg_hdr->src_node_name = node_name_;
      msg_hdr->session_uuid = session_uuid_;

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
              oid = ObjectID(0x5a5a+id);
              std::ostringstream convert;
              convert << i;
              std::string object_data = "I am object number " + convert.str();

              put_req->volume_offset         = volume_offset;
              put_req->data_obj_id.digest =
                  std::string((const char *)oid.GetId(), (size_t)oid.GetLen());
//              put_req->data_obj_id.hash_high = oid.GetHigh();
//              put_req->data_obj_id.hash_low  = oid.GetLow();
              put_req->data_obj_len          = object_data.size();
              put_req->data_obj              = object_data;
              msg_hdr->num_objects           = 1;

              try {
                  updatePutObj(oid, object_data);
                  fdspDPAPI->PutObject(msg_hdr, put_req);
                  FDS_PLOG(g_fdslog) << "Sent put obj message to SM"
                      << " for volume offset " << put_req->volume_offset
                      << " with object ID " << oid << " and data "
                      << object_data;
              } catch(...) {
                  FDS_PLOG(g_fdslog) << "Failed to put obj message to SM"
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
      ackedGets = 0;
      FDS_ProtocolInterface::FDSP_GetObjTypePtr get_req(
          new FDS_ProtocolInterface::FDSP_GetObjType);

      msg_hdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_GET_OBJ_REQ;
      msg_hdr->src_id   = FDS_ProtocolInterface::FDSP_STOR_HVISOR;
      msg_hdr->dst_id   = FDS_ProtocolInterface::FDSP_STOR_MGR;
      msg_hdr->result   = FDS_ProtocolInterface::FDSP_ERR_OK;
      msg_hdr->err_code = ERR_OK;
      msg_hdr->src_node_name = node_name_;
      msg_hdr->session_uuid = session_uuid_;

      int v = 2;
      msg_hdr->glob_volume_id = vols[v];
      int num_reads = 20;
      for (int k = 0; k < num_reads; ++k) {
          for (fds_uint32_t i = 1; i < (num_updates+1); i++) {
              id = i+v*(num_updates+1);

              oid = ObjectID(0x5a5a+id);
              get_req->data_obj_id.digest =
                  std::string((const char *)oid.GetId(), (size_t)oid.GetLen());
//              get_req->data_obj_id.hash_high = oid.GetHigh();
//              get_req->data_obj_id.hash_low  = oid.GetLow();
              get_req->data_obj_len          = 0;

              try {
                  fdspDPAPI->GetObject(msg_hdr, get_req);
                  FDS_PLOG(g_fdslog) << "Sent get obj message to SM"
                      << " with object ID " << oid;
              } catch(...) {
                  FDS_PLOG(g_fdslog) << "Failed get obj message to SM"
                      << " with object ID " << oid;
                  return -1;
              }
          }
      }

      // system("../bin/fdscli --auto-tier-migration on --domain-id 1");
      /*
       * Spin and wait for the gets to complete.
       */
      fds_uint64_t acks = ackedGets.load();
      while (acks != (num_reads * num_updates)) {
          FDS_PLOG(g_fdslog) << "Received " << acks
              << " of " << num_updates
              << " get response";
          sleep(1);
          acks = ackedGets.load();
      }
      FDS_PLOG(g_fdslog) << "Received all " << acks << " of "
          << (num_reads * num_updates) << " get responses";

      /* step 3 -- we need to wait for at least 30 seconds to let
       * ranking engine to get the hot objects from the stat tracker.
       * Ranking engine processes hot objects from the stat tracker on
       * repeating timer every 30 seconds (can change in ObjectRankEngine
       * constructor.  */
      sleep(40);
      // system("../bin/fdscli --auto-tier-migration off --domain-id 1");

      /* step 4 -- can either start migrator or can hack ranking engine
       * to call migrate as soon it promoted hot objs/ demotes existing objs
       * in the rank table. */
      //TODO

      ackedGets = 0;
      v = 0;
      msg_hdr->glob_volume_id = vols[v];
      num_reads = 1;
      for (int k = 0; k < num_reads; ++k) {
          for (fds_uint32_t i = 1; i < (num_updates+1); i++) {
              id = i+v*(num_updates+1);

              oid = ObjectID(0x5a5a+id);
              get_req->data_obj_id.digest =
                  std::string((const char *)oid.GetId(), (size_t)oid.GetLen());
//              get_req->data_obj_id.hash_high = oid.GetHigh();
//              get_req->data_obj_id.hash_low  = oid.GetLow();
              get_req->data_obj_len          = 0;

              try {
                  fdspDPAPI->GetObject(msg_hdr, get_req);
                  FDS_PLOG(g_fdslog) << "Sent get obj message to SM"
                      << " with object ID " << oid;
              } catch(...) {
                  FDS_PLOG(g_fdslog) << "Failed get obj message to SM"
                      << " with object ID " << oid;
                  return -1;
              }
          }
      }

      acks = ackedGets.load();
      while (acks != (num_reads * num_updates)) {
          FDS_PLOG(g_fdslog) << "Received " << acks
              << " of " << num_updates
              << " get response";
          sleep(1);
          acks = ackedGets.load();
      }
      FDS_PLOG(g_fdslog) << "Received all " << acks << " of "
          << (num_reads * num_updates) << " get responses";

      /* we should look at perf stat output to actually see if migrations happened */

      FDS_PLOG(g_fdslog) << "Ending test: basic_migration()";

      return 0;
  }

#ifdef SM_PROBE_TEST
  fds_uint32_t basic_probe() {
      FDS_PLOG(g_fdslog) << "Starting test: basic_probe()";

      probe_mod_param_t probe_param;
      SmUtProbe probe("SM unit test probe", probe_param, NULL, this);

      Module *sm_probe_vec[] = {
          &probe,
          &gl_probeBlkLib,
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

      /*
       * This will listen forever...
       */
      gl_probeBlkLib.probe_run_main(&probe);

      return 0;
  }
#endif

 public:
  /*
   * The non-const refernce is OK.
   */
  explicit SmUnitTest(const FdsConfigAccessor& conf_helper)
  {
      conf_helper_ = conf_helper;
      objMapLock = new fds_mutex("Added object map lock");

      setup_session_endpoints();

      unit_tests.push_back("basic_update");
      unit_tests.push_back("basic_uq");
      unit_tests.push_back("basic_dedupe");
      unit_tests.push_back("basic_migration");

      num_updates = conf_helper_.get<int>("num_updates");

      nextReqId = 0;
      reqMutex = new fds_mutex("pending io mutex");
  }

  ~SmUnitTest() {
      /*
       * TODO: The g_fdslog can be deleted while
       * Ice messages are still incoming and using
       * the log, which is a problem.
       * Leak the log for now and fix this soon using
       * coordination so that outstanding I/Os are
       * accounted for prior to destruction.
       */
      // delete fdspDataPathResp;
      delete reqMutex;
      delete objMapLock;
  }

  fds_log* GetLogPtr() {
      return g_fdslog;
  }

  void setup_session_endpoints()
  {
      fdspDataPathResp.reset(new TestResp(this));

      nst_ = boost::shared_ptr<netSessionTbl>(new netSessionTbl(FDSP_STOR_HVISOR));

      dp_session_ = nst_->startSession<netDataPathClientSession>(
          conf_helper_.get<std::string>("sm_ip"),
          conf_helper_.get<int>("sm_data_port"),
          FDSP_STOR_MGR,
          1, /* number of channels */
          fdspDataPathResp);
      fdspDPAPI = dp_session_->getClient();  // NOLINT
      session_uuid_ = dp_session_->getSessionId();
      node_name_ = "localhost-sm";
  }

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
#ifdef SM_PROBE_TEST
          result = basic_probe();
#endif
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

  int Run() {
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
      return result;
  }
};

/*
 * Ice request communication class.
 */
class TestClient : public FdsProcess {
 public:
  TestClient(int argc, char *argv[],
             const std::string &default_config_path,
             const std::string &base_path, Module **mod_vec)
      : FdsProcess(argc, argv, default_config_path, base_path, mod_vec)
  {
      result_ = 0;
  }

  ~TestClient() {
  }

  void run() {
      /*
       * Process the cmdline args.
       */
      std::string testname;

      SmUnitTest unittest(conf_helper_);

      testname = conf_helper_.get<std::string>("testname");
      if (testname.empty()) {
          result_ = unittest.Run();
      } else {
          result_ = unittest.Run(testname);
      }
  }

  int get_result() {
      return result_;
  }

 private:
  int result_;
};

}  // namespace fds

int main(int argc, char* argv[]) {
    fds::Module *sm_testVec[] = {
        &gl_objStats,
        NULL
    };
    fds::TestClient sm_client(argc, argv, "sm_ut.conf", "fds.sm_ut.", sm_testVec);

    sm_client.main();
    return sm_client.get_result();
}
