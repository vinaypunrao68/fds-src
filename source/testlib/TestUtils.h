/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_TESTUTILS_H_
#define SOURCE_INCLUDE_TESTUTILS_H_

#include <fdsp/svc_types_types.h>
#include <fds_error.h>
#include <concurrency/taskstatus.h>

/**
* @brief Polls until predicate is true or timeoutMs expires
*
* @param pred predicate
* @param sleepStepMs amount this thread sleeps in ms between polls
* @param timeoutMs timeout
*
* @return 
*/
#define POLL_MS(pred, sleepStepMs, timeoutMs) \
{ \
    uint32_t sleptMs = 0; \
    while ((pred) == false && sleptMs < (uint32_t) (timeoutMs)) { \
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepStepMs)); \
        sleptMs += sleepStepMs; \
    } \
}

namespace fpi = FDS_ProtocolInterface;

namespace fds {
/* Forward declarations */
class Platform;

/**
* @brief Utility functions for tests
*/
namespace TestUtils {

struct Waiter : concurrency::TaskStatus {
    using concurrency::TaskStatus::TaskStatus;
    void doneWith(const Error &e);
    void reset(uint32_t numTasks);
    Error awaitResult();
    Error awaitResult(ulong timeout);

    Error error;
};


/* DEPRECATED.  DON'T USE */
inline fpi::SvcUuid getAnyNonResidentSmSvcuuid(Platform* platform,
                                                          bool uuid_hack = false)
{ return fpi::SvcUuid(); }

/* DEPRECATED.  DON'T USE */
inline FDS_ProtocolInterface::SvcUuid getAnyNonResidentDmSvcuuid(Platform* platform,
                                                          bool uuid_hack = false)
{ return fpi::SvcUuid(); }

/* DEPRECATED.  DON'T USE */
inline bool enableFault(const fpi::SvcUuid &svcUuid, const std::string &faultId)
{ return false; }

/* DEPRECATED.  DON'T USE */
inline bool disableFault(const fpi::SvcUuid &svcUuid, const std::string &faultId)
{ return false; }

} // namespace TestUtils
}  // namespace fds

#endif  // SOURCE_INCLUDE_TESTUTILS_H_
