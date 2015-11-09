/*
 * Copyright 2015 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_ORCH_MGR_INCLUDE_OMMONITORWELLKNOWNPMS_H_
#define SOURCE_ORCH_MGR_INCLUDE_OMMONITORWELLKNOWNPMS_H_

#include <chrono>
#include <thread>
#include <unordered_map>
#include <map>

#include "fdsp/svc_types_types.h"
#include "fdsp/common_types.h"
#include "net/SvcMgr.h"
#include "concurrency/RwLock.h"
#include "omutils.h"
#include "kvstore/configdb.h"

namespace fds
{
    class OrchMgr;

    typedef std::unordered_map<fpi::SvcUuid, double, SvcUuidHash> PmMap;

    class OMMonitorWellKnownPMs : public HasLogger
    {
        public:
            OMMonitorWellKnownPMs();
            virtual ~OMMonitorWellKnownPMs();

            PmMap getKnownPMsMap();
            bool  isWellKnown(fpi::SvcUuid uuid);
            void  updateKnownPMsMap(fpi::SvcUuid uuid, double timestamp, bool updateSvcState);

        protected:
            bool fShutdown;
            std::thread* runner;
            PmMap wellKnownPMsMap;
            PmMap removedPMsMap;
            std::map<int64_t, int32_t> retryMap;
            fds_rwlock pmMapLock;
            fds_mutex dbLock;
            fds_mutex genMapLock;

            void run();
            void  shutdown();
            Error getLastHeardTime(fpi::SvcUuid uuid, double& timestamp);
            Error removeFromPMsMap(PmMap::iterator iter);
            void  cleanUpOldState(fpi::SvcUuid uuid, bool updateSvcState);
            void  handleRetryOnInactive(fpi::SvcUuid uuid);
            void  handleStaleEntry(fpi::SvcUuid uuid);
            Error handleActiveEntry(fpi::SvcUuid svcUuid);
    };
} // namespace fds
#endif  // SOURCE_ORCH_MGR_INCLUDE_OMMONITORWELLKNOWNPMS_H_
