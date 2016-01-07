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
#include <OMMonitorWellKnownPMs.h>
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
#define DOMAINRESTART_MASK      (1)

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

typedef std::pair<int64_t, int32_t> PmMsg;

class OM_Module;

class OrchMgr: public SvcProcess {
  private:

    /* net session tbl for OM control path*/
    boost::shared_ptr<netSessionTbl> omcp_session_tbl;
    std::string                      my_node_name;
    /* config path server is run on this thread */
    boost::shared_ptr<std::thread>   cfgserver_thread;
    /* svc start message monitoring done on these threads */
    boost::shared_ptr<std::thread>   svcStartThread;
    boost::shared_ptr<std::thread>   svcStartRetryThread;
    fds_mutex                        *om_mutex;
    std::string                      node_id_to_name[MAX_OM_NODES];

    /*
     * Command Line configurable
     */
    int         conf_port_num; /* config port to listen for cli commands */
    int         ctrl_port_num; /* control port (register node + config cmds from AM) */
    std::string stor_prefix;
    fds_bool_t  test_mode;

    /* policy manager */
    VolPolicyMgr                    *policy_mgr;
    /**
     * @details
     * Must always point to the same address after initial assignment!
     * Other objects, such as service handlers, will refer to this address.
     * TODO: Would prefer to declare the address as const, can not do so
     * without refactoring of SvcProcess::setupConfigDb_().
     */
    kvstore::ConfigDB               *configDB;


    /* Monitoring start messages sent to the PM */
    std::mutex                      toSendQMutex; // protect both toSendQ and retryMap
    std::vector<PmMsg>              toSendMsgQueue;
    std::condition_variable         toSendQCondition;
    std::map<int64_t, int32_t>      retryMap;

    std::mutex                      sentQMutex; // protect sentQ
    std::vector<PmMsg>              sentMsgQueue;
    std::condition_variable         sentQCondition;


  protected:
    virtual void setupSvcInfo_() override;
    /**
     * @details
     * Call once only. External service handler objects are known to
     * cache the configDB address.
     */
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

    bool                     loadFromConfigDB();
    // default  policy  desc  for s3 bucket
    void                     defaultS3BucketPolicy();
    DmtColumnPtr             getDMTNodesForVolume(fds_volid_t volId);
    kvstore::ConfigDB*       getConfigDB();

    static VolPolicyMgr      *om_policy_mgr();
    static const std::string &om_stor_prefix();

    // Methods related to monitoring/retrying start messages sent to the PM
    void                     svcStartMonitor();
    void                     svcStartRetryMonitor();
    void                     constructMsgParams(int64_t uuid,
                                                NodeUuid& node_uuid,
                                                bool& flag);
    void                     addToSendQ(PmMsg msg, bool retry);
    void                     addToSentQ(PmMsg msg);
    bool                     isInSentQ(int64_t uuid);
    void                     removeFromSentQ(PmMsg msg);

    std::unique_ptr<OMMonitorWellKnownPMs> omMonitor;
    DeleteScheduler deleteScheduler;
    fds_bool_t      enableTimeline;
    boost::shared_ptr<fds::snapshot::Manager> snapshotMgr;
};

std::thread* runConfigService(OrchMgr* om);
extern OrchMgr *gl_orch_mgr;

}  // namespace fds

#endif  // SOURCE_ORCH_MGR_INCLUDE_ORCHMGR_H_
