package com.formationds.commons.model.type;

import com.formationds.protocol.svc.types.FDSP_MgrIdType;
import com.formationds.protocol.svc.types.FDSP_NodeState;
import com.formationds.protocol.svc.types.FDSP_Node_Info_Type;
import com.formationds.commons.model.AccessManagerService;
import com.formationds.commons.model.DataManagerService;
import com.formationds.commons.model.OrchestrationManagerService;
import com.formationds.commons.model.PlatformManagerService;
import com.formationds.commons.model.Service;
import com.formationds.commons.model.StorageManagerService;
import java.util.Optional;

public enum ServiceType {

        PM,
        SM,
        DM,
        AM,
        OM;

        public static Optional<Service> find( final FDSP_Node_Info_Type fdspNodeInfoType ) {

            Optional<Service> service = Optional.empty();

            final FDSP_MgrIdType type = fdspNodeInfoType.getNode_type();
            switch( type ) {
                case FDSP_ACCESS_MGR:
                    service = buildService( AM, fdspNodeInfoType );
                    break;
                case FDSP_DATA_MGR:
                    service = buildService( DM, fdspNodeInfoType );
                    break;
                case FDSP_PLATFORM:
                    service = buildService( PM, fdspNodeInfoType );
                    break;
                case FDSP_STOR_MGR:
                    service = buildService( SM, fdspNodeInfoType );
                    break;
                case FDSP_ORCH_MGR:
                    service = buildService( OM, fdspNodeInfoType );
                    break;
            }


            return service;
        }
        
        /**
         * Helper to build a service of a particular type
         * 
         * @param serviceType the {@link com.formationds.commons.model.type.ServiceType}
         * @param fdspNodeInfoType the {@link FDS_ProtocolInterface.FDSP_Node_Info_Type}
         *
         * @return Returns {@link java.util.Optional} with the
         *         {@link com.formationds.commons.model.Service} set if found.
         *         Otherwise, Optional.empty().
         */
        private static Optional<Service> buildService(
            final ServiceType serviceType,
            final FDSP_Node_Info_Type fdspNodeInfoType ) {
        	
        	Optional<Service> service = Optional.empty();
        	
        	switch( serviceType ) {
                case AM:
                    service = Optional.of( new AccessManagerService() );
                    break;
                case DM:
                    service = Optional.of( new DataManagerService() );
                    break;
                case OM:
                    service = Optional.of( new OrchestrationManagerService() );
                    break;
                case PM:
                    service = Optional.of( new PlatformManagerService() );
                    break;
                case SM:
                    service = Optional.of( new StorageManagerService() );
                    break;
             }

            if( service.isPresent() ) {

                service.get().setAutoName( serviceType.name() );
                service.get().setPort( fdspNodeInfoType.getData_port() );
                service.get().setUuid( fdspNodeInfoType.getService_uuid() );
                service.get()
                       .setStatus(
                           getServiceState(
                               fdspNodeInfoType.getNode_state() ) );
            }
        	
        	return service;
        }

        private static ServiceStatus getServiceState(
            final FDSP_NodeState nodeState ) {

            switch( nodeState ) {
                case FDS_Node_Discovered:
                    return ServiceStatus.INACTIVE;
                case FDS_Node_Down:
                    return ServiceStatus.INACTIVE;
                case FDS_Start_Migration:
                    return ServiceStatus.INACTIVE;
                case FDS_Node_Rmvd:
                    return ServiceStatus.INVALID;
                case FDS_Node_Up:
                    return ServiceStatus.ACTIVE;
            }

            return ServiceStatus.INACTIVE;
        }
}
