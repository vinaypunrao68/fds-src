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
                                    handleStaleEntry(svc);
                                }
                                break;
                            }
                            default: //SVC_STATUS_ACTIVE, SVC_STATUS_DISCOVERED
                            {

                                if ( gl_orch_mgr->getConfigDB()->getStateSvcMap(svcUuid.svc_uuid)
                                     == fpi::SVC_STATUS_INACTIVE )
                                {
                                    // It is very possible that this thread set a given PM
                                    // to down when it didn't hear a heartbeat back.
                                    // However, svc layer still thinks it's active, so attempt retry
                                    // of the heartbeat msg
                                    handleRetryOnInactive(svcUuid);

                                } else {
                                    Error err = handleActiveEntry(svcUuid, svc, svcMgr);

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

    void OMMonitorWellKnownPMs::updateKnownPMsMap
        (
        fpi::SvcUuid svcUuid,
        double timestamp
        )
    {
        cleanUpOldState(svcUuid);

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
        fpi::SvcUuid uuid
        )
    {
        {
            // Clear out previous known states related
            // to this  uuid
            fds_mutex::scoped_lock l(genMapLock);
            removedPMsMap[uuid] = 0;

            retryMap[uuid.svc_uuid] = 0;
        } // release generic map lock

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

    void OMMonitorWellKnownPMs::handleRetryOnInactive
        (
        fpi::SvcUuid svcUuid
        )
    {
        LOGDEBUG << "Inactive PM service found:" << std::hex << svcUuid.svc_uuid << std::dec;

        fds_mutex::scoped_lock l(genMapLock);

        // Will attempt 3 retries
        if ( (retryMap[svcUuid.svc_uuid] < 3 )  && (removedPMsMap[svcUuid] != 0) )
        {

            LOGDEBUG << "Will re-check heartbeat of PM:"
                     << std::hex << svcUuid.svc_uuid << std::dec;

            retryMap[svcUuid.svc_uuid] += 1;

            auto curTime       = std::chrono::system_clock::now().time_since_epoch();
            auto curTimeInMin  = std::chrono::duration<double,std::ratio<60>>(curTime).count();

            auto elapsedTime = curTimeInMin - removedPMsMap[svcUuid];

            OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
            Error err(ERR_OK);

            err = local->om_heartbeat_check(svcUuid);

            if (!err.ok())
            {
                LOGDEBUG << "Received an error while re-checking heartbeat!"
                         << err.GetErrName() << ":" << err.GetErrno();
            }

        } else {

            auto curTime       = std::chrono::system_clock::now().time_since_epoch();
            auto curTimeInMin  = std::chrono::duration<double,std::ratio<60>>(curTime).count();

            auto elapsedTime = curTimeInMin - removedPMsMap[svcUuid];

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
        fpi::SvcInfo svc
        )
    {
        // Update service state in the configDB, svclayer Map
        {
        fds_mutex::scoped_lock l(dbLock);
        fds::change_service_state(gl_orch_mgr->getConfigDB(),
                                  svc.svc_id.svc_uuid.svc_uuid,
                                  fpi::SVC_STATUS_INACTIVE,
                                  false);
        }
        auto iter = wellKnownPMsMap.find(svc.svc_id.svc_uuid);
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
                     << std::hex << svc.svc_id.svc_uuid.svc_uuid
                     << std::dec << "from wellKnown map: NOT FOUND";
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
                bool foundSvcInfo = svcMgr->getSvcInfo(svcUuid, info);
                if (foundSvcInfo) {
                    handleStaleEntry(info);
                }
                else
                {
                    LOGDEBUG << "Unable to find PM:"
                             << std::hex << svc_uuid << std::dec
                             << " in the svcLayer map";
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

            updateKnownPMsMap(svcUuid, curTimeInMin);

        }

        return err;
    }
} // namespace fds
