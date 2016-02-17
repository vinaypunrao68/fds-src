/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <vector>

#include <fds_module.h>
#include <fds_volume.h>
#ifndef SOURCE_DATA_MGR_INCLUDE_DMUTIL_H_
#define SOURCE_DATA_MGR_INCLUDE_DMUTIL_H_
namespace fds {
namespace dmutil {
// location of volume
std::string getVolumeDir(const FdsRootDir* root,
                         fds_volid_t volId, fds_volid_t snapId = invalid_vol_id);
std::string getTempDir(const FdsRootDir* root = NULL, fds_volid_t volId = invalid_vol_id);
// location of all snapshots for a volume
std::string getSnapshotDir(const FdsRootDir* root, fds_volid_t volId);

// location of catalog journals
std::string getTimelineDir(const FdsRootDir* root, fds_volid_t volId);
std::string getVolumeMetaDir(const FdsRootDir* root, fds_volid_t volId);
std::string getLevelDBFile(const FdsRootDir* root, fds_volid_t volId, fds_volid_t snapId = invalid_vol_id);

/**
* @brief Returns list of volume id in dm catalog under FdsRootDir root
*
* @param root
* @param vecVolumes
*/
void getVolumeIds(const FdsRootDir* root, std::vector<fds_volid_t>& vecVolumes);

std::string getTimelineDBPath(const FdsRootDir* root);
}  // namespace dmutil
}  // namespace fds
#endif  // SOURCE_DATA_MGR_INCLUDE_DMUTIL_H_
