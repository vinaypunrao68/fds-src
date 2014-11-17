/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <fds_process.h>
#include <platform/platform-lib.h>
#include <object-store/SmDiskMap.h>
#include <sys/statvfs.h>
#include <utility>

namespace fds {

SmDiskMap::SmDiskMap(const std::string& modName)
        : Module(modName.c_str()),
          bitsPerToken_(0),
          superblock(new SmSuperblockMgr()),
          capacityMap(nullptr),
          test_mode(false) {
}

SmDiskMap::~SmDiskMap() {
}

int SmDiskMap::mod_init(SysParams const *const param) {
    Module::mod_init(param);
    test_mode = g_fdsprocess->get_fds_config()->get<bool>("fds.sm.testing.standalone");

    // get list of HDD and SSD devices
    getDiskMap();

    // Create and populate the capcity map
    // TODO(brian): See if we want to move this into getDiskMap for performance
    capacityMap = new std::unordered_map<fds_uint16_t,
            std::pair<fds_uint64_t, fds_uint64_t>>();

    for (auto disk_id : ssd_ids) {
        struct statvfs statbuf;
        std::string diskPath = getDiskPath(disk_id);
        if (statvfs(diskPath.c_str(), &statbuf) < 0) {
            LOGERROR << "Could not read disk " << diskPath;
        }
        // Populate the capacityMap for this diskID
        capacityMap->emplace(disk_id,
                std::make_pair((statbuf.f_bfree * statbuf.f_bsize),
                        (statbuf.f_blocks * statbuf.f_frsize)));
    }

    // we are not going to read superblock
    // until we get our first DLT; when we get DLT
    // SM knows which tokens it owns
    return 0;
}

fds_bool_t SmDiskMap::ssdTrackCapacityAdd(ObjectID oid,
        fds_uint64_t writeSize, fds_uint32_t fullThreshold) {
    fds_uint16_t diskId = getDiskId(oid, diskio::flashTier);
    // Get the capacity information for this disk
    std::pair<fds_uint64_t, fds_uint64_t>ssdCap = capacityMap->at(diskId);
    // Add the writeSize
    ssdCap.first += writeSize;
    // Check if we're over threshold now
    if (ssdCap.first > (ssdCap.second * (fullThreshold / 100))) {
        return false;
    }
    return true;
}

void SmDiskMap::ssdTrackCapacityDelete(ObjectID oid, fds_uint64_t writeSize) {
    fds_uint16_t diskId = getDiskId(oid, diskio::flashTier);
    // Get the capacity information for this disk
    capacityMap->at(diskId).first -= writeSize;
}

void SmDiskMap::mod_startup() {
}

void SmDiskMap::mod_shutdown() {
}

void SmDiskMap::getDiskMap() {
    int           idx;
    fds_uint64_t  uuid;
    std::string   path, dev;

    const FdsRootDir *dir = g_fdsprocess->proc_fdsroot();
    std::ifstream map(dir->dir_dev() + std::string("/disk-map"), std::ifstream::in);

    fds_verify(map.fail() == false);
    while (!map.eof()) {
        map >> dev >> idx >> std::hex >> uuid >> std::dec >> path;
        if (map.fail()) {
            break;
        }
        LOGNORMAL << "dev " << dev << ", path " << path << ", uuid " << uuid
                  << ", idx " << idx;
        if (strstr(path.c_str(), "hdd") != NULL) {
            fds_verify(hdd_ids.count(idx) == 0);
            hdd_ids.insert(idx);
        } else if (strstr(path.c_str(), "ssd") != NULL) {
            fds_verify(ssd_ids.count(idx) == 0);
            ssd_ids.insert(idx);
        } else {
            fds_panic("Unknown path: %s\n", path.c_str());
        }
        fds_verify(disk_map.count(idx) == 0);
        disk_map[idx] = path;
    }
    if (disk_map.size() == 0) {
        fds_panic("Can't find any devices\n");
    }
}

Error SmDiskMap::handleNewDlt(const DLT* dlt) {
    Error err(ERR_OK);
    // get list of DLT tokens that this SM is responsible for
    // according to the DLT
    NodeUuid mySvcUuid;
    if (!test_mode) {
        mySvcUuid = *(Platform::plf_get_my_svc_uuid());
    } else {
        mySvcUuid = 1;
    }
    const TokenList& dlt_toks = dlt->getTokens(mySvcUuid);
    // if there are no DLT tokens that belong to this SM
    // we don't care about this DLT
    if (dlt_toks.size() == 0) {
        LOGWARN << "DLT does not contain any tokens owned by this SM";
        return ERR_INVALID_DLT;
    }

    // see if this is the first DLT we got
    fds_bool_t first_dlt = true;
    if (bitsPerToken_ == 0) {
        bitsPerToken_ = dlt->getNumBitsForToken();
    } else {
        // we already got DLT and set initial disk map
        first_dlt = false;

        // dlt width should not change
        fds_verify(bitsPerToken_ == dlt->getNumBitsForToken());
    }
    LOGDEBUG << "Will handle new DLT, bits per token " << bitsPerToken_;
    LOGTRACE << *dlt;

    // get list of SM tokens that this SM is responsible for
    SmTokenSet sm_toks;
    for (TokenList::const_iterator cit = dlt_toks.cbegin();
         cit != dlt_toks.cend();
         ++cit) {
        sm_toks.insert(smTokenId(*cit));
    }

    if (first_dlt) {
        // tell superblock about existing disks and SM tokens
        // in this first implementation once we reported initial
        // set of disks and SM tokens, they are not expected to
        // change -- we need to implement handling changes in disks
        // and SM tokens
        err = superblock->loadSuperblock(hdd_ids, ssd_ids, disk_map, sm_toks);
        LOGDEBUG << "Loaded superblock " << err << " " << *superblock;
    } else {
        // TODO(Anna) before beta, we are supporting 4-node cluster, so
        // expecting no changes in SM owned tokens; will have to handle this
        // properly by 1.0
        // so make sure token ownership did not change
        SmTokenSet ownToks = superblock->getSmOwnedTokens();
        fds_verify(ownToks == sm_toks);
        LOGNOTIFY << "No change in SM tokens or disk map";
        return ERR_DUPLICATE;  // to indicate we already set disk map/superblock
    }

    return err;
}

fds_token_id
SmDiskMap::smTokenId(fds_token_id tokId) {
    return tokId & SMTOKEN_MASK;
}

fds_token_id
SmDiskMap::smTokenId(const ObjectID& objId) const {
    fds_token_id tokId = DLT::getToken(objId, bitsPerToken_);
    return tokId & SMTOKEN_MASK;
}

fds_token_id
SmDiskMap::smTokenId(const ObjectID& objId,
                     fds_uint32_t bitsPerToken) {
    fds_token_id tokId = DLT::getToken(objId, bitsPerToken);
    return tokId & SMTOKEN_MASK;
}

SmTokenSet
SmDiskMap::getSmTokens() const {
    return superblock->getSmOwnedTokens();
}

SmTokenSet
SmDiskMap::getSmTokens(fds_uint16_t diskId) const {
    return superblock->getSmOwnedTokens(diskId);
}

fds_uint16_t
SmDiskMap::getDiskId(const ObjectID& objId,
                     diskio::DataTier tier) const {
    fds_token_id smTokId = smTokenId(objId);
    return superblock->getDiskId(smTokId, tier);
}

fds_uint16_t
SmDiskMap::getDiskId(fds_token_id smTokId,
                     diskio::DataTier tier) const {
    return superblock->getDiskId(smTokId, tier);
}

std::string
SmDiskMap::getDiskPath(fds_token_id smTokId,
                       diskio::DataTier tier) const {
    fds_uint16_t diskId = superblock->getDiskId(smTokId, tier);
    fds_verify(disk_map.count(diskId) > 0);
    return disk_map.at(diskId);
}

std::string
SmDiskMap::getDiskPath(fds_uint16_t diskId) const {
    fds_verify(disk_map.count(diskId) > 0);
    return disk_map.at(diskId);
}

DiskIdSet
SmDiskMap::getDiskIds(diskio::DataTier tier) const {
    DiskIdSet diskIds;
    if (tier == diskio::diskTier) {
        diskIds.insert(hdd_ids.begin(), hdd_ids.end());
    } else if (tier == diskio::flashTier) {
        diskIds.insert(ssd_ids.begin(), ssd_ids.end());
    } else {
        fds_panic("Unknown tier request from SM disk map\n");
    }
    return diskIds;
}

}  // namespace fds
