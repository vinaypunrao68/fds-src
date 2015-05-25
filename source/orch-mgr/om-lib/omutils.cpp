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
    // TODO(prem): mkae it use lower case soon
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
        // update configDB with which services this platform has
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

        LOGDEBUG << "node UUID: " << std::hex << nodeInfo.node_uuid << std::dec
                 << " service UUID: " << std::hex << nodeInfo.service_uuid << std::dec
                 << " service status: " << svcinfo.svc_status 
                 << " service type: " << svcinfo.svc_type;

        switch ( svcinfo.svc_status )
        {
            case fpi::SVC_STATUS_INACTIVE:
                nodeInfo.node_state = fpi::FDS_Node_Down;
                break;
            case fpi::SVC_STATUS_INVALID:
                nodeInfo.node_state = fpi::FDS_Node_Down;
                break;
            case fpi::SVC_STATUS_ACTIVE:
                nodeInfo.node_state = fpi::FDS_Node_Up;
                break;
        }

        return nodeInfo;
    }
}  // namespace fds
