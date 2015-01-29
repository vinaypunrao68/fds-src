/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <fds_process.h>
#include <object-store/SmDiskMap.h>
#include <sys/statvfs.h>
#include <utility>

#include "platform/platform_consts.h"
#include "platform/platform.h"

namespace fds {

SmDiskMap::SmDiskMap(const std::string& modName)
        : Module(modName.c_str()),
          bitsPerToken_(0),
          superblock(new SmSuperblockMgr()),
          ssdIdxMap(nullptr),
          test_mode(false) {
    for (int i = 0; i < 60; ++i) {
        maxSSDCapacity[i] = ATOMIC_VAR_INIT(0ull);
        consumedSSDCapacity[i] = ATOMIC_VAR_INIT(0ull);
    }
}

SmDiskMap::~SmDiskMap() {
}

int SmDiskMap::mod_init(SysParams const *const param) {
    Module::mod_init(param);
    test_mode = g_fdsprocess->get_fds_config()->get<bool>("fds.sm.testing.standalone");

    // get list of HDD and SSD devices
    getDiskMap();

    // Create mapping from ssd idx # to array idx #
    ssdIdxMap = new std::unordered_map<fds_uint16_t, fds_uint8_t>();
    fds_uint8_t arrIdx = 0;
    for (auto disk_id : ssd_ids) {
        struct statvfs statbuf;
        std::string diskPath = getDiskPath(disk_id);
        if (statvfs(diskPath.c_str(), &statbuf) < 0) {
            LOGERROR << "Could not read disk " << diskPath;
        }
        // Add the mapping
        ssdIdxMap->emplace(disk_id, arrIdx);

        // Populate the capacityMap for this diskID
        fds_uint64_t totalSize = statbuf.f_blocks * statbuf.f_frsize;
        fds_uint64_t consumedSize = totalSize - (statbuf.f_bfree * statbuf.f_bsize);
        maxSSDCapacity[arrIdx] = totalSize;
        consumedSSDCapacity[arrIdx] = consumedSize;
        // Now increase arrIdx
        ++arrIdx;
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
    fds_uint8_t arrId = ssdIdxMap->at(diskId);
    // Check if we're over threshold now
    fds_uint64_t newConsumed = consumedSSDCapacity[arrId] + writeSize;
    fds_uint64_t capThresh = maxSSDCapacity[arrId] * (fullThreshold / 100.);
    if (newConsumed > capThresh) {
        LOGDEBUG << "SSD write would exceed full threshold: Threshold: "
                    << capThresh << " current usage: " << consumedSSDCapacity[arrId]
                    << " of " << maxSSDCapacity[arrId] << " Write size: " << writeSize;
        return false;
    }
    // Add the writeSize
    consumedSSDCapacity[arrId] += writeSize;
    LOGDEBUG << "new SSD utilized cap for diskID[" << diskId << "]: " << consumedSSDCapacity[arrId];
    return true;
}

void SmDiskMap::ssdTrackCapacityDelete(ObjectID oid, fds_uint64_t writeSize) {
    fds_uint16_t diskId = getDiskId(oid, diskio::flashTier);
    fds_uint8_t arrId = ssdIdxMap->at(diskId);
    // Get the capacity information for this disk
    consumedSSDCapacity[arrId] -= writeSize;
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
    std::ifstream map(dir->dir_dev() + DISK_MAP_FILE, std::ifstream::in);

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

    // TODO(Anna) get list of SM tokens that this SM is responsible for
    // this is code below. For now commenting out and pretending SM is
    // responsible for all SM tokens. The behavior is correct, except not
    // very efficient -- we will have to open levelDBs for tokens that we
    // know SM will not receive any data, GC will try to run for these
    // tokens (but it will find out that there is nothing to GC), etc.
    // We have to implement actual token ownership after beta 2
    SmTokenSet sm_toks;
    /*
     * this is code we want to have, but need to implement updating
     * superblock and other SM state when we gain or lose ownership
     * of an SM token...
    for (TokenList::const_iterator cit = dlt_toks.cbegin();
         cit != dlt_toks.cend();
         ++cit) {
        sm_toks.insert(smTokenId(*cit));
    }
    */
    for (fds_token_id tokId = 0; tokId < SMTOKEN_COUNT; ++tokId) {
        sm_toks.insert(tokId);
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
        // TODO(Anna) implement updating sm tokens in superblock when
        // SM token ownership changes; for now it is correct if SM loses
        // tokens but superblock still thinks we have them (GC maybe less
        // efficient, but still correct). However, we will still assert if
        // SM gains tokens...
        // make sure that new SM tokens are subset of what superblock thinks
        // are SM's owned tokens
        SmTokenSet ownToks = superblock->getSmOwnedTokens();
        fds_bool_t newToksSubsetOfOldToks = std::includes(ownToks.begin(),
                                                          ownToks.end(),
                                                          sm_toks.begin(),
                                                          sm_toks.end());
        fds_verify(newToksSubsetOfOldToks);
        LOGNOTIFY << "New SM tokens are same or a subset of old SM tokens "
                  << "owned by this SM. # of previous SM tokens "
                  << ownToks.size() << ", # of new SM tokens " << sm_toks.size();
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

fds_uint32_t
SmDiskMap::getTotalDisks() const {
    return (hdd_ids.size() + ssd_ids.size());
}

}  // namespace fds
