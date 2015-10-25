package com.formationds.om.webkit.rest.v07.platform;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.ConfigurationService;
import com.formationds.commons.model.Domain;
import com.formationds.commons.model.Node;
import com.formationds.commons.model.Service;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.commons.model.type.NodeState;
import com.formationds.commons.model.type.ServiceType;
import com.formationds.protocol.svc.types.FDSP_MgrIdType;
import com.formationds.protocol.svc.types.FDSP_Node_Info_Type;
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


    private final ConfigurationService.Iface configPathClient;

    public ListNodes( final ConfigurationService.Iface configPathClient ) {

        this.configPathClient = configPathClient;

    }

    public Resource handle( final Request request,
                            final Map<String, String> routeParameters )
        throws Exception {

        List<com.formationds.protocol.svc.types.FDSP_Node_Info_Type> list =
            configPathClient.ListServices(0);
        logger.debug("Size of service list: {}", list.size());

        Map<String, Node> clusterMap = computeNodeMap( list );

        /*
         * TODO ( Tinius ) Domain ( Global and Local ) finish implementation
         *
         * for now we only support on global domain and one local domain,
         * so hard hard code the global domain to "fds"
         */

        final Domain domain = Domain.uuid( 1L ).domain( "fds" ).build();
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
     *
     * @return Returns {@link Map} of keys {@link String} and value {@link Node}
     */
    public Map<String, Node> computeNodeMap( List<com.formationds.protocol.svc.types.FDSP_Node_Info_Type> list ){
    	
    	final Map<String,Node> clusterMap = new HashMap<>( );
        
    	if ( list == null || list.isEmpty() ) {

    		return clusterMap;

    	}

      /*
       * TODO(Tinius) The Service Layer thinks they need a well known UUID for OM.
       *
       * There are several issues with this. But since they never produced a
       * design specification or got any approvals before they implemented what
       * we have now, no one had the chance to bring up concerns with the
       * design/architectural approach.
       *
       * find the OM and set its UUID to the Node ID its running one
       *
       * So the method om below is a HACK to allow the OM to group services
       * by their node ID ( our screwed up UUID ).
       */
      om( list );
      for( final FDSP_Node_Info_Type info : list ) {

        final Optional<Service> service = ServiceType.find( info );
        if( service.isPresent( ) )
        {
          /*
           * TODO(Tinius) once redesigned figure out what this should really be. Looking at the native side ( C++ ) 
           * I don't see any use of the ip_hi_addr, so it will take a little more digging to determine the what/how
           * of the high-order bytes
           */
          final String ipv6Addr = "::1";

          final String ipv4Addr =
                  ipAddr2String( info.getIp_lo_addr( ) )
                          .orElse( String.valueOf( info.getIp_lo_addr( ) ) );

          final String nodeUUID = String.valueOf( info.getNode_uuid( ) );
          NodeState nodeState = NodeState.UP;

          final Optional<NodeState> optional =
                  NodeState.byFdsDefined( info.getNode_state( )
                                              .name( ) );

          if( optional.isPresent( ) )
          {

            nodeState = optional.get( );

          }

          if( !clusterMap.containsKey( nodeUUID ) )
          {

            clusterMap.put( nodeUUID,
                            Node.uuid( nodeUUID )
                                .ipV6address( ipv6Addr )
                                .ipV4address( ipv4Addr )
                                .state( nodeState )
                                .name( nodeName( ipv4Addr ) )
                                .build( ) );
          }

          Service serviceInstance = service.get( );
          Node thisNode = clusterMap.get( nodeUUID );
          thisNode.addService( serviceInstance );

        }
        else
        {

          logger.warn( "Unexpected service found -- {}",
                       info.toString( ) );

        }
      }

      return clusterMap;
    }

  protected void om( final List<FDSP_Node_Info_Type> list )
  {
    for( final FDSP_Node_Info_Type node : list )
    {
      logger.debug( "TYPE::{}", node.getNode_type() );
      if( node.getNode_type().equals( FDSP_MgrIdType.FDSP_ORCH_MGR ) )
      {
        logger.trace( "Found OM service {}", node );
        final Optional<Long> pmUUID = findOmPlatformUUID( list,
                                                          ipAddr2String( node.getIp_lo_addr() ) );
        if( pmUUID.isPresent() )
        {
          logger.trace( "Found PM uuid {} ", pmUUID.get() );
          logger.trace( "Updating OM service Node UUID {} ( before )",
                        node.getNode_uuid() );
          node.setNode_uuid( pmUUID.get() );
          logger.trace( "Updated OM service Node UUID {} ( after )",
                        node.getNode_uuid( ) );
        }
      }
    }
  }

  protected Optional<Long> findOmPlatformUUID( final List<FDSP_Node_Info_Type> list,
                                               final Optional<String> omNodeName )
  {
    for( final FDSP_Node_Info_Type node : list )
    {
      logger.debug( "TYPE::{}", node.getNode_type() );
      if( node.getNode_type().equals( FDSP_MgrIdType.FDSP_PLATFORM ) )
      {
        logger.trace( "Found PM service {} ", node );

        if( omNodeName.isPresent() )
        {
          final Optional<String> pmNodeName = ipAddr2String( node.getIp_lo_addr() );
          if( pmNodeName.isPresent() &&
              pmNodeName.get().equalsIgnoreCase( omNodeName.get( ) ) )
          {
            return Optional.of( node.getNode_uuid( ) );
          }
        }
      }
    }

    return Optional.empty();
  }

  protected Optional<String> ipAddr2String( final Long ipAddr )
  {
    try
    {

      return Optional.of( InetAddress.getByAddress( htonl( ipAddr ) )
                                     .getHostAddress( ) );

    }
    catch( UnknownHostException e )
    {

      logger.error( "Failed to convert " + ipAddr + " to its IPv4 address",
                    e );
    }

    return Optional.empty( );
  }

  protected String nodeName( final String ipv4Addr )
  {
    // TODO what should we do for the node name? default to ip address for now
    return ipv4Addr;
  }

  private byte[] htonl( long x )
  {
    byte[] res = new byte[ 4 ];
    for( int i = 0; i < 4; i++ )
    {

      res[ i ] = ( new Long( x >>> 24 ) ).byteValue( );
      x <<= 8;

    }

    return res;
  }
}
