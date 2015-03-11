/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ORCH_MGR_INCLUDE_ORCHMGR_H_
#define SOURCE_ORCH_MGR_INCLUDE_ORCHMGR_H_

#include <cstdio>
#include <string>
#include <vector>

#include <fds_types.h>
#include <fds_error.h>
#include <fds_config.hpp>
#include <fds_process.h>
#include <fdsp/FDSP_ConfigPathReq.h>
#include <fdsp/FDSP_OMControlPathReq.h>
#include <fdsp/sm_types_types.h>
#include <fds_typedefs.h>
#include <util/Log.h>
#include <concurrency/Mutex.h>
#include <omutils.h>
#include <OmVolPolicy.hpp>
#include <OmAdminCtrl.h>
#include <kvstore/configdb.h>
#include <snapshot/manager.h>
#include <deletescheduler.h>
#include <net/SvcProcess.h>
#include <net/PlatNetSvcHandler.h>

#define MAX_OM_NODES            (512)
#define DEFAULT_LOC_DOMAIN_ID   (1)
#define DEFAULT_GLB_DOMAIN_ID   (1)

namespace fpi = FDS_ProtocolInterface;

namespace FDS_ProtocolInterface {
    struct CtrlNotifyMigrationStatus;
    struct FDSP_OMControlPathReqProcessor;
    struct FDSP_OMControlPathReqIf;
    struct FDSP_OMControlPathRespClient;
    struct FDSP_OMControlPathReqIf;
    struct FDSP_ConfigPathReqProcessor;
    struct FDSP_ConfigPathReqIf;
    struct FDSP_ConfigPathRespClient;
}

class netSessionTbl;
template <class A, class B, class C> class netServerSessionEx;
typedef netServerSessionEx<fpi::FDSP_OMControlPathReqProcessor,
                fpi::FDSP_OMControlPathReqIf,
                fpi::FDSP_OMControlPathRespClient> netOMControlPathServerSession;
typedef netServerSessionEx<fpi::FDSP_ConfigPathReqProcessor,
                fpi::FDSP_ConfigPathReqIf,
                fpi::FDSP_ConfigPathRespClient> netConfigPathServerSession;

namespace fds {

class OM_Module;

class OrchMgr: public SvcProcess {
  private:
    fds_log *om_log;
    SysParams *sysParams;

    /* net session tbl for OM control path*/
    boost::shared_ptr<netSessionTbl> omcp_session_tbl;
    netOMControlPathServerSession *omc_server_session;
    boost::shared_ptr<fpi::FDSP_OMControlPathReqIf> omcp_req_handler;
    std::string my_node_name;

    /* net session tbl for OM config path server */
    boost::shared_ptr<netSessionTbl> cfg_session_tbl;
    netConfigPathServerSession *cfg_server_session;
    boost::shared_ptr<fpi::FDSP_ConfigPathReqIf> cfg_req_handler;
    /* config path server is run on this thread */
    boost::shared_ptr<std::thread> cfgserver_thread;


    int current_dlt_version;
    FDS_ProtocolInterface::Node_Table_Type current_dlt_table;
    fds_mutex *om_mutex;
    std::string node_id_to_name[MAX_OM_NODES];
    const int table_type_dlt = 0;

    /*
     * Cmdline configurables
     */
    int conf_port_num; /* config port to listen for cli commands */
    int ctrl_port_num; /* control port (register node + config cmds from AM) */
    std::string stor_prefix;
    fds_bool_t test_mode;

    /* policy manager */
    VolPolicyMgr           *policy_mgr;
    kvstore::ConfigDB      *configDB;
    void SetThrottleLevelForDomain(int domain_id, float throttle_level);

  protected:
    virtual void setupSvcInfo_() override;

  public:
    OrchMgr(int argc, char *argv[], OM_Module *omModule);
    ~OrchMgr();
    void start_cfgpath_server();

    /**** From FdsProcess ****/
    virtual void proc_pre_startup() override;
    virtual void proc_pre_service() override;
    /*
     * Runs the orch manager server.
     * Does not return until the server is no longer running
     */
    virtual int  run() override;
    virtual void interrupt_cb(int signum) override;

    virtual void registerSvcProcess() override;

    bool loadFromConfigDB();
    void defaultS3BucketPolicy();  // default  policy  desc  for s3 bucket
    DmtColumnPtr getDMTNodesForVolume(fds_volid_t volId);
    kvstore::ConfigDB* getConfigDB();

    static VolPolicyMgr      *om_policy_mgr();
    static const std::string &om_stor_prefix();

    int CreatePolicy(const FdspMsgHdrPtr& fdsp_msg,
                     const FdspCrtPolPtr& crt_pol_req);
    int DeletePolicy(const FdspMsgHdrPtr& fdsp_msg,
                     const FdspDelPolPtr& del_pol_req);
    int ModifyPolicy(const FdspMsgHdrPtr& fdsp_msg,
                     const FdspModPolPtr& mod_pol_req);
    void NotifyQueueFull(const fpi::FDSP_MsgHdrTypePtr& fdsp_msg,
                        const fpi::FDSP_NotifyQueueStateTypePtr& queue_state_req);

    fds::snapshot::Manager snapshotMgr;
    DeleteScheduler deleteScheduler;
};

/* config path: cli -> OM  */
class FDSP_ConfigPathReqHandler : virtual public fpi::FDSP_ConfigPathReqIf {
  public:
        explicit FDSP_ConfigPathReqHandler(OrchMgr *oMgr);

        int32_t CreateVol(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_CreateVolType& crt_vol_req);
        int32_t CreateVol(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_CreateVolTypePtr& crt_vol_req);

        int32_t DeleteVol(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_DeleteVolType& del_vol_req);
        int32_t DeleteVol(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_DeleteVolTypePtr& del_vol_req);

        int32_t ModifyVol(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_ModifyVolType& mod_vol_req);
        int32_t ModifyVol(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_ModifyVolTypePtr& mod_vol_req);

        int32_t SnapVol(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_CreateVolType& snap_vol_req);
        int32_t SnapVol(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_CreateVolTypePtr& snap_vol_req);

        int32_t CreatePolicy(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_CreatePolicyType& crt_pol_req);
        int32_t CreatePolicy(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_CreatePolicyTypePtr& crt_pol_req);

        int32_t DeletePolicy(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_DeletePolicyType& del_pol_req);
        int32_t DeletePolicy(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_DeletePolicyTypePtr& del_pol_req);

        int32_t ModifyPolicy(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_ModifyPolicyType& mod_pol_req);
        int32_t ModifyPolicy(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_ModifyPolicyTypePtr& mod_pol_req);

        int32_t AttachVol(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_AttachVolCmdType& atc_vol_req);
        int32_t AttachVol(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_AttachVolCmdTypePtr& atc_vol_req);

        int32_t DetachVol(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_AttachVolCmdType& dtc_vol_req);
        int32_t DetachVol(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_AttachVolCmdTypePtr& dtc_vol_req);

        int32_t AssociateRespCallback(const int64_t ident);
        int32_t AssociateRespCallback(boost::shared_ptr<int64_t>& ident); // NOLINT

        int32_t CreateDomain(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_CreateDomainType& crt_dom_req);
        int32_t CreateDomain(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_CreateDomainTypePtr& crt_dom_req);

        int32_t DeleteDomain(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_CreateDomainType& del_dom_req);
        int32_t DeleteDomain(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_CreateDomainTypePtr& del_dom_req);

        int32_t ShutdownDomain(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_ShutdownDomainType& shut_dom_req);
        int32_t ShutdownDomain(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_ShutdownDomainTypePtr& shut_dom_req);

        int32_t RemoveServices(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_RemoveServicesType& rm_svc_req);
        int32_t RemoveServices(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_RemoveServicesTypePtr& rm_svc_req);

        int32_t SetThrottleLevel(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_ThrottleMsgType& throttle_msg);
        int32_t SetThrottleLevel(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_ThrottleMsgTypePtr& throttle_msg);

        void GetVolInfo(
            ::FDS_ProtocolInterface::FDSP_VolumeDescType& _return,
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_GetVolInfoReqType& vol_info_req);
        void GetVolInfo(
            ::FDS_ProtocolInterface::FDSP_VolumeDescType& _return,
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_GetVolInfoReqTypePtr& vol_info_req);

        int32_t ActivateAllNodes(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_ActivateAllNodesType& act_node_msg);
        int32_t ActivateAllNodes(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_ActivateAllNodesTypePtr& act_node_msg);

        int32_t ActivateNode(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_ActivateOneNodeType& act_node_msg);
        int32_t ActivateNode(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_ActivateOneNodeTypePtr& act_node_msg);

        int32_t ScavengerCommand(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_ScavengerType& gc_msg);
        int32_t ScavengerCommand(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_ScavengerTypePtr& gc_msg);

        void ListServices(std::vector<fpi::FDSP_Node_Info_Type> & ret,
                          const fpi::FDSP_MsgHdrType& fdsp_msg);
        void ListServices(std::vector<fpi::FDSP_Node_Info_Type> & ret,
                          boost::shared_ptr<fpi::FDSP_MsgHdrType>& fdsp_msg);

        void ListVolumes(
            std::vector<FDS_ProtocolInterface::FDSP_VolumeDescType> & _return,
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg);
        void ListVolumes(
            std::vector<FDS_ProtocolInterface::FDSP_VolumeDescType> & _return,
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg);

  private:
    OrchMgr *orchMgr;
};

/* OM control path: SH/SM/DM to OM */
class FDSP_OMControlPathReqHandler : virtual public fpi::FDSP_OMControlPathReqIf,
        virtual public PlatNetSvcHandler {
  public:
        explicit FDSP_OMControlPathReqHandler(OrchMgr *oMgr);

        void RegisterNode(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_RegisterNodeType& reg_node_req);
        void RegisterNode(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_RegisterNodeTypePtr& reg_node_req);

        void migrationDone(
                boost::shared_ptr<fpi::AsyncHdr>& hdr,
                boost::shared_ptr<fpi::CtrlNotifyMigrationStatus>& status);

  private:
        OrchMgr* orchMgr;
};

std::thread* runConfigService(OrchMgr* om);
extern OrchMgr *gl_orch_mgr;

}  // namespace fds

#endif  // SOURCE_ORCH_MGR_INCLUDE_ORCHMGR_H_
