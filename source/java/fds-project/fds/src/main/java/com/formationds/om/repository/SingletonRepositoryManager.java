/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */
package com.formationds.om.repository;

import com.formationds.commons.togglz.feature.flag.FdsFeatureToggles;
import com.formationds.om.helper.SingletonConfiguration;
import com.formationds.om.repository.influxdb.InfluxEventRepository;
import com.formationds.om.repository.influxdb.InfluxMetricRepository;
import com.formationds.om.repository.influxdb.InfluxMetricSeriesPerVolumeRepo;
import com.formationds.om.repository.influxdb.InfluxRepository;
import com.google.common.base.Preconditions;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.lang.reflect.InvocationHandler;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CompletableFuture;

/**
 * Factory to access to Metrics and Events repositories.
 *
 * @author ptinius
 */
public class SingletonRepositoryManager {

    public static final SingletonRepositoryManager instance = new SingletonRepositoryManager();

    private SingletonRepositoryManager() {}

    public static final Logger logger = LoggerFactory.getLogger( SingletonRepositoryManager.class );

    /**
     * singleton instance of SingletonMetricsRepository
     */
    public static SingletonRepositoryManager instance() {
        return instance;
    }

    /**
     * An invocation handler for dynamic proxies to the MetricRepository interface
     */
    static class InfluxRepositoryFactory {

        static final int DEFAULT_INFLUXDB_PORT = 8086;

        /**
         * @return a proxy to the InfluxRepository
         *
         * @throws IllegalStateException if no database implementation is enabled.
         */
        public static MetricRepository newMetricRepository() {

            String url = getInfluxDBUrl();

            // TODO: credentials need to be externalized to a secure store.
            logger.info( String.format( "InfluxDB Metric DB url is %s", url ) );
            MetricRepository influxRepository =
                    MetricRepositoryProxyIH.newMetricRepositoryProxy( url, "root", "root" );

            influxRepository.open( null );

            return influxRepository;
        }

        static String getInfluxDBUrl() {
            String url = SingletonConfiguration.getPlatformConfig()
                                               .defaultString( "fds.om.influxdb.url", null );

            if ( url == null || url.trim().isEmpty() ) {

                logger.warn( "fds.om.influxdb.url is not defined in the configuration.  " +
                        "Using default based on local host address." );
                String host = "localhost";
                try {
                    host = InetAddress.getLocalHost().getHostAddress();
                } catch ( UnknownHostException uhe ) {
                    // ignore - use localhost
                }

                url = String.format( "http://%s:%d",
                                     host,
                                     DEFAULT_INFLUXDB_PORT );
            }
            return url;
        }

        /**
         * @return a proxy to the EventRepository.
         *
         * @throws IllegalStateException if no database implementation is enabled.
         */
        public static EventRepository newEventRepository() {
            String url = getInfluxDBUrl();

            // TODO: credentials need to be externalized to a secure store.
            logger.info( String.format( "InfluxDB Event DB url is %s", url ) );
            InfluxEventRepository influxRepository = new InfluxEventRepository( url,
                                                                                "root",
                                                                                "root".toCharArray() );

            influxRepository.open( null );

            return influxRepository;
        }
    }
    /**
     * An invocation handler for dynamic proxies to the MetricRepository interface
     */
    static class MetricRepositoryProxyIH implements InvocationHandler {

        /**
         * @return a proxy to the InfluxRepository that handles writing to multiple
         *   destination repositories and querying from a designated primary.
         *
         * @throws IllegalStateException if no database implementation is enabled.
         */
        public static MetricRepository newMetricRepositoryProxy(String url,
                                                                String user,
                                                                String pwd) {

            boolean influx1SEnabled = FdsFeatureToggles.INFLUX_ONE_SERIES.isActive();
            boolean influxSPVEnabled = FdsFeatureToggles.INFLUX_SERIES_PER_VOLUME.isActive();
            boolean querySPV = FdsFeatureToggles.INFLUX_QUERY_SERIES_PER_VOLUME.isActive();

            if ( !(influx1SEnabled || influxSPVEnabled) ) {
                logger.warn( "Expecting at least one metric database implementation is enabled.  " +
                        "Defaulting to use the 'influxdb_one_series' implementation.  " );
                influx1SEnabled = true;
                influxSPVEnabled = false;
                querySPV = false;
            }

            InfluxMetricRepository influxRepository1S = null;
            InfluxMetricRepository influxRepositorySPV = null;
            if ( influx1SEnabled ) {
                logger.info( "InfluxDB Single Series is enabled." );
                influxRepository1S = new InfluxMetricRepository( url,
                                                                 user,
                                                                 pwd.toCharArray() );
            }

            if ( influxSPVEnabled ) {
                logger.info( "InfluxDB Series Per Volume feature is enabled." );

                // TODO: credentials need to be externalized to a secure store.
                logger.info( String.format( "InfluxDB url is %s", url ) );
                influxRepositorySPV = new InfluxMetricSeriesPerVolumeRepo( url,
                                                                           user,
                                                                           pwd.toCharArray() );
            }

            // Repository proxy is currently setup to handle a single primary
            // that is the one that results are reported from.  Any secondary
            // repositories are written to and queried from but results are not
            // reported except to trace logs if tracing is enabled.
            // This is admittedly overkill for the current use case of 2 influx destinations.
            // However, I am also thinking about the possibility of adding a StatService
            // Metric Repository and this approach would handle that integration as well).

            MetricRepository primary = null;
            List<MetricRepository> secondaryMetricRepositories = new ArrayList<>();

            if ( querySPV && influxSPVEnabled ) {
                primary = influxRepositorySPV;
                if ( influx1SEnabled )
                    secondaryMetricRepositories.add( influxRepository1S );
            } else {
                primary = influxRepository1S;
                if ( influxSPVEnabled ) {
                    secondaryMetricRepositories.add( influxRepositorySPV );
                }
            }

            return (MetricRepository) Proxy.newProxyInstance( SingletonRepositoryManager.class.getClassLoader(),
                                                              new Class[] { MetricRepository.class },
                                                              new MetricRepositoryProxyIH( primary,
                                                                                           secondaryMetricRepositories ) );
        }

        private final MetricRepository primaryMetricRepository;
        private final List<MetricRepository> secondaryMetricRepositories = new ArrayList<>();

        /**
         *
         * @param primary index into the list of metric repositories for the instance
         *                           that should provide results to callers.  The primary.
         * @param secondaryMetricRepositories the list metric repositories to send data to.
         */
        public MetricRepositoryProxyIH( MetricRepository primary,
                                        List<MetricRepository> secondaryMetricRepositories ) {

            Preconditions.checkNotNull( primary,
                                        "A primary metric repository is required." );
            this.primaryMetricRepository = primary;
            this.secondaryMetricRepositories.addAll( secondaryMetricRepositories );
        }

        @Override
        public Object invoke( Object proxy, Method method, Object[] args ) throws Throwable {
            try {
                for (MetricRepository r : secondaryMetricRepositories) {
                    doInvoke( r, false, proxy, method, args );
                }
                return doInvoke(primaryMetricRepository, true, proxy, method, args);
            } catch ( InvocationTargetException ite ) {
                // unwrap cause and rethrow
                throw ite.getTargetException();
            }
        }

        protected Object doInvoke( MetricRepository target,
                                   boolean isPrimary,
                                   Object proxy,
                                   Method method,
                                   Object[] args ) throws Throwable {

            InvocationTargetException thrown = null;
            Object ret = null;

            logger.trace( "Invoking method {} against {}({}) with {} args",
                          method.getName(),
                          target.getClass(),
                          System.identityHashCode( target ),
                          args == null ? 0 : args.length );

            try {
                Object result = method.invoke( target, args );
                if (isPrimary) {
                    ret = result;
                }
            } catch (InvocationTargetException ite) {
                logger.warn( "Failed Invoking method {} against {}({}) with {} args; Message: {}",
                          method.getName(),
                          target.getClass(),
                          System.identityHashCode( target ),
                          args == null ? 0 : args.length,
                          ite.getMessage());
                logger.debug( "Invoke failed", ite );

                if (isPrimary) {
                    thrown = ite;
                    logger.debug( "Primary metric repository invocation failed.  Will continue trying any secondaries." );
                } else {
                    logger.debug( "NOTE: Ignoring secondary metric repository invocation failure" );
                }
            } finally {
            }

            if (thrown != null)
                throw thrown;

            return ret;
        }
    }

    private MetricRepository metricsRepository = null;
    private EventRepository  eventRepository   = null;

    /**
     * @throws IllegalStateException if no database implementation is enabled, or if already initialized
     */
    synchronized public void initializeRepositories() {

        if ( metricsRepository != null && eventRepository != null ) {
            throw new IllegalStateException( "Repositories are already initialized.  Can not re-initialize them." );
        }

        initializeMetricRepository();
        initializeEventRepository();
    }

    /**
     * Initialize the metric repository.
     */
    protected void initializeMetricRepository() {
        metricsRepository = InfluxRepositoryFactory.newMetricRepository();
        CompletableFuture<Void> cf = CompletableFuture.runAsync( () -> {
            ((InfluxRepository<?,?>)metricsRepository).initializeCache();
        } );
    }

    /**
     * Initialize the event repository.
     */
    protected void initializeEventRepository() {
        eventRepository = InfluxRepositoryFactory.newEventRepository();
        CompletableFuture<Void> cf = CompletableFuture.runAsync( () -> {
            ((InfluxRepository<?,?>)eventRepository).initializeCache();
        } );
    }

    /**
     * @return the {@link EventRepository}
     */
    public EventRepository getEventRepository() {
        return eventRepository;
    }

    /**
     * @return Returns the {@link MetricRepository}
     */
    public MetricRepository getMetricsRepository() {
        return metricsRepository;
    }

    // testing hack to allow mocking of the repository.  should not be considered part of API
    void metricRepo(MetricRepository repo) {
        metricsRepository = repo;
    }

    // testing hack to allow mocking of the repository.  should not be considered part of API!
    void eventRepo(EventRepository repo) {
        eventRepository = repo;
    }
}
