

/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

/*
 * Unit tests for orchestration manager. These
 * test should only require the orchestration
 * manager to be up and running.
 */

#include <iostream>  // NOLINT(*)
#include <string>
#include <list>
#include <thread>

#include <fds_types.h>
#include <fds_error.h>
#include <fds_volume.h>
#include <fds_process.h>
#include <NetSession.h>
#include <util/Log.h>
#include <concurrency/Mutex.h>

namespace fds {

typedef boost::shared_ptr<FDS_ProtocolInterface::FDSP_ConfigPathReqClient> FDSP_ConfigPathReqClientPtr;

class ControlPathReq : public FDS_ProtocolInterface::FDSP_ControlPathReqIf {
  public:
    ControlPathReq() { }
    ControlPathReq(fds_log* plog) {
        test_log = plog;
    }
    
    ~ControlPathReq() { }
    
    void SetLog(fds_log* plog) {
        assert(test_log == NULL);
        test_log = plog;
    }

    void PrintVolumeDesc(const std::string& prefix, const VolumeDesc* voldesc) {
        if (test_log) {
            assert(voldesc);
            FDS_PLOG(test_log) << prefix << ": volume " << voldesc->name
                               << " id " << voldesc->volUUID
                               << "; capacity" << voldesc->capacity
                               << "; policy id " << voldesc->volPolicyId
                               << " iops_min " << voldesc->iops_min
                               << " iops_max " << voldesc->iops_max
                               << " rel_prio " << voldesc->relativePrio;
        }
    }

    void NotifyAddVol(const FDSP_MsgHdrType& fdsp_msg,
                      const FDSP_NotifyVolType& not_add_vol_req) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }
      
    void NotifyAddVol(FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
                      FDS_ProtocolInterface::FDSP_NotifyVolTypePtr& vol_msg) {
        assert(vol_msg->type == FDS_ProtocolInterface::FDSP_NOTIFY_ADD_VOL);
        VolumeDesc *vdb = new VolumeDesc(vol_msg->vol_desc);
        PrintVolumeDesc("NotifyAddVol", vdb);  
        delete vdb;
    }
  
    void NotifySnapVol(const FDSP_MsgHdrType& fdsp_msg,
                      const FDSP_NotifyVolType& not_snap_vol_req) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }

    void NotifySnapVol(FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
                      FDS_ProtocolInterface::FDSP_NotifyVolTypePtr& vol_msg) {
        assert(vol_msg->type == FDS_ProtocolInterface::FDSP_NOTIFY_SNAP_VOL);
        // VolumeDesc *vdb = new VolumeDesc(vol_msg->vol_desc);
        // PrintVolumeDesc("NotifySnapVol", vdb);
        // delete vdb;
    }


    void NotifyRmVol(const FDSP_MsgHdrType& fdsp_msg,
                     const FDSP_NotifyVolType& not_rm_vol_req) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }

    void NotifyRmVol(FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
                     FDS_ProtocolInterface::FDSP_NotifyVolTypePtr& vol_msg) {
        FDS_PLOG(test_log) << "NotifyRmVol recvd";
    }

    void NotifyModVol(const FDSP_MsgHdrType& fdsp_msg,
                      const FDSP_NotifyVolType& not_mod_vol_req) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }

    void NotifyModVol(FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
                      FDS_ProtocolInterface::FDSP_NotifyVolTypePtr& vol_msg) {
        assert(vol_msg->type == FDS_ProtocolInterface::FDSP_NOTIFY_MOD_VOL);
        VolumeDesc *vdb = new VolumeDesc(vol_msg->vol_desc);
        PrintVolumeDesc("NotifyModVol", vdb);  
        delete vdb;
    }
      
    void AttachVol(const FDSP_MsgHdrType& fdsp_msg, const FDSP_AttachVolType& atc_vol_req) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }

    void AttachVol(FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
		   FDS_ProtocolInterface::FDSP_AttachVolTypePtr& vol_msg) {
        if (msg_hdr->result == FDS_ProtocolInterface::FDSP_ERR_OK) 
        { 
            VolumeDesc *vdb = new VolumeDesc(vol_msg->vol_desc);
            PrintVolumeDesc("NotifyVolAttach", vdb);
            delete vdb;
        }
        else {
            std::string msg_str(msg_hdr->err_msg);
            FDS_PLOG(test_log) << "NotifyVolAttach: received " << msg_str;
        }
    }
    
    void DetachVol(const FDSP_MsgHdrType& fdsp_msg,
                   const FDSP_AttachVolType& dtc_vol_req) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
     }

    void DetachVol(FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
		   FDS_ProtocolInterface::FDSP_AttachVolTypePtr& vol_msg) {
        FDS_PLOG(test_log) << "DetachVol recvd";
    }
 
    void SnapVol(const FDSP_MsgHdrType& fdsp_msg,
                   const FDSP_AttachVolType& snap_vol_req) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
     }
    void SnapVol(FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
                   FDS_ProtocolInterface::FDSP_AttachVolTypePtr& vol_msg) {
        FDS_PLOG(test_log) << "SnapVol recvd";
    }


    void NotifyNodeAdd(const FDSP_MsgHdrType& fdsp_msg,
                       const FDSP_Node_Info_Type& node_info) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }

    void NotifyNodeAdd(FDSP_MsgHdrTypePtr& msg_hdr, 
		       FDSP_Node_Info_TypePtr& node_info) {
         FDS_PLOG(test_log) << "NotifyNodeAdd recvd"
                            << " for node " << node_info->node_name;
    }

    void NotifyNodeActive(const FDSP_MsgHdrType& fdsp_msg,
                          const FDSP_ActivateNodeType& act_node_req) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }

    void NotifyNodeActive(FDSP_MsgHdrTypePtr& msg_hdr, 
		       FDSP_ActivateNodeTypePtr& act_node_req) {
         FDS_PLOG(test_log) << "NotifyNodeActive recvd"
                            << " for node " << act_node_req->node_name;
    }

    void NotifyNodeRmv(const FDSP_MsgHdrType& fdsp_msg,
                       const FDSP_Node_Info_Type& node_info) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }

    void NotifyNodeRmv(FDSP_MsgHdrTypePtr& msg_hdr, 
		       FDSP_Node_Info_TypePtr& node_info) {
        FDS_PLOG(test_log) << "NotifyNodeRmv recvd";
    }
    
    void NotifyDLTUpdate(const FDSP_MsgHdrType& fdsp_msg,
                         const FDSP_DLT_Data_Type& dlt_info) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }

    void NotifyDLTUpdate(FDSP_MsgHdrTypePtr& msg_hdr,
			 FDSP_DLT_Data_TypePtr& dlt_info) {
#if 0 //SAN 
        FDS_PLOG(test_log) << "Received a DLT update" << std::endl;
        for (fds_uint32_t i = 0; i < dlt_info->DLT.size(); i++) {
            for (fds_uint32_t j = 0; j < dlt_info->DLT[i].size(); j++) {
                FDS_PLOG(test_log) << "Bucket " << i << " entry " << j
                                   << " value " << dlt_info->DLT[i][j] << std::endl;
            }
        }
#endif
    }

    void PushMetaDMTReq(const FDSP_MsgHdrType& fdsp_msg,
                        const FDSP_PushMeta& push_meta_resp) {}
    void PushMetaDMTReq(FDSP_MsgHdrTypePtr& fdsp_msg,
                        FDSP_PushMetaPtr& push_meta_resp) {}

    void NotifyDMTClose(const FDSP_MsgHdrType& fdsp_msg,
                        const FDSP_DmtCloseType& dmt_close) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }
    void NotifyDMTClose(FDSP_MsgHdrTypePtr& fdsp_msg,
                        FDSP_DmtCloseTypePtr& dmt_close) {
    }

    void NotifyDLTClose(const FDSP_MsgHdrType& fdsp_msg,
                        const FDSP_DltCloseType& dlt_info) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }

    void NotifyDLTClose(FDSP_MsgHdrTypePtr& msg_hdr,
                        FDSP_DltCloseTypePtr& dlt_info) {
        FDS_PLOG(test_log) << "Received a DLT close notification";
    }

    void NotifyDMTUpdate(const FDSP_MsgHdrType& msg_hdr,
			 const FDSP_DMT_Type& dmt_info) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }

    void NotifyDMTUpdate(FDSP_MsgHdrTypePtr& msg_hdr,
			 FDSP_DMT_TypePtr& dmt_info) {
        FDS_PLOG(test_log) << "Received a DMT update" << std::endl;
        /*
        for (fds_uint32_t i = 0; i < dmt_info->DMT.size(); i++) {
            for (fds_uint32_t j = 0; j < dmt_info->DMT[i].size(); j++) {
                FDS_PLOG(test_log) << "Bucket " << i << " entry " << j
                                   << " value " << dmt_info->DMT[i][j] << std::endl;
            }
        }
        */
    }
    
    void SetThrottleLevel(const FDSP_MsgHdrType& msg_hdr,
                          const FDSP_ThrottleMsgType& throttle_msg) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }

    void SetThrottleLevel(FDSP_MsgHdrTypePtr& msg_hdr,
                          FDSP_ThrottleMsgTypePtr& throttle_msg) {
        std::cout << "Received SetThrottleLevel with level = "
                  << throttle_msg->throttle_level;
    }

    void SetQoSControl(const FDSP_MsgHdrType& fdsp_msg,
                       const FDSP_QoSControlMsgType& qos_msg) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }

    void SetQoSControl(FDSP_MsgHdrTypePtr& fdsp_msg, FDSP_QoSControlMsgTypePtr& qos_msg) {
    }


    void TierPolicy(const FDSP_TierPolicy &tier) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }

    void TierPolicy(FDSP_TierPolicyPtr &tier) {
    }

    void TierPolicyAudit(const FDSP_TierPolicyAudit &audit) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }

    void TierPolicyAudit(FDSP_TierPolicyAuditPtr &audit) {
    }

    void NotifyBucketStats(const FDS_ProtocolInterface::FDSP_MsgHdrType& msg_hdr,
			   const FDS_ProtocolInterface::FDSP_BucketStatsRespType& buck_stats_msg) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }

    void NotifyBucketStats(FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
			   FDS_ProtocolInterface::FDSP_BucketStatsRespTypePtr& buck_stats_msg) {
    }

    void NotifyStartMigration(const FDS_ProtocolInterface::FDSP_MsgHdrType& msg_hdr,
                         const FDS_ProtocolInterface::FDSP_DLT_Data_Type& dlt_info) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }

    void NotifyStartMigration(FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
			 FDS_ProtocolInterface::FDSP_DLT_Data_TypePtr& dlt_info) {
    }

    void NotifyScavengerCmd(const FDS_ProtocolInterface::FDSP_MsgHdrType& msg_hdr,
                            const FDS_ProtocolInterface::FDSP_ScavengerType& gc_info) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }

    void NotifyScavengerCmd(FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
                            FDS_ProtocolInterface::FDSP_ScavengerTypePtr& gc_info) {
    }

private:
  fds_log* test_log;
};

class TestResp: public FDS_ProtocolInterface::FDSP_OMControlPathRespIf {
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

    void CreateBucketResp(const FDSP_MsgHdrType& fdsp_msg,
                          const FDSP_CreateVolType& crt_buck_rsp) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }
    
    void CreateBucketResp(boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg,
                          boost::shared_ptr<FDSP_CreateVolType>& crt_buck_rsp) {
        // Your implementation goes here
        FDS_PLOG(test_log) << "CreateBucketResp\n";
    }
    
    void DeleteBucketResp(const FDSP_MsgHdrType& fdsp_msg,
                          const FDSP_DeleteVolType& del_buck_rsp) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }
    void DeleteBucketResp(boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg,
                          boost::shared_ptr<FDSP_DeleteVolType>& del_buck_rsp) {
        // Your implementation goes here
        FDS_PLOG(test_log) << "DeleteBucketResp\n";
    }

    void ModifyBucketResp(const FDSP_MsgHdrType& fdsp_msg,
                          const FDSP_ModifyVolType& mod_buck_rsp) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }
    void ModifyBucketResp(boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg,
                          boost::shared_ptr<FDSP_ModifyVolType>& mod_buck_rsp) {
        // Your implementation goes here
        FDS_PLOG(test_log) << "ModifyBucketResp\n";
    }

    void AttachBucketResp(const FDSP_MsgHdrType& fdsp_msg,
                          const FDSP_AttachVolCmdType& atc_buck_req) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }
    void AttachBucketResp(boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg,
                          boost::shared_ptr<FDSP_AttachVolCmdType>& atc_buck_req) {
        // Your implementation goes here
        FDS_PLOG(test_log) << "AttachBucketResp\n";
    }

    void RegisterNodeResp(const FDSP_MsgHdrType& fdsp_msg,
                          const FDSP_RegisterNodeType& reg_node_rsp) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }
    void RegisterNodeResp(boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg,
                          boost::shared_ptr<FDSP_RegisterNodeType>& reg_node_rsp) {
        // Your implementation goes here
        FDS_PLOG(test_log) << "RegisterNodeResp\n";
    }

    void NotifyQueueFullResp(const FDSP_MsgHdrType& fdsp_msg,
                             const FDSP_NotifyQueueStateType& queue_state_rsp) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }
    void NotifyQueueFullResp(boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg,
                             boost::shared_ptr<FDSP_NotifyQueueStateType>& queue_state_rsp) {
        // Your implementation goes here
        FDS_PLOG(test_log) << "NotifyQueueFullResp\n";
    }

    void NotifyPerfstatsResp(const FDSP_MsgHdrType& fdsp_msg,
                             const FDSP_PerfstatsType& perf_stats_rsp) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }
    void NotifyPerfstatsResp(boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg,
                             boost::shared_ptr<FDSP_PerfstatsType>& perf_stats_rsp) {
        // Your implementation goes here
        FDS_PLOG(test_log) << "NotifyPerfstatsResp\n";
    }
    
    void TestBucketResp(const FDSP_MsgHdrType& fdsp_msg,
                        const FDSP_TestBucket& test_buck_rsp) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }
    void TestBucketResp(boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg,
                        boost::shared_ptr<FDSP_TestBucket>& test_buck_rsp) {
        // Your implementation goes here
        FDS_PLOG(test_log) << "TestBucketResp\n";
    }
    
    void GetDomainStatsResp(const FDSP_MsgHdrType& fdsp_msg,
                            const FDSP_GetDomainStatsType& get_stats_rsp) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }
    void GetDomainStatsResp(boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg,
                            boost::shared_ptr<FDSP_GetDomainStatsType>& get_stats_rsp) {
        // Your implementation goes here
        FDS_PLOG(test_log) << "GetDomainStatsResp\n";
    }

    void MigrationDoneResp(const FDSP_MsgHdrType& fdsp_msg,
                           const FDSP_MigrationStatusType& status_rsp) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }
    void MigrationDoneResp(boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg,
                            boost::shared_ptr<FDSP_MigrationStatusType>& status_rsp) {
        // Your implementation goes here
        FDS_PLOG(test_log) << "MigrationDoneResp\n";
    }
};

class OmUnitTest {
  private:
    std::list<std::string>  unit_tests;
    
    fds_log *test_log;
    
    fds_uint32_t om_port_num;
    fds_uint32_t cp_port_num;
    fds_uint32_t om_ip;
    fds_uint32_t num_updates;

    boost::shared_ptr<netSessionTbl> net_session_tbl;
    std::vector<std::thread*> server_threads;

    /*
     * Helper function to construct msg hdr
     */
    void initOMMsgHdr(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr) {
        msg_hdr->minor_ver = 0;
        msg_hdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_PUT_OBJ_REQ;
        msg_hdr->msg_id =  1;
        
        msg_hdr->major_ver = 0xa5;
        msg_hdr->minor_ver = 0x5a;
        
        msg_hdr->num_objects = 1;
        msg_hdr->frag_len = 0;
        msg_hdr->frag_num = 0;
        
        msg_hdr->tennant_id = 0;
        msg_hdr->local_domain_id = 0;
        
        msg_hdr->src_id = FDS_ProtocolInterface::FDSP_STOR_HVISOR;
        msg_hdr->dst_id = FDS_ProtocolInterface::FDSP_ORCH_MGR;
        
        msg_hdr->err_code = ERR_OK;
        msg_hdr->result   = FDS_ProtocolInterface::FDSP_ERR_OK;
    }
    
    /*
     * Helper function to init vol info
     */
    void initVolInfo(FDS_ProtocolInterface::FDSP_VolumeInfoType* vol_info, const std::string& vol_name) {
        vol_info->vol_name = vol_name;
        vol_info->tennantId = 0;
        vol_info->localDomainId = 0;
        vol_info->globDomainId = 0;
        
        vol_info->capacity = 0;
        vol_info->volType = FDS_ProtocolInterface::FDSP_VOL_BLKDEV_TYPE;
        vol_info->defConsisProtocol =
                FDS_ProtocolInterface::FDSP_CONS_PROTO_STRONG;
        vol_info->appWorkload =
                FDS_ProtocolInterface::FDSP_APP_WKLD_TRANSACTION;
        vol_info->volPolicyId = 0;
        vol_info->mediaPolicy =
                FDS_ProtocolInterface::FDSP_MEDIA_POLICY_HDD;
        
        vol_info->defReplicaCnt = 0;
        vol_info->defWriteQuorum = 0;
        vol_info->defReadQuorum = 0;
        
        vol_info->archivePolicyId = 0;
        vol_info->placementPolicy = 0;
    }
    
    /*
     * Helper function to init vol desc
     */
    void initVolDesc(FDS_ProtocolInterface::FDSP_VolumeDescType* vol_desc) {
        vol_desc->vol_name = std::string("test");
        vol_desc->tennantId = 0;
        vol_desc->localDomainId = 0;
        vol_desc->globDomainId = 0;
        
        vol_desc->capacity = 0;
        vol_desc->volType = FDS_ProtocolInterface::FDSP_VOL_BLKDEV_TYPE;
        vol_desc->defConsisProtocol =
                FDS_ProtocolInterface::FDSP_CONS_PROTO_STRONG;
        vol_desc->appWorkload =
                FDS_ProtocolInterface::FDSP_APP_WKLD_TRANSACTION;
        vol_desc->volPolicyId = 0;
        vol_desc->mediaPolicy =
                FDS_ProtocolInterface::FDSP_MEDIA_POLICY_HDD;

        vol_desc->defReplicaCnt = 0;
        vol_desc->defWriteQuorum = 0;
        vol_desc->defReadQuorum = 0;
        
        vol_desc->archivePolicyId = 0;
        vol_desc->placementPolicy = 0;
        
        vol_desc->volPolicyId = 0;
    }

    void initDiskCapabilities(FDS_ProtocolInterface::FDSP_AnnounceDiskCapability* disk_cap) {
        disk_cap->disk_iops_max = 5000;
        disk_cap->disk_iops_min = 1000;
        disk_cap->disk_capacity = 102400;
        disk_cap->disk_latency_max = 10000;
        disk_cap->disk_latency_min = 1000;
        disk_cap->ssd_iops_max = 20000;
        disk_cap->ssd_iops_min = 10000;
        disk_cap->ssd_capacity = 1024;
        disk_cap->ssd_latency_max = 1000;
        disk_cap->ssd_latency_min = 100;
        disk_cap->disk_type = 0;
    }
    
    /*
     * Helper functions to create and remember net sessions
     */
    FDSP_ConfigPathReqClientPtr getConfigClient() {
        return net_session_tbl->\
                getClientSession<netConfigPathClientSession>(om_ip, om_port_num)->\
                getClient();
    }

    static void run_server_thread(netSessionTbl* session_tbl, netSession* server_session) {
        session_tbl->listenServer(server_session);
    }

    /* simulate creating an OMClient node (SH, SM, or DM based on 
     * omclient_node_name; Will start server session to listen
     * for control messages from OM and start client session to
     * send OMControl messages to OM; will add session table t o
     * 'tbl_list'
     */
    netOMControlPathClientSession*
    createOmClientComm(const std::string& omclient_node_name,
                            FDS_ProtocolInterface::FDSP_MgrIdType omclient_node_type,
                            int omclient_port,
                            std::list<netSessionTbl*>& tbl_list) {
        
        netOMControlPathClientSession* client_session = NULL;
	netSession* server_session = NULL;
    boost::shared_ptr<TestResp> resp_handler_ptr(new TestResp(test_log));        
        boost::shared_ptr<ControlPathReq> cpreq_handler_ptr(new ControlPathReq(test_log));
        netSessionTbl* session_tbl = new netSessionTbl(omclient_node_name,
                                                       0x7f000001,
                                                       omclient_port,
                                                       1,
                                                       omclient_node_type);
        if (!session_tbl) {
            return NULL;
        }
        tbl_list.push_back(session_tbl);

	/* server session on the same 'node' to listen for control messages from OM */
	server_session = session_tbl->createServerSession<netControlPathServerSession>(
        0x7f000001,
        omclient_port,
        omclient_node_name,
        FDS_ProtocolInterface::FDSP_ORCH_MGR,
        cpreq_handler_ptr);

	/* start thread to listen for control msgs from OM */
	std::thread* new_thread = new std::thread(&OmUnitTest::run_server_thread, session_tbl, server_session);
	if (new_thread) 
	  server_threads.push_back(new_thread);


	/* client session to send OMControl messages to OM */
        client_session = session_tbl->startSession<netOMControlPathClientSession>(0x7f000001,
                                                   cp_port_num,
                                                   FDS_ProtocolInterface::FDSP_ORCH_MGR,
                                                   1,
                                                   resp_handler_ptr);


        return client_session;
    }

    void clearOmClientComms(std::list<netSessionTbl*>& list) {
        for (std::list<netSessionTbl*>::iterator it = list.begin();
             it != list.end();
             ++it) {
            netSessionTbl* tbl = *it;
            tbl->endAllSessions();
            delete tbl;
        }
        list.clear();
        for (uint i = 0; i < server_threads.size(); ++i) {
            server_threads[i]->join();
        }
        while (server_threads.size() > 0) {
            std::thread* th = server_threads.back();
            delete th;
            server_threads.pop_back();
        }
    }

    boost::shared_ptr<FDSP_OMControlPathReqClient>
    register_node(FDS_ProtocolInterface::FDSP_MsgHdrTypePtr msg_hdr,
                  const std::string node_name,
                  FDS_ProtocolInterface::FDSP_MgrIdType node_type,
                  fds_uint32_t port,
                  fds_uint32_t ip_addr,
                  std::list<netSessionTbl*> & nst_list) {
        
        FDS_ProtocolInterface::FDSP_RegisterNodeTypePtr reg_node_msg(
            new FDS_ProtocolInterface::FDSP_RegisterNodeType);
        
        netOMControlPathClientSession* ocp_client_session;
        
        reg_node_msg->domain_id  = 0;
        reg_node_msg->ip_hi_addr = 0;
        reg_node_msg->ip_lo_addr = ip_addr;
        reg_node_msg->data_port  = 0;
        /*
         * Add some zeroed out disk info
         */
        initDiskCapabilities(&(reg_node_msg->disk_info));
        
        /*
         * Create and endpoint to reflect the "node" that
         * we're registering, so that "node" can recieve
         * updates from the OM.
         */
        reg_node_msg->control_port = port;
        reg_node_msg->node_type = node_type;
        
        /*
         * TODO: Make the name just an int since the OM turns this into
         * a UUID to address the node. Fix this by adding a UUID int to
         * the FDSP.
         */
        reg_node_msg->node_name = node_name;
        
        ocp_client_session = createOmClientComm(reg_node_msg->node_name,
                                                reg_node_msg->node_type,
                                                reg_node_msg->control_port,
                                                nst_list);
        if (!ocp_client_session) {
            FDS_PLOG(test_log) << "ERROR: failed to create OmClientComm for node "
                               << reg_node_msg->node_name;
            return boost::shared_ptr<FDSP_OMControlPathReqClient>();
        }
        
        ocp_client_session->getClient()->RegisterNode(msg_hdr, reg_node_msg);
        FDS_PLOG(test_log) << "Completed node registration at IP "
                           << fds::ipv4_addr_to_str(reg_node_msg->ip_lo_addr)
                           << " control port " << reg_node_msg->control_port;

        return ocp_client_session->getClient();
    } 

 
    /*
     * Unit test funtions
     */
    fds_int32_t node_reg() {
        FDS_PLOG(test_log) << "Starting test: node_reg()";

        /* this test will send register node from simulated
         * AM, SM, and DM nodes. Since we are simulating 
         * 'num_updates' number of nodes, we should create
         * separate netSessionTbl for each 'node'
         */
        std::list<netSessionTbl*> session_tbl_list;

        FDS_ProtocolInterface::FDSP_MsgHdrTypePtr msg_hdr(
            new FDS_ProtocolInterface::FDSP_MsgHdrType);
        initOMMsgHdr(msg_hdr);
        
        FDS_ProtocolInterface::FDSP_RegisterNodeTypePtr reg_node_msg(
            new FDS_ProtocolInterface::FDSP_RegisterNodeType());
        
        reg_node_msg->domain_id  = 0;
        reg_node_msg->ip_hi_addr = 0;
        /*
         * Add some zeroed out disk info
         */
        initDiskCapabilities(&(reg_node_msg->disk_info));
        
        /*
         * We're expecting to contact OM on localhost.
         * Set hex 127.0.0.1 value.
         * TODO: Make this cmdline configurable.
         */
        reg_node_msg->ip_lo_addr = 0x7f000001;
        /*
         * This unit test isn't using data path requests
         * so we leave the port as 0.
         */
        reg_node_msg->data_port = 0;
        
        for (fds_uint32_t i = 0; i < num_updates; i++) {
            netOMControlPathClientSession* ocp_client_session;

            /*
             * TODO: Make the name just an int since the OM turns this into
             * a UUID to address the node. Fix this by adding a UUID int to
             * the FDSP.
             */
            reg_node_msg->node_name = std::to_string(i);

            /*
             * Create and endpoint to reflect the "node" that
             * we're registering, so that "node" can recieve
             * updates from the OM.
             * TODO(thrift) since we are simulating registering
             * a node from SM, DM, and SH, we need to create 
             * appropriate sessions
             */
            reg_node_msg->control_port = cp_port_num + i + 1;
            if ((i % 3) == 0) {
                /* SM -> OM */
                reg_node_msg->node_type = FDS_ProtocolInterface::FDSP_STOR_MGR;
            } else if ((i % 2) == 0) {
                /* DM -> OM */
                reg_node_msg->node_type = FDS_ProtocolInterface::FDSP_DATA_MGR;
            } else {
                /* SH -> OM */
                reg_node_msg->node_type = FDS_ProtocolInterface::FDSP_STOR_HVISOR;
            }

            ocp_client_session = createOmClientComm(reg_node_msg->node_name,
                                                    reg_node_msg->node_type,
                                                    reg_node_msg->control_port,
                                                    session_tbl_list);
            
            ocp_client_session->getClient()->RegisterNode(msg_hdr, reg_node_msg);
            
            FDS_PLOG(test_log) << "Sent node registration " << i << " at IP "
                               << fds::ipv4_addr_to_str(reg_node_msg->ip_lo_addr)
                               << " control port " << reg_node_msg->control_port;
        }

        // since we are using async calls, wait a bit before exiting, 
        // TODO: signal completion
        sleep(5);        

        // clear session table
        clearOmClientComms(session_tbl_list);

        FDS_PLOG(test_log) << "Ending test: node_reg()";
        return 0;
    }
    
    fds_int32_t vol_reg() {
        FDS_PLOG(test_log) << "Starting test: vol_reg()";

        FDSP_ConfigPathReqClientPtr fdspConfigPathAPI = getConfigClient();


        FDS_ProtocolInterface::FDSP_MsgHdrTypePtr msg_hdr(
            new FDS_ProtocolInterface::FDSP_MsgHdrType);
        initOMMsgHdr(msg_hdr);
        
        FDS_ProtocolInterface::FDSP_CreateVolTypePtr crt_vol(
            new FDS_ProtocolInterface::FDSP_CreateVolType());
        
        initOMMsgHdr(msg_hdr);

        for (fds_uint32_t i = 0; i < num_updates; i++) {
            crt_vol->vol_name = std::string("Volume ") + std::to_string(i+1);
            (crt_vol->vol_info).vol_name = crt_vol->vol_name;
            // Currently set capacity to 0 size no one has register a storage
            // node with OM to increase/create initial capacity.
            (crt_vol->vol_info).capacity = 0;
            (crt_vol->vol_info).volType = FDS_ProtocolInterface::FDSP_VOL_BLKDEV_TYPE;
            (crt_vol->vol_info).defConsisProtocol =
                    FDS_ProtocolInterface::FDSP_CONS_PROTO_STRONG;
            (crt_vol->vol_info).appWorkload =
                    FDS_ProtocolInterface::FDSP_APP_WKLD_TRANSACTION;
            (crt_vol->vol_info).volPolicyId = 0;
            (crt_vol->vol_info).mediaPolicy =
                    FDS_ProtocolInterface::FDSP_MEDIA_POLICY_HDD;

            FDS_PLOG(test_log) << "OM unit test client creating volume "
                               << (crt_vol->vol_info).vol_name
                               << " and capacity " << (crt_vol->vol_info).capacity;
            
            fdspConfigPathAPI->CreateVol(msg_hdr, crt_vol);

            FDS_PLOG(test_log) << "OM unit test client completed creating volume.";
        }
        
        FDS_PLOG(test_log) << "Ending test: vol_reg()";
        return 0;
    }
    
    fds_int32_t policy_test() {
        fds_int32_t result = 0;
        FDS_PLOG(test_log) << "Starting test: volume policy";
        
        if (num_updates == 0) {
            FDS_PLOG(test_log) << "Nothing to do for test volume policy, num_updates == 0";
            return result;
        }

        FDSP_ConfigPathReqClientPtr fdspConfigPathAPI = getConfigClient();
        
        FDS_ProtocolInterface::FDSP_MsgHdrTypePtr msg_hdr(
            new FDS_ProtocolInterface::FDSP_MsgHdrType);
        initOMMsgHdr(msg_hdr);
        
        FDS_ProtocolInterface::FDSP_CreatePolicyTypePtr crt_pol(
            new FDS_ProtocolInterface::FDSP_CreatePolicyType());
        
        /* first define policies we will create */
        FDS_VolumePolicy pol_tab[num_updates];
        for (fds_uint32_t i = 0; i < num_updates; ++i)
        {
            pol_tab[i].volPolicyName = std::string("Policy ") + std::to_string(i+1);
            pol_tab[i].volPolicyId = i+1;
            pol_tab[i].iops_min = 100;
            pol_tab[i].iops_max = 200;
            pol_tab[i].relativePrio = (i+1)%8 + 1;
        }

        /* create policies */
        for (fds_uint32_t i = 0; i < num_updates; ++i)
        {
            crt_pol->policy_name = pol_tab[i].volPolicyName;
            (crt_pol->policy_info).policy_name = crt_pol->policy_name;
            (crt_pol->policy_info).policy_id = pol_tab[i].volPolicyId;
            (crt_pol->policy_info).iops_min = pol_tab[i].iops_min;
            (crt_pol->policy_info).iops_max = pol_tab[i].iops_max;
            (crt_pol->policy_info).rel_prio = pol_tab[i].relativePrio;

            FDS_PLOG(test_log) << "OM unit test client creating policy "
                               << crt_pol->policy_name
                               << "; id " << (crt_pol->policy_info).policy_id
                               << "; iops_min " << (crt_pol->policy_info).iops_min
                               << "; iops_max " << (crt_pol->policy_info).iops_max
                               << "; rel_prio " << (crt_pol->policy_info).rel_prio;
            
            fdspConfigPathAPI->CreatePolicy(msg_hdr, crt_pol);
            
            FDS_PLOG(test_log) << "OM unit test client completed creating policy";
            
        }

        /* register SH, DM, and SM nodes */
        std::list<netSessionTbl*> session_tbl_list;
        if (!register_node(msg_hdr,
                          "SM_policytest_node",
                          FDS_ProtocolInterface::FDSP_STOR_MGR,
                          cp_port_num+1,
                          0x7f000001, /* 127.0.0.1 */
                           session_tbl_list)) {
            return -1;
        }

        if (!register_node(msg_hdr,
                           "DM_policytest_node",
                           FDS_ProtocolInterface::FDSP_DATA_MGR,
                           cp_port_num+2,
                           0x7f000001, /* 127.0.0.1 */
                           session_tbl_list)) {
            return -1;
        }

        if (!register_node(msg_hdr,
                           "SH_policytest_node",
                           FDS_ProtocolInterface::FDSP_STOR_HVISOR,
                           cp_port_num+3,
                           0x7f000001, /* 127.0.0.1 */
                           session_tbl_list)) {
            return -1;
        }

        /* since those are async calls, lets wait a bit, but would be better to signal 
         * sleeping also ok, we will just not receive volume attach, create, etc notifications
         * on our simulated nodes */
        sleep(2);

        /* create volumes, one for each policy */
        /* SM and DM should receive 'volume add' notifications that include volume policy details */
        FDS_ProtocolInterface::FDSP_CreateVolTypePtr crt_vol(
            new FDS_ProtocolInterface::FDSP_CreateVolType());
        
        FDS_PLOG(test_log) << "OM unit test client will create volumes, one for each policy";
        const int vol_start_uuid = 900;
        for (fds_uint32_t i = 0; i < num_updates; ++i)
        {
            crt_vol->vol_name = std::string("Volume ") + std::to_string(vol_start_uuid + i);
            (crt_vol->vol_info).vol_name = crt_vol->vol_name;
            // Currently set capacity to 0 size no one has register a storage
            // node with OM to increase/create initial capacity.
            (crt_vol->vol_info).capacity = 0;
            (crt_vol->vol_info).volType = FDS_ProtocolInterface::FDSP_VOL_BLKDEV_TYPE;
            (crt_vol->vol_info).defConsisProtocol =
                    FDS_ProtocolInterface::FDSP_CONS_PROTO_STRONG;
            (crt_vol->vol_info).appWorkload =
                    FDS_ProtocolInterface::FDSP_APP_WKLD_TRANSACTION;
            (crt_vol->vol_info).volPolicyId = pol_tab[i].volPolicyId;
            (crt_vol->vol_info).mediaPolicy =
                    FDS_ProtocolInterface::FDSP_MEDIA_POLICY_HDD;;

            FDS_PLOG(test_log) << "OM unit test client creating volume "
                               << (crt_vol->vol_info).vol_name
                               << " and policy " << (crt_vol->vol_info).volPolicyId;
            
            fdspConfigPathAPI->CreateVol(msg_hdr, crt_vol);
            
            FDS_PLOG(test_log) << "OM unit test client completed creating volume.";
        }

        /* attach volume 0 to SH node (node id = 2) */
        /* SH should receive first volume policy details */
        FDS_ProtocolInterface::FDSP_AttachVolCmdTypePtr att_vol(
            new FDS_ProtocolInterface::FDSP_AttachVolCmdType());
        att_vol->vol_name = std::string("Volume ") + std::to_string(vol_start_uuid);
        att_vol->node_id = "SH_policytest_node"; 
        msg_hdr->src_node_name = "SH_policytest_node"; 
        FDS_PLOG(test_log) << "OM unit test client attaching volume "
                           << att_vol->vol_name 
                           << " to node " << att_vol->node_id; 

        fdspConfigPathAPI->AttachVol(msg_hdr, att_vol);
        FDS_PLOG(test_log) << "OM unit test client completed attaching volume.";
        
        /* modify policy with id 0 */
        /* NOTE: for now modify policy will not change anything for existing volumes 
         * need to come back to this unit test when modify policy is implemented end-to-end */
        if (num_updates > 0)
        {
            FDS_ProtocolInterface::FDSP_ModifyPolicyTypePtr mod_pol(
                new FDS_ProtocolInterface::FDSP_ModifyPolicyType());
                        
            mod_pol->policy_name = pol_tab[0].volPolicyName;
            mod_pol->policy_id = pol_tab[0].volPolicyId;
            (mod_pol->policy_info).policy_name = mod_pol->policy_name;
            (mod_pol->policy_info).policy_id = mod_pol->policy_id;
            /* new values */
            (mod_pol->policy_info).iops_min = 333;
            (mod_pol->policy_info).iops_max = 1000;
            (mod_pol->policy_info).rel_prio = 3;

            FDS_PLOG(test_log) << "OM unit test modifying policy "
                               << mod_pol->policy_name
                               << "; id " << (mod_pol->policy_info).policy_id;
            
            fdspConfigPathAPI->ModifyPolicy(msg_hdr, mod_pol);
            
            FDS_PLOG(test_log) << "OM unit test client completed modifying policy";
        }

        /* modify volume's min/max iops & priority info for first volume (that attached to SH node */
        FDS_ProtocolInterface::FDSP_ModifyVolTypePtr mod_vol(
            new FDS_ProtocolInterface::FDSP_ModifyVolType());
        initVolDesc(&(mod_vol->vol_desc));
        mod_vol->vol_name = std::string("Volume ") + std::to_string(vol_start_uuid);
        (mod_vol->vol_desc).vol_name = mod_vol->vol_name;
        (mod_vol->vol_desc).iops_min = 888;
        (mod_vol->vol_desc).iops_max = 999;
        (mod_vol->vol_desc).rel_prio = 9;
        FDS_PLOG(test_log) << "OM unit test client modifying volume "
                           << mod_vol->vol_name 
                           << " iops_min " << (mod_vol->vol_desc).iops_min
                           << " iops_max " << (mod_vol->vol_desc).iops_max
                           << " rel_prio " << (mod_vol->vol_desc).rel_prio;

        fdspConfigPathAPI->ModifyVol(msg_hdr, mod_vol);
        FDS_PLOG(test_log) << "OM unit test client completed modifying volume " << mod_vol->vol_name;

        /* delete all policies */
        FDS_ProtocolInterface::FDSP_DeletePolicyTypePtr del_pol(
            new FDS_ProtocolInterface::FDSP_DeletePolicyType());
        
        for (fds_uint32_t i = 0; i < num_updates; ++i)
        {
            del_pol->policy_name = pol_tab[i].volPolicyName;
            del_pol->policy_id = pol_tab[i].volPolicyId; 
            
            FDS_PLOG(test_log) << "OM unit test client deleting policy "
                               << del_pol->policy_name
                               << "; id" << del_pol->policy_id;
            
            fdspConfigPathAPI->DeletePolicy(msg_hdr, del_pol);
            
            FDS_PLOG(test_log) << "OM unit test client completed deleting policy"; 
            
        }
        FDS_PLOG(test_log) << " Finished test: policy";
     
        // clear session table
        clearOmClientComms(session_tbl_list);

        return result;
    }
    
    fds_int32_t buckets_test() {
        FDS_PLOG(test_log) << "Starting test: buckets_test()";

        FDS_ProtocolInterface::FDSP_MsgHdrTypePtr msg_hdr(
            new FDS_ProtocolInterface::FDSP_MsgHdrType);
        initOMMsgHdr(msg_hdr);
        
        /* we should register AM node that will send test bucket msg */
        std::list<netSessionTbl*> session_tbl_list;
        boost::shared_ptr<FDSP_OMControlPathReqClient> ocp_client =
                register_node(msg_hdr,
                              "test_AM",
                              FDS_ProtocolInterface::FDSP_STOR_HVISOR,
                              cp_port_num+4,
                              0x7f000001, /* 127.0.0.1 */
                              session_tbl_list);
        if (!ocp_client)
            return -1;
        
        /* test bucket that we created with prev tests or it does not exist if we run only this test  */
        FDS_ProtocolInterface::FDSP_TestBucketPtr test_buck_msg(
            new FDS_ProtocolInterface::FDSP_TestBucket);
        
        test_buck_msg->bucket_name = std::string("Volume 901");
        initVolInfo(&(test_buck_msg->vol_info), test_buck_msg->bucket_name);
        test_buck_msg->attach_vol_reqd = true;
        test_buck_msg->accessKeyId = "x";
        test_buck_msg->secretAccessKey = "y";
        
        ocp_client->TestBucket(msg_hdr, test_buck_msg);
        /* since TestBucket is async, lets sleep a bit to see if we get 
         * vol attach notification (we should get attach vol if this test
         * runs after policy test, if standalone, test bucket will be 
         * negative */
        sleep(2);
        
        FDS_PLOG(test_log) << "Completed test bucket request for bucket " 
                           << test_buck_msg->bucket_name;
        
        // clear session table
        clearOmClientComms(session_tbl_list);

        FDS_PLOG(test_log) << "Ending test: buckets_test()";
        return 0;
    }
    


  public:
    OmUnitTest()
            : net_session_tbl(new netSessionTbl("OM_unit_test",
                                                0x7f000001,
                                                0,
                                                1,
                                                FDS_ProtocolInterface::FDSP_CLI_MGR)) {

        test_log = new fds_log("om_test", "logs");
        
        unit_tests.push_back("node_reg");
        //unit_tests.push_back("vol_reg");
        //unit_tests.push_back("policy");
        //unit_tests.push_back("buckets_test");
        
        num_updates = 5;
    }

    explicit OmUnitTest(fds_uint32_t om_port_arg,
                        fds_uint32_t cp_port_arg,
                        fds_uint32_t num_up_arg)
            : OmUnitTest() {
        om_port_num = om_port_arg;
        cp_port_num = cp_port_arg;
        om_ip = 0x7f000001;
        num_updates = 3; //num_up_arg;

        /* start config client session */
        net_session_tbl->startSession<netConfigPathClientSession>(om_ip,
                                      om_port_num, 
                                      FDS_ProtocolInterface::FDSP_ORCH_MGR,
                                      1,
                                      boost::shared_ptr<FDSP_ConfigPathRespIf>());
        
    }

    ~OmUnitTest() {
        net_session_tbl->endClientSession(om_ip, om_port_num);
        delete test_log;
    }
    
    fds_log* GetLogPtr() {
        return test_log;
    }
    
    fds_int32_t Run(const std::string& testname) {
        int result = 0;
        std::cout << "Running unit test \"" << testname << "\"" << std::endl;
        
        if (testname == "node_reg") {
            result = node_reg();
        } else if (testname == "vol_reg") {
            result = vol_reg();
        } else if (testname == "policy") {
            result = policy_test();
        } else if (testname == "buckets_test") {
            result = buckets_test();
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
}  // namespace fds

int main(int argc, char* argv[]) {

    init_process_globals("fds-timer-test.log");

    /*
     * Grab cmdline params
     */
    fds_uint32_t om_port_num = 0;
    fds_uint32_t cp_port_num = 0;
    fds_uint32_t num_updates = 10;
    std::string testname;
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "--testname=", 11) == 0) {
            testname = argv[i] + 11;
        } else if (strncmp(argv[i], "--port=", 7) == 0) {
            om_port_num = strtoul(argv[i] + 7, NULL, 0);
        } else if (strncmp(argv[i], "--cp_port=", 10) == 0) {
            cp_port_num = strtoul(argv[i] + 10, NULL, 0);
        } else if (strncmp(argv[i], "--num_updates=", 14) == 0) {
            num_updates = atoi(argv[i] + 14);
        } else {
            std::cerr << "Invalid command line: Unknown argument "
                      << argv[i] << std::endl;
            return -1;
        }
    }
    
    // TODO(config) we did not port om_client.conf yet to new config 
    // boost::shared_ptr<fds::FdsConfig> omc_config(new fds::FdsConfig("om_client.conf"));
    if (om_port_num == 0) {
        /*
         * Read from config where to contact OM
         */
        om_port_num = 8903; // omc_config->get<int>("fds.omc.om.config_port");
        //om_props->getPropertyAsInt("OMgr.ConfigPort");
    }
    
    /*
     * Create control path server to listen for messages from OM.
     * TODO: We should fork this off into a separate function so
     * that we can create multiple clients with different ports
     * that they're listening on.
     */
    if (cp_port_num == 0) {
        /*
         * Read from config which port to listen for control path.
         * We overload the OMgrClient since this is the port the
         * OM thinks it will be talking to.
         */
        cp_port_num = 8904; //omc_config->get<int>("fds.omc.control_port");
    }
    
    fds::OmUnitTest unittest(om_port_num,
                             cp_port_num,
                             num_updates);
    if (testname.empty()) {
        unittest.Run();
    } else {
        unittest.Run(testname);
    }
    
    return 0;
}
