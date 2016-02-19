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

    /**
     * TODO(Neil)
     * We should *really* be doing changing service states with SvcInfo
     * instead of just UUIDs. This will prevent incorrect overwrites of the
     * service states due to the lack of incarnation number.
     * This method stays here for legacy purposes, but we need to change all the
     * callers to use SvcInfo. Right now, many callers do not have that info, but
     * only UUID.
     */
    void change_service_state( kvstore::ConfigDB* configDB,
                               const fds_uint64_t svc_uuid, 
                               const fpi::ServiceStatus svc_status )
    {
        // No incarnation number provided... do lookup and call the one below
        fpi::SvcUuid uuid;
        uuid.svc_uuid = svc_uuid;
        fds_mutex dbLock; // for macro only
        UPDATE_CONFIGDB_SERVICE_STATE(configDB, uuid, svc_status);
    }

    // See header file
    void change_service_state( kvstore::ConfigDB* configDB,
                               const fpi::SvcInfoPtr svcInfo,
                               const fpi::ServiceStatus svc_status,
                               const bool updateSvcMap )
    {       
        fpi::SvcUuid svcUuid;
        svcUuid.svc_uuid = svcInfo->svc_id.svc_uuid.svc_uuid;

        /*
         * Update configDB with the new status for the given service on the given node
         *
         */
        if ( configDB && configDB->changeStateSvcMap( svcInfo, svc_status ) )
        {
            LOGDEBUG << "Successfully updated configdbs service ID ( " 
                     << std::hex << svcUuid << std::dec
                     << " ) state to ( " << svc_status << " )";

            if ( updateSvcMap )
            {       
                fpi::SvcInfo svc;

                bool ret = MODULEPROVIDER()->getSvcMgr()->getSvcInfo( svcUuid, svc );    
                if ( ret )
                {
                    svc.svc_status = svc_status;

                    std::vector<fpi::SvcInfo> svcs;
                    svcs.push_back( svc );

                    MODULEPROVIDER()->getSvcMgr()->updateSvcMap( svcs );
                    //ConfigDB svcMap is already updated in the above changeStateSvcMap call

                    LOGDEBUG << "Successfully updated svcmaps service ID ( " 
                             << std::hex << svcUuid << std::dec
                             << " ) state to ( " << svc_status << " )";     
                }
            }     
        }
        else
        {
            LOGWARN << "Failed to changed service ID ( " 
                    << std::hex << svcUuid << std::dec << " ) "
                    << "state to ( " << svc_status << " )";
        }
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

    bool isSameSvcInfoInstance( fpi::SvcInfo svcInfo )
    {
        std::vector<fpi::SvcInfo> entries;
        MODULEPROVIDER()->getSvcMgr()->getSvcMap( entries );
        if ( entries.size() > 0 )
        {
            for ( auto svc : entries )
            {
                if ( svc.svc_id.svc_uuid.svc_uuid == svcInfo.svc_id.svc_uuid.svc_uuid )
                {
                    LOGDEBUG << "service: " << svcInfo.name
                             << " SvcInfo: " << svcInfo.incarnationNo
                             << "  SvcMap: " << svc.incarnationNo
                             << " status: " << svcInfo.svc_status;

                    if ( svcInfo.incarnationNo < svc.incarnationNo )
                    {
                        LOGDEBUG << svcInfo.name
                                 << " uuid( "
                                 << std::hex << svcInfo.svc_id.svc_uuid.svc_uuid << std::dec
                                 << " ) incarnation number is older then service map, not safe to change the status!";

                        return false;
                    }
                    else if ( svcInfo.incarnationNo > svc.incarnationNo )
                    {
                        LOGDEBUG << svcInfo.name
                                 << " uuid( "
                                 << std::hex << svcInfo.svc_id.svc_uuid.svc_uuid << std::dec
                                 << " ) incarnation number is newer then service map, not safe to change the status!";

                        return false;
                    }
                    /*
                     * entries are the same, service and service map are the same or service incarnation is zero
                     */
                    else if ( ( svc.incarnationNo == svcInfo.incarnationNo ) ||
                              ( svcInfo.incarnationNo == 0 ) )
                    {
                        if ( svcInfo.incarnationNo == 0 )
                        {
                            // update incarnation number
                            svcInfo.incarnationNo = util::getTimeStampSeconds();
                        }

                        LOGDEBUG << svcInfo.name
                                 << " uuid( "
                                 << std::hex << svcInfo.svc_id.svc_uuid.svc_uuid << std::dec
                                 << " ) incarnation number is the same as service map or is zero, safe to change its status!";

                        return true;
                    }
                }
            }
        }

        LOGDEBUG << " No Matching Service Map enteries found for service "
                 << svcInfo.name << ":" << std::hex << svcInfo.svc_id.svc_uuid.svc_uuid << std::dec;

        return false;
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

        bool found = false;
        fpi::SvcInfoPtr svcPtr;
        for (std::vector<fpi::SvcInfo>::const_iterator iter = svcInfos.begin();
                iter != svcInfos.end(); ++iter)
        {
            if (iter->svc_id.svc_uuid.svc_uuid == serviceTypeId.svc_uuid)
            {
                svcPtr = boost::make_shared<fpi::SvcInfo>(*iter);
                found = true;
                break;
            }
        }

        if (!found)
        {
            LOGDEBUG << "Unable to find service in list. Making a fake svcPtr. Fix this?"; \
            svcPtr = boost::make_shared<fpi::SvcInfo>();
            svcPtr->svc_id.svc_uuid.svc_uuid = serviceTypeId.svc_uuid;
        }

        DltDmtUtil::getInstance()->addToRemoveList(serviceTypeId.svc_uuid, type);

        // ToDo : Modify below call to use the other change_service_state function so the
        // call will go to the below function through the macro call. In which case make
        // sure to REMOVE the scoped lock in send_remove_service since the macro acquires
        // a separate lock
        change_service_state( configDB,
                              svcPtr,
                              fpi::SVC_STATUS_REMOVED,
                              true );

    }

}  // namespace fds
