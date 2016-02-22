/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */
package com.formationds.platform.svclayer;

import com.formationds.commons.util.thread.ThreadFactories;
import com.formationds.protocol.svc.PlatNetSvc;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.apache.thrift.TMultiplexedProcessor;
import org.apache.thrift.TProcessor;
import org.apache.thrift.server.TNonblockingServer;
import org.apache.thrift.transport.TNonblockingServerSocket;
import org.apache.thrift.transport.TTransportException;

import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;
import java.util.HashMap;
import java.util.Map;

/**
 * The Service Layer SvcServer class runs a Thrift server that accepts events for the
 * specified extension of the PlatNetSvc.Iface.
 *
 * @param <S>
 */
public class SvcServer<S extends PlatNetSvc.Iface> {

    static final Logger logger = LogManager.getLogger();

    /**
     * Shutdown hook that ensure the server is shutdown when the JVM shuts down
     */
    private static class SvcServerShutdownHook extends Thread {
        private final SvcServer<?> svcServer;

        SvcServerShutdownHook( SvcServer<?> s ) {
            this.svcServer = s;
        }

        public void run() {
            try {
                svcServer.stopAndWait( 60, TimeUnit.SECONDS );
            } catch ( Throwable t ) {
                logger
                    .warn( "Failed to stop Service Layer Proxy server within 60 second timeout. Forcing executor shutdown." );
            } finally {
                svcServer.serviceLayerExecutor.shutdownNow();
            }
        }
    }

    /**
     *
     */
    public static class ServerLifecycleException extends RuntimeException {
        public ServerLifecycleException( String message, Throwable cause ) {
            super( message, cause );
        }

        public ServerLifecycleException( String message ) {
            super( message );
        }
    }

    private final InetSocketAddress       address;
    private final S                       serviceLayerHandler;
    private final String                  thriftServiceName;
    private final ThriftServiceDescriptor serviceDescriptor;
    private final ExecutorService         serviceLayerExecutor;

    private final SvcServerShutdownHook shutdownHook;

    private TNonblockingServer thriftServer;

    /**
     * @param port    the port to bind on
     * @param handler the service interface to forward events to
     * @param thriftServiceName empty string if non-multiplexed server, otherwise a Thrift service name
     */
    public SvcServer( int port, S handler, String thriftServiceName ) {
        this( new InetSocketAddress( port ), handler, thriftServiceName );
    }

    /**
     * @param host    the host address to bind to.
     * @param port    the port to bind on
     * @param handler the service interface to forward events to
     * @param thriftServiceName empty string if non-multiplexed server, otherwise a Thrift service name
     */
    public SvcServer( String host, int port, S handler, String thriftServiceName ) {
        this( new InetSocketAddress( host, port ), handler, thriftServiceName );
    }

    /**
     * @param host    the host inet address to bind to.
     * @param port    the port to bind on
     * @param handler the service interface to forward events to
     * @param thriftServiceName empty string if non-multiplexed server, otherwise a Thrift service name
     */
    public SvcServer( InetAddress host, int port, S handler, String thriftServiceName ) {
        this( new InetSocketAddress( host, port ), handler, thriftServiceName );
    }

    /**
     * @param address the bind address for the service layer
     * @param handler the service interface to forward events to
     * @param thriftServiceName empty string if non-multiplexed server, otherwise a Thrift service name
     */
    public SvcServer( InetSocketAddress address, S handler, String thriftServiceName ) {

        this.address = address;
        this.serviceLayerHandler = handler;
        this.thriftServiceName = thriftServiceName;
        this.serviceDescriptor = ThriftServiceDescriptor.newDescriptor( handler.getClass().getInterfaces()[0]
                                                                            .getEnclosingClass() );
        this.serviceLayerExecutor = Executors.newSingleThreadExecutor( ThreadFactories
                                                                           .newThreadFactory( "jsvc-layer-proxy" ) );

        this.shutdownHook = new SvcServerShutdownHook( this );
    }

    /**
     * Start the server
     * <p/>
     * This method is idempotent.  If the server is already running it will silently return.
     *
     * @throws ServerLifecycleException if an error occurs starting the server
     */
    public void startAndWait( long timeout, TimeUnit unit ) throws ServerLifecycleException, InterruptedException,
                                                                   TimeoutException {
        try {
            Future<Void> f = start();
            if ( timeout <= 0 ) {
                f.get();
            } else {
                f.get( timeout, unit );
            }
        } catch ( ExecutionException ee ) {
            Throwable cause = ee.getCause();
            if ( cause instanceof ServerLifecycleException ) {
                throw (ServerLifecycleException) cause;
            } else {
                throw new ServerLifecycleException( "Failed to start server.", ee.getCause() );
            }
        }
    }

    /**
     * Start the server and return a future that will be completed once the server is fully started
     * and accepting connections.
     *
     * @return a Future that will be completed once the server is fully started and accepting connections
     */
    public Future<Void> start() {
        return CompletableFuture.runAsync( this::startServiceLayerServer );
    }

    private void startServiceLayerServer() { this.startServiceLayerServer( this.address ); }

    synchronized private void startServiceLayerServer( final InetSocketAddress addr ) {

        if ( thriftServer != null && thriftServer.isServing() ) {
            logger.info( "Thrift server is already running." );
            return;
        }

        Runnable runnable = () -> {
            try {
                Runtime.getRuntime().addShutdownHook( shutdownHook );

                logger.trace( "Starting Thrift Server on bind address " + addr );
                // Note on Thrift service compatibility:
                // If a backward incompatible change arises, pass additional pairs of
                // handler and Thrift service name. Similiarly if the Thrift service
                // API wants to be broken up.
                Map<String, S> handlers = new HashMap<String, S>();
                // For now, there is only one
                handlers.put( thriftServiceName, serviceLayerHandler );
                thriftServer = createThriftServer( addr, handlers );
                // thrift has an internal event handler, but it is not yet implemented in the (ancient) version
                // we are using.
                thriftServer.serve();
            } catch ( TTransportException e ) {
                throw new RuntimeException( e );
            }
        };

        serviceLayerExecutor.execute( runnable );
    }

    /**
     * Create a thrift non-blocking server on the specified port.  The server is not started.
     *
     * @param address the socket address to bind to
     *
     * @return the new server.  The server is not started
     *
     * @throws TTransportException
     */
    @SuppressWarnings( "unchecked" )
    private TNonblockingServer createThriftServer( InetSocketAddress address, Map<String, S> handlers ) throws TTransportException {

        TNonblockingServerSocket transport = new TNonblockingServerSocket( address );
        /**
         * FEATURE TOGGLE: enable subscriptions (async replication)
         * Mon Dec 28 16:51:58 MST 2015
         */
        if ( this.thriftServiceName.equals("") ) {

            S handler;
            for ( Map.Entry<String, S> e : handlers.entrySet()) {
                handler = e.getValue();
                TNonblockingServer.Args args = new TNonblockingServer.Args( transport )
                                                   .processor( (TProcessor) serviceDescriptor.getProcessorFactory()
                                                                                             .apply( handler ) );
                return new TNonblockingServer( args );
            }
            throw new TTransportException( "Failed to start server, no handler" );
        }

        // Multiplexed server
        TMultiplexedProcessor processor = new TMultiplexedProcessor();
        for (String thriftServiceName : handlers.keySet()) {
            processor.registerProcessor(thriftServiceName,
                (TProcessor) serviceDescriptor.getProcessorFactory().apply(handlers.get(thriftServiceName)));
        }
        TNonblockingServer.Args args = new TNonblockingServer.Args( transport )
                                           .processor( processor );
        return new TNonblockingServer( args );
    }

    /**
     * @throws ServerLifecycleException
     * @throws InterruptedException
     */
    public void stopAndWait( long timeout, TimeUnit unit ) throws ServerLifecycleException, InterruptedException,
                                                                  TimeoutException {
        try {
            Future<Void> f = stop();
            if ( timeout <= 0 ) {
                f.get();
            } else {
                f.get( timeout, unit );
            }
        } catch ( ExecutionException ee ) {
            throw new ServerLifecycleException( "Failed to start server.", ee.getCause() );
        }
    }

    /**
     * Stop the server.
     * <p/>
     * This method is idempotent.  If the server is already stopped it will silently return without
     * modifying the state of the server and no error is returned.
     *
     * @return a Future that will be completed once the server is fully stopped.
     *
     * @throws ServerLifecycleException if an error occurs stopping the server
     */
    public Future<Void> stop() throws ServerLifecycleException {
        CompletableFuture<Void> completableFuture = new CompletableFuture<Void>();

        if ( thriftServer != null && thriftServer.isServing() ) {

            return CompletableFuture.runAsync( () -> {
                try {
                    thriftServer.stop();
                } finally {
                    Runtime.getRuntime().removeShutdownHook( shutdownHook );
                }
            } );

        } else {

            completableFuture.complete( null );
            return completableFuture;

        }
    }
}
