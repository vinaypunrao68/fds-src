package com.formationds.client.v08.converters;

import com.formationds.client.v08.model.Node;
import com.formationds.client.v08.model.Node.NodeAddress;
import com.formationds.client.v08.model.Node.NodeState;
import com.formationds.client.v08.model.Service;
import com.formationds.client.v08.model.ServiceType;
import com.formationds.protocol.FDSP_MgrIdType;
import com.formationds.protocol.FDSP_NodeState;
import com.formationds.protocol.FDSP_Node_Info_Type;
import com.formationds.protocol.SvcID;
import com.formationds.protocol.SvcUuid;
import com.formationds.protocol.svc.types.SvcInfo;

import java.net.Inet4Address;
import java.net.Inet6Address;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;

import static com.formationds.client.v08.model.Service.ServiceState;
import static com.formationds.client.v08.model.Service.ServiceStatus;

@SuppressWarnings( "unused" )
public class PlatformModelConverter
{

  public static Node convertToExternalNode( List<FDSP_Node_Info_Type> nodeInfoTypes )
          throws UnknownHostException
  {

    Map<ServiceType, List<Service>> services = new HashMap<>( );
    NodeState state = NodeState.UNKNOWN;
    Long nodeId = 0L;
    NodeAddress address = null;

    for( FDSP_Node_Info_Type internalService : nodeInfoTypes )
    {

      Service externalService = convertToExternalService( internalService );

      // get the node info from whomever, but choose the PM if we can
      if( address == null || externalService.getType( )
                                            .equals( ServiceType.PM ) )
      {
        state = convertToExternalNodeState( internalService.getNode_state( ) );
          nodeId = internalService.getNode_uuid( );

        Inet4Address ipv4
                = ( Inet4Address ) InetAddress.getByAddress(
                     htonl( internalService.getIp_lo_addr( ) ) );

        // TODO: Figure this out
        Inet6Address ipv6 = null;

        //noinspection ConstantConditions
        address = new NodeAddress( ipv4, ipv6 );
      }

      List<Service> likeServiceList
              = services.get( externalService.getType( ) );

      if( likeServiceList == null )
      {
        likeServiceList = new ArrayList<>( );
      }

      likeServiceList.add( externalService );

      services.put( externalService.getType( ), likeServiceList );

    }

    Node node = null;

    if( address != null )
    {
      node = new Node( nodeId, address, state, services );
    }

    return node;
  }

  public static List<FDSP_Node_Info_Type> convertToInternalNode( Node node )
  {
    return null;
  }

  public static NodeState convertToExternalNodeState( FDSP_NodeState internalNodeState )
  {

    switch( internalNodeState )
    {
      case FDS_Node_Discovered:
        return NodeState.DISCOVERED;
      case FDS_Node_Down:
        return NodeState.DOWN;
      case FDS_Node_Up:
        return NodeState.UP;
      case FDS_Start_Migration:
      case FDS_Node_Rmvd:
      default:
        return NodeState.UNKNOWN;
    }
  }

  public static FDSP_NodeState convertToInternalNodeState( NodeState externalState )
  {

    switch( externalState )
    {
      case DISCOVERED:
        return FDSP_NodeState.FDS_Node_Discovered;
      case UP:
        return FDSP_NodeState.FDS_Node_Up;
      case DOWN:
        return FDSP_NodeState.FDS_Node_Down;
      case UNKNOWN:
      default:
        return FDSP_NodeState.FDS_Node_Rmvd;
    }
  }

  public static Service convertToExternalService( FDSP_Node_Info_Type nodeInfoType )
  {

    Long extId = nodeInfoType.getService_uuid( );
    int extControlPort = nodeInfoType.getControl_port( );
    Optional<ServiceType> optType
            = convertToExternalServiceType( nodeInfoType.getNode_type( ) );
    Optional<ServiceState> state
            = convertToExternalServiceState( nodeInfoType.getNode_state( ) );

    ServiceType extType = null;
    ServiceStatus extStatus;

    if( optType.isPresent( ) )
    {
      extType = optType.get( );
    }

    if( state.isPresent( ) )
    {
      extStatus = new Service.ServiceStatus( state.get( ) );
    }
    else
    {
      extStatus = new Service.ServiceStatus( Service.ServiceState.UNREACHABLE );
    }

    return new Service( extId, extType, extControlPort,
                                           extStatus );
  }

  public static FDSP_Node_Info_Type convertToInternalService( Node externalNode,
                                                              Service externalService )
  {

    FDSP_Node_Info_Type nodeInfo = new FDSP_Node_Info_Type( );

    nodeInfo.setControl_port( externalService.getPort( ) );
    nodeInfo.setNode_uuid( externalService.getId( ) );
    nodeInfo.setNode_id( externalNode.getId( )
                                     .intValue( ) );
    nodeInfo.setNode_name( externalNode.getName( ) );

    Optional<FDSP_NodeState> optState
            = convertToInternalServiceState( externalService.getStatus( )
                                                            .getServiceState( ) );

    if( optState.isPresent( ) )
    {
      nodeInfo.setNode_state( optState.get( ) );
    }

    Optional<FDSP_MgrIdType> optType
            = convertToInternalServiceType( externalService.getType( ) );

    if( optType.isPresent( ) )
    {
      nodeInfo.setNode_type( optType.get( ) );
    }

    //TODO:  The IP addresses

    return nodeInfo;
  }

  public static com.formationds.protocol.svc.types.ServiceStatus convertToInternalServiceStatus
  (
  ServiceState externalServiceState
  )
  {
	// Conversion of model.Service.ServiceStatus to svc.types.ServiceStatus
	com.formationds.protocol.svc.types.ServiceStatus internalStatus;
	
	switch(externalServiceState){
	case DEGRADED:
	case LIMITED:
	case NOT_RUNNING:
	case ERROR:
	case UNEXPECTED_EXIT:
		internalStatus = com.formationds.protocol.svc.types.ServiceStatus.SVC_STATUS_INACTIVE;
		break;
	case UNREACHABLE:
		internalStatus = com.formationds.protocol.svc.types.ServiceStatus.SVC_STATUS_INVALID;
        break;
	default: //Running,Initializing,Shutting Down
        internalStatus = com.formationds.protocol.svc.types.ServiceStatus.SVC_STATUS_ACTIVE;
        break;
	}

	return internalStatus;
  }

  public static SvcInfo convertServiceToSvcInfoType( final String nodeIp, Service service)
  {
    final Long newId = -1L;
    if( service.getId( ) == null )
    {
      service.setId( newId );
    }

    SvcUuid svcUid = new SvcUuid( service.getId( ) );
    SvcID sId = new SvcID( svcUid, service.getType( )
                                            .name( ) );
	
	Optional<FDSP_MgrIdType> optType
	  	    = convertToInternalServiceType(service.getType());
	
	ServiceState externalServiceState = service.getStatus().getServiceState();

	com.formationds.protocol.svc.types.ServiceStatus internalStatus
	                         = convertToInternalServiceStatus(externalServiceState);

	SvcInfo svcInfo = new SvcInfo( sId,
                                   service.getPort( ),
                                   optType.get( ),
                                   internalStatus,
                                   "auto-name", nodeIp, 0, "name",
                                   new HashMap<>());
	
	  return svcInfo;	  
  }

  public static Optional<ServiceType> convertToExternalServiceType( FDSP_MgrIdType type )
  {

    Optional<ServiceType> externalType;

    switch( type )
    {
      case FDSP_ACCESS_MGR:
        externalType = Optional.of( ServiceType.AM );
        break;
      case FDSP_DATA_MGR:
        externalType = Optional.of( ServiceType.DM );
        break;
      case FDSP_ORCH_MGR:
        externalType = Optional.of( ServiceType.OM );
        break;
      case FDSP_PLATFORM:
        externalType = Optional.of( ServiceType.PM );
        break;
      case FDSP_STOR_MGR:
        externalType = Optional.of( ServiceType.SM );
        break;
      default:
        externalType = Optional.empty( );
    }

    return externalType;
  }

  public static Optional<FDSP_MgrIdType> convertToInternalServiceType( ServiceType externalType )
  {

    Optional<FDSP_MgrIdType> internalType;

    switch( externalType )
    {
      case AM:
        internalType = Optional.of( FDSP_MgrIdType.FDSP_ACCESS_MGR );
        break;
      case DM:
        internalType = Optional.of( FDSP_MgrIdType.FDSP_DATA_MGR );
        break;
      case OM:
        internalType = Optional.of( FDSP_MgrIdType.FDSP_ORCH_MGR );
        break;
      case PM:
        internalType = Optional.of( FDSP_MgrIdType.FDSP_PLATFORM );
        break;
      case SM:
        internalType = Optional.of( FDSP_MgrIdType.FDSP_STOR_MGR );
        break;
      default:
        internalType = Optional.empty( );
    }

    return internalType;
  }

  public static Optional<ServiceState> convertToExternalServiceState( FDSP_NodeState internalState )
  {

    Optional<ServiceState> externalState;

    switch( internalState )
    {
      case FDS_Node_Down:
      case FDS_Node_Discovered:
        externalState = Optional.of( ServiceState.NOT_RUNNING );
        break;

      case FDS_Node_Up:
        externalState = Optional.of( ServiceState.RUNNING );
        break;
      case FDS_Node_Rmvd:
        externalState = Optional.of( ServiceState.UNREACHABLE );
        break;
      case FDS_Start_Migration:
        externalState = Optional.of( ServiceState.INITIALIZING );
        break;
      default:
        externalState = Optional.of( ServiceState.RUNNING );
    }

    return externalState;
  }

  public static Optional<FDSP_NodeState> convertToInternalServiceState( ServiceState externalStatus )
  {

    Optional<FDSP_NodeState> internalState;

    switch( externalStatus )
    {
      case DEGRADED:
      case LIMITED:
      case NOT_RUNNING:
      case ERROR:
      case UNEXPECTED_EXIT:
        internalState = Optional.of( FDSP_NodeState.FDS_Node_Down );
        break;
      case UNREACHABLE:
        internalState = Optional.of( FDSP_NodeState.FDS_Node_Rmvd );
        break;
      case INITIALIZING:
        internalState = Optional.of( FDSP_NodeState.FDS_Start_Migration );
        break;
      default:
        internalState = Optional.of( FDSP_NodeState.FDS_Node_Up );
        break;
    }

    return internalState;
  }

  private static byte[] htonl( long x )
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
