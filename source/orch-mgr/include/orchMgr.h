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
#include <fdsp/sm_types_types.h>
#include <fds_typedefs.h>
#include <util/Log.h>
#include <concurrency/Mutex.h>
#include <omutils.h>
#include <OmVolume.h>
#include "OMMonitorWellKnownPMs.h"
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

    /* net session tbl for OM control path*/
    boost::shared_ptr<netSessionTbl> omcp_session_tbl;
    std::string my_node_name;

    /* config path server is run on this thread */
    boost::shared_ptr<std::thread> cfgserver_thread;



    fds_mutex *om_mutex;
    std::string node_id_to_name[MAX_OM_NODES];

    /*
     * Command Line configurable
     */
    int conf_port_num; /* config port to listen for cli commands */
    int ctrl_port_num; /* control port (register node + config cmds from AM) */
    std::string stor_prefix;
    fds_bool_t test_mode;

    /* policy manager */
    VolPolicyMgr           *policy_mgr;
    kvstore::ConfigDB      *configDB;
    OMMonitorWellKnownPMs  *omMonitor;

  protected:
    virtual void setupSvcInfo_() override;
    virtual void setupConfigDb_() override;

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
    static OMMonitorWellKnownPMs*    om_monitor();

    DeleteScheduler deleteScheduler;

    fds_bool_t enableSnapshotSchedule;
    boost::shared_ptr<fds::snapshot::Manager> snapshotMgr;
};

std::thread* runConfigService(OrchMgr* om);
extern OrchMgr *gl_orch_mgr;

}  // namespace fds

#endif  // SOURCE_ORCH_MGR_INCLUDE_ORCHMGR_H_
