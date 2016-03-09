/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <OmIntUtilApi.h>
#include <fds_uuid.h>
#include <net/net_utils.h>
#include <fdsp_utils.h>
#include <util/properties.h>
#include <kvstore/kvstore.h>
#include <kvstore/configdb.h>
#include <util/stringutils.h>
#include <OmResources.h>

#include "net/SvcMgr.h"
#include "OmExtUtilApi.h"

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


    /***********************************************************************************
     *                              Converter functions
     ***********************************************************************************/
    
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

    fpi::FDSP_NodeState fromServiceStatus(fpi::ServiceStatus svcStatus)
    {
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

    /***********************************************************************************
     * Utility functions related to service state change actions
     ***********************************************************************************/

    // Usage: For functions related to add/remove/start/stop services
    // that require manipulation of passed in svcInfo vector depending
    //  on the current state of the system
    void updateSvcInfoList ( std::vector<fpi::SvcInfo>& svcInfos,
                             bool smFlag,
                             bool dmFlag,
                             bool amFlag )
    {
        LOGDEBUG << "Updating svcInfoList "
                 << " smFlag: " << smFlag
                 << " dmFlag: " << dmFlag
                 << " amFlag: " << amFlag
                 << " size of vector: " << svcInfos.size();

        std::vector<fpi::SvcInfo>::iterator iter;

        iter = isServicePresent(svcInfos, FDS_ProtocolInterface::FDSP_MgrIdType::FDSP_STOR_MGR );
        if ((iter != svcInfos.end()) && smFlag)
        {
            LOGDEBUG << "Erasing SM svcInfo from list";
            svcInfos.erase(iter);
        }


        iter = isServicePresent(svcInfos, FDS_ProtocolInterface::FDSP_MgrIdType::FDSP_DATA_MGR );

        if ((iter != svcInfos.end()) && dmFlag)
        {
            LOGDEBUG << "Erasing DM svcInfo from list";
            svcInfos.erase(iter);
        }


        iter = isServicePresent(svcInfos, FDS_ProtocolInterface::FDSP_MgrIdType::FDSP_ACCESS_MGR );
        if ( ( iter != svcInfos.end( ) ) && amFlag )
        {
            LOGDEBUG << "Erasing AM svcInfo from list";
            svcInfos.erase(iter);
        }
    }

    SvcInfoIter isServicePresent( std::vector<fpi::SvcInfo>& svcInfos,
                                  FDS_ProtocolInterface::FDSP_MgrIdType svcType )
    {
        std::vector<fpi::SvcInfo>::iterator iter;
        iter = std::find_if(svcInfos.begin(), svcInfos.end(),
                            [svcType](fpi::SvcInfo info)->bool
                            {
                            return info.svc_type == svcType;
                            });
        return iter;

    }

    fpi::SvcInfo getNewSvcInfo( FDS_ProtocolInterface::FDSP_MgrIdType type )
    {
        LOGDEBUG << "Creating new SvcInfo of type:" << type;
        fpi::SvcInfo* info = new fpi::SvcInfo();
        info->__set_svc_type(type);
        // should not matter here whether it is inactive_stopped or
        // inactive_failed
        info->__set_svc_status(fpi::SVC_STATUS_INACTIVE_STOPPED);
        return *info;

    }

    void getServicesToStart( bool start_sm,
                             bool start_dm,
                             bool start_am,
                             kvstore::ConfigDB* configDB,
                             NodeUuid nodeUuid,
                             std::vector<fpi::SvcInfo>& svcInfoList )
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

    void retrieveSvcId( int64_t pmID,
                        fpi::SvcUuid& svcUuid,
                        FDS_ProtocolInterface::FDSP_MgrIdType svcType )
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

    void populateAndRemoveSvc( fpi::SvcUuid serviceTypeId,
                               fpi::FDSP_MgrIdType type,
                               std::vector<fpi::SvcInfo> svcInfos,
                               kvstore::ConfigDB* configDB )
    {

        OmExtUtilApi::getInstance()->addToRemoveList(serviceTypeId.svc_uuid, type);

        updateSvcMaps( configDB,
                       serviceTypeId.svc_uuid,
                       fpi::SVC_STATUS_REMOVED,
                       type);
    }



    /***********************************************************************************
     * Functions related to service state update handling
     ***********************************************************************************/

    /*
         * Update the svcMap in both the configDB and svc layer
         *
         * @param configDB           pointer to OM's configDB object
         * @param svc_uuid           svc uuid of the service that is being updated
         * @param svc_status         desired status of the service
         * @param handlerUpdate      this update call is coming in through om-svc-handler
         *                           The implication being that a svcInfo was passed in
         * @param registration       true:  if the update is being called from the svc
         *                                    registration path
         *                           false: svc update is coming from any other code path
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
                        fpi::FDSP_MgrIdType      svcType,
                        bool                     handlerUpdate,   // false by default
                        bool                     registration,   // false by default
                        fpi::SvcInfo             incomingSvcInfo // new, empty object by default
                      )
    {
        fpi::SvcUuid    uuid;
        fpi::SvcInfo    svcLayerInfoUpdate;
        fpi::SvcInfo    dbInfoUpdate;
        fpi::SvcInfoPtr svcLayerInfoPtr;
        fpi::SvcInfoPtr dbInfoPtr;

        uuid.svc_uuid = svc_uuid;

        bool ret = false;

        LOGNORMAL << "!!Acquiring update map lock";

        std::mutex  updateMapMutex;
        std::unique_lock<std::mutex> updateLock(updateMapMutex);


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


            if (incomingSvcInfo.incarnationNo == 0) // Can only be the case if a service is being added
            {
                LOGDEBUG << "Updating zero incarnation number!";
                incomingSvcInfo.incarnationNo = util::getTimeStampSeconds();
            }
            /*
             * ======================================================
             *  Retrieve svcLayer record
             * ======================================================
             * */
            ret = MODULEPROVIDER()->getSvcMgr()->getSvcInfo(uuid, svcLayerInfoUpdate);
            if ( ret )
            {
                svcLayerRecordFound = true;
                //updateKnownRecord = knownRecordNeedsUpdate(incomingSvcInfo, svcLayerInfoUpdate);
            } else {

                LOGWARN << "!!Could not find svcInfo for uuid:"
                          << std::hex << svc_uuid << std::dec
                          << " in the svcMap";

                svcLayerInfoUpdate = incomingSvcInfo;
            }

            /*
             * ======================================================
             *  Retrieve DB record
             * ======================================================
             * */
            ret = configDB->getSvcInfo(svc_uuid, dbInfoUpdate);

            if (ret)
            {
                dbRecordFound = true;
                dbInfoPtr = boost::make_shared<fpi::SvcInfo>(dbInfoUpdate);
            } else {
                LOGWARN << "!!Could not find SvcInfo for uuid:"
                        << std::hex << svc_uuid << std::dec
                        << " in the OM's configDB";

                dbInfoUpdate    = incomingSvcInfo;
                dbInfoPtr = boost::make_shared<fpi::SvcInfo>(dbInfoUpdate);
            }


            // The only time both incarnationNos can be zero is when a service is getting added
            // into the cluster for the very first time. The ADDED, STARTED status is set prior to even
            // sending the start to PM, so the service processes aren't running yet. In these cases
            // the svcInfo is not fully populated yet so if this condition is true we will NOT
            // update the svcLayer svcMap because that can end up with broken pipes because of
            // invalid port numbers. SvcLayer svcmap will get updated as always when the svc
            // registers.
            bool svcAddition = false;

            if ( (svcLayerInfoUpdate.incarnationNo == 0 && dbInfoUpdate.incarnationNo == 0) )
            {
                if ( !(svc_status == fpi::SVC_STATUS_ADDED || svc_status == fpi::SVC_STATUS_STARTED) )
                {
                    LOGWARN << "!!No record for svc found in SvcLayer or ConfigDB!! Will not "
                            << "make any updates, returning..";
                    return;

                } else {
                    svcAddition = true;
                }
            } else if ((svc_status == fpi::SVC_STATUS_ADDED || svc_status == fpi::SVC_STATUS_STARTED)) {
                svcAddition = true;
            }

            /*
             * ========================================================
             *  Depending on which records were retrieved, take action
             *  Note: In the table below, >, <, == are used to indicate
             *  results after comparison of incarnation numbers
             * ========================================================
             * */

//+----------------+---------------------+--------------------------+------------------+---------------------------+
//| dbRecordFound  | svcLayer  | Incoming VS found Record |  Apply incoming? |              Action                 |
//|                |RecordFound|                          |                  |                                     |
//+----------------+-------------- -------+--------------------------+------------------+--------------------------+
//     True        |   False   |   incoming >= current    |      Yes         | Update DB, svcLayer with incoming   |
//                 |           |   incoming < current     |      No          | Only update svcLayer with DB record |
//                 |           |   incoming = 0           |      Yes         | Update DB, svcLayer with incoming   |
//                 |           |                          |                  | (after updating 0 incarnationNo)    |
//+----------------+-----------+--------------------------+-------------------------------------+
//     False       |   True    |   incoming >= current    |      Yes         | Update svcLayer, DB with incoming   |
//                 |           |   incoming < current     |      No          | Only update DB with svcLayer record |
//                 |           |   incoming = 0           |      Yes         | Update svcLayer, DB with incoming   |
//                 |           |                          |                  | (after updating 0 incarnationNo)    |
//+----------------+-----------+--------------------------+-------------------------------------+
//     False       |   False   |   only incoming is valid |      Yes         | Check incoming incarnationNo, if    |
//                 |           |                          |                  | 0, update, then update DB & svcLayer|
//+----------------+-----------+--------------------------+-------------------------------------+
//     True        |   True    |   incoming >= svcLayer   |      Yes         | Update svcLayer, DB with incoming   |
//                 |           |   svcLayer > configDB    |                  |
//                 |           |                          |                  |
//                 |           |   incoming < svcLayer    |      No          | DB has most current, update svcLayer|
//                 |           |   svcLayer < configDB    |                  | with DB record, ignore incoming     |
//                 |           |                          |                  |                                     |
//                 |           |   incoming < svcLayer    |      No          | svcLayer & DB have most current     |
//                 |           |   svcLayer == configDB   |                  | No updates are necessary            |
//                 |           |                          |                  |                                     |
//                 |           |   incoming >= svcLayer   |      No          | DB has most current, update svcLayer|
//                 |           |   svcLayer < configDB    |                  | with DB record, ignore incoming     |
//                 |           |                          |                  |                                     |
//                 |           |   incoming >= svcLayer   |      Yes         | Incoming has the most current       |
//                 |           |   svcLayer == configDB   |                  |                                     |
//                 |           |                          |                  |
//                 |           |   incoming < svcLayer    |      No          | SvcLayer has most current, update DB|
//                 |           |   svcLayer > configDB    |                  | with svcLyr record, ignore incoming |
//                 |           |                          |                  |
//                 |           |   incoming = 0           |      Yes         | Update svcLayer, DB with incoming   |
//+----------------+-----------+--------------------------+------------------+-------------------------------------+


            if (!svcLayerRecordFound && !dbRecordFound)
            {
                LOGNORMAL << "!!Case: No svcLayer or DB record found, updating both with incoming";
                // 1. Update DB to incoming
                // 2. Update svcLayer to incoming
                // 3. Broadcast svcMap

                configDB->changeStateSvcMap( dbInfoPtr );

                if ( !svcAddition )
                {
                    MODULEPROVIDER()->getSvcMgr()->updateSvcMap( {svcLayerInfoUpdate} );
                }
                //OM_NodeDomainMod::om_loc_domain_ctrl()->om_bcast_svcmap();

            } else if ( !svcLayerRecordFound && dbRecordFound ) {

                // DB has a record, check if it is more current than incoming
                validUpdate = OmExtUtilApi::getInstance()->isIncomingUpdateValid(incomingSvcInfo, dbInfoUpdate);

                if (validUpdate)
                {
                    LOGNORMAL << "!!Case: No svcLayer record, found DB record, updating both with incoming";
                    // 1. Update DB to incoming
                    // 2. Update svcLayer to incoming
                    // 3. Broadcast svcMap
                    fpi::SvcInfoPtr incomingSvcInfoPtr = boost::make_shared<fpi::SvcInfo>(incomingSvcInfo);
                    configDB->changeStateSvcMap( incomingSvcInfoPtr );

                    if ( !svcAddition )
                    {
                        MODULEPROVIDER()->getSvcMgr()->updateSvcMap( {incomingSvcInfo} );
                    }
                   // OM_NodeDomainMod::om_loc_domain_ctrl()->om_bcast_svcmap();

                } else {
                    LOGNORMAL << "!!Case: No svcLayer record, found DB record, updating svcLayer with DB record";
                    // 1. No update to DB
                    // 2. Update svcLayer to DB
                    // 3. Broadcast svcMap
                    if ( !svcAddition )
                    {
                        MODULEPROVIDER()->getSvcMgr()->updateSvcMap( {dbInfoUpdate} );
                    }
                   // OM_NodeDomainMod::om_loc_domain_ctrl()->om_bcast_svcmap();
                }

            } else if ( svcLayerRecordFound && !dbRecordFound ) {

                // Only svcLayer has a record, check if it is more current than incoming
                validUpdate = OmExtUtilApi::getInstance()->isIncomingUpdateValid(incomingSvcInfo, svcLayerInfoUpdate);

                if (validUpdate)
                {
                    LOGNORMAL << "!!Case: No DB record, found svcLayer record, updating both with incoming";
                    // 1. Update DB to incoming
                    // 2. Update svcLayer to incoming
                    // 3. Broadcast svcMap
                    configDB->changeStateSvcMap( boost::make_shared<fpi::SvcInfo>(incomingSvcInfo) );
                    ///if ( !svcAddition )
                    {
                        MODULEPROVIDER()->getSvcMgr()->updateSvcMap( {incomingSvcInfo} );
                    }
                    //OM_NodeDomainMod::om_loc_domain_ctrl()->om_bcast_svcmap();

                } else {
                    LOGNORMAL << "!!Case: No DB record, found svcLayer record, updating DB with svcLayer";
                    // svcLayer has most current info, update the DB since it has no record of this svc
                    // 1. Update DB to svcLayer
                    // 2. No update to svcLayer
                    // 3. No svcMap broadcast
                    configDB->changeStateSvcMap( boost::make_shared<fpi::SvcInfo>(svcLayerInfoUpdate) );
                    //OM_NodeDomainMod::om_loc_domain_ctrl()->om_bcast_svcmap();
                }
            } else if ( svcLayerRecordFound && dbRecordFound ) {

                svcLayerInfoPtr = boost::make_shared<fpi::SvcInfo>(svcLayerInfoUpdate);
                dbInfoPtr       = boost::make_shared<fpi::SvcInfo>(dbInfoUpdate);

                validUpdate = OmExtUtilApi::getInstance()->isIncomingUpdateValid(incomingSvcInfo, svcLayerInfoUpdate);

                // What we are trying to determine with the below two values is the relationship
                // between the svcLayer and DB record.
                // With the dbHasOlderRecord value, we can trust the return if true, ie dbRecord < svcLyrRecord.
                // However, if we get a false value, there are two possibilities:
                // 1. dbRecord > svcLayerRecord or 2. dbRecord == svcLayerRecord
                // The sameRecords computation helps us determine with certainty which case it is
                // and take action accordingly. Which is also why you will see sameRecords being checked
                // only when this dbHasOlderRecord value is false.
                bool dbHasOlderRecord = dbRecordNeedsUpdate(svcLayerInfoUpdate, dbInfoUpdate);
                bool sameRecords = areRecordsSame(svcLayerInfoUpdate, dbInfoUpdate);

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
                    //OM_NodeDomainMod::om_loc_domain_ctrl()->om_bcast_svcmap();

                }  else if ( validUpdate && !dbHasOlderRecord && !sameRecords ) {

                    LOGNORMAL << "!!Case: Both svcLayer & DB record found, updating svcLayer with DB record";
                    // incoming >= svcLayer
                    // svcLayer < configDB
                    // 1. No update to DB
                    // 2. Update svcLayer to DB
                    // 3. Broadcast svcMap

                    MODULEPROVIDER()->getSvcMgr()->updateSvcMap( {dbInfoUpdate} );
                    //OM_NodeDomainMod::om_loc_domain_ctrl()->om_bcast_svcmap();

                } else if ( validUpdate && !dbHasOlderRecord && sameRecords ) {

                    LOGNORMAL << "!!Case: Both svcLayer & DB record found, updating both to incoming";
                    // incoming >= svcLayer
                    // svcLayer == configDB
                    // 1. Update DB to incoming
                    // 2. Update svcLayer to incoming
                    // 3. Broadcast svcMap
                    configDB->changeStateSvcMap( boost::make_shared<fpi::SvcInfo>(incomingSvcInfo) );
                    MODULEPROVIDER()->getSvcMgr()->updateSvcMap( {incomingSvcInfo} );
                    //OM_NodeDomainMod::om_loc_domain_ctrl()->om_bcast_svcmap();

                } else if ( !validUpdate && dbHasOlderRecord ) {

                    LOGNORMAL << "!!Case: Both svcLayer & DB record found, updating DB with svcLayer record";
                    // incoming < svcLayer
                    // svcLayer > configDB
                    // 1. Update DB to svcLayer
                    // 2. No update to svcLayer
                    // 3. No svcMap broadcast

                    configDB->changeStateSvcMap( boost::make_shared<fpi::SvcInfo>(svcLayerInfoUpdate) );

                }  else if ( !validUpdate && !dbHasOlderRecord && sameRecords ) {

                    LOGNORMAL << "!!Case: Both svcLayer & DB record found, no updates necessary";

                    // Reuse the svcAddition flag simply to prevent the broadcasting of the map
                    svcAddition = true;
                    // incoming < svcLayer
                    // svcLayer == configDB
                    // 1. No update to DB
                    // 2. No update to svcLayer
                    // 3. Broadcast svcMap

                    //MODULEPROVIDER()->getSvcMgr()->updateSvcMap( {dbInfoUpdate} );
                   // OM_NodeDomainMod::om_loc_domain_ctrl()->om_bcast_svcmap();
                }   else if ( !validUpdate && !dbHasOlderRecord && !sameRecords ) {

                    LOGNORMAL << "!!Case: Both svcLayer & DB record found, updating svcLayer with DB record";
                    // incoming < svcLayer
                    // svcLayer < configDB
                    // 1. No update to DB
                    // 2. Update svcLayer to DB
                    // 3. Broadcast svcMap

                    MODULEPROVIDER()->getSvcMgr()->updateSvcMap( {dbInfoUpdate} );
                   // OM_NodeDomainMod::om_loc_domain_ctrl()->om_bcast_svcmap();
                }
            }

            LOGNORMAL << "!!Releasing update map lock";
            updateLock.unlock();

            if ( !svcAddition )
            {
                OM_NodeDomainMod::om_loc_domain_ctrl()->om_bcast_svcmap();
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

            ret = configDB->getSvcInfo(svc_uuid, dbInfoUpdate);

            fpi::ServiceStatus initialDbStatus = fpi::SVC_STATUS_INVALID;
            if (ret)
            {
                initialDbStatus = dbInfoUpdate.svc_status;
                dbInfoUpdate.svc_status = svc_status;

            } else {
                LOGWARN << "!!Could not find SvcInfo for uuid:"
                        << std::hex << svc_uuid << std::dec
                        << " in the OM's configDB";

                dbInfoUpdate.svc_id.svc_uuid.svc_uuid = svc_uuid;
                dbInfoUpdate.svc_status    = svc_status;
                dbInfoUpdate.svc_type      = svcType;
                dbInfoUpdate.incarnationNo = 0;
            }

            dbInfoPtr = boost::make_shared<fpi::SvcInfo>(dbInfoUpdate);
            /*
             * ==========================================
             *  Setting up svcInfo object from svcLayer
             * ==========================================
             * */


            fpi::SvcInfo initialSvcLayerInfo;
            ret = MODULEPROVIDER()->getSvcMgr()->getSvcInfo(uuid, svcLayerInfoUpdate);

            if (ret)
            {
                initialSvcLayerInfo = svcLayerInfoUpdate;

                // update the status to what we need to update to, since
                // this svcInfoPtr will be treated as "incoming" record
                svcLayerInfoUpdate.svc_status = svc_status;

            } else {
                LOGWARN << "!!Could not find svcInfo for uuid:"
                          << std::hex << svc_uuid << std::dec
                          << " in the svcMap, generating new";

                svcLayerInfoUpdate.svc_id.svc_uuid.svc_uuid = uuid.svc_uuid;
                svcLayerInfoUpdate.svc_status    = svc_status;
                svcLayerInfoUpdate.svc_type      = svcType;
                svcLayerInfoUpdate.incarnationNo = 0;
             }

            svcLayerInfoPtr = boost::make_shared<fpi::SvcInfo>(svcLayerInfoUpdate);

            // The only time both incarnationNos can be zero is when a service is getting added
            // into the cluster for the very first time. The ADDED, STARTED status is set prior to even
            // sending the start to PM, so the service processes aren't running yet.
            // Could be that svcLayer is unaware of the svc too
            bool svcAddition = false;

            if ( (svcLayerInfoUpdate.incarnationNo == 0 && dbInfoUpdate.incarnationNo == 0) )
            {
                if ( !(svc_status == fpi::SVC_STATUS_ADDED || svc_status == fpi::SVC_STATUS_STARTED) )
                {
                    LOGWARN << "!!No record for svc found in SvcLayer or ConfigDB!! Will not "
                            << "make any updates, returning..";
                    return;

                } else {
                    svcAddition = true;
                }
            } else if ((svc_status == fpi::SVC_STATUS_ADDED || svc_status == fpi::SVC_STATUS_STARTED)) {
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

            bool dbHasOlderRecord = dbRecordNeedsUpdate(svcLayerInfoUpdate, dbInfoUpdate);
            bool sameRecords = areRecordsSame(svcLayerInfoUpdate, dbInfoUpdate);

            if ( dbHasOlderRecord)
            {
                LOGDEBUG << "!!ConfigDB has older record, updating it with svcLayer record";
                // If the flag is true implies that svcLayer has a more recent
                // record of the service in question, so first update the record
                // in configDB
                configDB->changeStateSvcMap( svcLayerInfoPtr );

                // Update the dbInfoUpdate record that is being managed by the dbInfoPtr to the
                // latest (from the update above)
                // we will use this record now to update svcLayer
                ret = configDB->getSvcInfo(svc_uuid, dbInfoUpdate);

                if (!ret)
                {
                    LOGWARN << "!!Could not retrieve updated svc record from configDB, potentially making outdated updates ";
                }
            } else {

                // ConfigDB has latest record, go ahead and update it for the new svc status,
                // given that the statuses are different.
                // This record will be used to update svcLayer

                if (initialDbStatus != fpi::SVC_STATUS_INVALID)
                {
                    if (dbInfoUpdate.svc_status != initialDbStatus)
                    {
                        configDB->changeStateSvcMap( dbInfoPtr );
                    } else {
                        LOGNOTIFY << "!!ConfigDB already reflects desired state for svc:"
                                  << std::hex << svc_uuid << std::dec << " state:"
                                  << OmExtUtilApi::printSvcStatus(initialDbStatus);
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
                /*
                 *                 if ( initialSvcLayerInfo.svc_id.svc_uuid.svc_uuid != 0 &&
                     svcLayerNewerInfo.svc_status != initialSvcLayerInfo.svc_status &&
                     svcLayerNewerInfo.incarnationNo != 0 &&
                     ( svcLayerNewerInfo.incarnationNo > svcLayerInfoUpdate.incarnationNo ||
                       ( svcLayerNewerInfo.incarnationNo ==  svcLayerInfoUpdate.incarnationNo &&
                         svcLayerNewerInfo.svc_status != svcLayerInfoUpdate.svc_status ) ) ) // this last bit will ALWAYS be true.
                 */
                // If we had a valid initialSvcLayerInfo object &&
                // The service status in svcLayer has changed from when we began this update &&
                // the latest record does not have a zero incarnation number (==> it's valid) &&
                // either (1) newer info has a bigger incarnationNo than incoming update
                //        (2) both latest record and incoming update have the same incarnationNo
                //            but different statuses.
                if ( initialSvcLayerInfo.incarnationNo != 0 &&
                     svcLayerNewerInfo.incarnationNo != 0 &&
                     svcLayerNewerInfo.svc_status != initialSvcLayerInfo.svc_status &&
                     svcLayerNewerInfo.incarnationNo > svcLayerInfoUpdate.incarnationNo )
                {
                    LOGNOTIFY << "!!Svc:" << std::hex << uuid.svc_uuid << std::dec
                              << " has already changed in service layer from incoming [incarnation:"
                              << svcLayerInfoUpdate.incarnationNo << ", status:"
                              << OmExtUtilApi::printSvcStatus(svcLayerInfoUpdate.svc_status)
                              << "] to [incarnation:" << svcLayerNewerInfo.incarnationNo << ", status:"
                              << OmExtUtilApi::printSvcStatus(svcLayerNewerInfo.svc_status)
                              << "]. Initiating new update";


                    LOGNORMAL << "Releasing update map lock";
                    updateLock.unlock();
                    // Something has already changed in the svc layer, so the current update of
                    // status we are proceeding with is already out-dated
                    // Call a new update of configDB, and return from the current cycle
                    // No locks being acquired on function entry, so we should be OK with
                    // this recursive call.

                    //if (svcLayerNewerInfo.incarnationNo > svcLayerInfoUpdate.incarnationNo)
                    //{
                        // If the incarnationNo has changed, must take the first path to perform
                        // correct comparisons, set handler flag to true
                        updateSvcMaps( configDB, uuid.svc_uuid, svcLayerNewerInfo.svc_status,
                                       svcLayerNewerInfo.svc_type, true, false, svcLayerNewerInfo );

                    //}
                    /*else {
                        // If only status has changed, we don't care about incarnationNo comparisons
                        updateSvcMaps(configDB, uuid.svc_uuid, svcLayerNewerInfo.svc_status);
                    }
*/
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
                              << OmExtUtilApi::printSvcStatus(svcLayerNewerInfo.svc_status)
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
                LOGNOTIFY << "!!Updating SvcMgr svcMap now for uuid:" << std::hex << uuid.svc_uuid  << std::dec;

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

                LOGNORMAL << "Releasing update map lock";
                updateLock.unlock();
                // Broadcast the service map
                OM_NodeDomainMod::om_loc_domain_ctrl()->om_bcast_svcmap();
            } else {
                LOGNORMAL << "Releasing update map lock";
                updateLock.unlock();
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

    bool areRecordsSame(fpi::SvcInfo svcLayerInfo, fpi::SvcInfo dbInfo )
    {
        if ( svcLayerInfo.incarnationNo == dbInfo.incarnationNo &&
             svcLayerInfo.svc_status == dbInfo.svc_status)
        {
            return true;
        } else {
            return false;
        }

        return false;
    }
}  // namespace fds
