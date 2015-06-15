/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <stdlib.h>
#include <linux/limits.h>

#include <string>
#include <limits>

#include <fds_process.h>
#include <fds_module.h>

#include <net/SvcMgr.h>
#include <dm-vol-cat/DmPersistVolCat.h>

namespace fds {
const fds_uint64_t INVALID_BLOB_ID = 0;
/**
 * Index that identifies a catalog entry as describing a blob's metadata.
 * TODO(Andrew): If we're going to use an index that's in range of possible
 * use then we should enforce a max blob size that's less than this.
 */
const fds_uint32_t BLOB_META_INDEX = std::numeric_limits<fds_uint32_t>::max();
/**
 * Key that identifies a catalog entry as describing a volume's metadata.
 * The key follows a different format than blob entries, so shouldn't conflict,
 * but is just an arbitrary string so there's technically nothing preventing
 * collision.
 */
const fds_uint64_t VOL_META_ID = std::numeric_limits<fds_uint64_t>::max();

const BlobObjKey OP_TIMESTAMP_KEY(INVALID_BLOB_ID, 0);
const Record OP_TIMESTAMP_REC(reinterpret_cast<const char *>(&OP_TIMESTAMP_KEY),
        sizeof(BlobObjKey));

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
