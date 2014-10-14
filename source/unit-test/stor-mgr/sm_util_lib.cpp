/**
* Copyright 2014 Formation Data Systems, Inc.
*/

#include "sm_dataset.h"

#include <unistd.h>

#include <string>
#include <boost/shared_ptr.hpp>
#include <ObjectId.h>
#include <dlt.h>
#include <FdsRandom.h>

namespace fds {

    /** Reads the disk map in fds root directory and populates
    * a diskMap structure.
    *
    * @param dir a pointer to the fds root directory
    * @param diskMap the std::unordered_map structure to populate
    */
    static void SafeSmMgmt::getDiskMap(const FdsRootDir *dir,
            std::unordered_map<fds_uint16_t, std::string>* diskMap) {
        int           idx;
        fds_uint64_t  uuid;
        std::string   path, dev;

        std::ifstream map(dir->dir_dev() + std::string("/disk-map"),
                std::ifstream::in);

        diskMap->clear();
        fds_verify(map.fail() == nullptr);
        while (!map.eof()) {
            map >> dev >> idx >> std::hex >> uuid >> std::dec >> path;
            if (map.fail()) {
                break;
            }
            fds_verify(diskMap->count(idx) == 0);
            (*diskMap)[idx] = path;
        }
    }

    /**
    * Checks if there is an existing disk-map in /fds/dev/
    * and if it has at least one disk.
    *
    * @param dir a pointer to the fds root directory
    *
    * @return true if disk map exists, false otherwise
    */
    static fds_bool_t SafeSmMgmt::diskMapPresent(const FdsRootDir* dir) {
        std::string devDir = dir->dir_dev();
        std::string diskmapPath = devDir + std::string("disk-map");
        std::ifstream map(diskmapPath, std::ifstream::in);
        fds_uint32_t diskCount = 0;
        if (map.fail() == false) {
            // we opened a map, now see there is at least one disk
            int idx;
            fds_uint64_t uuid;
            std::string path, dev;
            if (!map.eof()) {
                map >> dev >> idx >> std::hex >> uuid >> std::dec >> path;
                if (!map.fail()) {
                    // we have at least one device in disk map, assuming
                    // devices already setup by platform
                    GLOGDEBUG << "Disk-map already setup!";
                    return true;
                }
            }
        }
        return false;
    }

    /** Checks if there is an existing disk-map in /fds/dev/
    * and if it has at least one disk. If so, the method does not
    * do anything -- the test will be just using already setup disk map
    * If there is no disk-map, the method creates /fds/dev/disk-map
    * and creates dirs for /fds/dev/hdd-* and /fds/dev/ssd-*
    *
    * @param dir a pointer to the fds root directory
    * @param simHddCount the number of magnetic disk drives to simulate
    * @param simSsdCount the number of solid state drives to simulate
    *
    * @return true if disk map was setup for the test
    */
    static fds_bool_t SafeSmMgmt::setupDiskMap(const FdsRootDir* dir,
            fds_uint32_t simHddCount,
            fds_uint32_t simSsdCount) {

        if (SafeSmMgmt::diskMapPresent(dir)) {
            return false;
        }

        GLOGDEBUG << "Will create fake disk map for testing";
        cleanFdsTestDev(dir);  // clean dirs just in case

        FdsRootDir::fds_mkdir(dir->dir_dev().c_str());
        std::ofstream omap(diskmapPath,
                std::ofstream::out | std::ofstream::trunc);
        int idx = 0;
        for (fds_uint32_t i = 0; i < simHddCount; ++i) {
            fds_uint64_t uuid = idx + 1234;
            std::string path = devDir + "hdd-test-" + std::to_string(idx);
            omap << path.c_str() << " " << idx << " "
                    << std::hex << uuid << std::dec
                    << " " << path.c_str()  << "\n";
            ++idx;
            GLOGNORMAL << "Creating disk "<< path;
            FdsRootDir::fds_mkdir(path.c_str());
        }
        for (fds_uint32_t i = 0; i < simSsdCount; ++i) {
            fds_uint64_t uuid = idx + 12340;
            std::string path = devDir + "ssd-test-" + std::to_string(idx);
            omap << path.c_str() << " " << idx << " "
                    << std::hex << uuid << std::dec
                    << " " << path.c_str()  << "\n";
            ++idx;
            GLOGNORMAL << "Creating disk "<< path;
            FdsRootDir::fds_mkdir(path.c_str());
        }
        return true;
    }

    /** Populate a DLT.
    *
    * @param dlt a pointer to the DLT object to populate
    *
    * @param sm_count a count of the number of SMs
    */
    void SafeSmMgmt::populateDlt(DLT *dlt, fds_uint32_t sm_count) {
        fds_uint64_t numTokens = pow(2, dlt->getWidth());
        fds_uint32_t column_depth = dlt->getDepth();
        DltTokenGroup tg(column_depth);
        fds_uint32_t first_sm_id = 1;
        fds_uint32_t cur_id = first_sm_id;
        for (fds_token_id i = 0; i < numTokens; i++) {
            for (fds_uint32_t j = 0; j < column_depth; j++) {
                NodeUuid uuid(cur_id);
                tg.set(j, uuid);
                cur_id = (cur_id % sm_count) + 1;
            }
            dlt->setNodes(i, tg);
        }
        dlt->generateNodeTokenMap();
    }

    DMT* SafeSmMgmt::newDLT(fds_uint32_t sm_count) {
        fds_uint32_t cols = (sm_count < 4) ? sm_count : 4;
        return new DLT(16, cols, 1, true);
    }


    /** Creates /fds/dev/disk-map and creates dirs for /fds/dev/hdd-* and /fds/dev/ssd-*.
    *
    * @param dir string pointing to FDS root directory
    * @param simHddCount integer number of HDDs to setup
    * @param simSsdCount integer number of SSDs to setup
    *
    * @return true if disk map was setup for the test
    */
    static fds_bool_t UnsafeSmMgmt::setupFreshDiskMap(const FdsRootDir* dir,
            fds_uint32_t simHddCount,
            fds_uint32_t simSsdCount) {
        std::string devDir = dir->dir_dev();
        std::string diskmapPath = devDir + std::string("disk-map");

        GLOGDEBUG << "Will create fake disk map for testing";
        cleanFdsTestDev(dir);  // clean dirs just in case

        FdsRootDir::fds_mkdir(dir->dir_dev().c_str());
        std::ofstream omap(diskmapPath,
                std::ofstream::out | std::ofstream::trunc);
        int idx = 0;
        for (fds_uint32_t i = 0; i < simHddCount; ++i) {
            fds_uint64_t uuid = idx + 1234;
            std::string path = devDir + "hdd-test-" + std::to_string(idx);
            omap << path.c_str() << " " << idx << " "
                    << std::hex << uuid << std::dec
                    << " " << path.c_str()  << "\n";
            ++idx;
            GLOGNORMAL << "Creating disk "<< path;
            FdsRootDir::fds_mkdir(path.c_str());
        }
        for (fds_uint32_t i = 0; i < simSsdCount; ++i) {
            fds_uint64_t uuid = idx + 12340;
            std::string path = devDir + "ssd-test-" + std::to_string(idx);
            omap << path.c_str() << " " << idx << " "
                    << std::hex << uuid << std::dec
                    << " " << path.c_str()  << "\n";
            ++idx;
            GLOGNORMAL << "Creating disk "<< path;
            FdsRootDir::fds_mkdir(path.c_str());
        }
        return true;
    }

    /** Clean the /fds/dev/ directory. This will be where we'll write the new disk map, etc.
    *
    * @param dir An FdsRootDir pointer
    */
    static void UnsafeSmMgmt::cleanFdsDev(const FdsRootDir* dir) {
        std::unordered_map<fds_uint16_t, std::string> diskMap;
        std::unordered_map<fds_uint16_t, std::string>::const_iterator cit;
        SmUtUtils::getDiskMap(dir, &diskMap);
        // remove token data and meta, and superblock
        for (cit = diskMap.cbegin(); cit != diskMap.cend(); ++cit) {
            const std::string rm_base = "rm -rf " + cit->second;
            // remove token files
            const std::string rm_data = rm_base + "//tokenFile*";;
            int ret = std::system((const char *)rm_data.c_str());
            // remove levelDB files
            const std::string rm_meta = rm_base + "//SNodeObjIndex_*";;
            ret = std::system((const char *)rm_meta.c_str());
            // remove superblock
            const std::string rm_sb = rm_base + "/SmSuper*";
            ret = std::system((const char *)rm_sb.c_str());
        }
    }


    int StandaloneSM::run() {
        Error err(ERR_OK);
        int ret = 0;
        GLOGDEBUG << "Starting stand alone SM for unit testing";

        // clean data/metadata from /fds/dev/hdd*/
        const FdsRootDir *dir = g_fdsprocess->proc_fdsroot();
        UnsafeSmMgmt::cleanFdsDev(dir);

        // add fake DLT
        fds_uint32_t sm_count = 1;
        fds_uint32_t cols = (sm_count < 4) ? sm_count : 4;
        DLT* dlt = new DLT(16, cols, 1, true);
        SafeSmMgmt::populateDlt(dlt, sm_count);
        objectStore->handleNewDlt(dlt);
        // register volume
        FdsConfigAccessor conf(get_conf_helper());
        volume_.reset(new TestVolume(singleVolId, "objectstore_ut_volume", conf));
        volTbl->registerVolume(volume_->voldesc_);
    }

    Error StandaloneSM::populate() {
        Error err(ERR_OK);
        // put all  objects in dataset to the object store
        for (fds_uint32_t i = 0; i < (volume_->testdata_).dataset_.size(); ++i) {
            ObjectID oid = (volume_->testdata_).dataset_[i];
            boost::shared_ptr<std::string> data =
                    (volume_->testdata_).dataset_map_[oid].getObjectData();
            err = put((volume_->voldesc_).volUUID, oid, data);
            if (!err.ok()) return err;
        }
        return err;
    }

} // namespace fds