/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <omutils.h>
#include <fds_uuid.h>
#include <net/net_utils.h>
#include <fdsp_utils.h>
#include <util/properties.h>
#include <kvstore/kvstore.h>
#include <kvstore/configdb.h>
#include <util/stringutils.h>
#include <OmResources.h>

#include "net/SvcMgr.h"

namespace fds 
{

    // TODO(prem): make it use lower case soon
    fds_uint64_t getUuidFromVolumeName(const std::string& name) {
        // std::string lowerName = fds::util::strlower(name);
        return fds_get_uuid64(name);
    }

    fds_uint64_t getUuidFromResourceName(const std::string& name) {
        // std::string lowerName = fds::util::strlower(name);
        return fds_get_uuid64(name);
    }



    
    fpi::FDSP_Node_Info_Type fromSvcInfo( const fpi::SvcInfo& svcinfo )
    {
        fpi::FDSP_Node_Info_Type nodeInfo = fpi::FDSP_Node_Info_Type( );

        fpi::SvcUuid svcUuid = svcinfo.svc_id.svc_uuid;

        nodeInfo.node_uuid = 
            fds::SvcMgr::mapToSvcUuid( svcUuid, fpi::FDSP_PLATFORM ).svc_uuid;
        nodeInfo.service_uuid = svcUuid.svc_uuid;

        nodeInfo.node_name = svcinfo.name;
        nodeInfo.node_type = svcinfo.svc_type; 
        nodeInfo.ip_lo_addr = net::ipString2Addr( svcinfo.ip );
        nodeInfo.control_port = 0;
        nodeInfo.data_port = svcinfo.svc_port;
        nodeInfo.migration_port = 0;

        switch ( svcinfo.svc_status )
        {
            case fpi::SVC_STATUS_INACTIVE_STOPPED:
            case fpi::SVC_STATUS_INACTIVE_FAILED:
            case fpi::SVC_STATUS_STOPPED:
            case fpi::SVC_STATUS_ADDED:
            case fpi::SVC_STATUS_REMOVED:
                nodeInfo.node_state = fpi::FDS_Node_Down;
                break;
            case fpi::SVC_STATUS_INVALID:
                nodeInfo.node_state = fpi::FDS_Node_Down;
                break;
            case fpi::SVC_STATUS_ACTIVE:
            case fpi::SVC_STATUS_STARTED:
                nodeInfo.node_state = fpi::FDS_Node_Up;
                break;
            case fpi::SVC_STATUS_DISCOVERED:
                nodeInfo.node_state = fpi::FDS_Node_Discovered;
                break;
            case fpi::SVC_STATUS_STANDBY:
                nodeInfo.node_state = fpi::FDS_Node_Standby;
                break;
            default:
                LOGERROR <<"Unexpected service status!";
        }

        return nodeInfo;
    }

    fpi::FDSP_NodeState fromServiceStatus(fpi::ServiceStatus svcStatus) {
        fpi::FDSP_NodeState retNodeState = fpi::FDS_Node_Down;
        switch ( svcStatus )
        {
            case fpi::SVC_STATUS_INACTIVE_STOPPED:
            case fpi::SVC_STATUS_INACTIVE_FAILED:
            case fpi::SVC_STATUS_STOPPED:
            case fpi::SVC_STATUS_ADDED:
            case fpi::SVC_STATUS_REMOVED:
                retNodeState = fpi::FDS_Node_Down;
                break;
            case fpi::SVC_STATUS_INVALID:
                retNodeState = fpi::FDS_Node_Down;
                break;
            case fpi::SVC_STATUS_ACTIVE:
            case fpi::SVC_STATUS_STARTED:
                retNodeState = fpi::FDS_Node_Up;
                break;
            case fpi::SVC_STATUS_DISCOVERED:
                retNodeState = fpi::FDS_Node_Discovered;
                break;
            case fpi::SVC_STATUS_STANDBY:
                retNodeState = fpi::FDS_Node_Standby;
                break;
            default:
                LOGERROR << "Unexpected service status, fix this method!";
        }
        return retNodeState;
    }

    // Usage: For functions related to add/remove/start/stop services
    // that require manipulation of passed in svcInfo vector depending
    //  on the current state of the system
    void
    updateSvcInfoList
        (
        std::vector<fpi::SvcInfo>& svcInfos,
        bool smFlag,
        bool dmFlag,
        bool amFlag
        )
    {
        LOGDEBUG << "Updating svcInfoList "
                 << " smFlag: " << smFlag
                 << " dmFlag: " << dmFlag
                 << " amFlag: " << amFlag
                 << " size of vector: " << svcInfos.size();

        std::vector<fpi::SvcInfo>::iterator iter;

        iter = fds::isServicePresent(svcInfos, FDS_ProtocolInterface::FDSP_MgrIdType::FDSP_STOR_MGR );
        if ((iter != svcInfos.end()) && smFlag)
        {
            LOGDEBUG << "Erasing SM svcInfo from list";
            svcInfos.erase(iter);
        }


        iter = fds::isServicePresent(svcInfos, FDS_ProtocolInterface::FDSP_MgrIdType::FDSP_DATA_MGR );

        if ((iter != svcInfos.end()) && dmFlag)
        {
            LOGDEBUG << "Erasing DM svcInfo from list";
            svcInfos.erase(iter);
        }


        iter = fds::isServicePresent(svcInfos, FDS_ProtocolInterface::FDSP_MgrIdType::FDSP_ACCESS_MGR );
        if ( ( iter != svcInfos.end( ) ) && amFlag )
        {
            LOGDEBUG << "Erasing AM svcInfo from list";
            svcInfos.erase(iter);
        }
    }

    std::vector<fpi::SvcInfo>::iterator isServicePresent
        (
        std::vector<fpi::SvcInfo>& svcInfos,
        FDS_ProtocolInterface::FDSP_MgrIdType svcType
        )
    {
        std::vector<fpi::SvcInfo>::iterator iter;
        iter = std::find_if(svcInfos.begin(), svcInfos.end(),
                            [svcType](fpi::SvcInfo info)->bool
                            {
                            return info.svc_type == svcType;
                            });
        return iter;

    }

    fpi::SvcInfo getNewSvcInfo
        (
        FDS_ProtocolInterface::FDSP_MgrIdType type
        )
    {
        LOGDEBUG << "Creating new SvcInfo of type:" << type;
        fpi::SvcInfo* info = new fpi::SvcInfo();
        info->__set_svc_type(type);
        // should not matter here whether it is inactive_stopped or
        // inactive_failed
        info->__set_svc_status(fpi::SVC_STATUS_INACTIVE_STOPPED);
        return *info;

    }

    void getServicesToStart
        (
        bool start_sm,
        bool start_dm,
        bool start_am,
        kvstore::ConfigDB* configDB,
        NodeUuid nodeUuid,
        std::vector<fpi::SvcInfo>& svcInfoList)
        {
            NodeServices services;

            // Services that are being started after a node or domain shutdown,
            // will already be present in the configurationDB.
            // If not, it is the very initial startup of the domain, we will
            // need to set up new svcInfos

            if (configDB->getNodeServices(nodeUuid, services)) {
                fpi::SvcInfo svcInfo;
                fpi::SvcUuid svcUuid;

                if ( start_am ) {
                    if (services.am.uuid_get_val() != 0) {
                        LOGDEBUG << "Service AM present in configDB for node:"
                                 << std::hex << nodeUuid.uuid_get_val()
                                 << std::dec;

                        svcUuid.svc_uuid = services.am.uuid_get_val();
                        bool ret = MODULEPROVIDER()->getSvcMgr()->getSvcInfo(svcUuid, svcInfo);
                        if (ret) {
                            svcInfoList.push_back(svcInfo);
                        } else {
                            LOGERROR <<"Error retrieving known AM svcInfo for node:"
                                     << std::hex << nodeUuid.uuid_get_val() << std::dec;
                        }
                    } else {
                        // It could be that other services are present but not this
                        // one. So add new.
                        svcInfoList.push_back(getNewSvcInfo(fpi::FDSP_ACCESS_MGR));
                    }
                }
                if ( start_sm ) {
                    if (services.sm.uuid_get_val() != 0) {
                        LOGDEBUG << "Service SM present in configDB for node:"
                                 << std::hex << nodeUuid.uuid_get_val()
                                 << std::dec;

                        svcUuid.svc_uuid = services.sm.uuid_get_val();
                        bool ret = MODULEPROVIDER()->getSvcMgr()->getSvcInfo(svcUuid, svcInfo);
                        if (ret) {
                            svcInfoList.push_back(svcInfo);
                        } else {
                            LOGERROR <<"Error retrieving known SM svcInfo for node:"
                                     << std::hex << nodeUuid.uuid_get_val() << std::dec;
                        }
                    } else {
                        svcInfoList.push_back(getNewSvcInfo(fpi::FDSP_STOR_MGR));
                    }
                }
                if ( start_dm ) {
                    if (services.dm.uuid_get_val() != 0) {
                        LOGDEBUG << "Service DM present in configDB for node:"
                                 << std::hex << nodeUuid.uuid_get_val()
                                 << std::dec;

                        svcUuid.svc_uuid = services.dm.uuid_get_val();
                        bool ret = MODULEPROVIDER()->getSvcMgr()->getSvcInfo(svcUuid, svcInfo);
                        if (ret) {
                            svcInfoList.push_back(svcInfo);
                        } else {
                            LOGERROR <<"Error retrieving known DM svcInfo for node:"
                                     << std::hex << nodeUuid.uuid_get_val() << std::dec;
                        }
                    } else {
                        svcInfoList.push_back(getNewSvcInfo(fpi::FDSP_DATA_MGR));
                    }
                }
            } else {
                /**
                * We interpret no Services information in ConfigDB,
                * this must be for a new Node.
                */
                if (start_am) {
                    svcInfoList.push_back(getNewSvcInfo(fpi::FDSP_ACCESS_MGR));
                }
                if (start_dm) {
                    svcInfoList.push_back(getNewSvcInfo(fpi::FDSP_DATA_MGR));
                }
                if (start_sm) {
                    svcInfoList.push_back(getNewSvcInfo(fpi::FDSP_STOR_MGR));
                }
            }
        }

    void retrieveSvcId
        (
        int64_t pmID,
        fpi::SvcUuid& svcUuid,
        FDS_ProtocolInterface::FDSP_MgrIdType svcType
        )
    {
        switch (svcType)
        {
            case fpi::FDSP_STOR_MGR:
                svcUuid.svc_uuid = pmID + fpi::FDSP_STOR_MGR;
                break;
            case fpi::FDSP_DATA_MGR:
                svcUuid.svc_uuid = pmID + fpi::FDSP_DATA_MGR;
                break;
            case fpi::FDSP_ACCESS_MGR:
                svcUuid.svc_uuid = pmID + fpi::FDSP_ACCESS_MGR;
                break;
            default:
                LOGDEBUG << "Not AM,DM,SM svc, skipping..";
                break;
        }

    }

    void populateAndRemoveSvc(fpi::SvcUuid serviceTypeId,
                              fpi::FDSP_MgrIdType type,
                              std::vector<fpi::SvcInfo> svcInfos,
                              kvstore::ConfigDB* configDB)
    {

        OmExternalApi::getInstance()->addToRemoveList(serviceTypeId.svc_uuid, type);

        updateSvcMaps( configDB,
                       serviceTypeId.svc_uuid,
                       fpi::SVC_STATUS_REMOVED);
    }

    /*
         * Update the svcMap in both the configDB and svc layer
         *
         * @param configDB           pointer to OM's configDB object
         * @param svc_uuid           svc uuid of the service that is being updated
         * @param svc_status         desired status of the service
         * @param handlerUpdate      this update call is coming in through om-svc-handler
         *                           The implication being that a svcInfo was passed in
         * @param registration       { true:  if the update is being called from the svc
         *                                    registration path
         *                             false: svc update is coming from any other code path
         *                           }
         * @param incomingSvcInfo    svcInfo of the svc that is either being registered,
         *                           or was received by the OM handler.
         *                           This info is guaranteed to be populated if the
         *                           handlerUpdate/registration flag is true. This svcInfo,
         *                           if valid, should not be modified in any way by this function
         *
        */

    void updateSvcMaps( kvstore::ConfigDB*       configDB,
                        const fds_uint64_t       svc_uuid,
                        const fpi::ServiceStatus svc_status,
                        bool                     handlerUpdate,   // false by default
                        bool                     registration,   // false by default
                        fpi::SvcInfo             incomingSvcInfo // new, empty object by default
                      )
    {
        fpi::SvcUuid    uuid;
        fpi::SvcInfo    svcLayerInfo;
        fpi::SvcInfo    dbInfo;
        fpi::SvcInfoPtr svcLayerInfoPtr;
        fpi::SvcInfoPtr dbInfoPtr;

        uuid.svc_uuid = svc_uuid;

        bool ret = false;

        if (handlerUpdate || registration)
        {
            /*
             * ======================================================
             *  Setting up svcInfo objects from configDB and svcLayer
             * ======================================================
             * */


            LOGDEBUG << "!!Incoming svcInfo from either OM handler or registration, svc:"
                     << std::hex << incomingSvcInfo.svc_id.svc_uuid.svc_uuid << std::dec;
            bool validUpdate = false;
            bool svcLayerRecordFound = false;
            bool dbRecordFound = false;


            /*
             * ======================================================
             *  Retrieve svcLayer record
             * ======================================================
             * */
            ret = MODULEPROVIDER()->getSvcMgr()->getSvcInfo(uuid, svcLayerInfo);
            if ( ret )
            {
                svcLayerRecordFound = true;
                //updateKnownRecord = knownRecordNeedsUpdate(incomingSvcInfo, svcLayerInfo);
            } else {

                LOGWARN << "!!Could not find svcInfo for uuid:"
                          << std::hex << svc_uuid << std::dec
                          << " in the svcMap";

                svcLayerInfo = incomingSvcInfo;
            }

            /*
             * ======================================================
             *  Retrieve DB record
             * ======================================================
             * */
            ret = configDB->getSvcInfo(svc_uuid, dbInfo);

            if (ret)
            {
                dbRecordFound = true;
                dbInfoPtr = boost::make_shared<fpi::SvcInfo>(dbInfo);
            } else {
                LOGWARN << "!!Could not find SvcInfo for uuid:"
                        << std::hex << svc_uuid << std::dec
                        << " in the OM's configDB";

                dbInfo    = incomingSvcInfo;
                dbInfoPtr = boost::make_shared<fpi::SvcInfo>(dbInfo);
            }


            /*
             * ========================================================
             *  Depending on which records were retrieved, take action
             *  Note: In the table below, >, <, == are used to indicate
             *  results after comparison of incarnation numbers
             * ========================================================
             * */

            //+----------------+---------------------+--------------------------+------------------+-------------------------------------+
            //| dbRecordFound  | svcLayerRecordFound | Incoming VS found Record |  Apply incoming? |              Action                 |
            //+----------------+---------------------+--------------------------+------------------+-------------------------------------+
            //     True        |       False         |   incoming >= current    |      Yes         | Update DB, svcLayer with incoming   |
            //                 |                     |   incoming < current     |      No          | Only update svcLayer with DB record |
            //                 |                     |   incoming = 0           |      Yes         | Update DB, svcLayer with incoming   |
            //                 |                     |                          |                  | (after updating 0 incarnationNo)    |
            //+----------------+---------------------+--------------------------+-------------------------------------+
            //     False       |       True          |   incoming >= current    |      Yes         | Update svcLayer, DB with incoming   |
            //                 |                     |   incoming < current     |      No          | Only update DB with svcLayer record |
            //                 |                     |   incoming = 0           |      Yes         | Update svcLayer, DB with incoming   |
            //                 |                     |                          |                  | (after updating 0 incarnationNo)    |
            //+----------------+---------------------+--------------------------+-------------------------------------+
            //     False       |       False         |   only incoming is valid |      Yes         | Check incoming incarnationNo, if    |
            //                 |                                                |                  | 0, update, then update DB & svcLayer|
            //+----------------+---------------------+--------------------------+-------------------------------------+
            //     True        |       True          |   incoming >= svcLayer   |      Yes         | Update svcLayer, DB with incoming   |
            //                 |                     |   svcLayer > configDB    |                  |
            //                 |                     |                          |                  |
            //                 |                     |   incoming < svcLayer    |      No          | DB has most current, update svcLayer|
            //                 |                     |   svcLayer < configDB    |                  | with DB record, ignore incoming     |
            //                 |                     |                          |                  |
            //                 |                     |   incoming >= svcLayer   |      No          | DB has most current, update svcLayer|
            //                 |                     |   svcLayer < configDB    |                  | with DB record, ignore incoming     |
            //                 |                     |                          |                  |
            //                 |                     |   incoming < svcLayer    |      No          | SvcLayer has most current, update DB|
            //                 |                     |   svcLayer > configDB    |                  | with svcLyr record, ignore incoming |
            //                 |                     |                          |                  |
            //                 |                     |   incoming = 0           |      Yes         | Update svcLayer, DB with incoming   |
            //+----------------+---------------------+--------------------------+------------------+-------------------------------------+


            if (!svcLayerRecordFound && !dbRecordFound)
            {
                LOGNORMAL << "!!Case: No svcLayer or DB record found, updating both with incoming";
                // 1. Update DB to incoming
                // 2. Update svcLayer to incoming
                // 3. Broadcast svcMap

                configDB->changeStateSvcMap( dbInfoPtr );
                MODULEPROVIDER()->getSvcMgr()->updateSvcMap( {svcLayerInfo} );
                OM_NodeDomainMod::om_loc_domain_ctrl()->om_bcast_svcmap();

            } else if ( !svcLayerRecordFound && dbRecordFound ) {

                // DB has a record, check if it is more current than incoming
                validUpdate = OmExternalApi::getInstance()->isIncomingUpdateValid(incomingSvcInfo, dbInfo);

                if (validUpdate)
                {
                    LOGNORMAL << "!!Case: No svcLayer record, found DB record, updating both with incoming";
                    // 1. Update DB to incoming
                    // 2. Update svcLayer to incoming
                    // 3. Broadcast svcMap
                    fpi::SvcInfoPtr incomingSvcInfoPtr = boost::make_shared<fpi::SvcInfo>(incomingSvcInfo);
                    configDB->changeStateSvcMap( incomingSvcInfoPtr );
                    MODULEPROVIDER()->getSvcMgr()->updateSvcMap( {incomingSvcInfo} );
                    OM_NodeDomainMod::om_loc_domain_ctrl()->om_bcast_svcmap();

                } else {
                    LOGNORMAL << "!!Case: No svcLayer record, found DB record, updating svcLayer with DB record";
                    // 1. No update to DB
                    // 2. Update svcLayer to DB
                    // 3. Broadcast svcMap
                    MODULEPROVIDER()->getSvcMgr()->updateSvcMap( {dbInfo} );
                    OM_NodeDomainMod::om_loc_domain_ctrl()->om_bcast_svcmap();
                }

            } else if ( svcLayerRecordFound && !dbRecordFound ) {

                // Only svcLayer has a record, check if it is more current than incoming
                validUpdate = OmExternalApi::getInstance()->isIncomingUpdateValid(incomingSvcInfo, svcLayerInfo);

                if (validUpdate)
                {
                    LOGNORMAL << "!!Case: No DB record, found svcLayer record, updating both with incoming";
                    // 1. Update DB to incoming
                    // 2. Update svcLayer to incoming
                    // 3. Broadcast svcMap
                    configDB->changeStateSvcMap( boost::make_shared<fpi::SvcInfo>(incomingSvcInfo) );
                    MODULEPROVIDER()->getSvcMgr()->updateSvcMap( {incomingSvcInfo} );
                    OM_NodeDomainMod::om_loc_domain_ctrl()->om_bcast_svcmap();

                } else {
                    LOGNORMAL << "!!Case: No DB record, found svcLayer record, updating DB with svcLayer";
                    // svcLayer has most current info, update the DB since it has no record of this svc
                    // 1. Update DB to svcLayer
                    // 2. No update to svcLayer
                    // 3. No svcMap broadcast
                    configDB->changeStateSvcMap( boost::make_shared<fpi::SvcInfo>(svcLayerInfo) );
                }
            } else if ( svcLayerRecordFound && dbRecordFound ) {

                svcLayerInfoPtr = boost::make_shared<fpi::SvcInfo>(svcLayerInfo);
                dbInfoPtr       = boost::make_shared<fpi::SvcInfo>(dbInfo);

                validUpdate = OmExternalApi::getInstance()->isIncomingUpdateValid(incomingSvcInfo, svcLayerInfo);
                bool dbHasOlderRecord = dbRecordNeedsUpdate(svcLayerInfo, dbInfo);


                if ( validUpdate && dbHasOlderRecord )
                {
                    LOGNORMAL << "!!Case: Both svcLayer & DB record found, updating both with incoming";
                    // incoming >= svcLayer
                    // svcLayer > configDB
                    // 1. Update DB to incoming
                    // 2. Update svcLayer to incoming
                    // 3. Broadcast svcMap

                    configDB->changeStateSvcMap( boost::make_shared<fpi::SvcInfo>(incomingSvcInfo) );
                    MODULEPROVIDER()->getSvcMgr()->updateSvcMap( {incomingSvcInfo} );
                    OM_NodeDomainMod::om_loc_domain_ctrl()->om_bcast_svcmap();

                } else if ( !validUpdate && dbHasOlderRecord ) {

                    LOGNORMAL << "!!Case: Both svcLayer & DB record found, updating DB with svcLayer record";
                    // incoming < svcLayer
                    // svcLayer > configDB
                    // 1. Update DB to svcLayer
                    // 2. No update to svcLayer
                    // 3. No svcMap broadcast

                    configDB->changeStateSvcMap( boost::make_shared<fpi::SvcInfo>(svcLayerInfo) );

                } else if ( validUpdate && !dbHasOlderRecord ) {

                    LOGNORMAL << "!!Case: Both svcLayer & DB record found, updating svcLayer with DB record";
                    // incoming >= svcLayer
                    // svcLayer < configDB
                    // 1. No update to DB
                    // 2. Update svcLayer to DB
                    // 3. Broadcast svcMap

                    MODULEPROVIDER()->getSvcMgr()->updateSvcMap( {dbInfo} );
                    OM_NodeDomainMod::om_loc_domain_ctrl()->om_bcast_svcmap();

                } else if ( !validUpdate && !dbHasOlderRecord ) {

                    LOGNORMAL << "!!Case: Both svcLayer & DB record found, updating svcLayer with DB record";
                    // incoming < svcLayer
                    // svcLayer < configDB
                    // 1. No update to DB
                    // 2. Update svcLayer to DB
                    // 3. Broadcast svcMap

                    MODULEPROVIDER()->getSvcMgr()->updateSvcMap( {dbInfo} );
                    OM_NodeDomainMod::om_loc_domain_ctrl()->om_bcast_svcmap();
                }
            }

        } else {

            LOGNORMAL << "!!Update coming from a non-registration/handler path";

            // In this path it is fair to assume that the update is coming in
            // for exactly the same incarnationNo but the service status is potentially
            // different, so this path won't have incarnation checks like above
            // because all that is passed in to the function is desired service status

            /*
             * ==========================================
             *  Setting up svcInfo object from configDB
             * ==========================================
             * */

            bool ret = false;
            // Do configDB related work first so the window
            // for svcLayer getting updated while we potentially
            // wait on locks is minimized

            ret = configDB->getSvcInfo(svc_uuid, dbInfo);

            fpi::ServiceStatus initialDbStatus = fpi::SVC_STATUS_INVALID;
            if (ret)
            {
                initialDbStatus = dbInfo.svc_status;
                dbInfo.svc_status = svc_status;

            } else {
                LOGWARN << "!!Could not find SvcInfo for uuid:"
                        << std::hex << svc_uuid << std::dec
                        << " in the OM's configDB";

                dbInfo.svc_id.svc_uuid.svc_uuid = svc_uuid;
                dbInfo.svc_status    = svc_status;
                dbInfo.incarnationNo = 0;
            }

            dbInfoPtr = boost::make_shared<fpi::SvcInfo>(dbInfo);
            /*
             * ==========================================
             *  Setting up svcInfo object from svcLayer
             * ==========================================
             * */


            fpi::SvcInfo initialSvcLayerInfo;
            ret = MODULEPROVIDER()->getSvcMgr()->getSvcInfo(uuid, svcLayerInfo);

            if (ret)
            {
                initialSvcLayerInfo = svcLayerInfo;

                // update the status to what we need to update to, since
                // this svcInfoPtr will be treated as "incoming" record
                svcLayerInfo.svc_status = svc_status;

            } else {
                LOGWARN << "!!Could not find svcInfo for uuid:"
                          << std::hex << svc_uuid << std::dec
                          << " in the svcMap, generating new";

                svcLayerInfo.svc_id.svc_uuid.svc_uuid = uuid.svc_uuid;
                svcLayerInfo.svc_status    = svc_status;
                svcLayerInfo.incarnationNo = 0;
             }

            svcLayerInfoPtr = boost::make_shared<fpi::SvcInfo>(svcLayerInfo);

            // The only time both incarnationNos can be zero is when a service is getting added
            // into the cluster for the very first time. The ADDED, STARTED status is set prior to even
            // sending the start to PM, so the service processes aren't running yet.
            // Could be that svcLayer is unaware of the svc too
            bool svcAddition = false;

            if ( !(svc_status == fpi::SVC_STATUS_ADDED || svc_status == fpi::SVC_STATUS_STARTED) &&
                   svcLayerInfo.incarnationNo == 0 && dbInfo.incarnationNo == 0 )
            {
                LOGWARN << "!!No record for svc found in SvcLayer or ConfigDB!! Will not make any updates, returning..";

                return;
            } else {
                svcAddition = true;
            }

            /*
             * ======================================================
             *  (1) Determine which entity has the most current info
             *  using incarnation number
             *  (2) If DB has the more current, determine whether it
             *  needs an update at all
             * ======================================================
             * */

            bool dbHasOlderRecord = dbRecordNeedsUpdate(svcLayerInfo, dbInfo);

            if ( dbHasOlderRecord )
            {
                LOGDEBUG << "!!ConfigDB has older record, updating it with svcLayer record";
                // If the flag is true implies that svcLayer has a more recent
                // record of the service in question, so first update the record
                // in configDB
                configDB->changeStateSvcMap( svcLayerInfoPtr );

                // Update the dbInfo record that is being managed by the dbInfoPtr to the
                // latest (from the update above)
                // we will use this record now to update svcLayer
                ret = configDB->getSvcInfo(svc_uuid, dbInfo);

                if (!ret)
                {
                    LOGWARN << "!!Could not retrieve updated svc record from configDB, potentially making outdated updates ";
                }
            } else {
                // db has latest record, go ahead and update it for the new svc status,
                // given that the statuses are different.
                // This record will be used to update svcLayer

                if (initialDbStatus != fpi::SVC_STATUS_INVALID)
                {
                    if (dbInfo.svc_status != initialDbStatus)
                    {
                        configDB->changeStateSvcMap( dbInfoPtr );
                    } else {
                        LOGNOTIFY << "!!ConfigDB already reflects desired state for svc:"
                                  << std::hex << svc_uuid << std::dec << " state:"
                                  << OmExternalApi::getInstance()->printSvcStatus(initialDbStatus);
                    }
                } else {
                    configDB->changeStateSvcMap( dbInfoPtr );
                }
            }

            /*
             * ======================================================
             *  Determine whether:
             *  (1) a change has taken place for this
             *  svc in svc layer while we were doing above checks.
             *  Initiate new update if that is the case
             *  (2) whether svcLayer svcMap requires an update at all
             * ======================================================
             * */

            fpi::SvcInfo svcLayerNewerInfo;
            ret = MODULEPROVIDER()->getSvcMgr()->getSvcInfo(uuid, svcLayerNewerInfo);

            if ( ret )
            {
                // If we had a valid initialSvcLayerInfo object &&
                // either (1) newer info has a bigger incarnationNo or (2) both original and new
                // have same incarnationNo but different statuses.
                if ( initialSvcLayerInfo.svc_id.svc_uuid.svc_uuid != 0 &&
                     ( svcLayerNewerInfo.incarnationNo > initialSvcLayerInfo.incarnationNo ||
                       (svcLayerNewerInfo.incarnationNo ==  initialSvcLayerInfo.incarnationNo &&
                        svcLayerNewerInfo.svc_status != initialSvcLayerInfo.svc_status) ) )
                {
                    LOGNOTIFY << "!!Svc:" << std::hex << uuid.svc_uuid << std::dec
                              << " has already changed in service layer from [incarnation:"
                              << initialSvcLayerInfo.incarnationNo << ", status:"
                              << OmExternalApi::getInstance()->printSvcStatus(initialSvcLayerInfo.svc_status)
                              << "] to [incarnation:" << svcLayerNewerInfo.incarnationNo << ", status:"
                              << OmExternalApi::getInstance()->printSvcStatus(svcLayerNewerInfo.svc_status)
                              << "]. Initiating new update";

                    // Something has already changed in the svc layer, so the current update of
                    // status we are proceeding with is already out-dated
                    // Call a new update of configDB, and return from the current cycle
                    // No locks being acquired on function entry, so we should be OK with
                    // this recursive call.

                    if (svcLayerNewerInfo.incarnationNo > initialSvcLayerInfo.incarnationNo)
                    {
                        // If the incarnationNo has changed, must take the first path to perform
                        // correct comparisons, set handler flag to true
                        updateSvcMaps(configDB, uuid.svc_uuid, svcLayerNewerInfo.svc_status, true, false, svcLayerNewerInfo);

                    } else {
                        // If only status has changed, we don't care incarnationNo comparisons
                        updateSvcMaps(configDB, uuid.svc_uuid, svcLayerNewerInfo.svc_status);
                    }

                    return;
                }

                // If svcLayer already had the latest status &&
                // the status has not changed since &&
                // the incarnation number is the same as the DB, we can afford not to do an update and
                // re-broadcast
                if ( (initialSvcLayerInfo.svc_status == svc_status) &&
                     (initialSvcLayerInfo.svc_status == svcLayerNewerInfo.svc_status) &&
                     (svcLayerNewerInfo.incarnationNo == dbInfoPtr->incarnationNo) )
                {
                    LOGNOTIFY << "!!SvcLayer already has the latest [incarnation:"
                              << svcLayerNewerInfo.incarnationNo << ", status:"
                              << OmExternalApi::getInstance()->printSvcStatus(svcLayerNewerInfo.svc_status)
                              << "] for service:" << std::hex << uuid.svc_uuid << std::dec
                              << " , no need to update & broadcast";

                    return;
                }
            }

            /*
             * ====================================================================
             *  As consistent a view as we can get to, update and broadcast svc map
             * ====================================================================
             * */

            // If the service is in the process of being added/started, do not update svcLayer because we do not
            // have incarnationNumbers yet. (it is 0)
            if ( !svcAddition )
            {
                LOGNOTIFY << "!!Updating SvcMgr svcMap now";

                // The only reason why an update to svcLayer svcMap will be rejected is if
                // in between us updating configDB, svcLayer received an update about this
                // same service and now has a newer incarnation number
                // In this case, we are still OK. A newer incarnation number implies a newer
                // svc process. So OM will definitely receive registration of the svc.
                // The associated update to configDB using this same method should pick up
                // the latest record svcLayer has and perform updates for both
                // (which will be applied under the condition incarnationNo is equal, but status is different)
                // Assumption is that EVERY change to service state of configDB comes through here
                MODULEPROVIDER()->getSvcMgr()->updateSvcMap( {*dbInfoPtr} );

                // Broadcast the service map
                OM_NodeDomainMod::om_loc_domain_ctrl()->om_bcast_svcmap();
            }
        }
    }

    /*
     * Function to compare service records in svcLayer and configDB
     */
    bool dbRecordNeedsUpdate( fpi::SvcInfo svcLayerInfo,
                              fpi::SvcInfo dbInfo )
    {
        fds_bool_t ret(false);

        // It should never be that incarnationNo for both records is 0. We should have
        // filtered those records before proceeding to this point

        if ( svcLayerInfo.incarnationNo == 0 && dbInfo.incarnationNo != 0 ) {
            // svcLayer does not have a record of this svc at all, so no update
            // required to the DB
            ret = false;
        } else if ( svcLayerInfo.incarnationNo != 0 && dbInfo.incarnationNo == 0 ) {
            // db does not have a record of this svc at all, so definitely
            // requires an update with the help of svcLayer record
            ret = true;
        } else if ( svcLayerInfo.incarnationNo < dbInfo.incarnationNo ) {
            // db has more current information, no update required
            ret = false;
        } else if ( svcLayerInfo.incarnationNo == dbInfo.incarnationNo ) {
            // db and svcLayer have the same information regarding incarnationNo, nothing
            // to do. The svc state will get updated separately
            ret = false;
        } else if ( svcLayerInfo.incarnationNo > dbInfo.incarnationNo ) {
            // SvcLayer has more recent information, will need to update the record
            // in the db first before updating svc state
            ret = true;
        } else {
            LOGWARN << "!!Indeterminate incarnation number for svc records, could lead to incorrect state updates";
            ret = false;
        }

        return ret;
    }
}  // namespace fds
