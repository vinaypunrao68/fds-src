/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_OMEXTUTILAPI_H_
#define SOURCE_INCLUDE_OMEXTUTILAPI_H_

#include <vector>
#include <utility>

#include <fds_typedefs.h>

#include <util/Log.h>
#include <kvstore/configdb.h>

namespace fds
{
    class OmExtUtilApi
    {
    public:

        /* Making this class a singleton for ease of access */
        static OmExtUtilApi* getInstance();

        ~OmExtUtilApi() { };

        /* Functions to keep track of removed nodes */
        void         addToRemoveList(int64_t uuid, fpi::FDSP_MgrIdType svc_type);
        bool         isMarkedForRemoval(int64_t uuid);
        void         clearFromRemoveList(int64_t uuid);
        int          getPendingNodeRemoves(fpi::FDSP_MgrIdType svc_type);

        /* Functions to hold info related to migration aborts on OM restart
         * for both DM and SM */

        // SM related
        void         setSMAbortParams(bool abort, fds_uint64_t version);
        bool         isSMAbortAfterRestartTrue();
        void         clearSMAbortParams();
        fds_uint64_t getSMTargetVersionForAbort();

        // DM related
        void         setDMAbortParams(bool abort, fds_uint64_t version);
        bool         isDMAbortAfterRestartTrue();
        void         clearDMAbortParams();
        fds_uint64_t getDMTargetVersionForAbort();

        /*
         * Other service related utility functions that will be used by both
         * the OM as well as other components such as svcMgr
         */
        bool         isIncomingUpdateValid(fpi::SvcInfo& incomingSvcInfo, fpi::SvcInfo currentInfo);
        bool         isTransitionAllowed(fpi::ServiceStatus incoming,
                                         fpi::ServiceStatus current,
                                         bool sameIncNo,
                                         bool greaterIncNo,
                                         bool zeroIncNo);

        std::vector<std::vector<fpi::ServiceStatus>> getAllowedTransitions();

        static std::string  printSvcStatus(fpi::ServiceStatus svcStatus);
    private:

        OmExtUtilApi();
        // Stop compiler generating methods to copy the object
        OmExtUtilApi(OmExtUtilApi const& copy);
        OmExtUtilApi& operator=(OmExtUtilApi const& copy);

        static OmExtUtilApi*   m_instance;

        /* Member to track removedNodes */
        std::vector<std::pair<int64_t, fpi::FDSP_MgrIdType>> removedNodes;

        /* Keep track of migration interruptions */
        bool                sendSMMigAbortAfterRestart;
        bool                sendDMMigAbortAfterRestart;
        fds_uint64_t        dltTargetVersionForAbort;
        fds_uint64_t        dmtTargetVersionForAbort;

        //+--------------------------+---------------------------------------+
        //|     Current State        |    Valid IncomingState                |
        //+--------------------------+---------------------------------------+
        //      ACTIVE               |  STANDBY, STOPPED, INACTIVE_FAILED
        //      INACTIVE_STOPPED     |  ACTIVE, STARTED, REMOVED
        //      DISCOVERED           |  ACTIVE
        //      STANDBY              |  REMOVED, ACTIVE
        //      ADDED                |  STARTED, REMOVED
        //      STARTED              |  ACTIVE, STOPPED, INACTIVE_FAILED
        //      STOPPED              |  INACTIVE_STOPPED, STARTED, REMOVED
        //      REMOVED              |  DISCOVERED
        //      INACTIVE_FAILED      |  ACTIVE, STOPPED
        //+--------------------------+---------------------------------------+

        std::vector<std::vector<fpi::ServiceStatus>> allowedStateTransitions =
        {
                // valid incoming for state: INVALID(0)
                { fpi::SVC_STATUS_ACTIVE, fpi::SVC_STATUS_INACTIVE_STOPPED,
                  fpi::SVC_STATUS_DISCOVERED, fpi::SVC_STATUS_STANDBY, fpi::SVC_STATUS_ADDED,
                  fpi::SVC_STATUS_STARTED, fpi::SVC_STATUS_STOPPED,
                  fpi::SVC_STATUS_REMOVED, fpi::SVC_STATUS_INACTIVE_FAILED },
                // valid incoming for state: ACTIVE (1)
                { fpi::SVC_STATUS_STANDBY, fpi::SVC_STATUS_STOPPED,
                  fpi::SVC_STATUS_INACTIVE_FAILED },
                // valid incoming for state: INACTIVE_STOPPED(2)
                { fpi::SVC_STATUS_ACTIVE,fpi::SVC_STATUS_STARTED, fpi::SVC_STATUS_REMOVED },
                // valid incoming for state: DISCOVERED(3)
                { fpi::SVC_STATUS_ACTIVE },
                // valid incoming for state: STANDBY(4)
                { fpi::SVC_STATUS_ACTIVE, fpi::SVC_STATUS_REMOVED },
                // valid incoming for state: ADDED(5)
                { fpi::SVC_STATUS_STARTED, fpi::SVC_STATUS_REMOVED },
                // valid incoming for state: STARTED(6)
                { fpi::SVC_STATUS_ACTIVE, fpi::SVC_STATUS_STOPPED,
                  fpi::SVC_STATUS_INACTIVE_FAILED },
                // valid incoming for state: STOPPED(7)
                { fpi::SVC_STATUS_INACTIVE_STOPPED,fpi::SVC_STATUS_STARTED,
                  fpi::SVC_STATUS_REMOVED },
                // valid incoming for state: REMOVED(8)
                { fpi::SVC_STATUS_DISCOVERED },
                // valid incoming for state: INACTIVE_FAILED(9)
                { fpi::SVC_STATUS_ACTIVE, fpi::SVC_STATUS_STOPPED }
        };
    };

} // namespace fds

#endif  // SOURCE_INCLUDE_OMEXTUTILAPI_H_
