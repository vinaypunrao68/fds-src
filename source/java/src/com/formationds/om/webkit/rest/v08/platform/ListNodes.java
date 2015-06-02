/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */
package com.formationds.om.webkit.rest.v08.platform;

import com.formationds.protocol.FDSP_Node_Info_Type;
import com.formationds.client.v08.model.Domain;
import com.formationds.client.v08.model.Node;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.util.thrift.ConfigurationApi;
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

public class ListNodes
    implements RequestHandler {

    private static final Logger logger =
        LoggerFactory.getLogger( ListNodes.class );


    private final ConfigurationApi configPathClient;

    public ListNodes( final ConfigurationApi configPathClient ) {

        this.configPathClient = configPathClient;

    }

    public Resource handle( final Request request,
                            final Map<String, String> routeParameters )
        throws Exception {

        List<com.formationds.protocol.FDSP_Node_Info_Type> list =
            configPathClient.ListServices(0);
        logger.debug("Size of service list: {}", list.size());

        Map<String, Node> clusterMap = computeNodeMap( list );

        /*
         * TODO (Tinius) Domain ( Global and Local ) finish implementation
         *
         * for now we only support on global domain and one local domain,
         * so hard hard code the global domain to "fds"
         */

        final Domain domain = new Domain( 1L, "fds", "fds" );
        domain.getNodes().addAll( clusterMap.values() );

        return new TextResource( ObjectModelHelper.toJSON( domain ) );
    }
    
    /**
     * temporary work-a-round for the defective list nodes.
     * 
     * In essence the "list" command just returns a list of service which need to be
     * grouped together in a node hierarchy (in this case) so that can be done here.
     *
     * Most if not all of this code will be thrown away once platformd/om
     * service issues are resolved. This fix is just to get us to beta 2.
     * @return
     */
    public Map<String, Node> computeNodeMap( List<com.formationds.protocol.FDSP_Node_Info_Type> list ){
    	
    	final Map<String,Node> clusterMap = new HashMap<>( );
        
    	if ( list == null || list.isEmpty() ){
    		return clusterMap;
    	}
    	
//
//        for( final FDSP_Node_Info_Type info : list ) {
//
//            final Optional<Service> service = ServiceType.find( info );
//            if( service.isPresent() ) {
//
//                final String ipv6Addr =
//                    ipAddr2String( info.getIp_hi_addr() )
//                        .orElse( String.valueOf( info.getIp_hi_addr() ) );
//                final String ipv4Addr =
//                    ipAddr2String( info.getIp_lo_addr() )
//                        .orElse( String.valueOf( info.getIp_lo_addr() ) );
//
//                final String nodeUUID = String.valueOf(
//                    info.getNode_uuid() );
//                NodeState nodeState = NodeState.UP;
//                final Optional<NodeState> optional =
//                    NodeState.byFdsDefined( info.getNode_state().name() );
//                if( optional.isPresent() ) {
//
//                     nodeState = optional.get();
//
//                }
//
//                if( !clusterMap.containsKey( nodeUUID ) ) {
//
//                    clusterMap.put( nodeUUID,
//                                    Node.uuid( nodeUUID )
//                                        .ipV6address( ipv6Addr )
//                                        .ipV4address( ipv4Addr )
//                                        .state( nodeState )
//                                        .name( nodeName( ipv4Addr ) )
//                                        .build() );
//                }
//
//                Service serviceInstance = service.get();
//                Node thisNode = clusterMap.get( nodeUUID );
//                
//                thisNode.addService( serviceInstance );
//
//            } else {
//
//                logger.warn( "Unexpected service found -- {}",
//                             info.toString() );
//            }
//        }    	
        
        return clusterMap;
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

    protected String nodeName( final String ipv4Addr ) {

        return ipv4Addr;

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


}
