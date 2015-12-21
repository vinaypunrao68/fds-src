/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_DLTDMTUTIL_H_
#define SOURCE_INCLUDE_DLTDMTUTIL_H_

#include <vector>
#include <utility>

#include <fds_resource.h>
#include <fds_typedefs.h>
#include <fds_defines.h>
#include <fds_module.h>

#include <util/Log.h>

namespace fds
{

typedef std::pair<fpi::FDSP_MgrIdType, int64_t> Svc;

    class DltDmtUtil
    {
    public:

        /* Making this class a singleton for ease of access */
        static DltDmtUtil* getInstance();

        ~DltDmtUtil() { };

        /* Functions to keep track of removed nodes */
        void    addToRemoveList(int64_t uuid);
        bool    isMarkedForRemoval(int64_t uuid);
        void    clearFromRemoveList(fds_int64_t nodeUuid);
        bool    isAnyRemovalPending(fds_int64_t& nodeUuid);

        /* Functions to hold info related to migration aborts on OM restart
         * for both DM and SM */

        // SM related
        void    setSMAbortParams(bool abort, int64_t version);
        bool    isSMAbortAfterRestartTrue();
        void    clearSMAbortParams();
        int64_t getSMTargetVersionForAbort();

        // DM related
        void    setDMAbortParams(bool abort, int64_t version);
        bool    isDMAbortAfterRestartTrue();
        void    clearDMAbortParams();
        int64_t getDMTargetVersionForAbort();

    private:

        DltDmtUtil();
        // Stop compiler generating methods to copy the object
        DltDmtUtil(DltDmtUtil const& copy);
        DltDmtUtil& operator=(DltDmtUtil const& copy);

        static DltDmtUtil*   m_instance;

        /* Member to track removedNodes */
        std::vector<int64_t> removedNodes;

        /* Keep track of migration interruptions */
        bool                 sendSMMigAbortAfterRestart;
        bool                 sendDMMigAbortAfterRestart;
       fds_uint64_t          dltTargetVersionForAbort;
       fds_uint64_t          dmtTargetVersionForAbort;
    };

} // namespace fds

#endif  // SOURCE_INCLUDE_DLTDMTUTIL_H_
