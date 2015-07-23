/*
 * Copyright 2015 by Formation Data Systems, Inc.
 */
#include <unordered_map>

#include "OMMonitorWellKnownPMs.h"
#include "OmResources.h"

namespace fds
{
    OMMonitorWellKnownPMs::OMMonitorWellKnownPMs(OrchMgr* om) : om(om)
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
                                LOGWARN << "Well known platformd UUID: "
                                        << std::hex
                                        << svcUuid.svc_uuid
                                        << std::dec
                                        << " status "
                                        <<  svc.svc_status
                                        << " expected status "
                                        << FDS_ProtocolInterface::SVC_STATUS_ACTIVE;
                                break;
                            default:
                                LOGDEBUG << "Well known platformd UUID: "
                                        << std::hex
                                        << svcUuid.svc_uuid
                                        << std::dec
                                        << " status "
                                        <<  svc.svc_status;

                                //TODO put logic into an independent function
                                PmMap::iterator iter = wellKnownPMsMap.find(svcUuid);
                                if ( iter != wellKnownPMsMap.end())
                                {
                                    auto curTime = std::chrono::system_clock::now().time_since_epoch();
                                    auto current = std::chrono::duration<double,std::ratio<60>>(curTime).count();
                                    double elapsedTime = current - iter->second;

                                    LOGDEBUG << "Last heard from PM:" << svcUuid.svc_uuid << " minutesLapsed" << elapsedTime;

                                    if ( elapsedTime > 6 )
                                    {
                                        // it's been more than 6 minutes since we heard from the PM, remove
                                        // from this map and configDB
                                        // configDB->deleteSvcMap(svc);
                                        removeFromPMsMap(iter);
                                    }
                                    else
                                    {
                                        //TODO try catch here
                                        OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
                                        Error err = local->om_heartbeat_check(svcUuid);
                                        if (!err.ok())
                                            LOGDEBUG << "Issue sending out heartbeat check";
                                    }

                                }
                        }
                    }
                }
            }

            // sleep for five minutes, before checking again.
            //TBRemoved
            LOGDEBUG << "OM thread about to sleep:";
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

    void OMMonitorWellKnownPMs::updateKnownPMsMap(fpi::SvcUuid uuid, double timestamp)
    {
        SCOPEDWRITE(pmMapLock);
        // we don't want to use "insert" op since we want to overwrite the timestamp
        // even if it exists
        if (wellKnownPMsMap.find(uuid) != wellKnownPMsMap.end()) {
            LOGDEBUG << "Updating last known time for PM:" << std::hex << uuid.svc_uuid
                     << std::dec << " with time:" << timestamp;
        }
        else
        {
            LOGDEBUG << "New PM added to map with ID:" << std::hex << uuid.svc_uuid <<
                     std::dec << " at time:" << timestamp;
        }

        wellKnownPMsMap[uuid] = timestamp;
    }

    Error OMMonitorWellKnownPMs::removeFromPMsMap(PmMap::iterator& iter) //verify if this iterator passing is correct
    {
        SCOPEDWRITE(pmMapLock);

        wellKnownPMsMap.erase(iter);

        return ERR_OK;

    }

    Error OMMonitorWellKnownPMs::getLastHeardTime(fpi::SvcUuid uuid, double& timestamp)
    {
        SCOPEDREAD(pmMapLock); // wondering if this shared read is what we want. What if a PM tries to update while this is being read?
        PmMap::const_iterator iter = wellKnownPMsMap.find(uuid);
        if (iter == wellKnownPMsMap.end()) {
            return ERR_NOT_FOUND;
        }

        timestamp = iter->second;

        return ERR_OK;
    }
} // namespace fds
