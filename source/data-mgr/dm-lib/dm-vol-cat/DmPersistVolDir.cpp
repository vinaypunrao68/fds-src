/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <stdlib.h>
#include <linux/limits.h>

#include <string>
#include <limits>

#include <fds_process.h>
#include <fds_module.h>
#include <dm-platform.h>
#include <net/net-service.h>

#include <dm-vol-cat/DmPersistVolDir.h>

namespace fds {
const fds_uint64_t INVALID_BLOB_ID = 0;
const fds_uint32_t BLOB_META_INDEX = std::numeric_limits<fds_uint32_t>::max();

const BlobObjKey OP_TIMESTAMP_KEY(INVALID_BLOB_ID, 0);
const Record OP_TIMESTAMP_REC(reinterpret_cast<const char *>(&OP_TIMESTAMP_KEY),
        sizeof(BlobObjKey));

Error DmPersistVolDir::syncCatalog(const NodeUuid & dmUuid) {
    std::string destIP;
    if (NetMgr::ep_mgr_singleton()->ep_uuid_binding(dmUuid.toSvcUuid(), 0, 0, &destIP) < 0) {
        LOGERROR << "Failed to sync catalog: Failed to get IP address for destination DM "
                << std::hex << dmUuid.uuid_get_val() << std::dec;
        return ERR_NOT_FOUND;
    }

    const FdsRootDir* root = g_fdsprocess->proc_fdsroot();
    std::string snapDir = root->dir_user_repo_snap()
            + std::to_string(dmUuid.uuid_get_val()) + std::string("-tmpXXXXXX");
    // FdsRootDir::fds_mkdir(snapDir.c_str());

    char* tempdir = mkdtemp(const_cast<char*>(snapDir.c_str()));

    if (!tempdir) {
        LOGERROR << "unable to create a temp dir";
        return ERR_NOT_FOUND;
    }

    snapDir = tempdir;
    snapDir += "/";
    snapDir += getVolIdStr() + "_vcat.ldb";

    NodeAgent::pointer node = Platform::plf_dm_nodes()->agent_info(dmUuid);
    DmAgent::pointer dm = agt_cast_ptr<DmAgent>(node);
    const std::string destDir = dm->get_node_root() + "user-repo/dm-names/";
    const std::string rsyncCmd = "sshpass -p passwd rsync -r " + snapDir + " root@" +
            destIP + ":" + destDir;

    // make local copy of catalog
    std::string rmCmd = "rm -rf " + snapDir;
    int ret = std::system(rmCmd.c_str());

    Error rc = copyVolDir(snapDir);
    if (!rc.ok()) {
        LOGERROR << "Failed to copy volume catalog for sync. volume: '" << std::hex << volId_
                << std::dec << "'";
        return rc;
    }

    // rsync
    LOGTRACE << "rsync command: " << rsyncCmd;
    ret = std::system(rsyncCmd.c_str());

    if (ret) {
        LOGERROR << "rsync command failed '" << rsyncCmd << "', code: '" << ret << "'";
        return ERR_DM_RSYNC_FAILED;
    }


    // remove the temp dir
    ret = std::system(rmCmd.c_str());

    if (ret) {
        LOGERROR << "rm command failed '" << rmCmd << "', code: '" << ret << "'";
        return ERR_DM_RM_FAILED;
    }

    return rc;
}

}  // namespace fds
