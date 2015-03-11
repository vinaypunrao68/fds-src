/**
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <unistd.h>

#include <vector>
#include <chrono>
#include <thread>
#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>

#include <ObjectId.h>
#include <dlt.h>
#include <odb.h>
#include <FdsRandom.h>

#include "platform/platform_consts.h"

namespace fds {

class SmUtUtils {
  public:
    static void populateDlt(DLT* dlt, fds_uint32_t sm_count,
                            fds_uint32_t first_sm_id = 1) {
        fds_uint64_t numTokens = pow(2, dlt->getWidth());
        fds_uint32_t column_depth = dlt->getDepth();
        DltTokenGroup tg(column_depth);
        fds_uint32_t cur_id = first_sm_id;
        fds_uint32_t count = 0;
        for (fds_token_id i = 0; i < numTokens; i++) {
            for (fds_uint32_t j = 0; j < column_depth; j++) {
                NodeUuid uuid(cur_id);
                tg.set(j, uuid);
                ++count;
                if ((count % sm_count) == 0) {
                    cur_id = first_sm_id;
                } else {
                    cur_id++;
                }
            }
            dlt->setNodes(i, tg);
        }
        dlt->generateNodeTokenMap();
    }

    // will create objectDB in odbDir, odbName is full path/filename
    static osm::ObjectDB *createObjectDB(const std::string& odbDir,
                                         const std::string& odbName) {
        osm::ObjectDB *odb = NULL;
        boost::filesystem::path odbPath(odbDir.c_str());
        if (boost::filesystem::exists(odbPath)) {
            // remove everything in this dir
            GLOGNOTIFY << "Directory " << odbDir << " exists, cleaning up";
            for (boost::filesystem::directory_iterator itEndDir, it(odbPath);
                 it != itEndDir;
                 ++it) {
                boost::filesystem::remove_all(it->path());
            }
        } else {
            GLOGNOTIFY << "Creating " << odbDir << " directory";
            boost::filesystem::create_directory(odbPath);
        }

        try
        {
            odb = new osm::ObjectDB(odbName, true);
        }
        catch(const osm::OsmException& e)
        {
            LOGERROR << "Failed to create ObjectDB " << odbName;
            LOGERROR << e.what();
            return NULL;
        }
        GLOGNOTIFY << "Opened ObjectDB " << odbName;
        return odb;
    }
    /**
     * Create a set of unique object IDs
     */
    static void createUniqueObjectIDs(fds_uint32_t numObjs,
                                      std::vector<ObjectID>& objset) {
        fds_uint64_t seed = RandNumGenerator::getRandSeed();
        RandNumGenerator rgen(seed);
        fds_uint32_t rnum = (fds_uint32_t)rgen.genNum();

        // populate set with unique objectIDs
        objset.clear();
        for (fds_uint32_t i = 0; i < numObjs; ++i) {
            std::string obj_data = std::to_string(rnum);
            ObjectID oid = ObjIdGen::genObjectId(obj_data.c_str(), obj_data.size());
            // we want every object ID in the set to be unique
            while (std::find(objset.begin(), objset.end(), oid) != objset.end()) {
                rnum = (fds_uint32_t)rgen.genNum();
                obj_data = std::to_string(rnum);
                oid = ObjIdGen::genObjectId(obj_data.c_str(), obj_data.size());
            }
            objset.push_back(oid);
            GLOGDEBUG << "Object set: " << oid;
            rnum = (fds_uint32_t)rgen.genNum();
        }
    }

    /**
     * Checks if there is an existing disk-map in /fds/dev/
     * and if it has at least one disk. If so, the method does not
     * do anything -- the test will be just using already setup disk map
     * If there is no disk-map, the method creates /fds/dev/disk-map
     * and creates dirs for /fds/dev/hdd-* and /fds/dev/ssd-*
     * @return true if disk map was setup for the test
     */
    static fds_bool_t setupDiskMap(const FdsRootDir* dir,
                                   fds_uint32_t simHddCount,
                                   fds_uint32_t simSsdCount) {
        std::string devDir = dir->dir_dev();
        FdsRootDir::fds_mkdir(dir->dir_dev().c_str());
        std::string diskmapPath = devDir + DISK_MAP_FILE;
        std::ifstream map(diskmapPath, std::ifstream::in);
        fds_uint32_t diskCount = 0;
        if (map.fail() == false) {
            // we opened a map, now see there is at least one disk
            int           idx;
            fds_uint64_t  uuid;
            std::string   path, dev;
            if (!map.eof()) {
                map >> dev >> idx >> std::hex >> uuid >> std::dec >> path;
                if (!map.fail()) {
                    // we have at least one device in disk map, assuming
                    // devices already setup by platform
                    GLOGDEBUG << "Looks like disk-map already setup, will use it";
                    return false;
                }
            }
        }
        GLOGDEBUG << "Will create fake disk map for testing";
        cleanFdsTestDev(dir);  // clean dirs just in case

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
    static void getDiskMap(
        const FdsRootDir *dir,
        std::unordered_map<fds_uint16_t, std::string>* diskMap) {
        int           idx;
        fds_uint64_t  uuid;
        std::string   path, dev;

        std::ifstream map(dir->dir_dev() + DISK_MAP_FILE, std::ifstream::in);

        diskMap->clear();
        fds_verify(map.fail() == false);
        while (!map.eof()) {
            map >> dev >> idx >> std::hex >> uuid >> std::dec >> path;
            if (map.fail()) {
                break;
            }
            fds_verify(diskMap->count(idx) == 0);
            (*diskMap)[idx] = path;
        }
    }
    static void cleanFdsTestDev(const FdsRootDir* dir) {
        // remove test dev dirs and disk-map from  <fds-root>/dev
        const std::string rm_dev = "rm -rf " + dir->dir_dev() + "*-test-*";
        int ret = std::system((const char *)rm_dev.c_str());
        const std::string rm_diskmap = "rm -rf " + dir->dir_dev() + "disk-map";
        ret = std::system((const char *)rm_diskmap.c_str());
    }
    static void cleanFdsDev(const FdsRootDir* dir) {
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
    /* This function removes everything under a dir.
     * But leaves the dir in place.
     */
    static void cleanAllInDir(const std::string& rmPath) {
        boost::filesystem::path removeInPath(rmPath.c_str());

        if (boost::filesystem::is_directory(rmPath.c_str())) {
            for (boost::filesystem::directory_iterator itEndDir, it(removeInPath);
                 it != itEndDir;
                 ++it) {
                GLOGNORMAL << "Removing "<< it->path();
                boost::filesystem::remove_all(it->path());
            }
        }
    }
    /// same as above but for fds root dir
    static void cleanAllInDir(const FdsRootDir* dir) {
        const std::string rmPath = dir->dir_dev();
        cleanAllInDir(rmPath);
    }
};

}  // namespace fds

