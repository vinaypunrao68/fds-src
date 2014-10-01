/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_TESTUTILS_H_
#define SOURCE_INCLUDE_TESTUTILS_H_

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

/* Forward declarations */
namespace FDS_ProtocolInterface {
class SvcUuid;
}

namespace fds {
/* Forward declarations */
class Platform;

/**
* @brief Utility functions for tests
*/
struct TestUtils {
    static FDS_ProtocolInterface::SvcUuid getAnyNonResidentSmSvcuuid(Platform* platform);
};

}  // namespace fds

#endif  // SOURCE_INCLUDE_TESTUTILS_H_
