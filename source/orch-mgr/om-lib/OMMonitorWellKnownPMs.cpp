/*
 * Copyright 2015 by Formation Data Systems, Inc.
 */
#include <unordered_map>

#include "OMMonitorWellKnownPMs.h"
#include "OmResources.h"
#include <orchMgr.h>
#include <net/SvcMgr.h>

namespace fds
{
    OMMonitorWellKnownPMs::OMMonitorWellKnownPMs
        (
        ): fShutdown(false)
    {
        runner = new std::thread(&OMMonitorWellKnownPMs::run, this);
    }

    OMMonitorWellKnownPMs::~OMMonitorWellKnownPMs()
    {
        shutdown();
        runner->join();
        delete runner;
    }

    void OMMonitorWellKnownPMs::run()
    {
        LOGDEBUG << "Starting Well Known PM monitoring thread";

        while ( !fShutdown )
        {
            auto svcMgr = MODULEPROVIDER()->getSvcMgr();
            std::vector<FDS_ProtocolInterface::SvcInfo> entries;

            svcMgr->getSvcMap( entries );
            if ( entries.size() > 0 )
            {
                for ( const auto svc : entries )
                {
                    if ( svc.svc_type == fpi::FDSP_PLATFORM )
                    {
                        fpi::SvcUuid svcUuid;
                        svcUuid.svc_uuid = svc.svc_id.svc_uuid.svc_uuid;
                        switch ( svc.svc_status )
                        {
                            case FDS_ProtocolInterface::SVC_STATUS_INVALID:
                            case FDS_ProtocolInterface::SVC_STATUS_INACTIVE:
                            {
                                auto mapIter = wellKnownPMsMap.find(svc.svc_id.svc_uuid);
                                if (mapIter == wellKnownPMsMap.end()) {
                                    LOGDEBUG << "PM:" << std::hex << svcUuid.svc_uuid << std::dec
                                             << " is no longer well known service, OM stopped"
                                                " checking heartbeat";
                                }
                                else
                                {
                                    LOGWARN << "Platformd UUID: "
                                            << std::hex
                                            << svcUuid.svc_uuid
                                            << std::dec
                                            << " status "
                                            <<  svc.svc_status
                                            << " expected status "
                                            << FDS_ProtocolInterface::SVC_STATUS_ACTIVE;
                                    //HEALTH_STATE_UNREACHABLE will cause a well known PM
                                    // to go into inactive state, in which case handle the
                                    // change in active to inactive
                                    handleStaleEntry(svc, svcMgr);
                                }
                                break;
                            }
                            default: //SVC_STATUS_ACTIVE, SVC_STATUS_DISCOVERED
                            {
                                Error err = handleActiveEntry(svcUuid, svc, svcMgr);

                                if (!err.ok())
                                    LOGDEBUG << "Problem sending out heartbeat check to PM:"
                                             << std::hex << svcUuid.svc_uuid << std::dec;

                            }
                        }
                    }
                }
            }
                // sleep for five minutes, before checking again.
                std::this_thread::sleep_for( std::chrono::minutes( 5 ) );
        }

        LOGDEBUG << "Stopping Well Known PM monitoring thread";
    }

    void OMMonitorWellKnownPMs::shutdown()
    {
        fShutdown = true;
    }

    PmMap OMMonitorWellKnownPMs::getKnownPMsMap()
    {
        return wellKnownPMsMap;
    }

    void OMMonitorWellKnownPMs::updateKnownPMsMap
        (
        fpi::SvcUuid svcUuid,
        double timestamp
        )
    {
        SCOPEDWRITE(pmMapLock);
        auto mapIter = wellKnownPMsMap.find(svcUuid);

        if (mapIter != wellKnownPMsMap.end()) {
            LOGDEBUG << "Updating last heard time for PM:"
                     << std::hex << svcUuid.svc_uuid << std::dec;
        }
        else
        {
            LOGDEBUG << "New PM added to well known map with ID:"
                     << std::hex << svcUuid.svc_uuid << std::dec;
        }

        // Don't use "insert" since we want to overwrite the timestamp
        // if entry exists
        wellKnownPMsMap[svcUuid] = timestamp;
    }

    Error OMMonitorWellKnownPMs::removeFromPMsMap
        (
        PmMap::iterator iter
        )
    {
        SCOPEDWRITE(pmMapLock);

        wellKnownPMsMap.erase(iter);

        return ERR_OK;

    }

    Error OMMonitorWellKnownPMs::getLastHeardTime
        (
        fpi::SvcUuid uuid,
        double& timestamp
        )
    {
        SCOPEDREAD(pmMapLock);
        auto iter = wellKnownPMsMap.find(uuid);
        if (iter == wellKnownPMsMap.end()) {
            return ERR_NOT_FOUND;
        }

        timestamp = iter->second;

        return ERR_OK;
    }

    void OMMonitorWellKnownPMs::handleStaleEntry
        (
        fpi::SvcInfo svc,
        SvcMgr* svcMgr
        )
    {
        std::vector<fpi::SvcInfo> staleEntries;

        svc.svc_status = fpi::SVC_STATUS_INACTIVE;
        staleEntries.push_back(svc);
        // Update svcManager svcMap
        // This should already be inactive, with the HEALTH_STATE_UNREACHABLE
        // timeout of 2 minutes triggering before monitor logic times out (6 minutes),
        // but in case it hasn't been updated, do so
        svcMgr->updateSvcMap(staleEntries);

        // Update service state in the configDB
        fds_mutex::scoped_lock l(dbLock);
        gl_orch_mgr->getConfigDB()->changeStateSvcMap(svc.svc_id.svc_uuid.svc_uuid, fpi::SVC_STATUS_INACTIVE);

        auto iter = wellKnownPMsMap.find(svc.svc_id.svc_uuid);
        if (iter == wellKnownPMsMap.end()) {
            LOGDEBUG << "Could not find PM:"
                     << std::hex << svc.svc_id.svc_uuid.svc_uuid
                     << std::dec << "in well known PM map";
        }
        else
        {
            // Remove from our well known PM map
            removeFromPMsMap(iter);
        }
    }

    Error OMMonitorWellKnownPMs::handleActiveEntry
        (
        fpi::SvcUuid svcUuid,
        fpi::SvcInfo svcInfo,
        SvcMgr* svcMgr
        )
    {
        auto iter = wellKnownPMsMap.find(svcUuid);
        Error err(ERR_OK);
        fpi::SvcInfo info;
        if ( iter != wellKnownPMsMap.end())
        {
            auto curTime       = std::chrono::system_clock::now().time_since_epoch();
            auto curTimeInMin  = std::chrono::duration<double,std::ratio<60>>(curTime).count();
            double elapsedTime = curTimeInMin - iter->second;

            LOGDEBUG << "Well known platformd UUID: "
                    << std::hex
                    << svcUuid.svc_uuid
                    << std::dec
                    << " status "
                    <<  svcInfo.svc_status
                    << " minutes since last heartbeat"
                    << elapsedTime;

            if ( elapsedTime > 6 )
            {
                // It has been more than 6 minutes since we heard from the PM,
                // treat it as a stale map entry
                bool foundSvcInfo = svcMgr->getSvcInfo(svcUuid, info);
                if (foundSvcInfo) {
                    handleStaleEntry(info, svcMgr);
                }
                else
                {
                    LOGDEBUG << "Unable to retrieve svcInfo of stale PM uuid:"
                             << std::hex << svcUuid.svc_uuid << std::dec;
                }
            }
            else
            {
                try {
                    OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
                    Error err = local->om_heartbeat_check(svcUuid);
                }
                catch(...) {
                    LOGERROR << "OMmonitor encountered exception while "
                             << "sending heartbeat check to PM uuid"
                             << std::hex << svcUuid.svc_uuid << std::dec;
                    err = Error(ERR_NOT_FOUND);
                }
            }
        }
        else
        {
            LOGDEBUG << "PM: " << std::hex << svcUuid.svc_uuid << std::dec
                     << "is *not* present in OMs well known PMmap!";
        }

        return err;
    }
} // namespace fds
