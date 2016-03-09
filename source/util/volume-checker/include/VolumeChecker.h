/*
 * Copyright 2016 Formation Data Systems, Inc.
 */

#ifndef SOURCE_UTIL_VOLUMECHECKER_INCLUDE_VOLUMECHECKER_H
#define SOURCE_UTIL_VOLUMECHECKER_INCLUDE_VOLUMECHECKER_H

#include <boost/shared_ptr.hpp>
#include <fdsp/svc_api_types.h>
#include <net/SvcMgr.h>
#include <net/SvcProcess.h>
#include <net/PlatNetSvcHandler.h>
#include <net/SvcRequestPool.h>
#include <net/SvcProcess.h>
#include <fdsp_utils.h>
#include <gtest/gtest_prod.h>
#include <MigrationUtility.h>
#include <FdsCrypto.h>
#include <boost/bimap.hpp>
#include <boost/bimap/unordered_set_of.hpp>
#include <boost/bimap/list_of.hpp>

namespace fds {

// Forward declaration
class VolumeChecker : public SvcProcess {
    // TODO(Neil) Doesn't seem to work... see if this can be figured out later
    template <typename T> friend struct ProcessHandle;
    friend struct DmGroupFixture;
    friend struct VolumeGroupFixture;
    FRIEND_TEST(VolumeGroupFixture, twoHappyDMs);
public:
    explicit VolumeChecker(int argc, char **argv, bool initAsModule);
    ~VolumeChecker() = default;
    void init(int argc, char **argv, bool initAsModule);
    int run() override;

    // Gets DMT/DLT and updates D{M/L}TMgr because we don't do mod_enable_services
    Error getDMT();
    Error getDLT();

    /**
     * Status Querying methods
     */
    // Enum of lifecycle of volume checker to be allowed for querying
    enum StatusCode {
        VC_NOT_STARTED,     // First started
        VC_RUNNING,         // Finished initializing
        VC_DM_HASHING,      // VC has finished initializing and is running phase 1
        VC_DM_DONE,         // VC has finished running phase 1
        VC_ERROR            // VC has seen some errors
    };
    // For the future, may return more than just status code, but progress as well
    using VcStatus = StatusCode;
    VcStatus getStatus();

    /**
     * UNIT TEST METHODS. Do not use anywhere else.
     */
    size_t testGetVgCheckerListSize();
    size_t testGetVgCheckerListSize(unsigned index);
    bool testVerifyCheckerListStatus(unsigned castCode);
private:
    using VolListType = std::vector<fds_volid_t>;

    /**
     * Private members for keeping checker functional
     */
    // Clears and then populates the volume list from the argv. Returns ERR_OK if populated.
    Error populateVolumeList(int argc, char **argv);

    // List of volumes to check
    VolListType volumeList;

    // Local cached copy of dmt mgr pointer
    DMTManagerPtr dmtMgr;

    // Local cached copy of dlt manager
    DLTManagerPtr dltMgr;

    // Max retires for get - TODO needs to be organized later
    int maxRetries = 10;

    // Batch size per levelDB iteration
    int batchSize;

    // If volumeChecker initialization succeeded
    bool initCompleted;

    // If the process should wait for a shutdown call
    bool waitForShutdown;

    // Keeping track of internal state machine
    StatusCode currentStatusCode;

    /**
     * Internal data structure of keeping track of each DM that is responsible
     * for the volume's metadata (phase 1)
     */
    Error runPhase1();

    struct DmCheckerMetaData {
        DmCheckerMetaData(fds_volid_t _volId,
                          fpi::SvcUuid _svcUuid,
                          int _batchSize) :
            volId(_volId),
            svcUuid(_svcUuid),
            status(NS_NOT_STARTED),
            batchSize(_batchSize),
            time_out(1000*10*60)    // 10 minutes
            {}

        ~DmCheckerMetaData() = default;

        // Sends the initial check message to the dm
        void sendVolChkMsg(const EPSvcRequestRespCb &cb);

        // Cached id of what this checker meta-data is checking for
        fds_volid_t volId;

        // Instead of using NodeUuid, use SvcUuid since it's easier to use for thrift
        fpi::SvcUuid svcUuid;

        // Current node's status
        enum chkNodeStatus {
            NS_NOT_STARTED,       // Just created
            NS_CONTACTED,         // Volume list has been sent to the node and should be working
            NS_FINISHED,          // The node has responded with a result
            NS_OUT_OF_SYNC,       // Mark this DM as out of sync
            NS_ERROR              // Error state, idle
        };
        chkNodeStatus status;

        // Local cached batchSize
        int batchSize;

        // stored result
        std::string hashResult;

        // stored timeout
        unsigned time_out;
    };

    // Map of volID -> set of metadata for DM quorum check
    using VolumeGroupTable = std::vector<std::pair<fds_volid_t, std::vector<DmCheckerMetaData>>>;
    VolumeGroupTable vgCheckerList;

    // For keeping track of messages sent
    MigrationTrackIOReqs *initialMsgTracker;

    // Prepares the accounting data structures
    void prepareDmCheckerMap();

    // Sends the volume check msgs to DMs();
    void sendVolChkMsgsToDMs();

    void sendOneVolChkMsg(const EPSvcRequestRespCb &cb = nullptr);

    /**
     *  Wait for all the DMs to respond and see if we are all running w/o errors
     *  Returns ERR_INVALID if at least one DM errored out.
     */
    Error waitForVolChkMsg();

    /**
     * Use a bimap to keep a sorted count of number of hashes
     */
    typedef boost::bimap<boost::bimaps::unordered_set_of<std::string>,
            boost::bimaps::list_of<unsigned>> bm_type;

    bm_type hashQuorumCheckMap;


    /**
     * If an error occurs during volume checking process, this will send out the
     * abort message to everyone to stop churning through levelDBs and wasting resources
     */
    void handleVolumeCheckerError();

    /**
     * Checks to see if any DM is out of sync from the hash map,
     * and mark the corresponding DM metadata out of sync.
     * Returns ERR_OK if everything's ok.
     */
    Error checkDMHashQuorum();
};

} // namespace fds

#endif // SOURCE_UTIL_VOLUMECHECKER_INCLUDE_VOLUMECHECKER_H
