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

    void change_service_state( kvstore::ConfigDB* configDB,
                               const fds_uint64_t svc_uuid, 
                               const fpi::ServiceStatus svc_status )
    {       
        /*
         * Update configDB with the new status for the given service on the given node
         */
        if ( configDB && configDB->changeStateSvcMap( svc_uuid, svc_status ) )
        {
            LOGDEBUG << "Successfully changed service ID ( " 
                     << std::hex << svc_uuid << std::dec << " ) "
                     << "state to ( " << svc_status << " )";
        }
        else
        {
            LOGWARN << "Failed to changed service ID ( " 
                    << std::hex << svc_uuid << std::dec << " ) "
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
            case fpi::SVC_STATUS_INACTIVE:
            case fpi::SVC_STATUS_STOPPED:
                nodeInfo.node_state = fpi::FDS_Node_Down;
                break;
            case fpi::SVC_STATUS_INVALID:
                nodeInfo.node_state = fpi::FDS_Node_Down;
                break;
            case fpi::SVC_STATUS_ACTIVE:
            case fpi::SVC_STATUS_ADDED:
            case fpi::SVC_STATUS_STARTED:
                nodeInfo.node_state = fpi::FDS_Node_Up;
                break;
            case fpi::SVC_STATUS_DISCOVERED:
                nodeInfo.node_state = fpi::FDS_Node_Discovered;
                break;    
        }

        return nodeInfo;
    }

    fpi::FDSP_NodeState fromServiceStatus(fpi::ServiceStatus svcStatus) {
        fpi::FDSP_NodeState retNodeState = fpi::FDS_Node_Down;
        switch ( svcStatus )
        {
            case fpi::SVC_STATUS_INACTIVE:
            case fpi::SVC_STATUS_STOPPED:
                retNodeState = fpi::FDS_Node_Down;
                break;
            case fpi::SVC_STATUS_INVALID:
                retNodeState = fpi::FDS_Node_Down;
                break;
            case fpi::SVC_STATUS_ACTIVE:
            case fpi::SVC_STATUS_ADDED:
            case fpi::SVC_STATUS_STARTED:
                retNodeState = fpi::FDS_Node_Up;
                break;
            case fpi::SVC_STATUS_DISCOVERED:
                retNodeState = fpi::FDS_Node_Discovered;
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
        if ((iter != svcInfos.end()) && amFlag)
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

    void getServicesToStart
        (
        bool start_sm,
        bool start_dm,
        bool start_am,
        kvstore::ConfigDB* configDB,
        NodeUuid nodeUuid,
        std::vector<fpi::SvcInfo>& svcInfoList)
        {

        /**
         * We are being asked to start all Services defined for the Node.
         * Which services are defined for this Node?
         */
        NodeServices services;

        // Services that are being started after a node or domain shutdown,
        // will already be present in the configurationDB.
        // If not, it is the very initial startup of the domain, we will
        // need to set up new svcInfos

        if (configDB->getNodeServices(nodeUuid, services)) {
            LOGDEBUG << "Services present in configDB for node:"
                     << std::hex << nodeUuid.uuid_get_val()
                     << std::dec;

            fpi::SvcInfo svcInfo;
            fpi::SvcUuid svcUuid;
            if ( start_am && (services.am.uuid_get_val() != 0) ) {
                //start_am = true;
                svcUuid.svc_uuid = services.am.uuid_get_val();
                bool ret = MODULEPROVIDER()->getSvcMgr()->getSvcInfo(svcUuid, svcInfo);
                if (ret) {
                    svcInfoList.push_back(svcInfo);
                }
            }
            if ( start_sm && (services.sm.uuid_get_val() != 0) ) {
                //start_sm = true;
                svcUuid.svc_uuid = services.sm.uuid_get_val();
                bool ret = MODULEPROVIDER()->getSvcMgr()->getSvcInfo(svcUuid, svcInfo);
                if (ret) {
                    svcInfoList.push_back(svcInfo);
                }
            }
            if ( start_dm && (services.dm.uuid_get_val() != 0) ) {
                //start_dm = true;
                svcUuid.svc_uuid = services.dm.uuid_get_val();
                bool ret = MODULEPROVIDER()->getSvcMgr()->getSvcInfo(svcUuid, svcInfo);
                if (ret) {
                    svcInfoList.push_back(svcInfo);
                }
            }
        } else {
            /**
            * We interpret no Services information in ConfigDB,
            * this must be for a new Node.
            */

            LOGDEBUG << "No svcInfos in configDB, creating new";
            if (start_am) {
                fpi::SvcInfo* amInfo = new fpi::SvcInfo();
                amInfo->__set_svc_type(fpi::FDSP_ACCESS_MGR);
                amInfo->__set_svc_status(fpi::SVC_STATUS_INACTIVE);
                svcInfoList.push_back(*amInfo);
            }
            if (start_dm) {
                fpi::SvcInfo* dmInfo = new fpi::SvcInfo();
                dmInfo->__set_svc_type(fpi::FDSP_DATA_MGR);
                dmInfo->__set_svc_status(fpi::SVC_STATUS_INACTIVE);
                svcInfoList.push_back(*dmInfo);
            }
            if (start_sm) {
                fpi::SvcInfo* smInfo = new fpi::SvcInfo();
                smInfo->__set_svc_type(fpi::FDSP_STOR_MGR);
                smInfo->__set_svc_status(fpi::SVC_STATUS_INACTIVE);
                svcInfoList.push_back(*smInfo);
            }
        }
    }
}  // namespace fds
