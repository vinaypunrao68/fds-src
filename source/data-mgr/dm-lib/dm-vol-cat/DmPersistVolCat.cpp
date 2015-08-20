/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

// Standard includes.
#include <cstdlib>
#include <limits>
#include <string>

// System includes.
#include <linux/limits.h>

// Internal includes.
#include "dm-vol-cat/DmPersistVolCat.h"
#include "leveldb/db.h"
#include "net/SvcMgr.h"
#include "fds_process.h"
#include "fds_module.h"
#include "fds_resource.h"

namespace fds {

Error DmPersistVolCat::syncCatalog(const NodeUuid & dmUuid) {
    const fpi::SvcUuid & dmSvcUuid = dmUuid.toSvcUuid();
    auto svcmgr = MODULEPROVIDER()->getSvcMgr();

    fpi::SvcInfo dmSvcInfo;
    if (!svcmgr->getSvcInfo(dmSvcUuid, dmSvcInfo)) {
        LOGERROR << "Failed to sync catalog: Failed to get IP address for destination DM "
                << std::hex << dmSvcUuid.svc_uuid << std::dec;
        return ERR_NOT_FOUND;
    }
    std::string destIP = dmSvcInfo.ip;

    // Get rsync username and passwd
    FdsConfigAccessor migrationConf(g_fdsprocess->get_fds_config(), "fds.dm.migration.");   
    rsyncUser   = migrationConf.get<std::string>("rsync_username");
    rsyncPasswd = migrationConf.get<std::string>("rsync_password");

    const FdsRootDir* root = g_fdsprocess->proc_fdsroot();
    std::string snapDir = root->dir_sys_repo_dm() + getVolIdStr() + "/"
            + std::to_string(dmUuid.uuid_get_val()) + std::string("-tmpXXXXXX");
    // FdsRootDir::fds_mkdir(snapDir.c_str());

    char* tempdir = mkdtemp(const_cast<char*>(snapDir.c_str()));

    if (!tempdir) {
        LOGERROR << "unable to create a temp directory with error " << errno;
        return ERR_NOT_FOUND;
    }

    snapDir = tempdir;
    snapDir += "/";
    snapDir += getVolIdStr() + "_vcat.ldb";

    std::string nodeRoot = svcmgr->getSvcProperty<std::string>(
        SvcMgr::mapToSvcUuid(dmSvcUuid, fpi::FDSP_PLATFORM),
        "fds_root");
    const std::string destDir = nodeRoot + "sys-repo/dm-names/" + getVolIdStr() + "/";
    const std::string rsyncCmd = "sshpass -p " + rsyncPasswd + " rsync -r --rsh='ssh -o StrictHostKeyChecking=no' " + snapDir +
            " " + rsyncUser + "@" + destIP + ":" + destDir;

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
    LOGMIGRATE << "Migrating catalog " << getVolIdStr() << " with rsync";
    ret = std::system(rsyncCmd.c_str());
    if (ret) {
        LOGERROR << "rsync command failed '" << rsyncCmd << "', code: '" << ret << "'";
        return ERR_DM_RSYNC_FAILED;
    }
    LOGMIGRATE << "Migrated catalog " << getVolIdStr() << " successfully";

    // remove the temp directory
    ret = std::system(rmCmd.c_str());
    if (ret) {
        LOGWARN << "rm command failed '" << rmCmd << "', code: '" << ret << "'";
    }

    return rc;
}

}  // namespace fds
