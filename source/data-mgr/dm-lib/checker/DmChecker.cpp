/* Copyright 2015 Formation Data Systems, Inc.
 */
#include <sys/stat.h>
#include <list>
#include <vector>
#include <string>
#include <boost/lexical_cast.hpp>
#include <checker/DmChecker.h>
#include <net/SvcMgr.h>
#include <fds_dmt.h>
#include <checker/LeveldbDiffer.h>
#include <dm-vol-cat/DmPersistVolDB.h>
#include <DmBlobTypes.h>
#include <util/stringutils.h>
#include <fdsp/ConfigurationService.h>

namespace fds {

DMOfflineCheckerEnv::DMOfflineCheckerEnv(int argc, char *argv[])
 : SvcProcess(argc, argv, "platform.conf", "fds.test.", "checker.log", nullptr) {
}

int DMOfflineCheckerEnv::main() {
    /* To force registration with OM set standalone to false */
    auto config = get_fds_config();
    config->set("fds.test.testing.standalone", false);

    /* Go through normal startup sequence where we register with OM */
    start_modules();

    /* Get volumes from config api */
    auto configSvc = allocRpcClient<fds::apis::ConfigurationServiceClient>(
        "127.0.0.1", 9090, 4);
    std::vector<fds::apis::VolumeDescriptor> volDescs;
    configSvc->listVolumes(volDescs, "");
    for_each(volDescs.begin(), volDescs.end(),
             [this](const fds::apis::VolumeDescriptor& d) {
             volumeList.push_back(static_cast<fds_volid_t>(d.volId));
             });

    /* Get DMT */
    svcMgr_->getDMT();
    return 0;
}

std::list<fds_volid_t> DMOfflineCheckerEnv::getVolumeIds() const {
    return volumeList;
}

uint32_t DMOfflineCheckerEnv::getReplicaCount(const fds_volid_t &volId) const {
    return svcMgr_->getDMTNodesForVolume(volId)->getLength();
}

std::string DMOfflineCheckerEnv::getCatalogPath(const fds_volid_t &volId,
                                                const uint32_t &replicaIdx) const {
    auto dmSvcUuid = svcMgr_->getDMTNodesForVolume(volId)->get(replicaIdx).toSvcUuid();
    std::string nodeRoot = svcMgr_->getSvcProperty<std::string>(
        SvcMgr::mapToSvcUuid(dmSvcUuid, fpi::FDSP_PLATFORM),
        "fds_root");
    std::string path;
    if (nodeRoot[nodeRoot.size()-1] == '/') {
        path = util::strformat("%ssys-repo/dm-names/%ld/%ld_vcat.ldb",
                               nodeRoot.c_str(), volId, volId);
    } else {
        path = util::strformat("%s/sys-repo/dm-names/%ld/%ld_vcat.ldb",
                               nodeRoot.c_str(), volId, volId);
    }

    struct stat buffer;   
    if (stat(path.c_str(), &buffer) == 0) {
        return path;
    } else {
        return "";
    }

}

/**
* @brief Level db adpater specific to DmPersistVolDB used in diffing.
* @see LevelDbDiffAdapter
*/
struct DmPersistVolDBDiffAdapter : LevelDbDiffAdapter {
    int compareKeys(leveldb::Iterator *itr1, leveldb::Iterator *itr2) const override {
        return comparator.Compare(itr1->key(), itr2->key());
    }

    bool compareValues(leveldb::Iterator *itr1,
                      leveldb::Iterator *itr2,
                      std::string &mismatchDetails) const override {
        /* Note this call assumes keys are equal */

        /* For now physical diff should be enough */
        if (isTimestampEntry(itr1)) {
            /* Ignore timestamp entry */
            return true;
        }
        return itr1->value() == itr2->value();

#if 0   /* Enable if logical diff is needed */
        bool ret = false;

        if (isTimestampEntry(itr1)) {
            /* Ignore timestamp entry */
            ret = true;
        } else if (isVolumeDescriptor(itr1)) {
            VolumeMetaDesc volDesc1 = VolumeMetaDesc(fpi::FDSP_MetaDataList());
            auto status = volDesc1.loadSerialized(itr1->value().ToString());
            fds_verify(status == ERR_OK);

            VolumeMetaDesc volDesc2 = VolumeMetaDesc(fpi::FDSP_MetaDataList());
            status = volDesc2.loadSerialized(itr2->value().ToString());
            fds_verify(status == ERR_OK);
            ret = (volDesc1 == volDesc2);
        } else if (isBlobDescriptor(itr1)) {
            BlobMetaDesc blobDesc1;
            auto status = blobDesc1.loadSerialized(itr1->value().ToString());
            fds_verify(status == ERR_OK);

            BlobMetaDesc blobDesc2;
            status = blobDesc2.loadSerialized(itr2->value().ToString());
            fds_verify(status == ERR_OK);
            ret = (blobDesc1 == blobDesc2);
        } else {
            /* For now physical diff should be enough */
            ret = (itr1->value() == itr2->value());
        }
        return ret;
#endif
    }

    std::string keyAsString(leveldb::Iterator *itr) const override { 
        const BlobObjKey *key = reinterpret_cast<const BlobObjKey *>(itr->key().data());
        std::stringstream ss;
        ss << key->blobId << ":" << key->objIndex;
        return ss.str();
    }

    leveldb::Comparator* getComparator() override {
        return &comparator;
    }

    static bool isTimestampEntry(leveldb::Iterator *itr) {
        const BlobObjKey *key = reinterpret_cast<const BlobObjKey *>(itr->key().data());
        return key->objIndex == 0 && key->blobId == 0;
    }
    static bool isVolumeDescriptor(leveldb::Iterator *itr) {
        const BlobObjKey *key = reinterpret_cast<const BlobObjKey *>(itr->key().data());
        return key->blobId == VOL_META_ID;
    }

    static bool isBlobDescriptor(leveldb::Iterator *itr) {
        const BlobObjKey *key = reinterpret_cast<const BlobObjKey *>(itr->key().data());
        return key->objIndex == BLOB_META_INDEX;
    }

    BlobObjKeyComparator comparator;
};

DMChecker::DMChecker(DMCheckerEnv *env) {
    this->env = env;
    totalMismatches = 0;
}

void DMChecker::logMisMatch(const fds_volid_t &volId,
                            const std::vector<DiffResult> &mismatches) {
    totalMismatches += mismatches.size();
    for (const auto &mismatch : mismatches) {
        LOGERROR << volId << " " << mismatch;
    }
}

uint64_t DMChecker::run() {
    DmPersistVolDBDiffAdapter diffAdapter;
    auto volumeList  = env->getVolumeIds();
    for (const auto &volId : volumeList) {
        auto replicaCnt = env->getReplicaCount(volId);

        auto primaryCatPath = env->getCatalogPath(volId, 0);
        if (primaryCatPath.empty()) {
            logMisMatch(volId,
                        {{boost::lexical_cast<std::string>(volId),
                        DiffResult::DELETED_KEY,
                        "Primary db doesn't exist"}});
            continue;
        }

        for (uint32_t i = 1; i < replicaCnt; i++) {
            LOGNORMAL << "Checking volId: " << volId << " primary against replica idx: " << i;
            auto backupCatPath = env->getCatalogPath(volId, i);
            if (backupCatPath.empty()) {
                logMisMatch(volId,
                            {{boost::lexical_cast<std::string>(volId),
                            DiffResult::DELETED_KEY,
                            "Replica db doesn't exist"}});
                continue;
            }

            LevelDbDiffer  differ(primaryCatPath, backupCatPath, &diffAdapter);
            std::vector<DiffResult> diffs;
            do {
                bool cont = differ.diff(2048, diffs);
                if (diffs.size() > 0) {
                    logMisMatch(volId, diffs);
                    diffs.clear();
                }
                if (!cont) {
                    break;
                }
            } while (true);
        }
    }
    return totalMismatches;
}
} // namespace fds

