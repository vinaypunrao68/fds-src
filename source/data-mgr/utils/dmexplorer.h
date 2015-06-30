/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATAMGR_UTILS_DMEXPLORER_H_
#define SOURCE_DATAMGR_UTILS_DMEXPLORER_H_

/**
 * This checker is meant to eventually evolve into a DM catalog
 * inspector/checker/fixer. An fsck for DM, of sorts. Today, the
 * the checker only looks at specific DM catalogs and doesn't
 * actually know much about the DM data layout or how to fix anything.
 */

#include <string>
#include <fds_process.h>
#include <dm-vol-cat/DmVolumeCatalog.h>

namespace fds {

class DMExplorer : public FdsProcess {
  public:
    /// TODO(Andrew): The checker is only taking a single
    /// catalog path today
    DMExplorer(int argc,
              char *argv[],
              const std::string & config,
              const std::string & basePath,
              Module *vec[],
              const std::string &moduleName);
    virtual ~DMExplorer() = default;

    virtual void proc_pre_startup() override {
        FdsProcess::proc_pre_startup();
    }
    virtual int run() override {
        return 0;
    }

    Error loadVolume(fds_volid_t volumeUuid);
    Error listBlobs(bool fStatsOnly = false);
    Error listBlobsWithObject(std::string strObjId);

    void blobInfo(const std::string& blobname);
    void getVolumeIds(std::vector<fds_volid_t>& vecVolumes);
  private:
    boost::shared_ptr<VolumeDesc> volDesc;
    boost::shared_ptr<DmVolumeCatalog> volCat;
};

}  // Namespace fds

#endif  // SOURCE_DATAMGR_UTILS_DMEXPLORER_H_
