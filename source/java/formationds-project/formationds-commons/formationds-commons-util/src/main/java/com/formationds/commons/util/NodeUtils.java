/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.util;

import com.formationds.protocol.FDSP_MgrIdType;
import com.formationds.protocol.FDSP_Node_Info_Type;
import com.formationds.protocol.SvcUuid;
import com.formationds.protocol.svc.types.SvcInfo;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import java.math.BigInteger;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;
import java.util.stream.Collectors;

/**
 * @author ptinius
 */
public class NodeUtils
{
    private static final Logger logger =
        LogManager.getLogger( NodeUtils.class );

    /**
     * @param uuid the {@link SvcUuid} representing the service uuid
     *
     * @return Returns the {@link SvcUuid} representing the platform/node uuid
     */
    public static SvcUuid getNodeUUID( final SvcUuid uuid )
    {
        return ResourceUUID.getUuid4Type( uuid, FDSP_MgrIdType.FDSP_PLATFORM );
    }

    /**
     * @param uuid the {@link SvcUuid} representing the service uuid
     *
     * @return Returns {@link String} representing the hex uuid and the simple name of the service
     */
    public static String mapToSvcUuidAndName( final SvcUuid uuid )
    {
        return String.format( "%x:%s",
                              uuid.svc_uuid,
                              mapToSvcName(
                                  new ResourceUUID(
                                      BigInteger.valueOf(
                                          uuid.svc_uuid ) ).type( ) ) );
    }

    /**
     * @param svcType the {@link FDSP_MgrIdType}
     *
     * @return Returns {@link String} representing the simple name of the service
     */
    public static String mapToSvcName( final FDSP_MgrIdType svcType )
    {
        switch( svcType )
        {
            case FDSP_PLATFORM:
                return "pm";
            case FDSP_STOR_MGR:
                return "sm";
            case FDSP_DATA_MGR:
                return "dm";
            case FDSP_ACCESS_MGR:
                return "am";
            case FDSP_ORCH_MGR:
                return "om";
            default:
                return "unknown";
        }
    }

    /**
     * @param svc the {@link SvcInfo} to be logged
     */
    public static void log( final SvcInfo svc )
    {
        final String string =
            String.format( "Svc handle svc_uuid: %s" +
                           " ip: %s" +
                           " port: %d" +
                           " incarnation: %d" +
                           " status: %s( %d )",
                           NodeUtils.mapToSvcUuidAndName( svc.getSvc_id( )
                                                            .svc_uuid ),
                           svc.getIp( ),
                           svc.getSvc_port( ),
                           svc.getIncarnationNo( ),
                           svc.getSvc_status( )
                              .name( ),
                           svc.getSvc_status()
                              .getValue() );

        String props = "";
        if( svc.getPropsSize( ) != 0 )
        {

            for( final String prop : svc.getProps().keySet( ) )
            {
                props += ( prop + " : " + svc.getProps().get( prop ) + "\n" );
            }
        }


        logger.trace( string + ( props.length() > 0 ? "\n" + props : "" ) );
    }

    /**
     * @param nodeInfos the {@link List} of {@link FDSP_Node_Info_Type}s
     *
     * @return Returns {@link Map} grouped by unique {@link FDSP_Node_Info_Type} field
     */
    public static Map<Long,List<FDSP_Node_Info_Type>> groupNodes(
        final List<FDSP_Node_Info_Type> nodeInfos,
        final long omNodeUuid )
    {
        logger.debug( "OM NODE UUID::{}", omNodeUuid );
        final ConcurrentMap<Long,List<FDSP_Node_Info_Type>> grouped =
            new ConcurrentHashMap<>( );

        if( NodeUtils.isMultiNode( nodeInfos ) &&
            !NodeUtils.isVirtualNode( nodeInfos ) )
        {
            grouped.putAll(
                nodeInfos.stream( )
                         .collect(
                             Collectors.groupingBy(
                                 FDSP_Node_Info_Type::getIp_lo_addr,
                                 Collectors.toList( ) ) ) );

            logger.debug( "Found multi-node configuration" );
        }
        else if( NodeUtils.isVirtualNode( nodeInfos ) ||
                 NodeUtils.isSingleNode( nodeInfos ) )
        {
            grouped.putAll(
                nodeInfos.stream()
                         .collect(
                             Collectors.groupingBy(
                                 FDSP_Node_Info_Type::getNode_uuid,
                                 Collectors.toList( ) ) ) );

            logger.debug( "Found virtualized-node configuration" );
        }

        /*
         * HACK ( Tinius )
         *
         * currently the OM service will show up as its own node. This is
         * because the when service layer team implemented service layer
         * they fit that yhe OM needs its own node/service uuid.
         *
         * In platform.conf the key 'fds.common.om_uuid' is set to 1024.
         * Which when used within the SvcInfo record for the OM, caused the
         * service uuid to be 1028 ( decimal ) 404 ( hex ).
         *
         * So we have to place the OM service into the correct node uuid
         * map.
         *
         * The OM should not have a special uuid ( node or service ). It
         * should use the node uuid of its residing node.
         */
        final List<FDSP_Node_Info_Type> omNodeList =
            grouped.get( omNodeUuid );

        if( omNodeList == null || omNodeList.isEmpty( ) )
        {
            logger.warn( "No OM node found, leaving unmodified. " +
                         "OM Node UUID not set to default?" );
        }
        else if( omNodeList.size( ) > 1 )
        {
            logger.warn( "More then one OM, only expected one, " +
                         "leaving unmodified. OM Node UUID not set to default?" );
        }
        else
        {
            final FDSP_Node_Info_Type om = omNodeList.get( 0 );

            for( final List<FDSP_Node_Info_Type> services : grouped.values() )
            {
                Long nodeUUID = null;
                final List<FDSP_Node_Info_Type> _omNode = new ArrayList<>( );

                for( final FDSP_Node_Info_Type service : services )
                {
                    if( service.getNode_type().equals( FDSP_MgrIdType.FDSP_PLATFORM ) )
                    {
                        if( service.getNode_root( )
                                   .equalsIgnoreCase(
                                       om.getNode_root( ) ) )
                        {
                            nodeUUID = service.getNode_uuid();
                            _omNode.add( om );
                            _omNode.addAll( grouped.get( nodeUUID ) );

                            break;
                        }
                    }
                }

                if( nodeUUID != null )
                {
                    grouped.replace( nodeUUID, _omNode );
                    grouped.remove( om.getNode_uuid() );
                    break;
                }
            }
        }

        return grouped;
    }

    /**
     * @param nodes the {@link List} of {@link FDSP_Node_Info_Type}s
     *
     * @return Returns {@code true} if a single-node
     */
    public static boolean isSingleNode( final List<FDSP_Node_Info_Type> nodes )
    {
        return ( ( numberOfNodes( nodes ) == 1 ) &&
                 ( numberOfPMs( nodes ) == 1 ) );
    }

    /**
     * @param nodes the {@link List} of {@link FDSP_Node_Info_Type}s
     *
     * @return Returns {@code true} if a multi-node
     */
    public static boolean isMultiNode( final List<FDSP_Node_Info_Type> nodes )
    {
        return !isSingleNode( nodes );
    }

    /**
     * @param nodes the {@link List} of {@link FDSP_Node_Info_Type}s
     *
     * @return Returns {@code true} if all on one node
     */
    public static boolean isVirtualNode( final List<FDSP_Node_Info_Type> nodes )
    {
        return ( ( numberOfNodes( nodes ) == 1 ) &&
                 ( numberOfPMs( nodes ) >= 1 ) );
    }

    private static int numberOfNodes( final List<FDSP_Node_Info_Type> nodes )
    {
        List<Long> ips = new ArrayList<>( );
        for( final FDSP_Node_Info_Type node : nodes )
        {
            if( ips.isEmpty() )
            {
                ips.add( node.getIp_lo_addr() );
            }
            else if( !ips.contains( node.getIp_lo_addr() ) )
            {
                ips.add( node.getIp_lo_addr() );
            }
        }

        return ips.size();
    }

    private static int numberOfPMs( final List<FDSP_Node_Info_Type> nodes )
    {
        List<Long> pms =
            nodes.stream( )
                 .filter( node -> node.getNode_type( )
                                      .equals(
                                          FDSP_MgrIdType.FDSP_PLATFORM ) )
                 .map( FDSP_Node_Info_Type::getIp_lo_addr )
                 .collect( Collectors.toList( ) );

        return pms.size( );
    }
}
