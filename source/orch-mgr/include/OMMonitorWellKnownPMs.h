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
#include <concurrency/RwLock.h>
#include "omutils.h"
#include <kvstore/configdb.h>

namespace fds
{
    class OrchMgr;

    typedef std::unordered_map<fpi::SvcUuid, double, SvcUuidHash> PmMap;

    class OMMonitorWellKnownPMs : public HasLogger
    {
        public:
            explicit OMMonitorWellKnownPMs(OrchMgr* om, kvstore::ConfigDB* db);
            virtual ~OMMonitorWellKnownPMs();

            void  shutdown();
            PmMap getKnownPMsMap();
            void  updateKnownPMsMap(fpi::SvcUuid uuid, double timestamp);
            Error getLastHeardTime(fpi::SvcUuid uuid, double& t);
            Error removeFromPMsMap(PmMap::iterator iter);
            void  handleStaleEntry(fpi::SvcInfo svc);
            Error handleActiveEntry(fpi::SvcUuid svcUuid);
        protected:
            bool fShutdown = false;
            std::thread* runner;
            OrchMgr* om;
            PmMap wellKnownPMsMap;
            fds_rwlock pmMapLock;
            fds_mutex dbLock;
            kvstore::ConfigDB *configDB;

            void run();
    };
} // namespace fds
#endif  // SOURCE_ORCH_MGR_INCLUDE_OMMONITORWELLKNOWNPMS_H_
