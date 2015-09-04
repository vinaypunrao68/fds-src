/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */
package com.formationds.platform.svclayer;

import FDS_ProtocolInterface.FDSP_ActivateNodeType;
import com.formationds.protocol.svc.PlatNetSvc;
import com.formationds.protocol.svc.types.AsyncHdr;
import com.formationds.protocol.svc.types.DomainNodes;
import com.formationds.protocol.svc.types.NodeInfoMsg;
import com.formationds.protocol.svc.types.ServiceStatus;
import com.formationds.protocol.svc.types.SvcInfo;
import com.formationds.util.thrift.ThriftClientFactory;
import org.apache.thrift.TException;

import java.nio.ByteBuffer;
import java.util.List;
import java.util.Map;

/**
 * Default implementation of the PlatNetSvc.Ifase that forwards all requests to the
 * client specified by the client factory.  This adapter is intended to be overridden to
 * provide specialized handling of specific requests as needed.
 */
public abstract class PlatNetSvcHandlerBase implements PlatNetSvc.Iface {

    /**
     *
     */
    public static class PlatformServiceClientFactory {

        /**
         * Create with default settings.  Must use #getClient(host, port) to access the client
         *
         * @see #newPlatformService(String, Integer, int, int, int)
         */
        public static ThriftClientFactory<PlatNetSvc.Iface> newPlatformService() {
            return newPlatformService( null, null,
                                       ThriftClientFactory.DEFAULT_MAX_POOL_SIZE,
                                       ThriftClientFactory.DEFAULT_MIN_IDLE,
                                       ThriftClientFactory.DEFAULT_MIN_EVICTION_IDLE_TIME_MS );
        }

        /**
         * @see #newPlatformService(String, Integer, int, int, int)
         */
        public static ThriftClientFactory<PlatNetSvc.Iface> newPlatformService( String host, Integer port ) {
            return newPlatformService( host, port,
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
         * @param host the optional default thrift server host.
         * @param port the optional default thrift server port.  If a host is specified, then the port is required.
         * @param maxPoolSize the max client pool size
         * @param minIdle the minimum number of connection to keep in the pool
         * @param softMinEvictionIdleTimeMillis the idle client eviction time.
         *
         * @return a ThriftClientFactory for the PlatNetSvc.Iface with the specified default host and port.
         */
        public static ThriftClientFactory<PlatNetSvc.Iface> newPlatformService( String host, Integer port,
                                                                                int maxPoolSize,
                                                                                int minIdle,
                                                                                int softMinEvictionIdleTimeMillis ) {
            return new ThriftClientFactory.Builder<>( PlatNetSvc.Iface.class )
                       .withHostPort( host, port )
                       .withPoolConfig( maxPoolSize, minIdle, softMinEvictionIdleTimeMillis )
                       .withClientFactory( PlatNetSvc.Client::new )
                       .build();
        }

        /*
         * prevent instantiation
         */
        private PlatformServiceClientFactory() {}
    }

    private ThriftClientFactory<PlatNetSvc.Iface> omNativePlatformClientFactory;

    /**
     */
    public PlatNetSvcHandlerBase() {
        this( PlatformServiceClientFactory.newPlatformService() );
    }

    /**
     * @param factory
     */
    protected PlatNetSvcHandlerBase( ThriftClientFactory<PlatNetSvc.Iface> factory ) {
        omNativePlatformClientFactory = factory;
    }

    @Override
    public void asyncReqt( AsyncHdr asyncHdr, ByteBuffer payload ) throws TException {
        omNativePlatformClientFactory.getClient().asyncReqt( asyncHdr, payload );
    }

    @Override
    public void asyncResp( AsyncHdr asyncHdr, ByteBuffer payload ) throws TException {
        omNativePlatformClientFactory.getClient().asyncResp( asyncHdr, payload );
    }

    @Override
    public List<SvcInfo> getSvcMap( long nullarg ) throws TException {
        return omNativePlatformClientFactory.getClient().getSvcMap( nullarg );
    }

    @Override
    public void notifyNodeActive( FDSP_ActivateNodeType info ) throws TException {
        omNativePlatformClientFactory.getClient().notifyNodeActive( info );
    }

    @Override
    public List<NodeInfoMsg> notifyNodeInfo( NodeInfoMsg info, boolean bcast ) throws TException {
        return omNativePlatformClientFactory.getClient().notifyNodeInfo( info, bcast );
    }

    @Override
    public DomainNodes getDomainNodes( DomainNodes dom ) throws TException {
        return omNativePlatformClientFactory.getClient().getDomainNodes( dom );
    }

    @Override
    public ServiceStatus getStatus( int nullarg ) throws TException {
        return omNativePlatformClientFactory.getClient().getStatus( nullarg );
    }

    @Override
    public Map<String, Long> getCounters( String id ) throws TException {
        return omNativePlatformClientFactory.getClient().getCounters( id );
    }

    @Override
    public void resetCounters( String id ) throws TException {
        omNativePlatformClientFactory.getClient().resetCounters( id );
    }

    @Override
    public void setConfigVal( String id, long value ) throws TException {
        omNativePlatformClientFactory.getClient().setConfigVal( id, value );
    }

    @Override
    public void setFlag( String id, long value ) throws TException {
        omNativePlatformClientFactory.getClient().setFlag( id, value );
    }

    @Override
    public long getFlag( String id ) throws TException {
        return omNativePlatformClientFactory.getClient().getFlag( id );
    }

    @Override
    public Map<String, Long> getFlags( int nullarg ) throws TException {
        return omNativePlatformClientFactory.getClient().getFlags( nullarg );
    }

    @Override
    public boolean setFault( String cmdline ) throws TException {
        return omNativePlatformClientFactory.getClient().setFault( cmdline );
    }
}
