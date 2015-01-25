/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATAMGR_UTILS_DMCHK_H_
#define SOURCE_DATAMGR_UTILS_DMCHK_H_

/**
 * This checker is meant to eventually evolve into a DM catalog
 * inspector/checker/fixer. An fsck for DM, of sorts. Today, the
 * the checker only looks at specific DM catalogs and doesn't
 * actually know much about the DM data layout or how to fix anything.
 */

#include <string>
#include <fds_process.h>
#include <dm-vol-cat/DmVolumeDirectory.h>

namespace fds {

#define DM_CATALOG_TYPE DmVolumeDirectory

class DmChecker : public FdsProcess {
  public:
    /// TODO(Andrew): The checker is only taking a single
    /// catalog path today
    DmChecker(int argc,
              char *argv[],
              const std::string & config,
              const std::string & basePath,
              Module *vec[],
              const std::string &moduleName,
              fds_volid_t volumeUuid);
    virtual ~DmChecker() = default;

    virtual void proc_pre_startup() override {
        FdsProcess::proc_pre_startup();
    }
    virtual int run() override {
        return 0;
    }

    void listBlobs();

  private:
    boost::shared_ptr<VolumeDesc> volDesc;
    boost::shared_ptr<DM_CATALOG_TYPE> volCat;
};

}  // Namespace fds

#endif  // SOURCE_DATAMGR_UTILS_DMCHK_H_
