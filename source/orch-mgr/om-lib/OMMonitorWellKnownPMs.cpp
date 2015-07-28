/*
 * Copyright 2015 by Formation Data Systems, Inc.
 */
#include <unordered_map>

#include "OMMonitorWellKnownPMs.h"
#include "OmResources.h"

namespace fds
{
    OMMonitorWellKnownPMs::OMMonitorWellKnownPMs(OrchMgr* om, kvstore::ConfigDB* db)
    : om(om),
      configDB(db)
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
            LOGDEBUG << " Woken up, OM to monitor PMs, entries in svcMgr map: " << entries.size();
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
                                //ERR_SVC_REQUEST_TIMEOUT can cause a well known PM
                                // to go into inactive state without this thread doing anything
                                LOGWARN << "Platformd UUID: "
                                        << std::hex
                                        << svcUuid.svc_uuid
                                        << std::dec
                                        << " status "
                                        <<  svc.svc_status
                                        << " expected status "
                                        << FDS_ProtocolInterface::SVC_STATUS_ACTIVE;
                                auto mapIter = wellKnownPMsMap.find(svc.svc_id.svc_uuid);
                                if (mapIter == wellKnownPMsMap.end()) {
                                    LOGDEBUG << "PM:" << std::hex << svcUuid.svc_uuid << std::dec
                                             << " is no longer a well known service, no action taken";
                                }
                                else
                                {
                                    handleStaleEntry(svc);
                                }
                                break;
                            }
                            default: //SVC_STATUS_ACTIVE, SVC_STATUS_DISCOVERED
                            {
                                LOGDEBUG << "Well known platformd UUID: "
                                        << std::hex
                                        << svcUuid.svc_uuid
                                        << std::dec
                                        << " status "
                                        <<  svc.svc_status;

                                Error err = handleActiveEntry(svcUuid);
                                if (!err.ok())
                                    LOGDEBUG << "Problem sending out heartbeat check to PM:"
                                             << std::hex << svcUuid.svc_uuid << std::dec;

                            }
                        }
                    }
                }
            }

                // sleep for five minutes, before checking again.
                //TBRemoved
                LOGDEBUG << "OM thread about to sleep:";
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

    void OMMonitorWellKnownPMs::updateKnownPMsMap(fpi::SvcUuid uuid, double timestamp)
    {
        SCOPEDWRITE(pmMapLock);
        // we don't want to use "insert" op since we want to overwrite the timestamp
        // even if it exists
        auto mapIter = wellKnownPMsMap.find(uuid);
        if (mapIter != wellKnownPMsMap.end()) {
            LOGDEBUG << "Updating last known time for PM:" << std::hex << uuid.svc_uuid
                     << std::dec;
        }
        else
        {
            LOGDEBUG << "New PM added to map with ID:" << std::hex << uuid.svc_uuid <<
                     std::dec;
        }

        wellKnownPMsMap[uuid] = timestamp;
        //wellKnownPMsMap.emplace(std::make_pair(uuid, timestamp));

    }

    Error OMMonitorWellKnownPMs::removeFromPMsMap(PmMap::iterator iter) //verify if this iterator passing is correct
    {
        SCOPEDWRITE(pmMapLock);

        wellKnownPMsMap.erase(iter);

        return ERR_OK;

    }

    Error OMMonitorWellKnownPMs::getLastHeardTime(fpi::SvcUuid uuid, double& timestamp)
    {
        SCOPEDREAD(pmMapLock); // wondering if this shared read is what we want. What if a PM tries to update while this is being read?
        auto iter = wellKnownPMsMap.find(uuid);
        if (iter == wellKnownPMsMap.end()) {
            return ERR_NOT_FOUND;
        }

        timestamp = iter->second;

        return ERR_OK;
    }

    void OMMonitorWellKnownPMs::handleStaleEntry(fpi::SvcInfo svc)
    {
        LOGDEBUG << "Changing PM:" << std::hex
                 << svc.svc_id.svc_uuid.svc_uuid << std::dec
                 << " from well known to not known service";

        std::vector<fpi::SvcInfo> staleEntries;
        auto svcMgr = MODULEPROVIDER()->getSvcMgr();

        svc.svc_status = fpi::SVC_STATUS_INACTIVE;
        staleEntries.push_back(svc);
        // Update svcManager svcMap
        svcMgr->updateSvcMap(staleEntries);

        // Remove service from the configDB
        fds_mutex::scoped_lock l(dbLock);
        configDB->changeStateSvcMap(svc.svc_id.svc_uuid.svc_uuid, fpi::SVC_STATUS_INACTIVE);

        auto iter = wellKnownPMsMap.find(svc.svc_id.svc_uuid);
        if (iter == wellKnownPMsMap.end()) {
            LOGDEBUG << "Could not find PM:" << std::hex << svc.svc_id.svc_uuid.svc_uuid << std::dec << "in well known PM map";
        }
        else
        {
            // Remove from our well known PM map
            removeFromPMsMap(iter);
        }

    }

    Error OMMonitorWellKnownPMs::handleActiveEntry(fpi::SvcUuid svcUuid)
    {
        auto iter = wellKnownPMsMap.find(svcUuid);
        Error err(ERR_OK);
        fpi::SvcInfo info;
        if ( iter != wellKnownPMsMap.end())
        {
            auto curTime = std::chrono::system_clock::now().time_since_epoch();
            auto current = std::chrono::duration<double,std::ratio<60>>(curTime).count();
            double elapsedTime = current - iter->second;

            LOGDEBUG << "PM:"
                     << std::hex << svcUuid.svc_uuid
                     << std::dec <<" minutesLapsed since last heartbeat:" << elapsedTime;

            if ( elapsedTime > 4 )
            {
                // It has been more than 6 minutes since we heard from the PM,
                // treat it as a stale map entry

                bool foundSvcInfo = MODULEPROVIDER()->getSvcMgr()->getSvcInfo(svcUuid, info);
                if (foundSvcInfo) {
                    handleStaleEntry(info);
                }
                else
                {
                    LOGDEBUG << "Failed to handle stale entry in well known PM map";
                }
            }
            else
            {
                LOGDEBUG << "Alive, send heartbeat check";
                //TODO try catch here
                try {
                OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
                Error err = local->om_heartbeat_check(svcUuid);
                }
                catch(...) {
                    LOGERROR << "OMmonitor encountered exception while "
                             << "sending heartbeat check to PMs";
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
