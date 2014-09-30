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
class MetaDatapathRespImpl;

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
     : Platform("Checker-Platform", FDSP_STOR_HVISOR,
                new DomainContainer("AM-Platform-NodeInv",
                                 NULL,
                                 new SmContainer(FDSP_STOR_HVISOR),
                                 new DmContainer(FDSP_STOR_HVISOR),
                                 new AmContainer(FDSP_STOR_HVISOR),
                                 new PmContainer(FDSP_STOR_HVISOR),
                                 new OmContainer(FDSP_STOR_HVISOR)),
                new DomainClusterMap("AM-Platform-ClusMap",
                                 NULL,
                                 new SmContainer(FDSP_STOR_HVISOR),
                                 new DmContainer(FDSP_STOR_HVISOR),
                                 new AmContainer(FDSP_STOR_HVISOR),
                                 new PmContainer(FDSP_STOR_HVISOR),
                                 new OmContainer(FDSP_STOR_HVISOR)),
                new DomainResources("AM-Resources"),
                NULL),
       cntrs_("Checker", g_cntrs_mgr.get())
     {}

    virtual ~BaseChecker() {}

    virtual void run_checker(const char *volume) = 0;
    virtual void mod_load_from_config();

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
 * LevelDBChecker module.  Performs the following
 * 1. Registers with OM and gets a DMT
 * 2. Get volumeID from somewhere -- use this to getDMTNodesForVolume
 * 3. Get primary DM uuid
 * 4. list_bucket with uuid/volid/volume name
 */
class LevelDBChecker : public BaseChecker {
 public:
    LevelDBChecker(const FdsConfigAccessor &conf_helper);
    LevelDBChecker();
    virtual ~LevelDBChecker();

    /* Module overrides */
    virtual void mod_startup() override;
    virtual void mod_shutdown() override;

    /* BaseChecker overrides */
    virtual void run_checker(const char *volume) override;

    fds_log* GetLog() { return g_fdslog;}
    void set_cfg_accessor(const FdsConfigAccessor &cfg_ac) { conf_helper_ = cfg_ac; }

 protected:
    void list_bucket(const NodeUuid &dm_node_id,
                     const fds_volid_t vol_id,
                     const std::string vol_name);

    FDS_ProtocolInterface::FDSP_QueryCatalogTypePtr
            query_catalog(const NodeUuid &dm_node_id,
                           const fds_volid_t vol_id,
                           const std::string vol_name,
                           const std::string blob_name);
    
    netMetaDataPathClientSession *get_metadatapath_session(const NodeUuid& node_id);

    FdsConfigAccessor conf_helper_;
    ClusterCommMgrPtr clust_comm_mgr_;

    concurrency::TaskStatus get_resp_monitor_;
    boost::shared_ptr<DatapathRespImpl> dp_resp_handler_;
    boost::shared_ptr<MetaDatapathRespImpl> md_resp_handler_;

    BlobInfoListType resp_vector;
    FDSP_BlobDigestType resp_digest;
    FDSP_BlobObjectList resp_obj_list;    
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
    virtual void proc_pre_startup(void) override;
    int run() override;
    void plf_load_node_data();

 private:
    BaseChecker *checker_;
    const char *volume;
};

}  // namespace fds
#endif
