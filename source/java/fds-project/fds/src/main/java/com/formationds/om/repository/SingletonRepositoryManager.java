/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */
package com.formationds.om.repository;

import com.formationds.om.helper.SingletonConfiguration;
import com.formationds.om.repository.influxdb.InfluxEventRepository;
import com.formationds.om.repository.influxdb.InfluxMetricRepository;
import com.formationds.om.repository.influxdb.InfluxRepository;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.net.InetAddress;
import java.net.UnknownHostException;
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
            logger.info( String.format( "InfluxDB url is %s", url ) );
            InfluxMetricRepository influxRepository = new InfluxMetricRepository( url,
                                                                                  "root",
                                                                                  "root".toCharArray() );

            influxRepository.open( null );

            return influxRepository;
        }

        private static String getInfluxDBUrl() {
            String url = SingletonConfiguration.instance()
                    .getConfig()
                    .getPlatformConfig()
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
            logger.info( String.format( "InfluxDB url is %s", url ) );
            InfluxEventRepository influxRepository = new InfluxEventRepository( url,
                                                                                "root",
                                                                                "root".toCharArray() );

            influxRepository.open( null );

            return influxRepository;
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
