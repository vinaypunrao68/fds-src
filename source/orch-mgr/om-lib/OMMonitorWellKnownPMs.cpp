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
                                if (mapIter != wellKnownPMsMap.end()) {
                                    LOGWARN << " Well-known PM: "
                                            << std::hex
                                            << svcUuid.svc_uuid
                                            << std::dec
                                            << " not ACTIVE. Status:"
                                            <<  svc.svc_status;
                                    //HEALTH_STATE_UNREACHABLE will cause a well known PM
                                    // to go into inactive state, in which case handle the
                                    // change in active to inactive
                                    handleStaleEntry(svcUuid);
                                } else {

                                    if (retryMap[svcUuid.svc_uuid] != 0) {
                                        // If this PM has been removed from wellKnown map
                                        // and now svcLayer knows it is inactive, clean out
                                        // any retry count. In case at some point svc layer
                                        // transitions this PM to active, then this thread
                                        // should attempt retries. Also possible that svc layer
                                        // transition to active will only happen upon service
                                        // re-registration in which case we will clean up anyway
                                        // but do it here just in case
                                        fds_mutex::scoped_lock l(genMapLock);

                                        LOGDEBUG << "Cleaning up retry count for PM:"
                                                 << std::hex << svcUuid.svc_uuid << std::dec;
                                        retryMap[svcUuid.svc_uuid] = 0;
                                    }
                                }
                                break;
                            }
                            default: //SVC_STATUS_ACTIVE, SVC_STATUS_DISCOVERED
                            {

                                if ( gl_orch_mgr->getConfigDB()->getStateSvcMap(svcUuid.svc_uuid)
                                     == fpi::SVC_STATUS_INACTIVE )
                                {
                                    // This thread set the PM to down(in the configDB) when it
                                    // didn't hear a heartbeat back.
                                    // However, svc layer still thinks it's active, so attempt retry
                                    handleRetryOnInactive(svcUuid);

                                } else {
                                    Error err = handleActiveEntry(svcUuid);

                                    if (!err.ok())
                                        LOGDEBUG << "Problem sending out heartbeat check to PM:"
                                                 << std::hex << svcUuid.svc_uuid << std::dec;
                                }
                            }
                        } // end of switch
                    } // end of ifSvcisPlatform
                } // end of for
            }
                // sleep for a minute, before checking again.
                std::this_thread::sleep_for( std::chrono::minutes( 1 ) );
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

    bool OMMonitorWellKnownPMs::isWellKnown
    (
    fpi::SvcUuid svcUuid
    )
    {
        SCOPEDREAD(pmMapLock);

        bool found = false;
        auto mapIter = wellKnownPMsMap.find(svcUuid);

        if ( mapIter != wellKnownPMsMap.end() )
            found = true;

        return found;
    }

    void OMMonitorWellKnownPMs::updateKnownPMsMap
        (
        fpi::SvcUuid svcUuid,
        double       timestamp,
        bool         updateSvcState
        )
    {
        cleanUpOldState(svcUuid, updateSvcState);


        SCOPEDWRITE(pmMapLock);

        auto mapIter = wellKnownPMsMap.find(svcUuid);

        if (mapIter != wellKnownPMsMap.end()) {
            LOGDEBUG << "Updating last heard time for PM:"
                     << std::hex << svcUuid.svc_uuid << std::dec;
        }
        else
        {
            LOGDEBUG << "PM added to well known map with ID:"
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

        LOGDEBUG << "Removing PM:" << std::hex
                 << (*iter).first.svc_uuid << std::dec
                 << " from OM's wellKnownPMs map";

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

    void OMMonitorWellKnownPMs::cleanUpOldState
        (
        fpi::SvcUuid uuid,
        bool         updateSvcState
        )
    {
        {
            // Clear out previous known states related
            // to this  uuid
            fds_mutex::scoped_lock l(genMapLock);
            removedPMsMap[uuid] = 0;

            retryMap[uuid.svc_uuid] = 0;
        } // release generic map lock

        if (updateSvcState) {
            if ( (gl_orch_mgr->getConfigDB()->getStateSvcMap(uuid.svc_uuid) )
                    == fpi::SVC_STATUS_INACTIVE )
            {
                fds_mutex::scoped_lock l(dbLock);
                // don't need to update svc layer, since it already knows this PM is active
                fds::change_service_state(gl_orch_mgr->getConfigDB(),
                                          uuid.svc_uuid,
                                          fpi::SVC_STATUS_ACTIVE,
                                          false);
            }
        }
    }

    void OMMonitorWellKnownPMs::handleRetryOnInactive
        (
        fpi::SvcUuid svcUuid
        )
    {
        LOGDEBUG << "Inactive PM service found:" << std::hex << svcUuid.svc_uuid << std::dec;

        fds_mutex::scoped_lock l(genMapLock);

        auto curTime       = std::chrono::system_clock::now().time_since_epoch();
        auto curTimeInMin  = std::chrono::duration<double,std::ratio<60>>(curTime).count();
        auto elapsedTime   = curTimeInMin - removedPMsMap[svcUuid];

        // Will attempt 3 retries
        if ( (retryMap[svcUuid.svc_uuid] < 3 )  && (removedPMsMap[svcUuid] != 0) )
        {

            LOGDEBUG << "Will re-check heartbeat of PM:"
                     << std::hex << svcUuid.svc_uuid << std::dec;

            retryMap[svcUuid.svc_uuid] += 1;

            OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
            Error err(ERR_OK);

            err = local->om_heartbeat_check(svcUuid);

            if (!err.ok())
            {
                LOGDEBUG << "Received an error while re-checking heartbeat!"
                         << err.GetErrName() << ":" << err.GetErrno();
            }

        } else {
            // If it has been 15 minutes, and svc layer STILL thinks this
            // svc is active, chances are it is, so attempt retries again
            // Setting retry count to 0 will allow for retries the next time
            // this thread wakes up
            if ( elapsedTime > 15 )
            {
                LOGDEBUG << "Will re-check heartbeat of PM:"
                         << std::hex << svcUuid.svc_uuid << std::dec
                         << "in a minute";
                retryMap[svcUuid.svc_uuid] = 0;

            } else {
                LOGDEBUG << "Retry threshold exceeded, will not re-check PM heartbeart for now:"
                         << std::hex << svcUuid.svc_uuid << std::dec;
            }
        }

    }

    void OMMonitorWellKnownPMs::handleStaleEntry
        (
        fpi::SvcUuid svcUuid
        )
    {
        // Update service state in the configDB, svclayer Map
        {
        fds_mutex::scoped_lock l(dbLock);
        fds::change_service_state(gl_orch_mgr->getConfigDB(),
                                  svcUuid.svc_uuid,
                                  fpi::SVC_STATUS_INACTIVE,
                                  false);
        }
        auto iter = wellKnownPMsMap.find(svcUuid);
        if (iter != wellKnownPMsMap.end()) {
            // Remove from our well known PM map
            removeFromPMsMap(iter);
            {
            // Insert to removedPMs map
            fds_mutex::scoped_lock l(genMapLock);
            removedPMsMap[(*iter).first] = (*iter).second;
            }
        }
        else
        {
            LOGDEBUG << "Could not remove PM:"
                     << std::hex << svcUuid.svc_uuid
                     << std::dec << "from wellKnown map: NOT FOUND";
        }
    }

    Error OMMonitorWellKnownPMs::handleActiveEntry
        (
        fpi::SvcUuid svcUuid
        )
    {
        auto iter = wellKnownPMsMap.find(svcUuid);
        Error err(ERR_OK);

        auto curTime       = std::chrono::system_clock::now().time_since_epoch();
        auto curTimeInMin  = std::chrono::duration<double,std::ratio<60>>(curTime).count();

        int64_t svc_uuid = svcUuid.svc_uuid;

        if ( iter != wellKnownPMsMap.end())
        {
            double elapsedTime = curTimeInMin - iter->second;

            LOGDEBUG << "PM uuid: "
                    << std::hex
                    << svc_uuid
                    << std::dec
                    << ", minutes since last heartbeat:"
                    << elapsedTime;

            if ( elapsedTime > 1.5 )
            {
                // It has been more than a minute since we heard from the PM,
                // treat it as a stale map entry
               handleStaleEntry(svcUuid);

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
                             << std::hex << svc_uuid << std::dec;
                    err = Error(ERR_NOT_FOUND);
                }
            }
        }
        else
        {
            LOGDEBUG << "Received heartbeat from PM: "
                     << std::hex << svc_uuid << std::dec
                     << ", re-adding to the well-known map";

            updateKnownPMsMap(svcUuid, curTimeInMin, true);

        }

        return err;
    }
} // namespace fds
