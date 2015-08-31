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

public class ThriftServiceDescriptor<T,
                                        IF,
                                        AIF,
                                        C extends TServiceClient,
                                        CF extends TServiceClientFactory<C>,
                                        AC extends TAsyncClient,
                                        ACF extends TAsyncClientFactory<AC>,
                                        PF extends TBaseProcessor<IF>> {

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

    public static <T> ThriftServiceDescriptor<T, ?, ?, ?, ?, ?, ?, ?> newDescriptor( Class<T> thriftServiceClass ) {
        return new ThriftServiceDescriptor<>( thriftServiceClass );
    }

    private final Class<T>   thriftWrapper;
    private       Class<CF>  serviceClientFactoryClass;
    private       Class<ACF> asyncServiceClientFactoryClass;
    private       Class<PF>  processorClass;

    private CF  serviceClientFactory;
    private ACF asyncServiceClientFactory;

    private ThriftServiceDescriptor( Class<T> thriftWrapper ) {
        //        PlatNetSvc.class,
        //        PlatNetSvc.Iface.class,
        //            PlatNetSvc.AsyncIface.class,
        //            PlatNetSvc.Client.class,
        //            PlatNetSvc.Client.Factory.class,
        //            PlatNetSvc.AsyncClient.class,
        //            PlatNetSvc.AsyncClient.Factory.class,
        //            PlatNetSvc.Processor.class
        this.thriftWrapper = thriftWrapper;

        this.serviceClientFactoryClass = extractClientFactory();
        this.asyncServiceClientFactoryClass = extractAsyncClientFactory();
        this.processorClass = extractProcessor();
    }

    // these all depend on thriftWrapper
    protected Class<IF> extractIface() { return (Class<IF>) extractClass( "Iface" ); }

    protected Class<AIF> extractAsyncIface() { return (Class<AIF>) extractClass( "AsyncIface" ); }

    protected Class<C> extractClient() { return (Class<C>) extractClass( "Client" ); }

    protected Class<CF> extractClientFactory() { return (Class<CF>) extractClass( extractClient(), "Factory" ); }

    protected Class<AC> extractAsyncClient() { return (Class<AC>) extractClass( "AsyncClient" ); }

    protected Class<ACF> extractAsyncClientFactory() {
        return (Class<ACF>) extractClass( extractAsyncClient(), "Factory" );
    }

    protected Class<PF> extractProcessor() { return (Class<PF>) extractClass( "Processor" ); }

    protected Class<?> extractClass( String simpleName ) {
        Preconditions.checkNotNull( thriftWrapper,
                                    "Thrift wrapper class must be defined before extracting internal classes." );

        Class<?> c = extractClass( thriftWrapper, simpleName );
        if ( c == null ) {
            throw new IllegalArgumentException( "Unable to locate inner class named '" + simpleName + "'" );
        }
        return c;
    }

    protected Class<?> extractClass( Class<?> c, String simpleName ) {

        Class<?> classes[] = c.getDeclaredClasses();
        for ( final Class<?> aClass : classes ) {
            String cname = aClass.getName();
            if ( cname.endsWith( "$" + simpleName ) ) {
                return aClass;
            }
        }
        return null;
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

    public ThriftClientFactory<IF> newThriftClientFactory( String host,
                                                           Integer port ) {
        return new ThriftClientFactory.Builder<>( extractIface() )
                   .withHostPort( host, port )
                   .withClientFactory( getClientFactory() )
                   .build();
    }

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