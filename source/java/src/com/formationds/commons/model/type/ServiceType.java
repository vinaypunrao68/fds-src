package com.formationds.commons.model.type;

import java.util.Optional;

import FDS_ProtocolInterface.FDSP_Node_Info_Type;

import com.formationds.commons.model.AccessManagerService;
import com.formationds.commons.model.DataManagerService;
import com.formationds.commons.model.OrchestrationManagerService;
import com.formationds.commons.model.PlatformManagerService;
import com.formationds.commons.model.Service;
import com.formationds.commons.model.StorageManagerService;

public enum ServiceType {

        PM {
            @Override
            public boolean is( final int controlPort ) {
                return ( ( controlPort >= 7000 ) && ( controlPort < 7009 ) );
            }

            @Override
            public ManagerType type( ) {
                return ManagerType.FDSP_PLATFORM;
            }
        },
        SM {
            @Override
            public boolean is( final int controlPort ) {
                return ( ( controlPort >= 7020 ) && ( controlPort < 7029 ) );
            }

            @Override
            public ManagerType type( ) {
                return ManagerType.FDSP_STOR_MGR;
            }
        },
        DM {
            @Override
            public boolean is( final int controlPort ) {
                return ( ( controlPort >= 7030 ) && ( controlPort < 7039 ) );
            }

            @Override
            public ManagerType type( ) {
                return ManagerType.FDSP_DATA_MGR;
            }
        },
        AM {
            @Override
            public boolean is( final int controlPort ) {
                return ( ( controlPort >= 7040 ) && ( controlPort < 7049 ) );
            }

            @Override
            public ManagerType type( ) {
                return ManagerType.FDSP_STOR_HVISOR;
            }
        };

        public ManagerType type( ) {
            throw new AbstractMethodError();
        }

        public boolean is( final int controlPort ) {
            throw new AbstractMethodError();
        }

        public static Optional<Service> find( final FDSP_Node_Info_Type fdspNodeInfoType ) {

            Service service = null;

            if( AM.is( fdspNodeInfoType.getControl_port() ) ) {

            	service = buildService( AM, fdspNodeInfoType );

            } else if( DM.is( fdspNodeInfoType.getControl_port() ) ) {

                service = buildService( DM,  fdspNodeInfoType );
                
            } else if( PM.is( fdspNodeInfoType.getControl_port() ) ) {

                service = buildService( PM, fdspNodeInfoType );
                
            } else if( SM.is( fdspNodeInfoType.getControl_port() ) ) {

                service = buildService( SM,  fdspNodeInfoType );
            }


            return service == null ? Optional.<Service>empty() : Optional.of( service );
        }
        
        /**
         * Helper to build a service of a particular type
         * 
         * @param serviceType
         * @return
         */
        private static Service buildService( final ServiceType serviceType, final FDSP_Node_Info_Type fdspNodeInfoType ){
        	
        	Service service = null;
        	
        	switch( serviceType ){
        		case DM:
        			service = new DataManagerService();
        			break;
        		case AM:
        			service = new AccessManagerService();
        			break;
        		case PM:
        			service = new PlatformManagerService();
        			break;
        		case SM:
        			service = new StorageManagerService();
        			break;
        		default:
        			service = new OrchestrationManagerService();
        			break;
        	}
        	
        	service.setAutoName( serviceType.name() );
        	service.setPort( fdspNodeInfoType.getControl_port() );
        	service.setUuid( fdspNodeInfoType.getNode_uuid() + AM.ordinal() );
        	service.setStatus( getServiceState() );
        	
        	return service;
        }

        private static ServiceStatus getServiceState( ) {
            /*
             * TODO (Tinius) get the correct state from the platformd
             *
             * Currently, the platformd doesn't provide real time
             * monitoring of the processes it spawns ( forks ).
             */

            return ServiceStatus.ACTIVE;
        }
}
