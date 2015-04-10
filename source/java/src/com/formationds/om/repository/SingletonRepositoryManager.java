/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */
package com.formationds.om.repository;

import com.formationds.commons.crud.JDORepository;
import com.formationds.commons.togglz.feature.flag.FdsFeatureToggles;
import com.formationds.om.repository.influxdb.InfluxEventRepository;
import com.formationds.om.repository.influxdb.InfluxMetricRepository;
import com.formationds.om.repository.influxdb.InfluxRepository;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.lang.reflect.InvocationHandler;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;

/**
 * Factory to access to Metrics and Events repositories.
 *
 * @author ptinius
 */
public enum SingletonRepositoryManager {

    instance;

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
    static class InfluxRepositoryProxyIH implements InvocationHandler {

        /**
         * @return a proxy to the InfluxRepository
         *
         * @throws IllegalStateException if no database implementation is enabled.
         */
        public static MetricRepository newMetricRepositoryProxy() {
            boolean influxEnabled = FdsFeatureToggles.PERSIST_INFLUXDB.isActive();
            boolean objectDbEnabled = FdsFeatureToggles.PERSIST_OBJECTDB.isActive();
            boolean influxQueryEnabled = FdsFeatureToggles.QUERY_INFLUXDB.isActive();

            if ( !(influxEnabled || objectDbEnabled) ) {
                throw new IllegalStateException( "At least one of the metric database implementations must be enabled." );
            }

            JDOMetricsRepository jdoRepository = null;
            InfluxMetricRepository influxRepository = null;

            if ( objectDbEnabled ) {
                jdoRepository = new JDOMetricsRepository( );
            }

            if ( influxEnabled ) {
                logger.info( "InfluxDB feature is enabled." );
                // TODO: need values from config
                influxRepository = new InfluxMetricRepository( "http://localhost:8086",
                                                               "root",
                                                               "root".toCharArray() );

            }

            return (MetricRepository) Proxy.newProxyInstance( SingletonRepositoryManager.class.getClassLoader(),
                                                              new Class[] { MetricRepository.class },
                                                              new InfluxRepositoryProxyIH( jdoRepository,
                                                                                           influxRepository,
                                                                                           influxQueryEnabled ) );
        }

        /**
         * @return a proxy to the EventRepository.
         *
         * @throws IllegalStateException if no database implementation is enabled.
         */
        public static EventRepository newEventRepositoryProxy() {
            boolean influxEnabled = FdsFeatureToggles.PERSIST_INFLUXDB.isActive();
            boolean objectDbEnabled = FdsFeatureToggles.PERSIST_OBJECTDB.isActive();
            boolean influxQueryEnabled = FdsFeatureToggles.QUERY_INFLUXDB.isActive();

            if ( !(influxEnabled || objectDbEnabled) ) {
                throw new IllegalStateException( "At least one of the metric database implementations must be enabled." );
            }

            JDOEventRepository jdoEventRepository = null;
            InfluxEventRepository influxEventRepository = null;

            if ( objectDbEnabled ) {
                jdoEventRepository = new JDOEventRepository();
            }

            if ( influxEnabled ) {
                logger.info( "InfluxDB feature is enabled." );
                // TODO: need values from config
                influxEventRepository = new InfluxEventRepository( "http://localhost:8086",
                                                                   "root",
                                                                   "root".toCharArray() );
            }

            return (EventRepository) Proxy.newProxyInstance( EventRepository.class.getClassLoader(),
                                                             new Class[] { EventRepository.class },
                                                             new InfluxRepositoryProxyIH( jdoEventRepository,
                                                                                          influxEventRepository,
                                                                                          influxQueryEnabled ) );
        }

        private final JDORepository    jdoRepository;
        private final InfluxRepository influxRepository;
        private final boolean          influxQueryEnabled;

        public InfluxRepositoryProxyIH( JDORepository jdoRepository,
                                        InfluxRepository influxRepository,
                                        boolean influxQueryEnabled ) {
            this.jdoRepository = jdoRepository;
            this.influxRepository = influxRepository;
            this.influxQueryEnabled = influxQueryEnabled;

            if ( influxRepository != null ) {
                influxRepository.open( null );
            }
        }

        @Override
        public Object invoke( Object proxy, Method method, Object[] args ) throws Throwable {
            try {
                return doInvoke( proxy, method, args );
            } catch ( InvocationTargetException ite ) {
                // unwrap cause and rethrow
                throw ite.getTargetException();
            }
        }

        protected Object doInvoke(Object proxy, Method method, Object[] args ) throws Throwable {

            switch ( method.getName() ) {
                case "save":

                    return doInvoke0( false, proxy, method, args );

                // generic, base query api
                case "query":
                case "count":
                // event-specific queries
                case "findLatestFirebreak":
                case "queryTenantUsers":
                // metric specific queries
                case "mostRecentOccurrenceBasedOnTimestamp":
                case "leastRecentOccurrenceBasedOnTimestamp":
                case "getLatestVolumeStatus":

                    return doInvoke0( true, proxy, method, args );

                default:

                    // everything else allow to pass-through on the JDO repository for now if it is enabled.
                    return doInvoke0( false, proxy, method, args );

            }
        }

        protected Object doInvoke0(boolean isQuery, Object proxy, Method method, Object[] args ) throws Throwable {
            if ( jdoRepository == null && influxRepository == null ) {
                throw new IllegalStateException( "No repository is enabled.  Ensure one of the repositories is enabled in the fds-features.conf file." );
            }

            Object results = null;
            if (jdoRepository != null) {
                results = method.invoke( jdoRepository, args );
            }

            if ( influxRepository != null ) {
                try {
                    Object influxResult = method.invoke( influxRepository, args );
                    if ( jdoRepository == null || (isQuery && influxQueryEnabled)) {
                        // use the results from the influx repo
                        results = influxResult;
                    }
                } catch (Exception e) {
                    //
                    if ( jdoRepository == null ) {
                        // error since ObjectDB repo is disabled
                        logger.error( "Failed to influx repository operation.", e );
                        throw e;
                    } else {
                        // only a warning since we are still using the ObjectDB repo as the main repo.
                        logger.warn( "Failed to execute influx repository operation.  Returning data from ObjectDB repo" + e.getMessage() );
                        logger.trace( "Influx repository error is ", e );
                    }
                }
            }
            return results;
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
     * Initialize the metric repository.  The implementation returned is dependent on the settings
     * of feature toggles:
     * <ul>
     *     <li>PERSIST_OBJECTDB</li>
     *     <li>PERSIST_INFLUXDB</li>
     *     <LI>QUERY_INFLUXDB</LI>
     * </ul>
     */
    protected void initializeMetricRepository() {
        metricsRepository = InfluxRepositoryProxyIH.newMetricRepositoryProxy();
    }

    /**
     * Initialize the event repository.  The implementation returned is dependent on the settings
     * of feature toggles:
     * <ul>
     *     <li>PERSIST_OBJECTDB</li>
     *     <li>PERSIST_INFLUXDB</li>
     *     <LI>QUERY_INFLUXDB</LI>
     * </ul>
     */
    protected void initializeEventRepository() {
        eventRepository = InfluxRepositoryProxyIH.newEventRepositoryProxy();
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
        if ( metricsRepository == null ) {
            this.metricsRepository = new JDOMetricsRepository();
        }

        return metricsRepository;
    }
}
