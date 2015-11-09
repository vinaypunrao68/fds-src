/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */
package com.formationds.platform.svclayer;

import com.formationds.protocol.svc.types.FDSP_ActivateNodeType;
import com.formationds.protocol.svc.PlatNetSvc;
import com.formationds.protocol.svc.types.AsyncHdr;
import com.formationds.protocol.svc.types.DomainNodes;
import com.formationds.protocol.svc.types.NodeInfoMsg;
import com.formationds.protocol.svc.types.ServiceStatus;
import com.formationds.protocol.svc.types.SvcInfo;
import com.formationds.util.thrift.ThriftClientFactory;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.apache.logging.log4j.Marker;
import org.apache.logging.log4j.MarkerManager;
import org.apache.thrift.TBase;
import org.apache.thrift.TException;
import org.apache.thrift.protocol.TProtocol;

import java.nio.ByteBuffer;
import java.util.List;
import java.util.Map;
import java.util.function.Function;

/**
 * Default implementation of the PlatNetSvc.Ifase that forwards all requests to the
 * client specified by the client factory.  This adapter is intended to be overridden to
 * provide specialized handling of specific requests as needed.
 */
public abstract class PlatNetSvcHandlerBase<S extends PlatNetSvc.Iface> implements PlatNetSvc.Iface {

    static final Logger logger       = LogManager.getLogger();
    static final Marker SVC_REQUEST  = MarkerManager.getMarker( "SVC_REQUEST" );
    static final Marker SVC_RESPONSE = MarkerManager.getMarker( "SVC_RESPONSE" );

    /**
     * Create with default settings.  Must use #getClient(host, port) to access the client
     *
     * @see #newClientFactory(Class, Function, String, Integer, int, int, int)
     */
    public static <F extends PlatNetSvc.Iface> ThriftClientFactory<F> newClientFactory( Class<F> serviceClass,
                                                                                        Function<TProtocol, F> clientFactory ) {
        return newClientFactory( serviceClass, clientFactory,
                                 null, null,
                                 ThriftClientFactory.DEFAULT_MAX_POOL_SIZE,
                                 ThriftClientFactory.DEFAULT_MIN_IDLE,
                                 ThriftClientFactory.DEFAULT_MIN_EVICTION_IDLE_TIME_MS );
    }

    /**
     * @see #newClientFactory(Class, Function, String, Integer, int, int, int)
     */
    public static <F extends PlatNetSvc.Iface> ThriftClientFactory<F> newClientFactory( Class<F> serviceClass,
                                                                                        Function<TProtocol, F> clientFactory,
                                                                                        String host, Integer port ) {
        return newClientFactory( serviceClass, clientFactory, host, port,
                                 ThriftClientFactory.DEFAULT_MAX_POOL_SIZE,
                                 ThriftClientFactory.DEFAULT_MIN_IDLE,
                                 ThriftClientFactory.DEFAULT_MIN_EVICTION_IDLE_TIME_MS );
    }

    /**
     * Create a new PlatNetSvc.Iface client factory.  The host and port are optional and if provided the
     * ThriftClientFactory#getClient() api will return a connection to that default client host and port.
     * <p/>
     * Otherwise, clients must specify a host and port using ThriftClientFactory#getClient(host, port).
     *
     * @param host                          the optional default thrift server host.
     * @param port                          the optional default thrift server port.  If a host is specified, then the port is required.
     * @param maxPoolSize                   the max client pool size
     * @param minIdle                       the minimum number of connection to keep in the pool
     * @param softMinEvictionIdleTimeMillis the idle client eviction time.
     *
     * @return a ThriftClientFactory for the PlatNetSvc.Iface with the specified default host and port.
     */
    public static <F extends PlatNetSvc.Iface> ThriftClientFactory<F> newClientFactory( Class<F> serviceClass,
                                                                                        Function<TProtocol, F> clientFactory,
                                                                                        String host,
                                                                                        Integer port,
                                                                                        int maxPoolSize,
                                                                                        int minIdle,
                                                                                        int softMinEvictionIdleTimeMillis ) {
        return new ThriftClientFactory.Builder<>( serviceClass )
                   .withHostPort( host, port )
                   .withPoolConfig( maxPoolSize, minIdle, softMinEvictionIdleTimeMillis )
                   .withClientFactory( clientFactory )
                   .build();
    }

    private final ThriftClientFactory<S> omNativePlatformClientFactory;

    /**
     * @param factory
     */
    protected PlatNetSvcHandlerBase( ThriftClientFactory<S> factory ) {
        omNativePlatformClientFactory = factory;
    }

    /**
     * @return the client factory that all commands are delegated to.
     */
    protected ThriftClientFactory<S> getOmNativePlatformClientFactory() { return omNativePlatformClientFactory; }

    private void logPayload( Marker marker, AsyncHdr asyncHdr, ByteBuffer payload ) {

        // short-circuit logging if not enabled (especially due to encoding payload and deserialization)
        if (!logger.isTraceEnabled()) {
            return;
        }

        logger.trace( marker,
                      "Processing Header {}; Payload Size={}", asyncHdr, payload.limit() );
        logger.trace( marker,
                      "Payload Data: 0x{}", HexDump.encodeHex( payload.array() ) );

        /* in order to deserialize the payload, we need to interrogate the Header
         * and extract the FDSPMsgTypeId.  Using the type id, we need to lookup the implementing Java class
         * in order to deserialize.  Of course, there is no direct link to the class -- we need to get the
         * typeid's enumerated name and hack off the "TypeId" and add the com.formationds.protocol.svc.types
         * package.
         */
        try {
            TBase o = SvcLayerSerializer.deserialize( asyncHdr.getMsg_type_id(), payload );
            logger.trace( marker,
                          "Deserialized Payload data: {}",
                          o );
        } catch ( Exception e ) {
            logger.warn( marker, "Failed to deserialize payload data for type {}", asyncHdr.getMsg_type_id() );
            logger.trace( marker, "\n{}", HexDump.encodeHexAndAscii( payload.array() ) );
            logger.warn( "", e );
        }
    }

    @Override
    public void asyncReqt( AsyncHdr asyncHdr, ByteBuffer payload ) throws TException {
        logger.trace( "asyncReqt: {}", asyncHdr );
        logPayload( SVC_REQUEST, asyncHdr, payload );
        omNativePlatformClientFactory.getClient().asyncReqt( asyncHdr, payload );
    }

    @Override
    public void asyncResp( AsyncHdr asyncHdr, ByteBuffer payload ) throws TException {
        logger.trace( "asyncResp: {}", asyncHdr );
        logPayload( SVC_RESPONSE, asyncHdr, payload );
        omNativePlatformClientFactory.getClient().asyncResp( asyncHdr, payload );
    }

    @Override
    public List<SvcInfo> getSvcMap( long nullarg ) throws TException {
        logger.trace( "getSvcMap" );
        return omNativePlatformClientFactory.getClient().getSvcMap( nullarg );
    }

    @Override
    public void notifyNodeActive( FDSP_ActivateNodeType info ) throws TException {
        logger.trace( "notifyNodeActive: {}", info );
        omNativePlatformClientFactory.getClient().notifyNodeActive( info );
    }

    @Override
    public List<NodeInfoMsg> notifyNodeInfo( NodeInfoMsg info, boolean bcast ) throws TException {
        logger.trace( "notifyNodeInfo: {}", info );
        return omNativePlatformClientFactory.getClient().notifyNodeInfo( info, bcast );
    }

    @Override
    public DomainNodes getDomainNodes( DomainNodes dom ) throws TException {
        logger.trace( "getDomainNodes: {},", dom );
        return omNativePlatformClientFactory.getClient().getDomainNodes( dom );
    }

    @Override
    public ServiceStatus getStatus( int nullarg ) throws TException {
        logger.trace( "getStatus" );
        return omNativePlatformClientFactory.getClient().getStatus( nullarg );
    }

    @Override
    public Map<String, Long> getCounters( String id ) throws TException {
        logger.trace( "getCounters: {}", id );
        return omNativePlatformClientFactory.getClient().getCounters( id );
    }

    @Override
    public void resetCounters( String id ) throws TException {
        logger.trace( "resetCounters: {}", id );
        omNativePlatformClientFactory.getClient().resetCounters( id );
    }

    @Override
    public void setConfigVal( String id, long value ) throws TException {
        logger.trace( "setConfigVal: {} = {}", id, value );
        omNativePlatformClientFactory.getClient().setConfigVal( id, value );
    }

    @Override
    public void setFlag( String id, long value ) throws TException {
        logger.trace( "setFlag:  {} = {}", id, value );
        omNativePlatformClientFactory.getClient().setFlag( id, value );
    }

    @Override
    public long getFlag( String id ) throws TException {
        logger.trace( "getFlag: {}", id );
        return omNativePlatformClientFactory.getClient().getFlag( id );
    }

    @Override
    public Map<String, Long> getFlags( int nullarg ) throws TException {
        logger.trace( "getFlags" );
        return omNativePlatformClientFactory.getClient().getFlags( nullarg );
    }

    @Override
    public Map<String,String> getConfig(int nullarg) throws TException {
        logger.trace( "getConfig" );
        return omNativePlatformClientFactory.getClient().getConfig( nullarg );
    }

    @Override
    public Map<String,String> getProperties(int nullarg) throws TException {
        logger.trace( "getProperties" );
        return omNativePlatformClientFactory.getClient().getProperties( nullarg );
    }

    @Override
    public String getProperty(String name) throws TException {
        logger.trace( "getProperty" );
        return omNativePlatformClientFactory.getClient().getProperty( name );
    }

    @Override
    public boolean setFault( String cmdline ) throws TException {
        logger.trace( "setFault: {}", cmdline );
        return omNativePlatformClientFactory.getClient().setFault( cmdline );
    }
}
