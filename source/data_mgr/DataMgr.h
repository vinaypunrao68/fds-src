/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

/*
 * Umbrella classes for the data manager component.
 */

#ifndef SOURCE_DATA_MGR_DATAMGR_H_
#define SOURCE_DATA_MGR_DATAMGR_H_

#include <Ice/Ice.h>

#include <unordered_map>
#include <string>

#include <fdsp/FDSP.h>
#include <fds_types.h>
#include <fds_err.h>
#include <fds_volume.h>

/* TODO: avoid cross module include, move API header file to include dir. */
#include <lib/OMgrClient.h>

#include <util/Log.h>
#include <VolumeMeta.h>
#include <concurrency/ThreadPool.h>
#include <concurrency/Mutex.h>

namespace fds {

#define DM_TP_THREADS 3

  class DataMgr : virtual public Ice::Application {
 private:
    typedef FDS_ProtocolInterface::FDSP_DataPathReqPtr  ReqHandlerPtr;
    typedef FDS_ProtocolInterface::FDSP_DataPathRespPrx RespHandlerPrx;

    /*
     * Ice handlers and comm endpoints.
     */
    ReqHandlerPtr  reqHandleSrv;
    RespHandlerPrx respHandleCli;
    OMgrClient     *omClient;

    fds_log *dm_log;

    /*
     * Cmdline configurables
     */
    fds_uint32_t port_num;    /* Data path port num */
    fds_uint32_t cp_port_num; /* Control path port num */
    std::string  stor_prefix; /* String prefix to make file unique */
    fds_bool_t   use_om;      /* Whether to bootstrap from OM */
    std::string  omIpStr;     /* IP addr of the OM */

    /*
     * Internal threadpool.
     */
    fds_uint32_t num_threads;
    fds_threadpool *_tp;

    /*
     * TODO: Move to STD shared or unique pointers. That's
     * safer.
     */
    std::unordered_map<fds_uint64_t, VolumeMeta*> vol_meta_map;
    /*
     * Used to protect access to vol_meta_map.
     */
    fds_mutex *vol_map_mtx;

    Error _add_if_no_vol(const std::string& vol_name,
                         fds_volid_t vol_uuid);
    Error _add_vol_locked(const std::string& vol_name,
                          fds_volid_t vol_uuid);
    Error _process_add_vol(const std::string& vol_name,
                           fds_volid_t vol_uuid);
    Error _process_rm_vol(fds_volid_t vol_uuid);

    Error _process_open(fds_volid_t vol_uuid,
                        fds_uint32_t vol_offset,
                        fds_uint32_t trans_id,
                        const ObjectID& oid);
    Error _process_commit(fds_volid_t vol_uuid,
                          fds_uint32_t vol_offset,
                          fds_uint32_t trans_id,
                          const ObjectID& oid);
    Error _process_abort();

    Error _process_query(fds_volid_t vol_uuid,
                         fds_uint32_t vol_offset,
                         ObjectID *oid);

    fds_bool_t volExistsLocked(fds_volid_t vol_uuid) const;

    static void vol_handler(fds_volid_t vol_uuid,
                            VolumeDesc* desc,
                            fds_vol_notify_t vol_action);

    static void node_handler(fds_int32_t  node_id,
                             fds_uint32_t node_ip,
                             fds_int32_t  node_st,
                             fds_uint32_t node_port,
                             FDS_ProtocolInterface::FDSP_MgrIdType node_type);

 public:
    DataMgr();
    ~DataMgr();

    virtual int run(int argc, char* argv[]);
    void interruptCallback(int arg);
    void swapMgrId(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg);
    fds_log* GetLog();
    std::string getPrefix() const;
    fds_bool_t volExists(fds_volid_t vol_uuid) const;

    /*
     * Nested class that manages the server interface.
     */
    class ReqHandler : public FDS_ProtocolInterface::FDSP_DataPathReq {
   private:
      Ice::CommunicatorPtr _communicator;

   public:
      explicit ReqHandler(const Ice::CommunicatorPtr& communicator);
      ~ReqHandler();

      void PutObject(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &msg_hdr,
                     const FDS_ProtocolInterface::FDSP_PutObjTypePtr &put_obj,
                     const Ice::Current&);

      void GetObject(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &msg_hdr,
                     const FDS_ProtocolInterface::FDSP_GetObjTypePtr& get_obj,
                     const Ice::Current&);

      void UpdateCatalogObject(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr
                               &msg_hdr,
                               const FDS_ProtocolInterface::
                               FDSP_UpdateCatalogTypePtr& update_catalog,
                               const Ice::Current&);

      void QueryCatalogObject(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr
                              &msg_hdr,
                              const FDS_ProtocolInterface::
                              FDSP_QueryCatalogTypePtr& query_catalog,
                              const Ice::Current&);

      void OffsetWriteObject(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr&
                             msg_hdr,
                             const FDS_ProtocolInterface::
                             FDSP_OffsetWriteObjTypePtr& offset_write_obj,
                             const Ice::Current&);

      void RedirReadObject(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr
                           &msg_hdr,
                           const FDS_ProtocolInterface::
                           FDSP_RedirReadObjTypePtr& redir_read_obj,
                           const Ice::Current&);

      void AssociateRespCallback(const Ice::Identity& ident,
                                 const Ice::Current& current);
    };

    /*
     * Nested class that manages the client interface.
     */
    class RespHandler : public FDS_ProtocolInterface::FDSP_DataPathResp {
   public:
      RespHandler();
      ~RespHandler();

      void PutObjectResp(const FDS_ProtocolInterface::
                         FDSP_MsgHdrTypePtr &msg_hdr,
                         const FDS_ProtocolInterface::
                         FDSP_PutObjTypePtr &put_obj,
                         const Ice::Current&);

      void GetObjectResp(const FDS_ProtocolInterface::
                         FDSP_MsgHdrTypePtr &msg_hdr,
                         const FDS_ProtocolInterface::
                         FDSP_GetObjTypePtr& get_obj,
                         const Ice::Current&);
      void UpdateCatalogObjectResp(const FDS_ProtocolInterface::
                                   FDSP_MsgHdrTypePtr &msg_hdr,
                                   const FDS_ProtocolInterface::
                                   FDSP_UpdateCatalogTypePtr& update_catalog,
                                   const Ice::Current&);
      void QueryCatalogObjectResp(const FDS_ProtocolInterface::
                                  FDSP_MsgHdrTypePtr &msg_hdr,
                                  const FDS_ProtocolInterface::
                                  FDSP_QueryCatalogTypePtr& query_catalog,
                                  const Ice::Current&);
      void OffsetWriteObjectResp(const FDS_ProtocolInterface::
                                 FDSP_MsgHdrTypePtr& msg_hdr,
                                 const FDS_ProtocolInterface::
                                 FDSP_OffsetWriteObjTypePtr& offset_write_obj,
                                 const Ice::Current&);
      void RedirReadObjectResp(const FDS_ProtocolInterface::
                               FDSP_MsgHdrTypePtr &msg_hdr,
                               const FDS_ProtocolInterface::
                               FDSP_RedirReadObjTypePtr& redir_read_obj,
                               const Ice::Current&);
    };
  };

}  // namespace fds

#endif  // SOURCE_DATA_MGR_DATAMGR_H_
