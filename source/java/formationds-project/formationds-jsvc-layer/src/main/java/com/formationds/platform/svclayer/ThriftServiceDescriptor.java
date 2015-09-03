/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */
package com.formationds.platform.svclayer;

import com.formationds.apis.ConfigurationService;
import com.formationds.protocol.om.OMSvc;
import com.formationds.protocol.svc.PlatNetSvc;
import com.formationds.protocol.svc.PlatNetSvc.AsyncClient;
import com.formationds.protocol.svc.PlatNetSvc.AsyncIface;
import com.formationds.protocol.svc.PlatNetSvc.Client;
import com.formationds.protocol.svc.PlatNetSvc.Client.Factory;
import com.formationds.protocol.svc.PlatNetSvc.Iface;
import com.formationds.protocol.svc.PlatNetSvc.Processor;
import com.formationds.util.thrift.ThriftClientFactory;
import com.google.common.base.Preconditions;
import org.apache.thrift.TBaseProcessor;
import org.apache.thrift.TServiceClient;
import org.apache.thrift.TServiceClientFactory;
import org.apache.thrift.async.TAsyncClient;
import org.apache.thrift.async.TAsyncClientFactory;
import org.apache.thrift.async.TAsyncClientManager;
import org.apache.thrift.protocol.TBinaryProtocol;
import org.apache.thrift.protocol.TProtocol;
import org.apache.thrift.protocol.TProtocolFactory;
import org.apache.thrift.transport.TNonblockingTransport;

import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;
import java.util.function.Function;

/**
 * Metadata describing a Thrift Service definition.  A valid thrift service class generated
 * by the thrift compiler contains the outer class definition (unfortunately does not implement
 * an interface or extend a base class that would make some of these acrobatics unnecessary); an
 * IFace defining the service methods as declared in the IDL; and AsyncIface; the sync/async
 * client and factories; and lastly, the processor.
 * <p>
 * This implementation is based on Thrift 0.9.0 generated code.
 * <p>
 * Usage is
 * <code>
 * class Server<T extends PlatNetSvc> {
 * <p>
 * public Server(T handler) {
 * this.serviceDescriptor = ThriftServiceDescriptor.newDescriptor( handler.getClass()
 * .getInterfaces()[0]
 * .getEnclosingClass() );
 * }
 * <p>
 * ...
 * private TNonblockingServer createThriftServer( InetSocketAddress address,
 * S handler ) throws TTransportException {
 * TNonblockingServerSocket transport = new TNonblockingServerSocket( address );
 * TNonblockingServer.Args args = new TNonblockingServer.Args( transport )
 * .processor( (TProcessor) serviceDescriptor.getProcessorFactory()
 * .apply( handler ) );
 * return new TNonblockingServer( args );
 * }
 * }
 * </code>
 * In the example above I have defined T as an extension of PlatNetSvc but in fact it can represent any
 * valid Thrift generated service.
 *
 * @param <T>
 * @param <IF>
 * @param <AIF>
 * @param <C>
 * @param <CF>
 * @param <AC>
 * @param <ACF>
 * @param <PF>
 */
public class ThriftServiceDescriptor<T,
                                        IF,
                                        AIF,
                                        C extends TServiceClient,
                                        CF extends TServiceClientFactory<C>,
                                        AC extends TAsyncClient,
                                        ACF extends TAsyncClientFactory<AC>,
                                        PF extends TBaseProcessor<IF>> {

    /**
     *
     * @return A ThriftServiceDescriptor for the PlatNetSvc
     */
    @SuppressWarnings("unchecked")
    public static ThriftServiceDescriptor<PlatNetSvc,
                                             Iface,
                                             AsyncIface,
                                             Client,
                                                 Factory,
                                                 AsyncClient,
                                                 AsyncClient.Factory,
                                             Processor<Iface>> newPlatNetSvcDescriptor() {

        return new ThriftServiceDescriptor( PlatNetSvc.class );
    }

    /**
     *
     * @return a ThriftServiceDescriptor for the OMSvc
     */
    @SuppressWarnings("unchecked")
    public static ThriftServiceDescriptor<OMSvc,
                                             OMSvc.Iface,
                                             OMSvc.AsyncIface,
                                             OMSvc.Client,
                                                 OMSvc.Client.Factory,
                                                 OMSvc.AsyncClient,
                                             OMSvc.AsyncClient.Factory,
                                             OMSvc.Processor<OMSvc.Iface>> newOMSvcDescriptor() {

        return new ThriftServiceDescriptor( OMSvc.class );
    }

    /**
     * @return a new ThriftServiceDescriptor for the OM ConfigurationService
     */
    @SuppressWarnings("unchecked")
    public static ThriftServiceDescriptor<ConfigurationService,
                                             ConfigurationService.Iface,
                                             ConfigurationService.AsyncIface,
                                                 ConfigurationService.Client,
                                                 ConfigurationService.Client.Factory,
                                             ConfigurationService.AsyncClient,
                                             ConfigurationService.AsyncClient.Factory,
                                             ConfigurationService.Processor<ConfigurationService.Iface>> newConfigSvcDescriptor() {

        return new ThriftServiceDescriptor( ConfigurationService.class );
    }

    /**
     *
     * @param thriftServiceClass the thrift service class
     * @param <T> the service type
     * @return the new ThriftServiceDescriptor
     */
    public static <T> ThriftServiceDescriptor<T, ?, ?, ?, ?, ?, ?, ?> newDescriptor( Class<T> thriftServiceClass ) {
        return new ThriftServiceDescriptor<>( thriftServiceClass );
    }

    private final Class<T>   thriftWrapper;
    private       Class<CF>  serviceClientFactoryClass;
    private       Class<ACF> asyncServiceClientFactoryClass;
    private       Class<PF>  processorClass;

    private CF  serviceClientFactory;
    private ACF asyncServiceClientFactory;

    /**
     *
     * @param thriftWrapper the thrift generated service wrapper class
     *
     * @throws IllegalArgumentException if the wrapper class is not a valid thrift-generated service class
     */
    private ThriftServiceDescriptor( Class<T> thriftWrapper ) {
        this.thriftWrapper = thriftWrapper;

        this.serviceClientFactoryClass = extractClientFactory();
        this.asyncServiceClientFactoryClass = extractAsyncClientFactory();
        this.processorClass = extractProcessor();
    }

    // these all depend on thriftWrapper
    @SuppressWarnings("unchecked")
    private Class<IF> extractIface() { return (Class<IF>) extractClass( "Iface" ); }

    @SuppressWarnings("unchecked")
    private Class<AIF> extractAsyncIface() { return (Class<AIF>) extractClass( "AsyncIface" ); }

    @SuppressWarnings("unchecked")
    private Class<C> extractClient() { return (Class<C>) extractClass( "Client" ); }

    @SuppressWarnings("unchecked")
    private Class<CF> extractClientFactory() { return (Class<CF>) extractClass( extractClient(), "Factory" ); }

    @SuppressWarnings("unchecked")
    private Class<AC> extractAsyncClient() { return (Class<AC>) extractClass( "AsyncClient" ); }

    @SuppressWarnings("unchecked")
    private Class<ACF> extractAsyncClientFactory() {
        return (Class<ACF>) extractClass( extractAsyncClient(), "Factory" );
    }

    @SuppressWarnings("unchecked")
    private Class<PF> extractProcessor() { return (Class<PF>) extractClass( "Processor" ); }

    /**
     * Given the simple name of a thrift service inner class, load the class.
     *
     * @param simpleName the name: Iface, AsyncIface, Client, Client.Factory, AsyncClient, AsyncClient.Factory, Processor
     *
     * @return the thrift service class.
     *
     * @throws IllegalArgumentException if an inner class of the Thrift Wrapper service class can not be found.
     */
    protected Class<?> extractClass( String simpleName ) {
        Preconditions.checkNotNull( thriftWrapper,
                                    "Thrift wrapper class must be defined before extracting internal classes." );

        Class<?> c = extractClass( thriftWrapper, simpleName );
        if ( c == null ) {
            throw new IllegalArgumentException( "Unable to locate inner class named '" + simpleName + "'" );
        }
        return c;
    }

    /**
     * Given the simple name of a thrift service inner class, load the class.
     *
     * @param c the class to search.  For the purposes of the Trift ServiceDescriptor, this is expected
     *          to be an inner class of the thrift service wrapper
     * @param simpleName the name: Iface, AsyncIface, Client, Client.Factory, AsyncClient,
     *                   AsyncClient.Factory, Processor
     *
     * @return the thrift service class.
     *
     * @throws IllegalArgumentException if an inner class of the Thrift Wrapper service class can not be found.
     */
    protected Class<?> extractClass( Class<?> c, String simpleName ) {

        Class<?> classes[] = c.getDeclaredClasses();
        for ( final Class<?> aClass : classes ) {
            String cname = aClass.getName();
            if ( cname.endsWith( "$" + simpleName ) ) {
                return aClass;
            }
        }
        throw new IllegalArgumentException( "Unable to locate inner class of " + c.getName() +
                                            " named '" + simpleName + "'" );
    }

    private Function<IF, PF> newProcessorFactory() {
        try {
            Constructor<PF> ctor = processorClass.getConstructor( extractIface() );
            return ( anif ) -> {
                try {
                    return ctor.newInstance( anif );
                } catch ( InvocationTargetException e ) {
                    throw new IllegalStateException( e.getCause() );
                } catch ( RuntimeException re ) {
                    throw re;
                } catch ( Exception e ) {
                    throw new IllegalStateException( e );
                }
            };
        } catch ( Exception e ) {
            throw new IllegalStateException( e );
        }
    }

    private CF newClientFactory() {
        try {
            return serviceClientFactoryClass.newInstance();
        } catch ( Exception e ) {
            throw new IllegalStateException( e );
        }
    }

    // TODO: pass in async client manager and protocol factory or make this return a BiFunction?
    private ACF newAsyncClientFactory() {
        try {
            Constructor<ACF> ctor = asyncServiceClientFactoryClass.getConstructor( TAsyncClientManager.class,
                                                                                   TProtocolFactory.class );
            return ctor.newInstance( new TAsyncClientManager(), new TBinaryProtocol.Factory() );
        } catch ( Exception e ) {
            throw new IllegalStateException( e );
        }
    }

    public Class<IF> getIfaceClass() { return extractIface(); }

    public Class<AIF> getAsyncIfaceClass() { return extractAsyncIface(); }

    public Class<C> getClientClass() { return extractClient(); }

    public Class<AC> getAsyncClientClass() { return extractAsyncClient(); }

    public Function<IF, PF> getProcessorFactory() {
        return newProcessorFactory();
    }

    @SuppressWarnings("unchecked")
    public Function<TProtocol, IF> getClientFactory() {
        if ( serviceClientFactory == null ) {
            serviceClientFactory = newClientFactory();
        }
        return ( tp ) -> (IF) serviceClientFactory.getClient( tp );
    }

    @SuppressWarnings("unchecked")
    public Function<TNonblockingTransport, AIF> getAsyncClientFactory() {
        if ( asyncServiceClientFactory == null ) {
            asyncServiceClientFactory = newAsyncClientFactory();
        }
        return ( tt ) -> (AIF) asyncServiceClientFactory.getAsyncClient( tt );
    }

    /**
     * Build a new thrift client factory for the service interface connecting to the specified host and port
     *
     * @param host the thrift server host
     * @param port the thrift server port
     *
     * @return the new ThriftClientFactory
     */
    public ThriftClientFactory<IF> newThriftClientFactory( String host,
                                                           Integer port ) {
        return new ThriftClientFactory.Builder<>( extractIface() )
                   .withHostPort( host, port )
                   .withClientFactory( getClientFactory() )
                   .build();
    }

    /**
     *
     * Build a new thrift client factory for the service interface connecting to the specified host and port
     *
     * @param host the thrift server host
     * @param port the thrift server port
     * @param maxPoolSize the maximum pool size for the client pool
     * @param minIdle the minimum idle connections to keep in the pool
     * @param softMinEvictionIdleTimeMillis the minimum idle eviction time
     *
     * @return the new ThriftClientFactory
     */
    public ThriftClientFactory<IF> newThriftClientFactory( String host,
                                                           Integer port,
                                                           int maxPoolSize,
                                                           int minIdle,
                                                           int softMinEvictionIdleTimeMillis ) {
        return new ThriftClientFactory.Builder<>( extractIface() )
                   .withHostPort( host, port )
                   .withPoolConfig( maxPoolSize, minIdle, softMinEvictionIdleTimeMillis )
                   .withClientFactory( getClientFactory() )
                   .build();
    }
}