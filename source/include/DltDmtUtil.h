/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_DLTDMTUTIL_H_
#define SOURCE_INCLUDE_DLTDMTUTIL_H_

#include <vector>
#include <utility>

#include <fds_typedefs.h>

#include <util/Log.h>

namespace fds
{
    class DltDmtUtil
    {
    public:

        /* Making this class a singleton for ease of access */
        static DltDmtUtil* getInstance();

        ~DltDmtUtil() { };

        /* Functions to keep track of removed nodes */
        void    addToRemoveList(int64_t uuid, fpi::FDSP_MgrIdType svc_type);
        bool    isMarkedForRemoval(int64_t uuid);
        void    clearFromRemoveList(int64_t uuid);
        int     getPendingNodeRemoves(fpi::FDSP_MgrIdType svc_type);

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

        // General utility function
        std::string printSvcStatus(fpi::ServiceStatus svcStatus);

    private:

        DltDmtUtil();
        // Stop compiler generating methods to copy the object
        DltDmtUtil(DltDmtUtil const& copy);
        DltDmtUtil& operator=(DltDmtUtil const& copy);

        static DltDmtUtil*   m_instance;

        /* Member to track removedNodes */
        std::vector<std::pair<int64_t, fpi::FDSP_MgrIdType>> removedNodes;

        /* Keep track of migration interruptions */
        bool                 sendSMMigAbortAfterRestart;
        bool                 sendDMMigAbortAfterRestart;
        fds_uint64_t         dltTargetVersionForAbort;
        fds_uint64_t         dmtTargetVersionForAbort;
    };

} // namespace fds

#endif  // SOURCE_INCLUDE_DLTDMTUTIL_H_
