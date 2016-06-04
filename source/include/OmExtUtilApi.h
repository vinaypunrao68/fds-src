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
        static bool  isIncomingUpdateValid(fpi::SvcInfo& incomingSvcInfo, fpi::SvcInfo currentInfo,
                                           std::string source, bool pingUpdate = false);
        static bool  isTransitionAllowed( fpi::ServiceStatus incoming,
                                          fpi::ServiceStatus current,
                                          bool sameIncNo,
                                          bool greaterIncNo,
                                          bool zeroIncNo,
                                          fpi::FDSP_MgrIdType type,
                                          bool pingUpdate = false);

        static std::vector<std::vector<fpi::ServiceStatus>> getAllowedTransitions();
        static std::string  printSvcStatus(fpi::ServiceStatus svcStatus);

        static const std::vector<std::vector<fpi::ServiceStatus>> allowedStateTransitions;
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
    };
} // namespace fds

#endif  // SOURCE_INCLUDE_OMEXTUTILAPI_H_
