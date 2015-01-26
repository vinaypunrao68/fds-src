package com.formationds.om.webkit.rest;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import FDS_ProtocolInterface.FDSP_ConfigPathReq;
import FDS_ProtocolInterface.FDSP_MsgHdrType;
import FDS_ProtocolInterface.FDSP_Node_Info_Type;
import com.formationds.commons.model.Domain;
import com.formationds.commons.model.Node;
import com.formationds.commons.model.Service;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.commons.model.type.ManagerType;
import com.formationds.commons.model.type.NodeState;
import com.formationds.commons.model.type.ServiceStatus;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import org.eclipse.jetty.server.Request;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;

public class ListServices
    implements RequestHandler {

    private static final Logger logger =
        LoggerFactory.getLogger( ListServices.class );


    private final FDSP_ConfigPathReq.Iface configPathClient;

    public ListServices( final FDSP_ConfigPathReq.Iface configPathClient ) {

        this.configPathClient = configPathClient;

    }

    public Resource handle( final Request request,
                            final Map<String, String> routeParameters )
        throws Exception {

        List<FDSP_Node_Info_Type> list =
            configPathClient.ListServices( new FDSP_MsgHdrType() );

        /**
         * temporary work-a-round for the defective list nodes.
         *
         * Most if not all of this code will be thrown away once platformd/om
         * service issues are resolved. This fix is just to get us to beta 2.
         */

        final Map<Long,Node> clusterMap = new HashMap<>( );
        if( list != null && !list.isEmpty() ) {

            for( final FDSP_Node_Info_Type info : list ) {

                final Optional<Service> service = ListService.find( info );
                if( service.isPresent() ) {

                    logger.debug( service.toString() );

                    final String ipv6Addr =
                        ipAddr2String( info.getIp_hi_addr() )
                            .orElse( String.valueOf( info.getIp_hi_addr() ) );
                    final String ipv4Addr =
                        ipAddr2String( info.getIp_lo_addr() )
                            .orElse( String.valueOf( info.getIp_lo_addr() ) );

                    final Long nodeUUID = info.getNode_uuid();

                    if( !clusterMap.containsKey( nodeUUID ) ) {

                        clusterMap.put( nodeUUID,
                                        Node.uuid( nodeUUID )
                                            .ipV6address( ipv6Addr )
                                            .ipV4address( ipv4Addr )
/*
 * TODO (Tinius) get the real state of the node
 */
                                            .state( NodeState.UP )
                                            .name( ipv4Addr )
                                            .build() );
                    }

                    clusterMap.get( nodeUUID )
                              .getServices()
                              .add( service.get() );


                } else {

                    logger.warn( "Unexpected service found -- {}",
                                 info.toString() );
                }
            }
        }

        /*
         * TODO (Tinius) Domain ( Global and Local ) finish implementation
         *
         * for now we only support on global domain and one local domain,
         * so hard hard code the global domain to "fds"
         */

        final Domain domain = Domain.uuid( 1L ).domain( "fds" ).build();
        domain.getNodes().addAll( clusterMap.values() );

        return new TextResource( ObjectModelHelper.toJSON( domain ) );
    }

    protected Optional<String> ipAddr2String( final Long ipAddr ) {

        try {

            return Optional.of( InetAddress.getByAddress( htonl( ipAddr ) )
                                           .getHostAddress() );

        } catch( UnknownHostException e ) {

            logger.error( "Failed to convert " + ipAddr + " its it IPv4 address", e );
        }

        return Optional.empty();
    }

    private byte[] htonl( long x )
    {
        byte[] res = new byte[4];
        for( int i = 0; i < 4; i++ ) {

            res[i] = ( new Long( x >>> 24 ) ).byteValue( );
            x <<= 8;

        }

        return res;
    }

    static enum ListService {
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
            final Long nodeUUID = fdspNodeInfoType.getNode_uuid();
            if( AM.is( fdspNodeInfoType.getControl_port() ) ) {

                service = Service.uuid( nodeUUID + AM.ordinal() )
                                 .autoName( AM.name() )
                                 .port( fdspNodeInfoType.getControl_port() )
                                 .status( getServiceState() )
                                 .type( AM.type() )
                                 .build();
            } else if( DM.is( fdspNodeInfoType.getControl_port() ) ) {

                service = Service.uuid( nodeUUID + DM.ordinal() )
                                 .autoName( DM.name() )
                                 .port( fdspNodeInfoType.getControl_port() )
                                 .status( getServiceState() )
                                 .type( DM.type() )
                                 .build();
            } else if( PM.is( fdspNodeInfoType.getControl_port() ) ) {

                service = Service.uuid( nodeUUID )
                                 .autoName( PM.name() )
                                 .port( fdspNodeInfoType.getControl_port() )
                                      .status( getServiceState() )
                                 .type( PM.type() )
                                 .build();
            } else if( SM.is( fdspNodeInfoType.getControl_port() ) ) {

                service = Service.uuid( nodeUUID )
                                 .autoName( SM.name() )
                                 .port( fdspNodeInfoType.getControl_port() )

                                 .status( getServiceState() )
                                 .type( SM.type() )
                                 .build();
            }


            return service == null ? Optional.<Service>empty() : Optional.of( service );
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
}
