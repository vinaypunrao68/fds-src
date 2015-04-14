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
#include "platform/node_agent.h"
#include <net/SvcMgr.h>
#include <net/net_utils.h>

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

    DeleteScheduler deleteScheduler;

    fds_bool_t enableSnapshotSchedule;
    boost::shared_ptr<fds::snapshot::Manager> snapshotMgr;
};

/* config path: cli -> OM  */
class FDSP_ConfigPathReqHandler : virtual public fpi::FDSP_ConfigPathReqIf {
  public:
        explicit FDSP_ConfigPathReqHandler(OrchMgr *oMgr);

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

void add_to_vector(std::vector<fpi::FDSP_Node_Info_Type> &vec, NodeAgent::pointer ptr);

std::thread* runConfigService(OrchMgr* om);
extern OrchMgr *gl_orch_mgr;

}  // namespace fds

#endif  // SOURCE_ORCH_MGR_INCLUDE_ORCHMGR_H_
