/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

namespace fds
{

#if 0
    // --------------------------------------------------------------------------------------
    // RPC request handlers
    // --------------------------------------------------------------------------------------
    PlatRpcReqt::PlatRpcReqt(const Platform *mgr) : plf_mgr(mgr)
    {
    }
    PlatRpcReqt::~PlatRpcReqt()
    {
    }

    void
    PlatRpcReqt::NotifyAddVol(const FDSP_MsgHdrType    &fdsp_msg,
                              const FDSP_NotifyVolType &not_add_vol_req)
    {
    }

    void
    PlatRpcReqt::NotifyAddVol(fpi::FDSP_MsgHdrTypePtr    &msg_hdr,
                              fpi::FDSP_NotifyVolTypePtr &msg)
    {
        if (plf_mgr->plf_vol_evt != NULL)
        {
            plf_mgr->plf_vol_evt->plat_add_vol(msg_hdr, msg);
        }
    }

    void
    PlatRpcReqt::NotifyRmVol(const FDSP_MsgHdrType    &fdsp_msg,
                             const FDSP_NotifyVolType &not_rm_vol_req)
    {
    }

    void
    PlatRpcReqt::NotifyRmVol(fpi::FDSP_MsgHdrTypePtr    &msg_hdr,
                             fpi::FDSP_NotifyVolTypePtr &msg)
    {
        if (plf_mgr->plf_vol_evt != NULL)
        {
            plf_mgr->plf_vol_evt->plat_rm_vol(msg_hdr, msg);
        }
    }

    void
    PlatRpcReqt::NotifyModVol(const FDSP_MsgHdrType    &fdsp_msg,
                              const FDSP_NotifyVolType &not_mod_vol_req)
    {
    }

    void
    PlatRpcReqt::NotifyModVol(fpi::FDSP_MsgHdrTypePtr    &msg_hdr,
                              fpi::FDSP_NotifyVolTypePtr &msg)
    {
        fds_verify(0);
    }

    void
    PlatRpcReqt::NotifySnapVol(const FDSP_MsgHdrType    &fdsp_msg,
                               const FDSP_NotifyVolType &not_snap_vol_req)
    {
    }

    void
    PlatRpcReqt::NotifySnapVol(fpi::FDSP_MsgHdrTypePtr    &msg_hdr,
                               fpi::FDSP_NotifyVolTypePtr &msg)
    {
        fds_verify(0);
    }

    void
    PlatRpcReqt::NotifyDMTUpdate(const FDSP_MsgHdrType &msg_hdr,
                                 const FDSP_DMT_Type   &dmt_info)
    {
    }

    void
    PlatRpcReqt::NotifyDMTUpdate(fpi::FDSP_MsgHdrTypePtr &msg_hdr,  // NOLINT
                                 fpi::FDSP_DMT_TypePtr   &dmt)
    {
        fds_verify(0);
    }

    void
    PlatRpcReqt::NotifyDMTClose(const FDSP_MsgHdrType &msg_hdr,
                                const FDSP_DmtCloseType &dmt)
    {
    }
    void
    PlatRpcReqt::NotifyDMTClose(fpi::FDSP_MsgHdrTypePtr    &msg_hdr,
                                fpi::FDSP_DmtCloseTypePtr &dmt_close)
    {
        fds_verify(0);
    }

    void
    PlatRpcReqt::PushMetaDMTReq(const FDSP_MsgHdrType &msg_hdr,
                                const FDSP_PushMeta &push_meta_resp)
    {
    }
    void
    PlatRpcReqt::PushMetaDMTReq(fpi::FDSP_MsgHdrTypePtr &msg_hdr,
                                fpi::FDSP_PushMetaPtr &push_meta_resp)
    {
        fds_verify(0);
    }

    void
    PlatRpcReqt::SetThrottleLevel(const FDSP_MsgHdrType      &msg_hdr,
                                  const FDSP_ThrottleMsgType &throttle_msg)
    {
    }

    void
    PlatRpcReqt::SetThrottleLevel(fpi::FDSP_MsgHdrTypePtr      &msg_hdr,
                                  fpi::FDSP_ThrottleMsgTypePtr &throttle_msg)
    {
        fds_verify(0);
    }

    void
    PlatRpcReqt::SetQoSControl(const FDSP_MsgHdrType& fdsp_msg,
                               const FDSP_QoSControlMsgType& qos_msg)
    {
    }

    void
    PlatRpcReqt::SetQoSControl(fpi::FDSP_MsgHdrTypePtr& fdsp_msg,
                               fpi::FDSP_QoSControlMsgTypePtr& qos_msg)
    {
        fds_verify(0);
    }

    void
    PlatRpcReqt::NotifyScavengerCmd(const fpi::FDSP_MsgHdrType    &msg_hdr,
                                    const fpi::FDSP_ScavengerType &gc_info)
    {
    }

    void
    PlatRpcReqt::NotifyScavengerCmd(fpi::FDSP_MsgHdrTypePtr    &hdr,
                                    fpi::FDSP_ScavengerTypePtr &gc_info)
    {
        fds_verify(0);
    }

    // --------------------------------------------------------------------------------------
    // RPC response handlers
    // --------------------------------------------------------------------------------------
    PlatRpcResp::PlatRpcResp(const Platform *mgr) : plf_mgr(mgr)
    {
    }
    PlatRpcResp::~PlatRpcResp()
    {
    }

    void
    PlatRpcResp::RegisterNodeResp(const FDSP_MsgHdrType         &fdsp_msg,
                                  const FDSP_RegisterNodeType   &reg_node_rsp)
    {
    }

    void
    PlatRpcResp::RegisterNodeResp(fpi::FDSP_MsgHdrTypePtr       &fdsp_msg,
                                  fpi::FDSP_RegisterNodeTypePtr &reg_node_rsp)
    {
        fds_verify(0);
    }

    void
    PlatRpcResp::MigrationDoneResp(const FDSP_MsgHdrType            &fdsp_msg,
                                   const FDSP_MigrationStatusType   &status_resp)
    {
    }

    void
    PlatRpcResp::MigrationDoneResp(fpi::FDSP_MsgHdrTypePtr          &fdsp_msg,
                                   fpi::FDSP_MigrationStatusTypePtr &status_resp)
    {
        fds_verify(0);
    }

    // --------------------------------------------------------------------------------------
    // RPC Data Path Response
    // --------------------------------------------------------------------------------------
    PlatDataPathResp::PlatDataPathResp(Platform *mgr) : plf_mgr(mgr)
    {
    }
    PlatDataPathResp::~PlatDataPathResp()
    {
    }

    void
    PlatDataPathResp::GetObjectResp(const FDSP_MsgHdrType& fdsp_msg,
                                    const FDSP_GetObjType& get_obj_req)
    {
    }

    void
    PlatDataPathResp::GetObjectResp(
        boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg,  // NOLINT
        boost::shared_ptr<FDSP_GetObjType>& get_obj_req)
    {
    }

    void
    PlatDataPathResp::PutObjectResp(const FDSP_MsgHdrType& fdsp_msg,
                                    const FDSP_PutObjType& put_obj_req)
    {
    }

    void
    PlatDataPathResp::PutObjectResp(
        boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg,  /// NOLINT
        boost::shared_ptr<FDSP_PutObjType>& put_obj_req)
    {
    }

    void
    PlatDataPathResp::DeleteObjectResp(const FDSP_MsgHdrType& fdsp_msg,
                                       const FDSP_DeleteObjType& del_obj_req)
    {
    }

    void
    PlatDataPathResp::DeleteObjectResp(
        boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg,  // NOLINT
        boost::shared_ptr<FDSP_DeleteObjType>& del_obj_req)
    {
    }

    void
    PlatDataPathResp::OffsetWriteObjectResp(const FDSP_MsgHdrType& fdsp_msg,
                                            const FDSP_OffsetWriteObjType& req)
    {
    }

    void
    PlatDataPathResp::OffsetWriteObjectResp(
        boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg,  // NOLINT
        boost::shared_ptr<FDSP_OffsetWriteObjType>& req)
    {
    }

    void
    PlatDataPathResp::RedirReadObjectResp(const FDSP_MsgHdrType& fdsp_msg,
                                          const FDSP_RedirReadObjType& req)
    {
    }

    void
    PlatDataPathResp::RedirReadObjectResp(
        boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg,  // NOLINT
        boost::shared_ptr<FDSP_RedirReadObjType>& req)
    {
    }

    void
    PlatDataPathResp::GetObjectMetadataResp(
        const FDSP_GetObjMetadataResp& metadata_resp)
    {
    }

    void
    PlatDataPathResp::GetObjectMetadataResp(
        boost::shared_ptr<FDSP_GetObjMetadataResp>& metadata_resp)  // NOLINT
    {
    }

#endif
}  // namespace fds
