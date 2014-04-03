/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_CHECKER_H_
#define SOURCE_INCLUDE_CHECKER_H_

#include <fds_module.h>
#include <fds_process.h>
#include <platform/platform-lib.h>
#include <ClusterCommMgr.h>
#include <NetSession.h>
#include <concurrency/taskstatus.h>
#include <fdsp/FDSP_types.h>

namespace fds {

class DatapathRespImpl;

/**
 * @brief Storage manager counters
 */
class CheckerCntrs : public FdsCounters
{
 public:
    CheckerCntrs(const std::string &id, FdsCountersMgr *mgr)
      : FdsCounters(id, mgr),
        obj_cnt("obj_cnt", this),
        fault_cnt("fault_cnt", this)
  {
  }

  /* Number of objects checked */
  NumericCounter obj_cnt;
  /* Number of faults */
  NumericCounter fault_cnt;
};

/**
 * Base checker class
 */
class BaseChecker : public Platform {
 public:
    BaseChecker(char const *const name)
     : Platform("Checker-Platform", FDSP_STOR_HVISOR, NULL, NULL, NULL, NULL),
       cntrs_("Checker", g_cntrs_mgr.get()),
       panic_on_corruption_(true)
     {}
    virtual ~BaseChecker() {}

    virtual void run_checker() = 0;

    virtual void log_corruption(const std::string& info);
    virtual void compare_against(
            FDSP_MigrateObjectMetadata& golden,
            std::vector<FDSP_MigrateObjectMetadata> md_list);
 protected:
    CheckerCntrs cntrs_;
    bool panic_on_corruption_;

};
typedef boost::shared_ptr<BaseChecker> BaseCheckerPtr;

/**
 * Directory base checker module.  Performs the following
 * 1. Registers with OM and gets a dlt
 * 2. From directory specified in the config, for each file in the directory calculates
 * the object id.  Based on the object id gets the token group from DLT.
 * 3. Issues GetObject request from SMs and ensures the file data matches the data
 * read from SM.
 */
class DirBasedChecker : public BaseChecker {
 public:
    DirBasedChecker(const FdsConfigAccessor &conf_helper);
    DirBasedChecker();
    virtual ~DirBasedChecker();

    /* Module overrides */
    virtual void mod_startup() override;
    virtual void mod_shutdown() override;

    /* BaseChecker overrides */
    virtual void run_checker() override;

    fds_log* GetLog() { return g_fdslog;}
    void set_cfg_accessor(const FdsConfigAccessor &cfg_ac) { conf_helper_ = cfg_ac; }

 protected:
    bool read_file(const std::string &filename, std::string &content);
    bool get_object(const NodeUuid& node_id,
            const ObjectID &oid, std::string &content);
    bool get_object_metadata(const NodeUuid& node_id,
            const ObjectID &oid, FDSP_MigrateObjectMetadata &md);
    netDataPathClientSession* get_datapath_session(const NodeUuid& node_id);

    virtual PlatRpcReqt *plat_creat_reqt_disp();
    virtual PlatRpcResp *plat_creat_resp_disp();

    FdsConfigAccessor conf_helper_;
    ClusterCommMgrPtr clust_comm_mgr_;
    concurrency::TaskStatus get_resp_monitor_;
    boost::shared_ptr<DatapathRespImpl> dp_resp_handler_;
};

/**
 * Checker process
 */
class FdsCheckerProc : public PlatformProcess {
 public:
    FdsCheckerProc(int argc, char *argv[],
                   const std::string &config_file,
                   const std::string &base_path,
                   Platform *platform,
                   Module **mod_vec);
    virtual ~FdsCheckerProc();
    virtual void setup(void) override;
    void run() override;
    void plf_load_node_data();
 private:
    BaseCheckerPtr checker_;
};

}  // namespace fds
#endif
