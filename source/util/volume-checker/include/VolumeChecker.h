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

namespace fds {

class VolumeChecker : public SvcProcess {
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
    enum vcStatusCode {
        VC_NOT_STARTED,     // First started
        VC_RUNNING          // VC has finished initializing and is running
    };
    // For the future, may return more than just status code, but progress as well
    using vcStatus = vcStatusCode;
    vcStatus getStatus();

private:
    using volListType = std::vector<fds_volid_t>;

    // Clears and then populates the volume list from the argv. Returns ERR_OK if populated.
    Error populateVolumeList(int argc, char **argv);

    // List of volumes to check
    volListType volumeList;

    // Local cached copy of dmt mgr pointer
    DMTManagerPtr dmtMgr;

    // Local cached copy of dlt manager
    DLTManagerPtr dltMgr;

    // Max retires for get - TODO needs to be organized later
    int maxRetries = 10;

    // If volumeChecker initialization succeeded
    bool initCompleted;

    // If the process should wait for a shutdown call
    bool waitForShutdown;

    // Keeping track of internal state machine
    vcStatusCode currentStatusCode;
};

} // namespace fds

#endif // SOURCE_UTIL_VOLUMECHECKER_INCLUDE_VOLUMECHECKER_H
