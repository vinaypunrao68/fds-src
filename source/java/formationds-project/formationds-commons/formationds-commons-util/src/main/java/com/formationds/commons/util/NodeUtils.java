/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.util;

import com.formationds.protocol.FDSP_MgrIdType;
import com.formationds.protocol.SvcUuid;
import com.formationds.protocol.svc.types.SvcInfo;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import java.math.BigInteger;

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
        if( svc.getPropsSize() != 0 )
        {

            for( final String prop : svc.getProps().keySet() )
            {
                props += ( prop + " : " + svc.getProps().get( prop ) + "\n" );
            }
        }


        logger.trace( string + ( props.length() > 0 ? "\n" + props : "" ) );
//        System.out.println( string + ( props.length() > 0 ? "\n" + props : "" ) );
    }
}
