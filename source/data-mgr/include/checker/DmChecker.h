/* Copyright 2015 Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_CHECKER_DMCHECKER_H_
#define SOURCE_DATA_MGR_INCLUDE_CHECKER_DMCHECKER_H_

#include <vector>
#include <list>
#include <string>
#include <fds_volume.h>
#include <net/SvcProcess.h>
#include <checker/LeveldbDiffer.h>

namespace fds {
/**
* @brief Interface for providing the environment necessary for running DM checker
*/
struct DMCheckerEnv {
    virtual ~DMCheckerEnv() = default;
    virtual std::list<fds_volid_t> getVolumeIds() const = 0;
    virtual uint32_t getReplicaCount(const fds_volid_t &volId) const = 0;
    virtual std::string getCatalogPath(const fds_volid_t &volId,
                                       const uint32_t &replicaIdx) const = 0;
};

/**
* @brief Checker environment implmementation for running checker in a offline mode
* At the moment offline mode requires OM to be up and all DM catalogs to be hosted
* on same node
*/
struct DMOfflineCheckerEnv : SvcProcess, DMCheckerEnv {
    DMOfflineCheckerEnv(int argc, char *argv[]);
    int main() override;
    int run() override { return 0; }
    std::list<fds_volid_t> getVolumeIds() const override;
    uint32_t getReplicaCount(const fds_volid_t &volId) const override;
    std::string getCatalogPath(const fds_volid_t &volId,
                               const uint32_t &replicaIdx) const override;

 protected:
    std::list<fds_volid_t> volumeList;
};

/**
* @brief DM Checker
*/
struct DMChecker {
    explicit DMChecker(DMCheckerEnv *env);
    virtual ~DMChecker() = default;
    void logMisMatch(const fds_volid_t &volId, const std::vector<DiffResult> &mismatches);
    uint64_t run();

 protected:
    DMCheckerEnv                    *env;
    uint64_t                        totalMismatches;
};
}  // namespace fds

#endif    // SOURCE_DATA_MGR_INCLUDE_CHECKER_DMCHECKER_H_
