/*
 * Copyright (c) 2016, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.util;

import com.formationds.client.v08.converters.PlatformModelConverter;
import com.formationds.commons.libconfig.ParsedConfig;
import com.formationds.nfs.XdiStaticConfiguration;
import com.formationds.om.helper.SingletonOmConfigApi;
import com.formationds.protocol.svc.types.FDSP_Node_Info_Type;
import com.formationds.protocol.svc.types.SvcUuid;
import com.formationds.util.Configuration;
import com.formationds.util.ServerPortFinder;
import com.formationds.xdi.AsyncAm;
import com.formationds.xdi.FakeAsyncAm;
import com.formationds.xdi.RealAsyncAm;
import org.apache.thrift.TException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;
import java.net.InetAddress;
import java.util.Optional;

/**
 * @author ptinius
 */
public class AsyncAmClientFactory
{
    private static final Logger logger = LoggerFactory.getLogger( AsyncAmClientFactory.class );

    private final Configuration configuration;

    public AsyncAmClientFactory( final Configuration configuration )
    {
        this.configuration = configuration;
    }

    /**
     * @param uuid the service uuid of the target access manager.
     *
     * @return Returns the {@link AsyncAm} client
     *
     * @throws IOException any I/O errors
     */
    public AsyncAm newClient( final SvcUuid uuid )
        throws IOException
    {
        return newClient( uuid, false );
    }

    /**
     * @param uuid the service uuid of the target access manager.
     * @param start should the client be started.
     *
     * @return Returns the {@link AsyncAm} client
     *
     * @throws IOException any I/O errors
     */
    public AsyncAm newClient( final SvcUuid uuid, final boolean start )
        throws IOException
    {
        final Optional<FDSP_Node_Info_Type> svc = lookup( uuid );
        if( !svc.isPresent() )
        {
            throw new IllegalStateException( "The specified service uuid [ " +
                                             uuid +
                                             " ] was not found." );
        }

        return newClient(
            InetAddress.getByAddress( PlatformModelConverter.htonl( svc.get()
                                                                       .getIp_lo_addr( ) ) )
                       .getHostAddress(),
            start );
    }

    /**
     * @param host the host, i.e. localhost
     *
     * @return Returns the {@link AsyncAm} client
     *
     * @throws IOException any I/O errors
     */
    public AsyncAm newClient( final String host )
        throws IOException
    {
        return newClient( host, false );
    }

    /**
     * @param host the host, i.e. localhost
     * @param start should the client be started.
     *
     * @return Returns the {@link AsyncAm} client
     *
     * @throws IOException any I/O errors
     */
    public AsyncAm newClient( final String host, final boolean start )
        throws IOException
    {
        final ParsedConfig platformDotConf = configuration.getPlatformConfig( );

        if( platformDotConf.defaultBoolean( "fds.am.memory_backend", false ) )
        {
            logger.debug( "Using memory backed asynchronous access manager." );
            return new FakeAsyncAm( );
        }

        final int pmPortNo = platformDotConf.defaultInt( "fds.pm.platform_port", 7000 );
        final int xdiServicePort = pmPortNo +
                                   platformDotConf.defaultInt( "fds.am.xdi_service_port_offset",
                                                               1899 );
        final int xdiResponsePort =
            new ServerPortFinder().findPort( "Async XDI ( " + host + " )",
                                             pmPortNo +
                                             platformDotConf.defaultInt( "fds.om.xdi_response_port_offset",
                                                                         2988 ) );
        final XdiStaticConfiguration xdiStaticConfig = configuration.getXdiStaticConfig( pmPortNo );

        final RealAsyncAm asyncAm = new RealAsyncAm( host,
                                                     xdiServicePort,
                                                     xdiResponsePort,
                                                     xdiStaticConfig.getAmTimeout( ) );
        if( start )
        {
            asyncAm.start( );
        }

        return asyncAm;
    }

    protected Optional<FDSP_Node_Info_Type> lookup( final SvcUuid uuid )
    {
        try
        {
            logger.trace( "Looking up service {} ( hex: {} )",
                          uuid.getSvc_uuid(),
                          Long.toHexString( uuid.getSvc_uuid() ) );
            return SingletonOmConfigApi.INSTANCE
                                       .api()
                                       .listLocalDomainServices( "local" )
                                       .stream()
                                       .filter( ( svc ) -> svc.getService_uuid() == uuid.getSvc_uuid() )
                                       .findFirst();
        }
        catch ( TException e )
        {
            return Optional.empty();
        }
    }
}
