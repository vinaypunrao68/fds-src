/**
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <unistd.h>

#include <vector>
#include <chrono>
#include <thread>
#include <string>
#include <boost/shared_ptr.hpp>

#include <ObjectId.h>
#include <dlt.h>
#include <FdsRandom.h>

namespace fds {

class SmUtUtils {
  public:
    static void populateDlt(DLT* dlt, fds_uint32_t sm_count) {
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
    static void getDiskMap(
        const FdsRootDir *dir,
        std::unordered_map<fds_uint16_t, std::string>* diskMap) {
        int           idx;
        fds_uint64_t  uuid;
        std::string   path, dev;

        std::ifstream map(dir->dir_dev() + std::string("/disk-map"),
                          std::ifstream::in);

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
    static void cleanFdsDev(const FdsRootDir* dir) {
        std::unordered_map<fds_uint16_t, std::string> diskMap;
        std::unordered_map<fds_uint16_t, std::string>::const_iterator cit;
        SmUtUtils::getDiskMap(dir, &diskMap);
        // remove token data and meta, and superblock
        for (cit = diskMap.cbegin(); cit != diskMap.cend(); ++cit) {
            const std::string rm_base = "rm -rf  " + cit->second;
            // remove token files
            const std::string rm_data = rm_base + "//tokenFile*";;
            int ret = std::system((const char *)rm_data.c_str());
            // remove levelDB files
            const std::string rm_meta = rm_base + "//SNodeObjIndex_*";;
            ret = std::system((const char *)rm_meta.c_str());
            // TODO(Anna) remove superblock
        }
    }
};

}  // namespace fds

