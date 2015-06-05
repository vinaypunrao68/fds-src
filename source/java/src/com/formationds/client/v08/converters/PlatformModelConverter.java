package com.formationds.client.v08.converters;

import java.net.Inet4Address;
import java.net.Inet6Address;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;

import com.formationds.client.v08.model.Node;
import com.formationds.client.v08.model.Node.NodeAddress;
import com.formationds.client.v08.model.Node.NodeState;
import com.formationds.client.v08.model.Service;
import com.formationds.client.v08.model.ServiceType;
import com.formationds.client.v08.model.Service.ServiceStatus;
import com.formationds.protocol.FDSP_MgrIdType;
import com.formationds.protocol.FDSP_NodeState;
import com.formationds.protocol.FDSP_Node_Info_Type;

public class PlatformModelConverter {

	public static Node convertToExternalNode( List<FDSP_Node_Info_Type> nodeInfoTypes ) throws UnknownHostException{
		
		Map<ServiceType, List<Service>> services = new HashMap<>();
		NodeState state = NodeState.UNKNOWN;
		Long nodeId = 0L;
		NodeAddress address = null;
		
		for ( FDSP_Node_Info_Type internalService : nodeInfoTypes ){
			
			Service externalService = convertToExternalService( internalService );
			
			// get the node info from whomever, but choose the PM if we can
			if ( address == null || externalService.getType().equals( ServiceType.PM ) ){
				
				state = convertToExternalNodeState( internalService.getNode_state() );
				nodeId = internalService.getNode_uuid();
				
				Inet4Address ipv4 = (Inet4Address) InetAddress.getByAddress( htonl( internalService.getIp_lo_addr() ) );
				
				// TODO: Figure this out
				Inet6Address ipv6 = null;
				
				address = new NodeAddress(ipv4, ipv6);
			}
			
			List<Service> likeServiceList = services.get( externalService.getType() );
			
			if ( likeServiceList == null ){
				likeServiceList = new ArrayList<>();
			}
			
			likeServiceList.add( externalService );
			
			services.put( externalService.getType(), likeServiceList );
			
		}
		
		Node node = null;
		
		if ( address != null ){
			node = new Node( nodeId, address, state, services );
		}
		
		return node;
	}
	
	public static List<FDSP_Node_Info_Type> convertToInternalNode( Node node ){
		return null;
	}
	
	public static NodeState convertToExternalNodeState( FDSP_NodeState internalNodeState ){
		
		switch( internalNodeState ){
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
	
	public static FDSP_NodeState convertToInternalNodeState( NodeState externalState ){
		
		switch( externalState ){
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

	public static Service convertToExternalService( FDSP_Node_Info_Type nodeInfoType ) {

		Long extId = nodeInfoType.getService_uuid();
		int extControlPort = nodeInfoType.getControl_port();
		Optional<ServiceType> optType = convertToExternalServiceType( nodeInfoType.getNode_type() );
		Optional<ServiceStatus> optStatus = convertToExternalServiceStatus( nodeInfoType.getNode_state() );

		ServiceType extType = null;
		ServiceStatus extStatus = null;

		if ( optType.isPresent() ) {
			extType = optType.get();
		}

		if ( optStatus.isPresent() ) {
			extStatus = optStatus.get();
		}

		Service externalService = new Service( extId, extType, extControlPort, extStatus );

		return externalService;
	}

	public static FDSP_Node_Info_Type convertToInternalService( Node externalNode, Service externalService ) {

		FDSP_Node_Info_Type nodeInfo = new FDSP_Node_Info_Type();

		nodeInfo.setControl_port( externalService.getPort() );
		nodeInfo.setNode_uuid( externalService.getId() );
		nodeInfo.setNode_id( externalNode.getId().intValue() );
		nodeInfo.setNode_name( externalNode.getName() );

		Optional<FDSP_NodeState> optState = convertToInternalServiceStatus( externalService.getStatus() );

		if ( optState.isPresent() ) {
			nodeInfo.setNode_state( optState.get() );
		}

		Optional<FDSP_MgrIdType> optType = convertToInternalServiceType( externalService.getType() );

		if ( optType.isPresent() ) {
			nodeInfo.setNode_type( optType.get() );
		}

		//TODO:  The IP addresses

		return nodeInfo;
	}
	
    public static Optional<ServiceType> convertToExternalServiceType( FDSP_MgrIdType type ) {

        Optional<ServiceType> externalType;

        switch ( type ) {
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
                externalType = Optional.empty();
        }

        return externalType;
    }

    public static Optional<FDSP_MgrIdType> convertToInternalServiceType( ServiceType externalType ) {

        Optional<FDSP_MgrIdType> internalType;

        switch ( externalType ) {
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
                internalType = Optional.empty();
        }

        return internalType;
    }

    public static Optional<ServiceStatus> convertToExternalServiceStatus( FDSP_NodeState internalState ) {

        Optional<ServiceStatus> externalStatus;

        switch ( internalState ) {
            case FDS_Node_Down:
                externalStatus = Optional.of( ServiceStatus.NOT_RUNNING );
                break;
            case FDS_Node_Up:
                externalStatus = Optional.of( ServiceStatus.RUNNING );
                break;
            case FDS_Node_Rmvd:
                externalStatus = Optional.of( ServiceStatus.UNREACHABLE );
                break;
            case FDS_Start_Migration:
                externalStatus = Optional.of( ServiceStatus.INITIALIZING );
                break;
            case FDS_Node_Discovered:
                externalStatus = Optional.of( ServiceStatus.NOT_RUNNING );
            default:
                externalStatus = Optional.of( ServiceStatus.RUNNING );
        }

        return externalStatus;
    }

    public static Optional<FDSP_NodeState> convertToInternalServiceStatus( ServiceStatus externalStatus ) {

        Optional<FDSP_NodeState> internalState;

        switch ( externalStatus ) {
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
        byte[] res = new byte[4];
        for( int i = 0; i < 4; i++ ) {

            res[i] = ( new Long( x >>> 24 ) ).byteValue( );
            x <<= 8;

        }

        return res;
    }
}
